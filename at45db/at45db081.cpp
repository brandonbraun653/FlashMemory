/* Boost Includes */
#include <boost/make_shared.hpp>

#include "at45db081.hpp"

using namespace Chimera::SPI;

#define BYTE_LEN(x) (sizeof(x)/sizeof(uint8_t))
#define STANDARD_PAGE_SIZE 264
#define BINARY_PAGE_SIZE 256

struct FlashSizes
{
	size_t numSectors;
	size_t numBlocks;
	size_t numPages;
};

struct AddressDescriptions
{
	uint8_t dummyBitsMSB;
	uint8_t addressBits;
	uint8_t dummyBitsLSB;
};

struct AddressScheme
{
	AddressDescriptions standardSize;
	AddressDescriptions binarySize;
};

struct MemoryAddressFormat
{
	AddressScheme page;
	AddressScheme block;
	AddressScheme sector;
	AddressScheme sector0ab;
	const uint8_t numAddressBytes;
};


/* Describes common delay times for most flash operations (in mS) */
struct FlashDelay
{
	uint8_t pageEraseAndProgramming;
	uint8_t pageProgramming;
	uint8_t pageErase;
	uint8_t blockErase;
	uint16_t sectorErase;
	uint16_t chipErase;
};


/* Useful for determining any runtime errors */
#define ADDRESS_OVERRUN			(1u << 0)
#define INVALID_SECTION_NUMBER	(1u << 1)


namespace Adesto
{
	namespace NORFlash
	{	
		/* This MUST be kept in the same order as FlashChip enum */
		static const FlashSizes chipSpecs[NUM_SUPPORTED_CHIPS] = 
		{
			{ 16, 512, 4096 },	//AT45DB081E
		};

		static const MemoryAddressFormat addressFormat[NUM_SUPPORTED_CHIPS] =
		{
			//AT45DB081E: See datasheet pgs. 13-14
			{
				{ {3, 12, 9}, {4, 12, 8} },	//Page
				{ {3, 9, 12}, {4, 9, 11} },	//Block
				{ {3, 4, 17}, {4, 4, 16} },	//Sector
				{ {3, 9, 12}, {4, 9, 11} },	//Sector 0a, 0b
				3							//Number of address bytes
			},
		};

		static const FlashDelay chipDelay[NUM_SUPPORTED_CHIPS] = 
		{
			//AT45DB081E: See datasheet pg.49
			{
				15,		//Page erase and programming 
				2,		//Page programming 
				12,		//Page erase
				30,		//Block erase
				700,	//Sector erase
				10000	//Chip erase
			},
		};


		AT45::AT45(FlashChip chip, const int& spiChannel)
		{
			device = chip;
			addressBytes = addressFormat[device].numAddressBytes;

			spi = boost::make_shared<SPIClass>(spiChannel);

			setup.clockFrequency = 1000000;
			setup.bitOrder = MSB_FIRST;
			setup.clockMode = MODE0;
			setup.dataSize = DATASIZE_8BIT;
			setup.mode = MASTER;

			memset(cmdBuffer, 0, SIZE_OF_ARRAY(cmdBuffer));
			

			#if defined(USING_FREERTOS)
			multiTXWakeup = xSemaphoreCreateBinary();
			singleTXWakeup = xSemaphoreCreateBinary();
			singleTXRXWakeup = xSemaphoreCreateBinary();
			#endif
		}

		Adesto::Status AT45::initialize(uint32_t clockFreq)
		{
			if (spi->begin(setup) != SPI_OK)
				return ERROR_SPI_INIT_FAILED;

			#if defined(USING_FREERTOS)
			spi->setMode(TXRX, DMA);
			spi->attachThreadTrigger(BUFFERED_TXRX_COMPLETE, &multiTXWakeup);
			spi->attachThreadTrigger(TX_COMPLETE, &singleTXWakeup);
			spi->attachThreadTrigger(TXRX_COMPLETE, &singleTXRXWakeup);
			#else			spi->setMode(TXRX, BLOCKING);
			#endif

			/*--------------------------------------
			 * Check for a proper device connection:
			 *	1) Get the manufacturer id at low freq (~1MHz for stability)
			 *	2) Retry again at the user requested frequency 
			 *-------------------------------------*/
			auto lowFreqInfo = getDeviceInfo();
			if (info.manufacturerID != JEDEC_CODE)
				return ERROR_UNKNOWN_JEDEC_CODE;

			spi->updateClockFrequency(clockFreq);
			auto hiFreqInfo = getDeviceInfo();
			
			if (memcmp(&lowFreqInfo, &hiFreqInfo, sizeof(AT45xx_DeviceInfo)) != 0)
				return ERROR_FAILED_HIGH_FREQUENCY_TRANSACTION;


			/*--------------------------------------
			* Initialize various other settings
			*-------------------------------------*/
			useBinaryPageSize();
			if (pageSize != getPageSizeConfig())
				return ERROR_PAGE_SIZE_MISMATCH;

			return FLASH_OK;
		}

