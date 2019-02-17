/********************************************************************************
*   File Name:
*       at45db081.cpp
*
*   Description:
*       Driver for the AT45DB NOR flash chip series from Adesto
*
*   2018-2019 | Brandon Braun | brandonbraun653@gmail.com
********************************************************************************/

/* C/C++ Includes */
#include <memory>

/* Chimera Includes */
#include <Chimera/logging.hpp>
#include <Chimera/utilities.hpp>

/* Driver Includes */
#include "at45db081.hpp"

using namespace Chimera::SPI;
using namespace Chimera::Logging;
using namespace Chimera::Modules::Memory;

#define BYTE_LEN(x) (sizeof(x)/sizeof(uint8_t))
static constexpr uint16_t STANDARD_PAGE_SIZE = 264;
static constexpr uint16_t BINARY_PAGE_SIZE = 256;
static constexpr uint8_t ADDRESS_OVERRUN = (1u << 0);
static constexpr uint8_t INVALID_SECTION_NUMBER	 = (1u << 1);

struct FlashSizes
{
	size_t numSectors = 0;
	size_t numBlocks = 0;
	size_t numPages = 0;
};

struct AddressDescriptions
{
	uint8_t dummyBitsMSB = 0;
	uint8_t addressBits = 0;
	uint8_t dummyBitsLSB = 0;
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

struct FlashDelay
{
	uint8_t pageEraseAndProgramming;
	uint8_t pageProgramming;
	uint8_t pageErase;
	uint8_t blockErase;
	uint16_t sectorErase;
	uint16_t chipErase;
};

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


		AT45::AT45(const FlashChip chip, const int& spiChannel)
		{
			device = chip;
			addressBytes = addressFormat[device].numAddressBytes;

			spi = std::make_unique<SPIClass>(spiChannel);

			setup.clockFrequency = 1000000;
            setup.bitOrder = BitOrder::MSB_FIRST;
            setup.clockMode = ClockMode::MODE0;
            setup.dataSize = DataSize::SZ_8BIT;
            setup.mode = Mode::MASTER;

    		cmdBuffer.fill(0);


			#if defined(REPLACE_ME_WITH_CHIMERA_FREERTOS)
			multiTXWakeup = xSemaphoreCreateBinary();
			singleTXWakeup = xSemaphoreCreateBinary();
			singleTXRXWakeup = xSemaphoreCreateBinary();
			#endif
		}

		Adesto::Status AT45::initialize(const uint32_t userClockFreq)
		{
            if (spi->init(setup) != Chimera::SPI::Status::OK)
				return ERROR_SPI_INIT_FAILED;

			#if defined(REPLACE_ME_WITH_CHIMERA_FREERTOS)
			spi->setMode(TXRX, DMA);
			spi->attachThreadTrigger(BUFFERED_TXRX_COMPLETE, &multiTXWakeup);
			spi->attachThreadTrigger(TX_COMPLETE, &singleTXWakeup);
			spi->attachThreadTrigger(TXRX_COMPLETE, &singleTXRXWakeup);
			#else
            spi->setPeripheralMode(Chimera::SPI::SubPeripheral::TXRX, Chimera::SPI::SubPeripheralMode::BLOCKING);
			#endif

			/*--------------------------------------
			 * Check for a proper device connection:
			 *	1) Get the manufacturer id at low freq (~1MHz for stability)
			 *	2) Retry again at the user requested frequency
			 *-------------------------------------*/
			auto lowFreqInfo = getDeviceInfo();
			if (info.manufacturerID != JEDEC_CODE)
				return ERROR_UNKNOWN_JEDEC_CODE;

			spi->setClockFrequency(userClockFreq);
			auto hiFreqInfo = getDeviceInfo();

			if (memcmp(&lowFreqInfo, &hiFreqInfo, sizeof(AT45xx_DeviceInfo)) != 0)
				return ERROR_FAILED_HIGH_FREQUENCY_TRANSACTION;

			/*--------------------------------------
			* Initialize various other settings
			*-------------------------------------*/
			spi->getClockFrequency(&clockFrequency);

			float clockErr = abs((float)clockFrequency - (float)userClockFreq) / (float)userClockFreq;
			if (clockErr > maxClockErr)
			{
				Console.log(Level::LVL_WARN, "Flash chip SPI clock freq not met. Tried: %.5f MHz, Got: %.5f MHz\r\n",
					((float)userClockFreq / 1000000.0f), ((float)clockFrequency / 1000000.0f));
			}

			return useBinaryPageSize();
		}

