#pragma once
#ifndef AT45DB081_HPP
#define AT45DB081_HPP

/* Standard C++ Includes */
#include <stdlib.h>

/* Boost Includes */
#include <boost/array.hpp>
#include <boost/shared_ptr.hpp>

/* Chimera Includes */
#include <Chimera/spi.hpp>

#if defined(USING_FREERTOS)
#include <Chimera/threading.hpp>
#endif

/* Supporting Includes */
#include "at45db081_definitions.hpp"

namespace Adesto
{
	enum FlashSection
	{
		PAGE,
		BLOCK,
		SECTOR
	};

	enum FlashChip
	{
		AT45DB081E,
		NUM_SUPPORTED_CHIPS
	};

	enum Status
	{
		FLASH_OK,
		ERROR_SPI_INIT_FAILED,
		ERROR_UNKNOWN_JEDEC_CODE,
		ERROR_FAILED_HIGH_FREQUENCY_TRANSACTION,
		ERROR_PAGE_SIZE_MISMATCH,
		ERROR_LENGTH_NOT_PAGE_ALIGNED,
		ERROR_ADDRESS_NOT_PAGE_ALIGNED,
		ERROR_ERASE_FAILURE,
		ERROR_ERASE_LENGTH_INVALID,
		ERROR_WRITE_FAILURE,
		ERROR_DEVICE_NOT_READY,

	};

	enum SRAMBuffer
	{
		BUFFER1,
		BUFFER2
	};

	struct EraseRange
	{
		size_t start;
		size_t end;
		bool rangeValid = false;
	};

	typedef void(*func_t)(void);

	namespace NORFlash
	{
		enum StatusRegister : uint16_t
		{
			READY_BUSY			= (1u << 15),
			COMPARE_RESULT		= (1u << 14),
			SECTOR_PROTECTION	= (1u << 9),
			PAGE_SIZE_CONFIG	= (1u << 8),
			ERASE_PGM_ERROR		= (1u << 5),
			SECTOR_LOCKDOWN_EN	= (1u << 3),
			BUFF2_PGM_SUSPEND	= (1u << 2),
			BUFF1_PGM_SUSPEND	= (1u << 1),
			ERASE_SUSPEND		= (1u << 0)
		};

		class AT45
		{
		public:
			AT45(FlashChip chip, const int& spiChannel);
			~AT45() = default;

			void test()
			{
				//buildAddressCommand(PAGE, 18);
				erase(0x00000000, sectorSize);
			}

			/** Start up the SPI driver and verify that a valid chip is connected 
			 *	@param[in] clockFreq The desired SPI clock frequency in Hz 
			 *  @return FLASH_OK if everything is fine, an error code of Adesto::Status if not.
			 **/
			Adesto::Status initialize(uint32_t clockFreq);

			/** Reads data directly from a starting address in internal memory, bypassing both SRAM buffers without modification.
			*	Will continue reading from memory until the chip select line is deactivated.
			*
			*	@param[in]	pageNumber		The starting page which from which to read (not an address)
			*	@param[in]	startAddress	The offset within that page to start reading from. Can be any value from 0 to page size
			*	@param[out]	dataOut			External array to hold the data
			*	@param[in]	len				Number of bytes to be read
			*	@param[in]	onComplete		Optional function pointer to execute upon task completion
			*	@return Always returns FLASH_OK
			**/
			Adesto::Status continuousRead(uint32_t pageNumber, uint16_t startAddress, uint8_t* dataOut, size_t len, func_t onComplete = nullptr);

			/** Reads data directly from a page in internal memory, bypassing both SRAM buffers without modification.
			 *	@note If the end of the buffer is reached before all requested bytes have been clocked out,
			 *	the data will then wrap around to the beginning of the buffer.
			 *
			 *	@param[in]	pageNumber		The page which from which to read (not an address)
			 *	@param[in]	startAddress	The offset within that page to start reading from. Can be any value from 0 to page size
			 *	@param[out]	dataOut			External array to hold the data
			 *	@param[in]	len				Number of bytes to be read
			 *	@param[in]	onComplete		Optional function pointer to execute upon task completion
			 *	@return Always returns FLASH_OK
			 **/
			Adesto::Status directPageRead(uint32_t pageNumber, uint16_t startAddress, uint8_t* dataOut, size_t len, func_t onComplete = nullptr);

			/** Reads data from one of the SRAM buffers independent of the main memory array.
			 *	@note If the end of the buffer is reached before all requested bytes have been clocked out, 
			 *	the data will then wrap around to the beginning of the buffer. 
			 *	
			 *	@param[in]	bufferNumber	Selects which SRAM buffer to read from 
			 *	@param[in]	startAddress	Starting address to read from. Can be any value from 0 to the current page size
			 *	@param[out] dataOut			External array to hold the data
			 *	@param[in]	len				Number of bytes to be read
			 *	@param[in]	onComplete		Optional function pointer to execute upon task completion
			 *	@return Always returns FLASH_OK
			 **/
			Adesto::Status bufferRead(SRAMBuffer bufferNumber, uint16_t startAddress, uint8_t* dataOut, size_t len, func_t onComplete = nullptr);