		Adesto::Status AT45::continuousRead(uint32_t pageNumber, uint16_t startAddress, uint8_t* dataOut, size_t len, func_t onComplete)
		{
			cmdBuffer[0] = CONT_ARR_READ_HF2;

			/* Shift and mask by either 9 or 8 bits */
			uint32_t fullAddress = (pageSize == STANDARD_PAGE_SIZE) ? ((pageNumber << 9) | (startAddress & 0x1FF)) : ((pageNumber << 8) | (startAddress & 0xFF));

			/* The command is comprised of an opcode (1 byte), an address (3 bytes), and 2 dummy bytes.
			 * The dummy bytes are used to initialize the read operation. */
			memcpy(cmdBuffer + 1, (uint8_t*)&fullAddress, 3);
			SPI_write(cmdBuffer, 6, false);

			#if defined(USING_FREERTOS)
			/* Block so we don't trigger on multi TX. In reality this is a very small amount of time */
			while (!isWriteComplete());
			#endif 

			/* FREERTOS: Safe to return immediately because data pointer is out of function scope */
			SPI_read(dataOut, len, true);

			if (onComplete)
				onComplete();

			return FLASH_OK;
		}

		Adesto::Status AT45::directPageRead(uint32_t pageNumber, uint16_t startAddress, uint8_t* dataOut, size_t len, func_t onComplete)
		{
			cmdBuffer[0] = MAIN_MEM_PAGE_READ;

			/* Shift and mask by either 9 or 8 bits */
			uint32_t fullAddress = (pageSize == STANDARD_PAGE_SIZE) ? ((pageNumber << 9) | (startAddress & 0x1FF)) : ((pageNumber << 8) | (startAddress & 0xFF));

			/* The command is comprised of an opcode (1 byte), an address (3 bytes), and 4 dummy bytes.
			 * The dummy bytes are used to initialize the read operation. */
			memcpy(cmdBuffer + 1, (uint8_t*)&fullAddress, 3);
			SPI_write(cmdBuffer, 8, false);

			#if defined(USING_FREERTOS)
			/* Block so we don't trigger on multi TX. In reality this is a very small amount of time */
			while (!isWriteComplete());
			#endif 

			/* FREERTOS: Safe to return immediately because data pointer is out of function scope */
			SPI_read(dataOut, len, true);

			if (onComplete)
				onComplete();

			return FLASH_OK;
		}

		Adesto::Status AT45::bufferRead(SRAMBuffer bufferNumber, uint16_t startAddress, uint8_t* dataOut, size_t len, func_t onComplete)
		{
			/* Mask off the first 8 or 9 bits */
			startAddress &= (pageSize == STANDARD_PAGE_SIZE) ? 0x1FF : 0xFF;

			cmdBuffer[0] = (bufferNumber == BUFFER1) ? BUFFER1_READ_HF : BUFFER2_READ_HF;

			/* Default read will use the High Frequency mode, so an extra dummy byte is needed on the
			 * end of the address. This yields a full 32 bit address with the first 15 or 16 bits ignored,
			 * the next 9 or 8 bits as the actual address within the page, and then the last 8 bits ignored.
			 *
			 * In practice this means that in the cmdBuffer, byte 1 holds the command, 3 & 4 hold the address,
			 * and 2 & 5 are completely ignored by the flash chip.  Total TX length is five bytes. */
			memcpy(cmdBuffer + 2, (uint8_t*)&startAddress, 2);
			SPI_write(cmdBuffer, 5, false);
			
			#if defined(USING_FREERTOS)
			/* Block so we don't trigger on multi TX. In reality this is a very small amount of time */
			while (!isWriteComplete());
			#endif 

			/* FREERTOS: Safe to return immediately because data pointer is out of function scope */
			SPI_read(dataOut, len, true);

			if (onComplete)
				onComplete();

			return FLASH_OK;
		}