		Adesto::Status AT45::directPageRead(const uint16_t pageNumber, const uint16_t pageOffset, uint8_t *const dataOut, const size_t len, func_t onComplete /*= nullptr*/)
		{
			cmdBuffer[0] = MAIN_MEM_PAGE_READ;
			buildReadWriteCommand(pageNumber, pageOffset);

    		/*------------------------------------------------
            The command is comprised of an opcode (1 byte), an address (3 bytes), and 4 dummy bytes.
            The dummy bytes are used to initialize the read operation.
            ------------------------------------------------*/
			SPI_write(cmdBuffer.cbegin(), 8, false);
			SPI_read(dataOut, len, true);

    		if (onComplete)
    		{
        		onComplete();
    		}

			return FLASH_OK;
		}

		Adesto::Status AT45::bufferRead(const SRAMBuffer bufferNumber, const uint16_t startAddress, uint8_t *const dataOut, const size_t len, func_t onComplete /*= nullptr*/)
		{
			if(clockFrequency > 50000000)	//50 MHz
				cmdBuffer[0] = (bufferNumber == BUFFER1) ? BUFFER1_READ_HF : BUFFER2_READ_HF;
			else
				cmdBuffer[0] = (bufferNumber == BUFFER1) ? BUFFER1_READ_LF : BUFFER2_READ_LF;

			SPI_write(cmdBuffer.cbegin(), 5, false);
			SPI_read(dataOut, len, true);

			if (onComplete)
				onComplete();

			return FLASH_OK;
		}

		Adesto::Status AT45::bufferLoad(const SRAMBuffer bufferNumber, const uint16_t startAddress, const uint8_t *const dataIn, const size_t len, func_t onComplete /*= nullptr*/)
		{
			/* In this case, the page number input is ignored by the flash chip. Only the offset within the buffer is valid.
			 * See datasheet section labeled 'Buffer Write' for more details. */
			cmdBuffer[0] = (bufferNumber == BUFFER1) ? BUFFER1_WRITE : BUFFER2_WRITE;
			buildReadWriteCommand(0x0000, startAddress);

			SPI_write(cmdBuffer.cbegin(), 4, false);
			SPI_write(dataIn, len, true);

			if (onComplete)
				onComplete();

			return FLASH_OK;
		}

		Adesto::Status AT45::bufferWrite(const SRAMBuffer bufferNumber, const uint16_t pageNumber, const bool erase, func_t onComplete /*= nullptr*/)
		{
			if (erase)
				cmdBuffer[0] = (bufferNumber == BUFFER1) ? BUFFER1_TO_MAIN_MEM_PAGE_PGM_W_ERASE : BUFFER2_TO_MAIN_MEM_PAGE_PGM_W_ERASE;
			else
				cmdBuffer[0] = (bufferNumber == BUFFER1) ? BUFFER1_TO_MAIN_MEM_PAGE_PGM_WO_ERASE : BUFFER2_TO_MAIN_MEM_PAGE_PGM_WO_ERASE;

			/* This command is opposite of 'bufferLoad()'. Only the page number is valid and then offset is ignored.
			 * See datasheet section labeled 'Buffer to Main Memory Page Program with/without Built-In Erase' for more details. */
			buildReadWriteCommand(pageNumber, 0x0000);

			SPI_write(cmdBuffer.cbegin(), 4, true);

			if (onComplete)
				onComplete();

			return FLASH_OK;
		}

		Adesto::Status AT45::pageWrite(const SRAMBuffer bufferNumber, const uint16_t bufferOffset, const uint16_t pageNumber, const uint8_t *const dataIn, const size_t len, func_t onComplete /*= nullptr*/)
		{
			/* Generate the full command sequence. See data sheet section labeled 'Main Memory Page Program through Buffer with Built-In Erase' */
			cmdBuffer[0] = (bufferNumber == BUFFER1) ? MAIN_MEM_PAGE_PGM_THR_BUFFER1_W_ERASE : MAIN_MEM_PAGE_PGM_THR_BUFFER2_W_ERASE;
			buildReadWriteCommand(pageNumber, bufferOffset);

			SPI_write(cmdBuffer.cbegin(), 4, false);
			SPI_write(dataIn, len, true);

			if (onComplete)
				onComplete();

			return FLASH_OK;
		}

