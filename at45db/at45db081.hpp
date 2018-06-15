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
		ERROR_WRITE_LENGTH_INVALID,
		ERROR_READ_LENGTH_INVALID,
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

		/**	Provides a user friendly interface for Adesto flash memory chips of the AT45 family. The class supports asynchronous operation
		 *	through FreeRTOS using DMA mode SPI. The SPI driver comes from the Chimera library, which is a high level HAL that is intended
		 *	to provide commonly used peripheral functions for a variety of microcontrollers. 
		 *
		 *	Care must be taken when passing in the pointers for reading/writing data. Due to using the Chimera HAL, it is not guaranteed that
		 *	a copy of the buffer data will be made, as that choice is left up to the device driver backend. For safety, it is good practice 
		 *	to keep the pointers in scope and not modify its data until the read/write/program operations are complete. 
		 **/
		class AT45
		{
		public:
			AT45(FlashChip chip, const int& spiChannel);
			~AT45() = default;

			/** Start up the SPI driver and verify that a valid chip is connected 
			 *	@param[in]	clockFreq		The desired SPI clock frequency in Hz 
			 *  @return FLASH_OK if everything is fine, an error code of Adesto::Status if not.
			 *
			 *	@note	By default selects blocking SPI mode. However, if using FreeRTOS, DMA SPI mode will be automatically selected
			 *			and most function calls become non-blocking. 
			 **/
			Adesto::Status initialize(uint32_t clockFreq);

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
			*	@param[out] dataIn			External array to transmit in (do not modify contents until write is complete)
			*	@param[in]	len				Number of bytes to write
			*	@param[in]	onComplete		Optional function pointer to execute upon task completion
			*	@return FLASH_OK if everything is fine, otherwise ERROR_WRITE_FAILURE
			**/
			Adesto::Status bufferLoad(SRAMBuffer bufferNumber, uint16_t startAddress, uint8_t* dataIn, size_t len, func_t onComplete = nullptr);

			/** Writes a full page of data stored in an SRAM buffer to memory
			 *	@param[in]	bufferNumber	Selects which SRAM buffer to write to memory
			 *	@param[in]	pageNumber		Page number in memory which will be written
			 *	@param[in]	erase			Selects whether or not to automatically erase the page before writing
			 *	@param[in]	onComplete		Optional function pointer to execute upon task completion
			 *	@return FLASH_OK
			 *
			 *	@note	If setting erase = false, the page must be erased by some other means before programming, otherwise an error will occur
			 **/
			Adesto::Status bufferWrite(SRAMBuffer bufferNumber, uint16_t pageNumber, bool erase, func_t onComplete = nullptr);

			/** Combines the operations of bufferLoad/Write. Data is written to an SRAM buffer and the chip automatically erases and programs
			 *	a given page address with the contents of the SRAM buffer. 
			 *	@param[in]	bufferNumber	Selects which SRAM buffer to use
			 *	@param[in]	bufferOffset	Selects the first byte in the SRAM buffer to be written
			 *	@param[in]	pageNumber		Page number in memory to write 
			 *	@param[in]	dataIn			Pointer to external buffer of data to write (do not modify contents until write is complete)
			 *	@param[in]	len				How many bytes should be written, up to a full page size
			 *	@param[in]	onComplete		Optional function pointer to execute upon task completion
			 *	@return FLASH_OK
			 *
			 *	@note	If only a partial page is written to the SRAM buffer, whatever data is left in SRAM will overwrite the full page in memory
			 **/
			Adesto::Status pageWrite(SRAMBuffer bufferNumber, uint16_t bufferOffset, uint16_t pageNumber, uint8_t* dataIn, size_t len, func_t onComplete = nullptr);

			/** A completely self-contained operation to reprogram any number of sequential bytes within a page, without modifying the rest
			 *	@param[in]	bufferNumber	Selects which SRAM buffer to use
			 *	@param[in]	pageNumber		Page number in memory to write
			 *	@param[in]	pageOffset		Selects the first byte in the page to be written
			 *	@param[in]	dataIn			Pointer to external buffer of data to write (do not modify contents until write is complete)
			 *	@param[in]	len				How many bytes should be written, up to a full page size
			 *	@param[in]	onComplete		Optional function pointer to execute upon task completion
			 *	@return FLASH_OK
			 **/
			Adesto::Status readModifyWrite(SRAMBuffer bufferNumber, uint16_t pageNumber, uint16_t pageOffset, uint8_t* dataIn, size_t len, func_t onComplete = nullptr);

			/** Utilizes SRAM buffer 1 to write a fixed number of bytes to a pre-erased page of memory. Only the bytes written will be programmed.
			 *	If the end of the buffer is reached before all bytes are written, the data will be wrapped around to the beginning of the buffer.
			 *	@param[in]	bufferOffset	Selects the first byte in the SRAM buffer to be written
			 *	@param[in]	pageNumber		Page number in memory to write
			 *	@param[in]	dataIn			Pointer to an external buffer of data to write (do not modify contents until write is complete)
			 *	@param[in]	len				How many bytes to write, up to a full page size
			 *	@param[in]	onComplete		Optional function pointer to execute upon task completion
			 *	@return FLASH_OK
			 *
			 *	@note	Any data already in SRAM buffer 1 will be clobbered, BUT only the bytes altered in SRAM will be altered in memory. For example,
			 *			if two bytes are written to SRAM, only two bytes will be written to memory.
			 **/
			Adesto::Status byteWrite(uint16_t bufferOffset, uint16_t pageNumber, uint8_t* dataIn, size_t len, func_t onComplete = nullptr);

			/** Writes a buffer of data to internal memory at some address 
			 *	@param[in]	address			Starting address to begin write
			 *	@param[in]	dataIn			Buffer of data (do not modify contents until write is complete)
			 *	@param[in]	len				Number of bytes to write
			 *	@param[in]	onComplete		Optional function pointer to execute upon task completion
			 *	@return FLASH_OK
			 **/
			Adesto::Status write(uint32_t address, uint8_t* dataIn, size_t len, func_t onComplete = nullptr);


			/** Writes a generic data type to internal memory
			 *	@param[in]	address			The starting address which from which to read
			 *	@param[in]	dataIn			Data structure to write
			 *	@param[in]	len				Number of bytes to be read
			 *	@param[in]	onComplete		Optional function pointer to execute upon task completion
			 *	@return Always returns FLASH_OK
			 **/
			template<typename T>
			Adesto::Status write(uint32_t address, T& dataIn, func_t onComplete = nullptr)
			{
				return write(address, reinterpret_cast<uint8_t*>(&dataIn), sizeof(T), onComplete);
			}

			/** Reads data directly from a starting address in internal memory, bypassing both SRAM buffers without modification.
			 *	Will continue reading from memory until the chip select line is deactivated.
			 *
			 *	@param[in]	address			The starting address which from which to read
			 *	@param[out]	dataOut			External array to hold the data
			 *	@param[in]	len				Number of bytes to be read
			 *	@param[in]	onComplete		Optional function pointer to execute upon task completion
			 *	@return Always returns FLASH_OK
			 **/
			Adesto::Status read(uint32_t address, uint8_t* dataOut, size_t len, func_t onComplete = nullptr);

			
			/** Reads data directly from internal memory into an instance of the output type.
			 *
			 *	@param[in]	address			The starting address which from which to read
			 *	@param[out]	dataOut			Data structure to read into
			 *	@param[in]	len				Number of bytes to be read
			 *	@param[in]	onComplete		Optional function pointer to execute upon task completion
			 *	@return Always returns FLASH_OK
			 **/
			template<typename T>
			Adesto::Status read(uint32_t address, T& dataOut, func_t onComplete = nullptr)
			{
				return read(address, reinterpret_cast<uint8_t*>(&dataOut), sizeof(T), onComplete);
			}

			/** Erase sections of the chip in multiples of the page size (default 256 bytes)
			 *	@param[in]	address			Location to start the erasing. Must be page aligned.
			 *	@param[in]	len				Number of bytes to be erased. Must be page aligned.
			 *  @param[in]	onComplete		Optional function pointer to execute upon task completion
			 *	@return FLASH_OK if everything is fine, an error code of Adesto::Status if not.
			 *
			 *	@note	Due to the segmented nature of this operation, it will not return until the memory has been erased. When using 
			 *			FreeRTOS, this only blocks the current thread.
			 *
			 *	@todo	Try and create an OO version of the FreeRTOS tasks such that a class member thread could be spawned to handle this stuff without blocking
			 **/
			Adesto::Status erase(uint32_t address, size_t len, func_t onComplete = nullptr);

			/** Starts the full chip erase process and then returns. Completion must be checked with AT45::isEraseComplete()
			 *	@return FLASH_OK
			 **/
			Adesto::Status eraseChip();

			/** Queries the flash chip status register and determines the page size configuration setting 
			 *	@return 256 if in 'power of 2' mode or 264 if in 'standard data flash' mode
			 **/
			uint16_t getPageSizeConfig();

			/** Grabs the current status register
			 *	@return status register value
			 **/
			uint16_t readStatusRegister();

			/** Queries the flash chip status register and checks if the device is ready
			 *	@return true if ready, false if not
			 **/
			bool isDeviceReady();

			/** Queries the flash chip status register and checks if an error occurred during programming or erasing 
			 *	@return true if error, false if not
			 **/
			bool isErasePgmError();

			bool isProgramComplete();
			
			bool isEraseComplete();

			Adesto::Status useBinaryPageSize();

			Adesto::Status useDataFlashPageSize();

			/** Reads the device manufacturer ID and device ID. Also updates internal copy.
			 *	@return A struct of type AT45xx_DeviceInfo 
			 **/
			AT45xx_DeviceInfo getDeviceInfo();

			#if defined(USING_FREERTOS)
			/**	Checks an internal semaphore to see if a read operation was completed
			 *	@return true if complete, false if not
			 **/
			bool isReadComplete();

			/**	Checks an internal semaphore to see if a write operation was completed
			 *	@return true if complete, false if not
			 **/
			bool isWriteComplete();
			#endif


			uint32_t maxAddress = 0;

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

			uint8_t addressBytes;
			uint8_t cmdBuffer[10];				/**< Buffer for holding a command sequence */
			uint8_t memoryAddress[3];			/**< Number of bytes needed for memory access addressing. Adesto seems to only use 3.*/

			volatile uint32_t errorFlags = 0;	/**< Bit-field indicating various errors as defined in .cpp file */



			void SPI_write(uint8_t* data, size_t len, bool disableSS = true);
			void SPI_read(uint8_t* data, size_t len, bool disableSS = true);
			
			

			/* Data structure to support indexing memory sections */
			struct MemoryRange
			{
				struct Range
				{
					size_t start;					/**< Which page/block/sector the range begins on */
					size_t end;						/**< Which page/block/sector the range ends on */
					size_t startPageOffset = 0;		/**< Starting address offset from beginning of the first page (zero if not a page) */
					size_t endPageOffset = 0;		/**< How many bytes to write from beginning of the last page (zero if not a page) */
				};

				Range sector;
				Range block;
				Range page;
			};

			/* Converts an address and length into whole sections that can be erased */
			MemoryRange getErasableSections(uint32_t address, size_t len);

			/* Converts an address and length into page numbers and their associated start/end byte offsets 
			 * for easy reading and writing arbitrary locations in memory */
			MemoryRange getWriteReadPages(uint32_t startAddress, size_t len);

			/* Erases a set of ranges as defined by a MemoryRange object */
			Adesto::Status eraseRanges(MemoryRange range, func_t onComplete = nullptr);

			void eraseSector(uint32_t sectorNumber);
			void eraseBlock(uint32_t blockNumber);
			void erasePage(uint32_t pageNumber);


			uint8_t* buildAddressCommand(FlashSection section, uint32_t indexingNumber);

			/* Parses an address into a section category */
			uint32_t getSectionFromAddress(FlashSection section, uint32_t rawAddress);

			/* Gets a section number's starting address in memory */
			uint32_t getSectionStartAddress(FlashSection section, uint32_t sectionNumber);

			/* Returns the capacity of the discovered chip in bytes */
			uint32_t getFlashSize();
		};
		typedef boost::shared_ptr<AT45> AT45_sPtr;
	}
}
#endif /* AT45DB081_HPP */