		Adesto::Status AT45::bufferLoad(SRAMBuffer bufferNumber, uint16_t startAddress, uint8_t* dataIn, size_t len, func_t onComplete)
		{
			/* Mask off the first 9 or 8 bits */
			startAddress &= (pageSize == STANDARD_PAGE_SIZE) ? 0x1FF : 0xFF;

			cmdBuffer[0] = (bufferNumber == BUFFER1) ? BUFFER1_WRITE : BUFFER2_WRITE;


			/* The full buffer write command is the opcode byte + 3 address bytes. The first address byte is 
			 * completely ignored and the last two hold the real address. Total TX length is four bytes.*/
			memcpy(cmdBuffer + 2, (uint8_t*)&startAddress, 2);
			SPI_write(cmdBuffer, 4, false);

			#if defined(USING_FREERTOS)
			/* Block so we don't trigger on multi TX. In reality this is a very small amount of time */
			while (!isWriteComplete()); 
			#endif 

			/* FREERTOS: Safe to return immediately because data pointer is out of function scope */
			SPI_write(dataIn, len, true);

			if (onComplete)
				onComplete();

			return FLASH_OK;
		}

		Adesto::Status AT45::bufferWrite(SRAMBuffer bufferNumber, uint16_t pageNumber, bool erase, func_t onComplete)
		{
			if (erase)
				cmdBuffer[0] = (bufferNumber == BUFFER1) ? BUFFER1_TO_MAIN_MEM_PAGE_PGM_W_ERASE : BUFFER2_TO_MAIN_MEM_PAGE_PGM_W_ERASE;
			else
				cmdBuffer[0] = (bufferNumber == BUFFER1) ? BUFFER1_TO_MAIN_MEM_PAGE_PGM_WO_ERASE : BUFFER2_TO_MAIN_MEM_PAGE_PGM_WO_ERASE;

			memcpy(cmdBuffer + 1, buildAddressCommand(PAGE, pageNumber), addressBytes);
			SPI_write(cmdBuffer, 4, true);

			if (onComplete)
				onComplete();

			return FLASH_OK;
		}

		Adesto::Status AT45::pageWrite(SRAMBuffer bufferNumber, uint16_t bufferOffset, uint16_t pageNumber, uint8_t* dataIn, size_t len, func_t onComplete)
		{
			/* Build up the main memory page address and SRAM buffer byte offset */
			const AddressDescriptions* config = (pageSize == STANDARD_PAGE_SIZE) ? &addressFormat[device].page.standardSize : &addressFormat[device].page.binarySize;
			uint32_t addressBitMask = (1u << config->addressBits) - 1u;
			uint32_t offsetBitMask = (1u << config->dummyBitsLSB) - 1u;
			uint32_t fullAddress = ((pageNumber & addressBitMask) << config->dummyBitsLSB) | (offsetBitMask & bufferOffset);
			memcpy(cmdBuffer + 1, (uint8_t*)&fullAddress, addressBytes);

			cmdBuffer[0] = (bufferNumber == BUFFER1) ? MAIN_MEM_PAGE_PGM_THR_BUFFER1_W_ERASE : MAIN_MEM_PAGE_PGM_THR_BUFFER2_W_ERASE;

			SPI_write(cmdBuffer, 4, false);

			#if defined(USING_FREERTOS)
			/* Block so we don't trigger on multi TX. In reality this is a very small amount of time */
			while (!isWriteComplete());
			#endif 

			/* FREERTOS: Safe to return immediately because data pointer is out of function scope */
			SPI_write(dataIn, len, true);

			if (onComplete)
				onComplete();

			return FLASH_OK;
		}