		Adesto::Status AT45::readModifyWriteManual(const SRAMBuffer bufferNumber, const uint16_t pageNumber, const uint16_t pageOffset, const uint8_t *const dataIn, const size_t len, func_t onComplete /*= nullptr*/)
		{
			Adesto::Status errorCode = FLASH_OK;

			uint8_t tempBuff[STANDARD_PAGE_SIZE];
			memset(tempBuff, 0xFF, STANDARD_PAGE_SIZE);

			/* Read the data from the current page */
			directPageRead(pageNumber, 0x0000, tempBuff, pageSize);

			while (!isReadComplete())
			{
				Chimera::delayMilliseconds(1);
			}

			/* Modify the requested data */
			memcpy(tempBuff + pageOffset, dataIn, len);

			/* Write the modified data back with built in erase */
			pageWrite(bufferNumber, 0x0000, pageNumber, tempBuff, pageSize);

			if (onComplete)
				onComplete();

			return errorCode;
		}

		Adesto::Status AT45::readModifyWrite(SRAMBuffer bufferNumber, uint16_t pageNumber, uint16_t pageOffset, uint8_t* dataIn, size_t len, func_t onComplete)
		{
			/* Generate the full command sequence. See datasheet section labeled 'Read-Modify-Write' */
			cmdBuffer[0] = (bufferNumber == BUFFER1) ? AUTO_PAGE_REWRITE1 : AUTO_PAGE_REWRITE2;
			buildReadWriteCommand(pageNumber, pageOffset);

			SPI_write(cmdBuffer.cbegin(), 4, false);
			SPI_write(dataIn, len, true);

			if (onComplete)
				onComplete();

			return FLASH_OK;
		}

		Adesto::Status AT45::byteWrite(const uint16_t pageNumber, const uint16_t pageOffset, const uint8_t *const dataIn, const size_t len, func_t onComplete /*= nullptr*/)
		{
			/* Generates the full command sequence. See datasheet section labeled 'Main Memory Byte/Page Program through Buffer 1 without Built-In Erase' */
			cmdBuffer[0] = MAIN_MEM_BP_PGM_THR_BUFFER1_WO_ERASE;
			buildReadWriteCommand(pageNumber, pageOffset);

			SPI_write(cmdBuffer.cbegin(), 4, false);
			SPI_write(dataIn, len, true);

			if (onComplete)
				onComplete();

			return FLASH_OK;
		}

		Adesto::Status AT45::write(const uint32_t address, const uint8_t *const dataIn, const size_t len, func_t onComplete)
		{
			if (len)
			{
				size_t currentByte = 0;
				size_t writeLen = 0;
				MemoryRange range = getWriteReadPages(address, len);

				/* Handles the first, likely partial page */
				writeLen = pageSize - range.page.startPageOffset;

				if (len < writeLen)
					writeLen = len;

				readModifyWriteManual(BUFFER1, range.page.start, range.page.startPageOffset, dataIn, writeLen);

				while (!isDeviceReady())
				{
					Chimera::delayMilliseconds(chipDelay[device].pageProgramming);
				}
				currentByte += writeLen;

				/* Ensure there were no programming errors */
				if (isErasePgmError())
					return ERROR_WRITE_FAILURE;

				/* Handles intermediate, full pages */
				for (int i = range.page.start + 1; i < range.page.end; i++)
				{
					pageWrite(BUFFER1, 0, i, (dataIn + currentByte), pageSize);

					while (!isDeviceReady())
					{
						Chimera::delayMilliseconds(chipDelay[device].pageProgramming);
					}

					currentByte += pageSize;

					if (isErasePgmError())
						return ERROR_WRITE_FAILURE;
				}

				/* Handles the last, likely partial page */
				if ((range.page.start != range.page.end) && (range.page.endPageOffset != 0))
				{
					readModifyWriteManual(BUFFER1, range.page.end, 0, (dataIn + currentByte), range.page.endPageOffset);

					while (!isDeviceReady())
					{
						Chimera::delayMilliseconds(chipDelay[device].pageProgramming);
					}

					if (isErasePgmError())
						return ERROR_WRITE_FAILURE;
				}

				return FLASH_OK;
			}
			else
				return ERROR_WRITE_LENGTH_INVALID;
		}

