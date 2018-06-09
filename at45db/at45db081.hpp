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
	};

	struct EraseRange
	{
		size_t start;
		size_t end;
		bool rangeValid = false;
	};

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
			 *	@param[in] clockFreq The desired frequency for the SPI clock in Hz 
			 *  @return FLASH_OK if everything is fine, a value of Adesto::Status if not.
			 **/
			Adesto::Status initialize(uint32_t clockFreq);


			Adesto::Status erase(uint32_t address, size_t len);

			Adesto::Status eraseChip();


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
			
			void useBinaryPageSize();
			void useDataFlashPageSize();

			Adesto::Status eraseRanges();
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