		Adesto::Status AT45::byteWrite(uint16_t bufferOffset, uint16_t pageNumber, uint8_t* dataIn, size_t len, func_t onComplete)
		{
			/* Build up the main memory page address and SRAM buffer byte offset */
			const AddressDescriptions* config = (pageSize == STANDARD_PAGE_SIZE) ? &addressFormat[device].page.standardSize : &addressFormat[device].page.binarySize;
			uint32_t addressBitMask = (1u << config->addressBits) - 1u;
			uint32_t offsetBitMask = (1u << config->dummyBitsLSB) - 1u;
			uint32_t fullAddress = ((pageNumber & addressBitMask) << config->dummyBitsLSB) | (offsetBitMask & bufferOffset);
			memcpy(cmdBuffer + 1, (uint8_t*)&fullAddress, addressBytes);

			cmdBuffer[0] = MAIN_MEM_BP_PGM_THR_BUFFER1_WO_ERASE;

			SPI_write(cmdBuffer, 4, false);

			#if defined(USING_FREERTOS)
			/* Block so we don't trigger on multi TX. In reality this is a very small amount of time */
			while (!isWriteComplete());
			#endif 

			/* FREERTOS: Safe to return immediately because data pointer is out of function scope */
			SPI_write(dataIn, len, true);

			if (onComplete)
				onComplete();

			return FLASH_OK;
		}

		Adesto::Status AT45::erase(uint32_t address, size_t len, func_t onComplete)
		{
			if (len)
			{
				/* Check alignment */
				if (address % pageSize) return ERROR_ADDRESS_NOT_PAGE_ALIGNED;
				if (len % pageSize)		return ERROR_LENGTH_NOT_PAGE_ALIGNED;

				/* Break the address range down into erasable modules */
				if (len >= sectorSize)
				{
					size_t numSectors = len / sectorSize;

					eraseRange_Sectors.start = getSectionNumberFromAddress(SECTOR, address);
					eraseRange_Sectors.end = eraseRange_Sectors.start + numSectors - 1;
					eraseRange_Sectors.rangeValid = true;

					len -= sectorSize * numSectors;
					address += sectorSize * numSectors;
				}

				if (len >= blockSize)
				{
					size_t numBlocks = len / blockSize;

					eraseRange_Blocks.start = getSectionNumberFromAddress(BLOCK, address);
					eraseRange_Blocks.end = eraseRange_Blocks.start + numBlocks - 1;
					eraseRange_Blocks.rangeValid = true;

					len -= blockSize * numBlocks;
					address += blockSize * numBlocks;
				}

				if (len >= pageSize)
				{
					size_t numPages = len / pageSize;

					eraseRange_Pages.start = getSectionNumberFromAddress(PAGE, address);
					eraseRange_Pages.end = eraseRange_Pages.start + numPages - 1;
					eraseRange_Pages.rangeValid = true;
				}
			}
			else
				return ERROR_ERASE_LENGTH_INVALID;
			
			return eraseRanges(onComplete);
		}

		Adesto::Status AT45::eraseChip()
		{
			uint32_t cmd = CHIP_ERASE;
			memcpy(cmdBuffer, (uint8_t*)&cmd, BYTE_LEN(cmd));

			SPI_write(cmdBuffer, BYTE_LEN(cmd), true);

			return FLASH_OK;
		}

		uint16_t AT45::getPageSizeConfig()
		{
			auto reg = readStatusRegister();
			return (reg & PAGE_SIZE_CONFIG) ? 256 : 264;
		}

		uint16_t AT45::readStatusRegister()
		{
			uint8_t reg[2];

			cmdBuffer[0] = STATUS_REGISTER_READ;

			SPI_write(cmdBuffer, BYTE_LEN(STATUS_REGISTER_READ), false);
			
			#if defined(USING_FREERTOS)
			while (!isWriteComplete());
			#endif 

			SPI_read(reg, 2, true);

			#if defined(USING_FREERTOS)
			while (!isReadComplete());
			#endif 

			return (uint16_t)((reg[0] << 8) | reg[1]);
		}

		bool AT45::isDeviceReady()
		{
			auto reg = readStatusRegister();
			return (reg & READY_BUSY);
		}

		bool AT45::isErasePgmError()
		{
			auto reg = readStatusRegister();
			return (reg & ERASE_PGM_ERROR);
		}

		bool AT45::isProgramComplete()
		{

		}

		bool AT45::isEraseComplete()
		{

		}