		Adesto::Status AT45::read(uint32_t address, uint8_t* dataOut, size_t len, func_t onComplete)
		{
			if (len)
			{
				/* Calculate the correct starting page and offset to begin reading from in memory */
				MemoryRange range = getWriteReadPages(address, len);
				uint32_t pageNumber = range.page.start;
				uint16_t pageOffset = range.page.startPageOffset;

				buildReadWriteCommand(pageNumber, pageOffset);

				/* The command is comprised of an opcode (1 byte), an address (3 bytes), and X dummy bytes.
				 * The dummy bytes are used to initialize the read operation for higher frequencies */
				size_t numDummyBytes = 0;

				if(clockFrequency > 50000000) //50MHz
					cmdBuffer[0] = CONT_ARR_READ_HF1;
				else
					cmdBuffer[0] = CONT_ARR_READ_LF;

				switch (cmdBuffer[0])
				{
				case CONT_ARR_READ_HF1:
					numDummyBytes = 1;
					break;

				case CONT_ARR_READ_HF2:
					numDummyBytes = 2;
					break;

				default:
					numDummyBytes = 0;
					break;
				}

				SPI_write(cmdBuffer.cbegin(), 4 + numDummyBytes, false);
				SPI_read(dataOut, len, true);

				if (onComplete)
					onComplete();

				return FLASH_OK;
			}
			else
				return ERROR_READ_LENGTH_INVALID;
		}

		Adesto::Status AT45::erase(const uint32_t address, const size_t len, func_t onComplete /*= nullptr*/)
		{
			if (len)
			{
				/* Erase functionality is forced to be page aligned at a minimum */
				if (address % pageSize) return ERROR_ADDRESS_NOT_PAGE_ALIGNED;
				if (len % pageSize)		return ERROR_LENGTH_NOT_PAGE_ALIGNED;

				return eraseRanges(getErasableSections(address, len), onComplete);
			}
			else
				return ERROR_ERASE_LENGTH_INVALID;
		}

		Adesto::Status AT45::eraseChip()
		{
			uint32_t cmd = CHIP_ERASE;
			memcpy(cmdBuffer.begin(), (uint8_t*)&cmd, sizeof(cmd));

			SPI_write(cmdBuffer.cbegin(), sizeof(cmd), true);

			return FLASH_OK;
		}

		uint16_t AT45::getPageSizeConfig()
		{
			auto reg = readStatusRegister();
			return (reg & PAGE_SIZE_CONFIG_Pos) ? 256 : 264;
		}

		uint16_t AT45::readStatusRegister(StatusRegister *const reg /*= nullptr*/)
		{
			uint8_t val[2];

			cmdBuffer[0] = STATUS_REGISTER_READ;

			SPI_write(cmdBuffer.cbegin(), BYTE_LEN(STATUS_REGISTER_READ), false);
			SPI_read(val, 2, true);

			uint16_t tmp = (uint16_t)((val[0] << 8) | val[1]);

			if (reg)
			{
				reg->compareResult				= tmp & COMPARE_RESULT_Pos;
				reg->deviceReady				= tmp & READY_BUSY_Pos;
				reg->eraseProgramError			= tmp & ERASE_PGM_ERROR_Pos;
				reg->eraseSuspend				= tmp & ERASE_SUSPEND_Pos;
				reg->pageSizeConfig				= tmp & PAGE_SIZE_CONFIG_Pos;
				reg->pgmSuspendStatusB1			= tmp & BUFF1_PGM_SUSPEND_Pos;
				reg->pgmSuspendStatusB2			= tmp & BUFF2_PGM_SUSPEND_Pos;
				reg->sectorLockdownEnabled		= tmp & SECTOR_LOCKDOWN_EN_Pos;
				reg->sectorProtectionStatus		= tmp & SECTOR_PROTECTION_Pos;
			}

			return tmp;
		}

		bool AT45::isDeviceReady(StatusRegister *const reg /*= nullptr*/)
		{
			auto val = readStatusRegister(reg);
			return (val & READY_BUSY_Pos);
		}

		bool AT45::isErasePgmError(StatusRegister *const reg /*= nullptr*/)
		{
			auto val = readStatusRegister(reg);
			return (val & ERASE_PGM_ERROR_Pos);
		}