			/** Writes data to one of the SRAM buffers independent of the main memory array.
			*	@note If the end of the buffer is reached before all bytes have been clocked in, the data will then wrap around to the beginning of the buffer.
			*
			*	@param[in]	bufferNumber	Selects which SRAM buffer to write to
			*	@param[in]	startAddress	Starting write address. Can be any value from 0 to the current page size
			*	@param[out] dataIn			External array to transmit in
			*	@param[in]	len				Number of bytes to write
			*	@param[in]	onComplete		Optional function pointer to execute upon task completion
			*	@return FLASH_OK if everything is fine, otherwise ERROR_WRITE_FAILURE
			**/
			Adesto::Status bufferWrite(SRAMBuffer bufferNumber, uint16_t startAddress, uint8_t* dataIn, size_t len, func_t onComplete = nullptr);


			/** Erase sections of the chip in multiples of the page size (default 256 bytes)
			 *	@param[in]	address		Location to start the erasing. Must be page aligned.
			 *	@param[in]	len			Number of bytes to be erased. Must be page aligned.
			 *  @param[in]	onComplete	Optional function pointer to execute upon task completion
			 *	@return @return FLASH_OK if everything is fine, an error code of Adesto::Status if not.
			 **/
			Adesto::Status erase(uint32_t address, size_t len, func_t onComplete = nullptr);

			/** Erases the entire chip. This could take a very long time.
			 *  @param[in]	onComplete	Optional function pointer to execute upon task completion
			 *	@return FLASH_OK if everything is fine, ERROR_ERASE_FAILURE if not.
			 **/
			Adesto::Status eraseChip(func_t onComplete = nullptr);


			/** Queries the flash chip status register and determines the page size configuration setting 
			*	@return 256 if in 'power of 2' mode or 264 if in 'standard data flash' mode
			**/
			uint16_t getPageSizeConfig();

			/** Queries the flash chip status register and checks if the device is ready
			 *	@return true if ready, false if not
			 **/
			bool deviceReady();

			/** Queries the flash chip status register and checks if an error occurred during programming or erasing 
			 *	@return true if error, false if not
			 **/
			bool erasePgmError();

			

			/** Grabs the current status register 
			 *	@return status register value
			 **/
			uint16_t readStatusRegister();

			/** Reads the device manufacturer ID and device ID. Also updates internal copy.
			 *	@return A struct of type AT45xx_DeviceInfo 
			 **/
			AT45xx_DeviceInfo getDeviceInfo();

			

		private:
			Chimera::SPI::SPIClass_sPtr spi;	/**< SPI object used for talking with the flash chip */
			Chimera::SPI::Setup setup;			/**< SPI initialization settings */
			
			FlashChip device;					/**< Holds the device model number */
			AT45xx_DeviceInfo info;				/**< Information regarding flash chip specifics */
			
			#if defined(USING_FREERTOS)
			SemaphoreHandle_t multiTXWakeup;
			SemaphoreHandle_t singleTXWakeup;
			SemaphoreHandle_t singleTXRXWakeup;
			#endif 

			//Note: These values appear constant over all AT45 chips 
			size_t pageSize = 264;				/**< Keeps track of the current page size configuration in bytes */
			size_t blockSize = 2112;			/**< Keeps track of the current block size configuration in bytes */
			size_t sectorSize = 65472;			/**< Keeps track of the current sector size configuration in bytes */

			uint8_t cmdBuffer[10];				/**< Buffer for holding a command sequence */
			uint8_t memoryAddress[3];			/**< Number of bytes needed for memory access addressing. Adesto seems to only use 3.*/

			volatile uint32_t errorFlags = 0;	/**< Bit-field indicating various errors as define in .cpp file */


			EraseRange eraseRange_Sectors;
			EraseRange eraseRange_Blocks;
			EraseRange eraseRange_Pages;


			
			void executeCMD(uint8_t* cmd, size_t cmdLen, uint8_t* buff = nullptr, size_t buffLen = 0);


			void SPI_write(uint8_t* data, size_t len, bool disableSS = true);
			void SPI_read(uint8_t* data, size_t len, bool disableSS = true);
			
			void useBinaryPageSize();
			void useDataFlashPageSize();

			Adesto::Status eraseRanges(func_t onComplete = nullptr);
			void eraseSector(uint32_t sectorNumber);
			void eraseBlock(uint32_t blockNumber);
			void erasePage(uint32_t pageNumber);


			uint8_t* buildAddressCommand(FlashSection section, uint32_t indexingNumber);
			uint32_t getSectionNumberFromAddress(FlashSection section, uint32_t rawAddress);
		};
		typedef boost::shared_ptr<AT45> AT45_sPtr;
	}
}
#endif /* AT45DB081_HPP */
