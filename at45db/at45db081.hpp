/********************************************************************************
*   File Name:
*       at45db081.hpp
*       
*   Description:
*       API definition for the Adesto AT45 series of NOR flash chips
*   
*   2019 | Brandon Braun | brandonbraun653@gmail.com
********************************************************************************/
#pragma once
#ifndef AT45DB081_HPP
#define AT45DB081_HPP

/* Standard C++ Includes */
#include <cstdlib>
#include <memory>

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
		enum StatusRegisterBitPos : uint16_t
		{
			READY_BUSY_Pos			= (1u << 15),
			COMPARE_RESULT_Pos		= (1u << 14),
			SECTOR_PROTECTION_Pos	= (1u << 9),
			PAGE_SIZE_CONFIG_Pos	= (1u << 8),
			ERASE_PGM_ERROR_Pos		= (1u << 5),
			SECTOR_LOCKDOWN_EN_Pos	= (1u << 3),
			BUFF2_PGM_SUSPEND_Pos	= (1u << 2),
			BUFF1_PGM_SUSPEND_Pos	= (1u << 1),
			ERASE_SUSPEND_Pos		= (1u << 0)
		};

		struct StatusRegister
		{
			bool deviceReady = false;
			bool compareResult = false;
			bool sectorProtectionStatus = false;
			bool pageSizeConfig = false;
			bool eraseProgramError = false;
			bool sectorLockdownEnabled = false;
			bool pgmSuspendStatusB1 = false;
			bool pgmSuspendStatusB2 = false;
			bool eraseSuspend = false;
		};

		/**	
        *   Provides a user friendly interface for Adesto flash memory chips of the AT45 family. The class supports asynchronous operation
		*	through FreeRTOS using DMA mode SPI. The SPI driver comes from the Chimera library, which is a high level HAL that is intended
		*	to provide commonly used peripheral functions for a variety of microcontrollers. 
		*
		*	Care must be taken when passing in the pointers for reading/writing data. Due to using the Chimera HAL, it is not guaranteed that
		*	a copy of the buffer data will be made, as that choice is left up to the device driver backend. For safety, it is good practice 
		*	to keep the pointers in scope and not modify its data until the read/write/program operations are complete. 
		*/
		class AT45
		{
		public:
			AT45(const FlashChip chip, const int& spiChannel);
			~AT45() = default;

			/** 
            *   Start up the SPI driver and verify that a valid chip is connected 
            *
			*	@note	By default selects blocking SPI mode. However, if using FreeRTOS, DMA SPI mode will 
            *           be automatically selected and most function calls become non-blocking. 
            *
			*	@param[in]	clockFreq		The desired SPI clock frequency in Hz 
			*   @return FLASH_OK if everything is fine, an error code of Adesto::Status if not.
			*/
			Adesto::Status initialize(const uint32_t userClockFreq);

			/** 
            *   Reads data directly from a page in internal memory, bypassing both SRAM buffers without modification.
			*	
            *   @note   If the end of the buffer is reached before all requested bytes have been clocked out,
			*	        the data will then wrap around to the beginning of the buffer.
			*
			*	@param[in]	pageNumber		The page which from which to read (not an address)
			*	@param[in]	pageOffset		The offset within that page to start reading from. Can be any value from 0 to page size
			*	@param[out]	dataOut			External array to hold the data
			*	@param[in]	len				Number of bytes to be read
			*	@param[in]	onComplete		Optional function pointer to execute upon task completion
			*	@return Always returns FLASH_OK
			*/
			Adesto::Status directPageRead(const uint16_t pageNumber, const uint16_t pageOffset, uint8_t *const dataOut, 
                                          const size_t len, func_t onComplete = nullptr);

			/** 
            *   Reads data from one of the SRAM buffers independent of the main memory array.
            *
			*	@note   If the end of the buffer is reached before all requested bytes have been clocked out, 
			*	        the data will then wrap around to the beginning of the buffer. 
			*	
			*	@param[in]	bufferNumber	Selects which SRAM buffer to read from 
			*	@param[in]	startAddress	Starting address to read from. Can be any value from 0 to the current page size
			*	@param[out] dataOut			External array to hold the data
			*	@param[in]	len				Number of bytes to be read
			*	@param[in]	onComplete		Optional function pointer to execute upon task completion
			*	@return Always returns FLASH_OK
			*/
			Adesto::Status bufferRead(const SRAMBuffer bufferNumber, const uint16_t startAddress, uint8_t *const dataOut, 
                                      const size_t len, func_t onComplete = nullptr);

			/** 
            *   Writes data to one of the SRAM buffers independent of the main memory array.
            *
			*	@note   If the end of the buffer is reached before all bytes have been clocked in, the data will then 
            *           wrap around to the beginning of the buffer.
			*
			*	@param[in]	bufferNumber	Selects which SRAM buffer to write to
			*	@param[in]	startAddress	Starting write address. Can be any value from 0 to the current page size
			*	@param[out] dataIn			External array to transmit in (do not modify contents until write is complete)
			*	@param[in]	len				Number of bytes to write
			*	@param[in]	onComplete		Optional function pointer to execute upon task completion
			*	@return FLASH_OK if everything is fine, otherwise ERROR_WRITE_FAILURE
			**/
			Adesto::Status bufferLoad(const SRAMBuffer bufferNumber, const uint16_t startAddress, const uint8_t *const dataIn, const size_t len, func_t onComplete = nullptr);

			/** 
            *   Writes a full page of data stored in an SRAM buffer to memory
            *
			*	@note	If setting erase = false, the page must be erased by some other means before 
            *           programming, otherwise an error will occur.
            *   
			*	@param[in]	bufferNumber	Selects which SRAM buffer to write to memory
			*	@param[in]	pageNumber		Page number in memory which will be written
			*	@param[in]	erase			Selects whether or not to automatically erase the page before writing
			*	@param[in]	onComplete		Optional function pointer to execute upon task completion
			*	@return FLASH_OK
			*/
			Adesto::Status bufferWrite(const SRAMBuffer bufferNumber, const uint16_t pageNumber, const bool erase, func_t onComplete = nullptr);

			/** 
            *   Combines the operations of bufferLoad/Write. Data is written to an SRAM buffer and the chip 
            *   automatically erases and programs a given page address with the contents of the SRAM buffer.
            *
			*	@note	If only a partial page is written to the SRAM buffer, whatever data is left in SRAM will 
            *           overwrite the full page in memory
            *
			*	@param[in]	bufferNumber	Selects which SRAM buffer to use
			*	@param[in]	bufferOffset	Selects the first byte in the SRAM buffer to be written
			*	@param[in]	pageNumber		Page number in memory to write 
			*	@param[in]	dataIn			Pointer to external buffer of data to write (do not modify contents until write is complete)
			*	@param[in]	len				How many bytes should be written, up to a full page size
			*	@param[in]	onComplete		Optional function pointer to execute upon task completion
			*	@return FLASH_OK
			*/
			Adesto::Status pageWrite(const SRAMBuffer bufferNumber, const uint16_t bufferOffset, const uint16_t pageNumber, 
                                     const uint8_t *const dataIn, const size_t len, func_t onComplete = nullptr);

			/** 
            *   A completely self-contained operation to reprogram any number of sequential bytes within a 
            *   page, without modifying the rest
            *   
			*	@param[in]	bufferNumber	Selects which SRAM buffer to use
			*	@param[in]	pageNumber		Page number in memory to write
			*	@param[in]	pageOffset		Selects the first byte in the page to be written
			*	@param[in]	dataIn			Pointer to external buffer of data to write (do not modify contents until write is complete)
			*	@param[in]	len				How many bytes should be written, up to a full page size
			*	@param[in]	onComplete		Optional function pointer to execute upon task completion
			*	@return FLASH_OK
			*/
			Adesto::Status readModifyWriteManual(const SRAMBuffer bufferNumber, const uint16_t pageNumber, const uint16_t pageOffset, 
                                                 const uint8_t *const dataIn, const size_t len, func_t onComplete = nullptr);
			
			
			Adesto::Status readModifyWrite(SRAMBuffer bufferNumber, uint16_t pageNumber, uint16_t pageOffset, uint8_t* dataIn, size_t len, func_t onComplete = nullptr) \
				__attribute__((deprecated("Currently the opcode associated with this command fails. Use readModifyWriteManual instead.")));

			/** 
            *   Utilizes SRAM buffer 1 to write a fixed number of bytes to a pre-erased page of memory. Only the 
            *   bytes written will be programmed. If the end of the buffer is reached before all bytes are written, 
            *   the data will be wrapped around to the beginning of the buffer.
            *
			*	@note	Any data already in SRAM buffer 1 will be clobbered, BUT only the bytes altered in SRAM 
            *           will be altered in memory. For example, if two bytes are written to SRAM, only two bytes 
            *           will be written to memory.
            *
			*	@param[in]	pageNumber		Page number in memory to write
			*	@param[in]	pageOffset		Selects the first byte in the SRAM buffer to be written
			*	@param[in]	dataIn			Pointer to an external buffer of data to write (do not modify contents until write is complete)
			*	@param[in]	len				How many bytes to write, up to a full page size
			*	@param[in]	onComplete		Optional function pointer to execute upon task completion
			*	@return FLASH_OK
			*/
			Adesto::Status byteWrite(const uint16_t pageNumber, const uint16_t pageOffset, const uint8_t *const dataIn, const size_t len, func_t onComplete = nullptr);

			/** 
            *   Writes a buffer of data to internal memory at some address 
            *
			*	@param[in]	address			Starting address to begin write
			*	@param[in]	dataIn			Buffer of data (do not modify contents until write is complete)
			*	@param[in]	len				Number of bytes to write
			*	@param[in]	onComplete		Optional function pointer to execute upon task completion
			*	@return FLASH_OK
			*/
			Adesto::Status write(const uint32_t address, const uint8_t *const dataIn, const size_t len, func_t onComplete = nullptr);

			/** 
            *   Writes a generic data type to internal memory
            *   
			*	@param[in]	address			The starting address which from which to read
			*	@param[in]	dataIn			Data structure to write
			*	@param[in]	len				Number of bytes to be read
			*	@param[in]	onComplete		Optional function pointer to execute upon task completion
			*	@return Always returns FLASH_OK
			*/
			template<typename T>
			Adesto::Status write(const uint32_t address, const T *const dataIn, func_t onComplete = nullptr)
			{
				return write(address, reinterpret_cast<const uint8_t *const >(dataIn), sizeof(T), onComplete);
			}

			/** 
            *   Reads data directly from a starting address in internal memory, bypassing both SRAM buffers without modification.
			*	Will continue reading from memory until the chip select line is deactivated.
			*
			*	@param[in]	address			The starting address which from which to read
			*	@param[out]	dataOut			External array to hold the data
			*	@param[in]	len				Number of bytes to be read
			*	@param[in]	onComplete		Optional function pointer to execute upon task completion
			*	@return Always returns FLASH_OK
			*/
			Adesto::Status read(const uint32_t address, uint8_t *const dataOut, const size_t len, func_t onComplete = nullptr);

			
			/** 
            *   Reads data directly from internal memory into an instance of the output type.
			*
			*	@param[in]	address			The starting address which from which to read
			*	@param[out]	dataOut			Data structure to read into
			*	@param[in]	len				Number of bytes to be read
			*	@param[in]	onComplete		Optional function pointer to execute upon task completion
			*	@return Always returns FLASH_OK
			*/
			template<typename T>
			Adesto::Status read(const uint32_t address, T *const dataOut, func_t onComplete = nullptr)
			{
				return read(address, reinterpret_cast<uint8_t *const>(dataOut), sizeof(T), onComplete);
			}

			/** 
            *   Erase sections of the chip in multiples of the page size (default 256 bytes)
			*
			*	@note	Due to the segmented nature of this operation, it will not return until the memory 
            *           has been erased. When using FreeRTOS, this only blocks the current thread.
			*
			*	@todo	Try and create an OO version of the FreeRTOS tasks such that a class member thread 
            *           could be spawned to handle this stuff without blocking
            *
			*	@param[in]	address			Location to start the erasing. Must be page aligned.
			*	@param[in]	len				Number of bytes to be erased. Must be page aligned.
			*   @param[in]	onComplete		Optional function pointer to execute upon task completion
			*	@return FLASH_OK if everything is fine, an error code of Adesto::Status if not.
			*/
			Adesto::Status erase(const uint32_t address, const size_t len, func_t onComplete = nullptr);

			/** 
            *   Starts the full chip erase process and then returns. Completion must be checked 
            *   with AT45::isEraseComplete()
            *   
			*	@return FLASH_OK
			*/
			Adesto::Status eraseChip();

			/** 
            *   Queries the flash chip status register and determines the page size configuration setting 
            *
			*	@return 256 if in 'power of 2' mode or 264 if in 'standard data flash' mode
			*/
			uint16_t getPageSizeConfig();

			/** 
            *   Grabs the current status register
            *
			*	@param[out] reg	    Optional argument to return back all status register parameters 
            *                       that were read, but in struct format for easy debugging
			*	@return status register value
			*/
			uint16_t readStatusRegister(StatusRegister *const reg = nullptr);

			/** 
            *   Queries the flash chip status register and checks if the device is ready
            *   
			*	@param[out]	reg	    Optional argument to return back all status register parameters that were read
			*	@return true if ready, false if not
			*/
			bool isDeviceReady(StatusRegister *const reg = nullptr);

			/** 
            *   Queries the flash chip status register and checks if an error occurred during programming or erasing
            *   
			*	@param[out]	reg	    Optional argument to return back all status register parameters that were read
			*	@return true if error, false if not
			*/
			bool isErasePgmError(StatusRegister *const reg = nullptr);

            /**
            *   Instruct the flash chip to use a binary (power of 2) page sizing
            *   @return FLASH_OK
            */
			Adesto::Status useBinaryPageSize();

            /**
            *   Instruct the flash chip to use the alternate page sizing
            *   @return FLASH_OK
            */
			Adesto::Status useDataFlashPageSize();

			/** 
            *   Reads the device manufacturer ID and device ID. Also updates internal copy.
            *
			*	@return A struct of type AT45xx_DeviceInfo 
			*/
			AT45xx_DeviceInfo getDeviceInfo();

			/**	
            *   Checks an internal semaphore to see if an SPI read operation was completed
			*	
            *   @return true if complete, false if not
			*/
			bool isReadComplete();

			/**	
            *   Checks an internal semaphore to see if an SPI write operation was completed
			*	
            *   @return true if complete, false if not
			*/
			bool isWriteComplete();

			/**
			*   Gets the capacity of the discovered chip in bytes
            *   
            *   @return Chip capacity
			*/ 
			uint32_t getFlashSize();

		private:
			Chimera::SPI::SPIClass_uPtr spi;	/**< SPI object used for talking with the flash chip */
			Chimera::SPI::Setup setup;			/**< SPI initialization settings */
			
			FlashChip device;					/**< Holds the device model number */
			AT45xx_DeviceInfo info;				/**< Information regarding flash chip specifics */
			
			#if defined(USING_FREERTOS)
			SemaphoreHandle_t multiTXWakeup;
			SemaphoreHandle_t singleTXWakeup;
			SemaphoreHandle_t singleTXRXWakeup;
			#endif 

			bool writeComplete = false;
			bool readComplete = false;
			uint32_t clockFrequency = 0;		/**< Contains actual frequency of the SPI clock in Hz */
			float maxClockErr = 0.2f;		    /**< Max percent error between requested clock and actual clock freq */

			//Note: These values appear constant over all AT45 chips 
			size_t pageSize = 264;				/**< Keeps track of the current page size configuration in bytes */
			size_t blockSize = 2112;			/**< Keeps track of the current block size configuration in bytes */
			size_t sectorSize = 65472;			/**< Keeps track of the current sector size configuration in bytes */

			uint8_t addressBytes;
    		std::array<uint8_t, 10> cmdBuffer;      /**< Buffer for holding a command sequence */
    		std::array<uint8_t, 3> memoryAddress;   /**< Number of bytes needed for memory access addressing. Adesto seems to only use 3.*/

			volatile uint32_t errorFlags = 0;	    /**< Bit-field indicating various errors as defined in .cpp file */

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

            /** 
            *   Converts an address and length into whole sections that can be erased
            *
            *   @param[in]  address     TODO
            *   @param[in]  len         TODO
            *   @return MemoryRange
            */
			MemoryRange getErasableSections(uint32_t address, size_t len);

			/**
            *   Converts an address and length into page numbers and their associated start/end byte offsets 
			*   for easy reading and writing arbitrary locations in memory.
            *
            *   @param[in]  address     TODO
            *   @param[in]  len         TODO
            *   @return MemoryRange
            */
			MemoryRange getWriteReadPages(const uint32_t startAddress, const size_t len);

			/** 
            *   Erases a set of ranges as defined by a MemoryRange object 
            *   
            *   @param[in]  range       TODO
            *   @return TODO
            */
			Adesto::Status eraseRanges(const MemoryRange range, func_t onComplete = nullptr);

            /** 
            *   Erases a given sector
            *
            *   @param[in]  sectorNumber    The sector to be erased
            *   @return void
            */
			void eraseSector(const uint32_t sectorNumber);
			
            /** 
            *   Erases a given block
            *
            *   @param[in]  blockNumber     The block to be erased
            *   @return void
            */
            void eraseBlock(const uint32_t blockNumber);

            /** 
            *   Erases a given page
            *
            *   @param[in]  pageNumber      The page to be erased
            *   @return void
            */
			void erasePage(const uint32_t pageNumber);

    		/** 
            *   Generates the appropriate command sequence for several read and write operations, automatically
            *	writing to the class member 'cmdBuffer'.
            *
            *   @note This command only works for several types of operations:
            *		. Direct Page Read (opcodes: 0xD2h)
            *		. Buffer Read (opcodes: 0xD1h, 0xD3h, 0xD4h, 0xD6h)
            *		. Buffer Write (opcodes: 0x84h, 0x87h)
            *		. Main Memory Page Program through Buffer with Built-In Erase (opcodes: 0x82h, 0x85h)
            *		. Main Memory Page Program through Buffer without Built-In Erase (opcodes: 0x88h, 0x89h)
            *		. Main Memory Byte/Page Program through Buffer 1 without Built-In Erase (opcodes: 0x02h)
            *		. Read-Modify-Write (opcodes: 0x58h, 0x59h)
            *
            *	@param[in]	pageNumber	The desired page number in memory
            *	@param[in]	offset		The desired offset within the page
            *   @return void
            */
			void buildReadWriteCommand(const uint16_t pageNumber, const uint16_t offset = 0x0000);

    		/** 
            *   Creates the command sequence needed to erase a particular flash section. 
            *   Automatically overwrites the class member 'cmdBuffer' with the appropriate data.
            *
            *	@param[in]	section			What type of section to erase (page, block, etc)
            *	@param[in]	sectionNumber	Which index of that section type to erase (0, 1, 2, etc)
            *   @return void
            */
			void buildEraseCommand(const FlashSection section, const uint32_t sectionNumber);

			/** 
            *   Parses an address into a section category 
            *
            *   @param[in]  section     TODO
            *   @param[in]  rawAddress  TODO
            *   @return TODO
            */
			uint32_t getSectionFromAddress(const FlashSection section, const uint32_t rawAddress);

			/** 
            *   Gets a section number's starting address in memory 
            *
            *   @param[in]  section         TODO
            *   @param[in]  sectionNumber   TODO
            *   @return TODO
            */
			uint32_t getSectionStartAddress(const FlashSection section, const uint32_t sectionNumber);

            /** 
            *   Writes data on the SPI bus
            *
            *   @param[in]  data        Data to be written
            *   @param[in]  len         How many bytes to be written
            *   @param[in]  disableSS   Optionally disable the chip select line after the transfer completes
            *   @return void
            */
    		void SPI_write(const uint8_t *const data, const size_t len, const bool disableSS = true);

            /** 
            *   Read data from the SPI bus
            *
            *   @param[in]  data        Where to read data into
            *   @param[in]  len         How much data to read
            *   @param[in]  disableSS   Optionally disable the chip select line after the transfer completes
            *   @return void
            */
    		void SPI_read(uint8_t *const data, const size_t len, const bool disableSS = true);

		};
		typedef std::shared_ptr<AT45> AT45_sPtr;
		typedef std::unique_ptr<AT45> AT45_uPtr;
	}
}
#endif /* AT45DB081_HPP */