		bool AT45::isReadComplete()
		{
			//If not using freeRTOS, spi is in blocking mode. Any read will be complete before calling this function.
			#if defined(REPLACE_ME_WITH_CHIMERA_FREERTOS)
			if ((xSemaphoreTake(singleTXRXWakeup, 0) == pdPASS) || readComplete)
			{
				readComplete = true;
				return true;
			}
			else
				return false;
			#else
			return true;
			#endif
		}

		bool AT45::isWriteComplete()
		{
			//If not using freeRTOS, spi is in blocking mode. Any write will be complete before calling this function.
			#if defined(REPLACE_ME_WITH_CHIMERA_FREERTOS)
			if ((xSemaphoreTake(singleTXWakeup, 0) == pdPASS) || writeComplete)
			{
				writeComplete = true;
				return true;
			}
			else
				return false;
			#else
			return true;
			#endif
		}

		Adesto::Status AT45::useBinaryPageSize()
		{
			uint32_t cmd = CFG_PWR_2_PAGE_SIZE;
			memcpy(cmdBuffer.begin(), (uint8_t*)&cmd, BYTE_LEN(cmd));

			SPI_write(cmdBuffer.cbegin(), BYTE_LEN(cmd), true);

			//Note: These values appear constant over all AT45 chips
			pageSize = 256;
			blockSize = 2048;
			sectorSize = 65536;

			if (pageSize != getPageSizeConfig())
				return ERROR_PAGE_SIZE_MISMATCH;

			return FLASH_OK;
		}

		Adesto::Status AT45::useDataFlashPageSize()
		{
			uint32_t cmd = CFG_STD_FLASH_PAGE_SIZE;
			memcpy(cmdBuffer.begin(), (uint8_t*)&cmd, BYTE_LEN(cmd));

			SPI_write(cmdBuffer.cbegin(), BYTE_LEN(cmd), true);

			//Note: These values appear constant over all AT45 chips
			pageSize = 264;
			blockSize = 2112;
			sectorSize = 67584;

			if (pageSize != getPageSizeConfig())
				return ERROR_PAGE_SIZE_MISMATCH;

			return FLASH_OK;
		}

		AT45xx_DeviceInfo AT45::getDeviceInfo()
		{
    		std::array<uint8_t, 3> data;
    		data.fill(0);
    		cmdBuffer.fill(0);

			cmdBuffer[0] = READ_DEVICE_INFO;
			SPI_write(cmdBuffer.cbegin(), BYTE_LEN(READ_DEVICE_INFO), false);
			SPI_read(data.begin(), data.size(), true);

			info.manufacturerID = data[0];
			info.familyCode = static_cast<FamilyCode>((uint8_t)(data[1] >> 5) & 0xFF);
			info.densityCode = static_cast<DensityCode>(data[1] & 0x1F);
			info.subCode = static_cast<SubCode>((data[2] >> 5) & 0xFF);
			info.productVariant = static_cast<ProductVariant>(data[2] & 0x1F);

			return info;
		}

		void AT45::SPI_write(const uint8_t *const data, const size_t len, const bool disableSS)
		{
			writeComplete = false;
			spi->writeBytes(data, len, disableSS);

			#if defined(REPLACE_ME_WITH_CHIMERA_FREERTOS)
			/* Block so we don't trigger on multi TX. In reality this is a very small amount of time */
			while (!isWriteComplete());
			#endif
		}

		void AT45::SPI_read(uint8_t *const data, const size_t len, const bool disableSS)
		{
			readComplete = false;

			//Use cmdBuffer array as source of dummy bytes for the TX/RX operation.
			spi->readBytes(data, len, disableSS);

			#if defined(REPLACE_ME_WITH_CHIMERA_FREERTOS)
			while (!isReadComplete());
			#endif
		}

		AT45::MemoryRange AT45::getErasableSections(uint32_t address, size_t len)
		{
			MemoryRange range;

			/* Break the address range down into erasable modules */
			if (len >= sectorSize)
			{
				size_t numSectors = len / sectorSize;

				range.sector.start = getSectionFromAddress(SECTOR, address);
				range.sector.end = range.sector.start + numSectors - 1;

				len -= sectorSize * numSectors;
				address += sectorSize * numSectors;
			}

			if (len >= blockSize)
			{
				size_t numBlocks = len / blockSize;

				range.block.start = getSectionFromAddress(BLOCK, address);
				range.block.end = range.block.start + numBlocks - 1;

				len -= blockSize * numBlocks;
				address += blockSize * numBlocks;
			}

			if (len >= pageSize)
			{
				size_t numPages = len / pageSize;

				range.page.start = getSectionFromAddress(PAGE, address);
				range.page.end = range.page.start + numPages - 1;
			}

			return range;
		}

