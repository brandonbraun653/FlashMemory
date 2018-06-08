#pragma once
#ifndef AT45DB081_HPP
#define AT45DB081_HPP

/* Standard C++ Includes */
#include <stdlib.h>

/* Boost Includes */
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
	namespace NORFlash
	{
		class AT45
		{
		public:
			static const size_t numSectors = 16;
			static const size_t numBlocks = 512;
			static const size_t numPages = 4096;
			static const size_t numBytesPerPage = 256;

			AT45(const int& spiChannel);
			~AT45() = default;

			//Need options to to run at desired clock frequency

			/** Start up the SPI driver and verify that a valid chip is connected 
			 *	@param[in] clockFreq The desired frequency for the SPI clock in Hz 
			 *  @return true if successful, false if not
			 **/
			bool initialize(uint32_t clockFreq);

			/** Grabs the current status register values 
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
			
			AT45xx_DeviceInfo info;				/**< Information regarding flash chip specifics */
			
			#if defined(USING_FREERTOS)
			SemaphoreHandle_t multiTXWakeup;
			SemaphoreHandle_t singleTXWakeup;
			#endif 

			uint8_t cmdBuffer[10];
			
			void executeCMD(uint8_t* cmd, size_t cmdLen, uint8_t* buff = nullptr, size_t buffLen = 0);
			
			void usePowerOf2FlashPageSize();

		};
		typedef boost::shared_ptr<AT45> AT45_sPtr;
	}
}
#endif /* AT45DB081_HPP */