		AT45xx_DeviceInfo AT45::getDeviceInfo()
		{
			uint8_t data[3];
			memset(data, 0, SIZE_OF_ARRAY(data));
			memset(cmdBuffer, 0, SIZE_OF_ARRAY(cmdBuffer));

			cmdBuffer[0] = READ_DEVICE_INFO;
			SPI_write(cmdBuffer, BYTE_LEN(READ_DEVICE_INFO), false);
			
			#if defined(USING_FREERTOS)
			while (!isWriteComplete());
			#endif
			
			SPI_read(data, 3, true);

			#if defined(USING_FREERTOS)
			while (!isReadComplete());
			#endif 

			info.manufacturerID = data[0];
			info.familyCode = static_cast<FamilyCode>((uint8_t)(data[1] >> 5) & 0xFF);
			info.densityCode = static_cast<DensityCode>(data[1] & 0x1F);
			info.subCode = static_cast<SubCode>((data[2] >> 5) & 0xFF);
			info.productVariant = static_cast<ProductVariant>(data[2] & 0x1F);

			return info;
		}

		#if defined(USING_FREERTOS)
		bool AT45::isReadComplete()
		{
			return xSemaphoreTake(singleTXRXWakeup, 0);
		}

		bool AT45::isWriteComplete()
		{
			return xSemaphoreTake(singleTXWakeup, 0);
		}
		#endif

		void AT45::SPI_write(uint8_t* data, size_t len, bool disableSS)
		{
			spi->write(data, len, disableSS);
		}

		void AT45::SPI_read(uint8_t* data, size_t len, bool disableSS)
		{
			//Use cmdBuffer array as source of dummy bytes for the TX/RX operation.
			spi->write(cmdBuffer, data, len, disableSS);
		}

		void AT45::useBinaryPageSize()
		{
			uint32_t cmd = CFG_PWR_2_PAGE_SIZE;
			memcpy(cmdBuffer, (uint8_t*)&cmd, BYTE_LEN(cmd));

			SPI_write(cmdBuffer, BYTE_LEN(cmd), true);

			//TODO: validate that the value is set 

			//Note: These values appear constant over all AT45 chips
			pageSize = 256;
			blockSize = 2048;
			sectorSize = 65536;
		}

		void AT45::useDataFlashPageSize()
		{
			uint32_t cmd = CFG_STD_FLASH_PAGE_SIZE;
			memcpy(cmdBuffer, (uint8_t*)&cmd, BYTE_LEN(cmd));

			SPI_write(cmdBuffer, BYTE_LEN(cmd), true);

			//TODO: validate that the value is set

			//Note: These values appear constant over all AT45 chips
			pageSize = 264;
			blockSize = 2112;
			sectorSize = 67584;
		}

		Adesto::Status AT45::eraseRanges(func_t onComplete)
		{
			//SECTOR ERASE
			if (eraseRange_Sectors.rangeValid)
			{
				for (int i = eraseRange_Sectors.start; i < eraseRange_Sectors.end + 1; i++)
				{
					eraseSector(i);

					while (!isDeviceReady())
					{
						//Delay the average time for a sector erase as specified by the datasheet 
						Chimera::delayMilliseconds(chipDelay[device].sectorErase);
					}

					if (isErasePgmError())
						return ERROR_ERASE_FAILURE;
				}

				eraseRange_Sectors.rangeValid = false;
			}

			//BLOCK ERASE
			if (eraseRange_Blocks.rangeValid)
			{
				for (int i = eraseRange_Blocks.start; i < eraseRange_Blocks.end + 1; i++)
				{
					eraseBlock(i);

					while (!isDeviceReady())
					{
						Chimera::delayMilliseconds(chipDelay[device].blockErase);
					}

					if (isErasePgmError())
						return ERROR_ERASE_FAILURE;
				}

				eraseRange_Blocks.rangeValid = false;
			}

			//PAGE ERASE
			if (eraseRange_Pages.rangeValid)
			{
				for (int i = eraseRange_Pages.start; i < eraseRange_Pages.end + 1; i++)
				{
					eraseBlock(i);

					while (!isDeviceReady())
					{
						Chimera::delayMilliseconds(chipDelay[device].pageErase);
					}

					if (isErasePgmError())
						return ERROR_ERASE_FAILURE;
				}

				eraseRange_Pages.rangeValid = false;
			}

			if (onComplete)
				onComplete();

			return FLASH_OK;
		}

		void AT45::eraseSector(uint32_t sectorNumber)
		{
			cmdBuffer[0] = SECTOR_ERASE;

			memcpy(cmdBuffer + 1, buildAddressCommand(SECTOR, sectorNumber), addressFormat[device].numAddressBytes);

			SPI_write(cmdBuffer, (BYTE_LEN(SECTOR_ERASE) + addressBytes), true);
		}