		AT45::MemoryRange AT45::getWriteReadPages(const uint32_t startAddress, const size_t len)
		{
			MemoryRange range;
			uint32_t endAddress = startAddress + len - 1;

			//Page number that contains the desired starting address
			range.page.start = getSectionFromAddress(PAGE, startAddress);

			//Address is guaranteed to be >= the section starting address
			range.page.startPageOffset = startAddress - getSectionStartAddress(PAGE, range.page.start);

			//Page number that we finish in
			range.page.end = getSectionFromAddress(PAGE, endAddress);

			//Address is guaranteed to be >= the ending section start address
			range.page.endPageOffset = endAddress - getSectionStartAddress(PAGE, range.page.end);

			return range;
		}

		Adesto::Status AT45::eraseRanges(const MemoryRange range, func_t onComplete /*= nullptr*/)
		{
			//SECTOR ERASE
			for (int i = range.sector.start; i < range.sector.end + 1; i++)
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

			//BLOCK ERASE
			for (int i = range.block.start; i < range.block.end + 1; i++)
			{
				eraseBlock(i);

				while (!isDeviceReady())
				{
					Chimera::delayMilliseconds(chipDelay[device].blockErase);
				}

				if (isErasePgmError())
					return ERROR_ERASE_FAILURE;
			}

			//PAGE ERASE
			for (int i = range.page.start; i < range.page.end + 1; i++)
			{
				eraseBlock(i);

				while (!isDeviceReady())
				{
					Chimera::delayMilliseconds(chipDelay[device].pageErase);
				}

				if (isErasePgmError())
					return ERROR_ERASE_FAILURE;
			}

			if (onComplete)
				onComplete();

			return FLASH_OK;
		}

		void AT45::eraseSector(const uint32_t sectorNumber)
		{
			cmdBuffer[0] = SECTOR_ERASE;
			buildEraseCommand(SECTOR, sectorNumber);

			SPI_write(cmdBuffer.cbegin(), (BYTE_LEN(SECTOR_ERASE) + addressBytes), true);
		}

		void AT45::eraseBlock(const uint32_t blockNumber)
		{
			cmdBuffer[0] = BLOCK_ERASE;
			buildEraseCommand(BLOCK, blockNumber);

			SPI_write(cmdBuffer.cbegin(), (BYTE_LEN(BLOCK_ERASE) + addressBytes), true);
		}

		void AT45::erasePage(const uint32_t pageNumber)
		{
			cmdBuffer[0] = PAGE_ERASE;
			buildEraseCommand(PAGE, pageNumber);

			SPI_write(cmdBuffer.cbegin(), (BYTE_LEN(PAGE_ERASE) + addressBytes), true);
		}

		void AT45::buildReadWriteCommand(const uint16_t pageNumber, const uint16_t offset /*= 0x0000*/)
		{
			/* Grab the correct page configuration. This informs the code how much bit shifting to apply */
			const AddressDescriptions* config;
			if (pageSize == STANDARD_PAGE_SIZE)
				config = &addressFormat[device].page.standardSize;
			else
				config = &addressFormat[device].page.binarySize;

			/* Generate masks of the correct bit width to clean up the input variables */
			uint32_t addressBitMask = (1u << config->addressBits) - 1u;
			uint32_t offsetBitMask  = (1u << config->dummyBitsLSB) - 1u;

			/*	The full address is really only 3 bytes wide. They are set up as follows, with 'a' == address bit,
			 *	'o' == offset bit and 'x' == don't care. This is the exact order in which it must be transmitted. (ie MSB first)
			 *								 Byte 1 | Byte 2 | Byte 3
			 *		For 264 byte page size: xxxaaaaa|aaaaaaao|oooooooo
			 *		For 256 byte page size: xxxxaaaa|aaaaaaaa|oooooooo
			 */
			uint32_t fullAddress = ((pageNumber & addressBitMask) << config->dummyBitsLSB) | (offsetBitMask & offset);


			/*	Note: Cannot use memcpy because it reverses the byte order expected by the flash chip.
			 *	For example, if the value of 'fullAddress' were 0xAABBCC, the memcpy would put the values into the cmdBuffer as 0xCCBBAA.
			 *	This is correct as far as the MCU is concerned, but the flash chip needs the data exactly as calculated: 0xAABBCC
			 */
			cmdBuffer[1] = (fullAddress & 0xFF0000) >> 16;
			cmdBuffer[2] = (fullAddress & 0x00FF00) >> 8;
			cmdBuffer[3] =  fullAddress & 0x0000FF;
		}

		void AT45::buildEraseCommand(const FlashSection section, const uint32_t sectionNumber)
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

			/*	Note: Cannot use memcpy because it reverses the byte order expected by the flash chip.
			*	For example, if the value of 'fullAddress' were 0xAABBCC, the memcpy would put the values into the cmdBuffer as 0xCCBBAA.
			*	This is correct as far as the MCU is concerned, but the flash chip needs the data exactly as calculated: 0xAABBCC
			*/
			cmdBuffer[1] = (fullAddress & 0xFF0000) >> 16;
			cmdBuffer[2] = (fullAddress & 0x00FF00) >> 8;
			cmdBuffer[3] = fullAddress & 0x0000FF;
		}

		uint32_t AT45::getSectionFromAddress(const FlashSection section, const uint32_t rawAddress)
		{
			uint32_t sectionNumber = 0;

			switch (section)
			{
			case PAGE:
				sectionNumber = rawAddress / pageSize;

				if (sectionNumber >= chipSpecs[device].numPages)
					errorFlags |= INVALID_SECTION_NUMBER;
				break;

			case BLOCK:
				sectionNumber = rawAddress / blockSize;

				if (sectionNumber >= chipSpecs[device].numBlocks)
					errorFlags |= INVALID_SECTION_NUMBER;
				break;

			case SECTOR: //Does not differentiate between Sector 0a/0b and will return 0 for both address ranges.
				sectionNumber = rawAddress / sectorSize;

				if (sectionNumber >= chipSpecs[device].numSectors)
					errorFlags |= INVALID_SECTION_NUMBER;
				break;
			};

			return sectionNumber;
		}

		uint32_t AT45::getSectionStartAddress(const FlashSection section, const uint32_t sectionNumber)
		{
			uint32_t address = 0u;

			switch (section)
			{
			case SECTOR:
				address = sectionNumber * sectorSize;
				break;

			case BLOCK:
				address = sectionNumber * blockSize;
				break;

			case PAGE:
				address = sectionNumber * pageSize;
				break;
			};

			return address;
		}

		uint32_t AT45::getFlashSize()
		{
			switch (info.densityCode)
			{
			case DENSITY_2MBIT:
				return 262144;
				break;

			case DENSITY_4MBIT:
				return 524288;
				break;

			case DENSITY_8MBIT:
				return 1048576;
				break;

			case DENSITY_16MBIT:
				return 2097152;
				break;

			case DENSITY_32MBIT:
				return 4194304;
				break;

			case DENSITY_64MBIT:
				return 8388608;
				break;
			}
		}


    	BlockStatus AT45::DiskOpen(const uint8_t volNum, BlockMode openMode)
    	{
            return BlockStatus::BLOCK_DEV_ENOSYS;
    	}

    	BlockStatus AT45::DiskClose(const uint8_t volNum)
    	{
            return BlockStatus::BLOCK_DEV_ENOSYS;
    	}

    	BlockStatus AT45::DiskRead(const uint8_t volNum, const uint64_t sectorStart, const uint32_t sectorCount, void *const readBuffer)
    	{
            return BlockStatus::BLOCK_DEV_ENOSYS;
    	}

    	BlockStatus AT45::DiskWrite(const uint8_t volNum, const uint64_t sectorStart, const uint32_t sectorCount, const void *const writeBuffer)
    	{
            return BlockStatus::BLOCK_DEV_ENOSYS;
    	}

    	BlockStatus AT45::DiskFlush(const uint8_t volNum)
    	{
        	return BlockStatus::BLOCK_DEV_ENOSYS;
    	}


	}   /* Namespace: NORFlash */
}   /* Namespace: Adesto */