		void AT45::eraseBlock(uint32_t blockNumber)
		{
			cmdBuffer[0] = BLOCK_ERASE;

			memcpy(cmdBuffer + 1, buildAddressCommand(BLOCK, blockNumber), addressFormat[device].numAddressBytes);

			SPI_write(cmdBuffer, (BYTE_LEN(BLOCK_ERASE) + addressBytes), true);
		}

		void AT45::erasePage(uint32_t pageNumber)
		{
			cmdBuffer[0] = PAGE_ERASE;

			memcpy(cmdBuffer + 1, buildAddressCommand(PAGE, pageNumber), addressFormat[device].numAddressBytes);

			SPI_write(cmdBuffer, (BYTE_LEN(PAGE_ERASE) + addressBytes), true);
		}

		uint8_t* AT45::buildAddressCommand(FlashSection section, uint32_t sectionNumber)
		{
			uint32_t fullAddress = 0u;
			const AddressDescriptions* config;

			switch (section)
			{
			case PAGE:
				if (pageSize == STANDARD_PAGE_SIZE)
					config = &addressFormat[device].page.standardSize;
				else
					config = &addressFormat[device].page.binarySize;

				//Indicate that an incorrect address will be generated below
				if (sectionNumber >= chipSpecs[device].numPages)
					errorFlags |= ADDRESS_OVERRUN;
				break;

			case BLOCK:
				if (pageSize == STANDARD_PAGE_SIZE)
					config = &addressFormat[device].block.standardSize;
				else
					config = &addressFormat[device].block.binarySize;

				if (sectionNumber >= chipSpecs[device].numBlocks)
					errorFlags |= ADDRESS_OVERRUN;
				break;

			case SECTOR:
				if (sectionNumber == 0)
				{
					if (pageSize == STANDARD_PAGE_SIZE)
						config = &addressFormat[device].sector0ab.standardSize;
					else
						config = &addressFormat[device].sector0ab.binarySize;
				}
				else
				{
					if (pageSize == STANDARD_PAGE_SIZE)
						config = &addressFormat[device].sector.standardSize;
					else
						config = &addressFormat[device].sector.binarySize;
				}

				if (sectionNumber >= chipSpecs[device].numSectors)
					errorFlags |= ADDRESS_OVERRUN;
				break;
			};

			// This ignores Sector 0a for simplicity reasons. The full address below directly corresponds
			// to Sector 0b, and the format seems common across all AT45 chips. Use Block 0 to get the address for Sector 0a.
			if ((section == SECTOR) && (sectionNumber == 0))
			{
				fullAddress = 1u << config->dummyBitsLSB;
			}
			else
			{
				uint32_t bitMask = (1u << config->addressBits) - 1u;
				fullAddress = (sectionNumber & bitMask) << config->dummyBitsLSB;
			}

			memcpy(memoryAddress, (uint8_t*)&fullAddress, addressFormat[device].numAddressBytes);
			
			return memoryAddress;
		}

		uint32_t AT45::getSectionNumberFromAddress(FlashSection section, uint32_t rawAddress)
		{
			uint32_t sectionNumber = 0;

			/* Grab the lowest whole number page first. If any remainder, it means we are in the middle of a page */
			switch (section)
			{
			case PAGE:
				sectionNumber = rawAddress / pageSize;
				if ((rawAddress % pageSize) > 0) sectionNumber += 1;

				if (sectionNumber >= chipSpecs[device].numPages)
					errorFlags |= INVALID_SECTION_NUMBER;
				break;

			case BLOCK:
				sectionNumber = rawAddress / blockSize;
				if ((rawAddress % blockSize) > 0) sectionNumber += 1;

				if (sectionNumber >= chipSpecs[device].numBlocks)
					errorFlags |= INVALID_SECTION_NUMBER;
				break;

			case SECTOR: //Does not differentiate between Sector 0a/0b and will return 0 for both address ranges.
				sectionNumber = rawAddress / sectorSize;
				if ((rawAddress % sectorSize) > 0) sectionNumber += 1;

				if (sectionNumber >= chipSpecs[device].numSectors)
					errorFlags |= INVALID_SECTION_NUMBER;
				break;
			};

			return sectionNumber;
		}
	}
}