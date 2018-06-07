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

			bool initialize(uint32_t clockFreq);

			AT45xx_DeviceInfo getDeviceInfo();

		private:
			Chimera::SPI::SPIClass_sPtr spi;	/**< SPI object used for talking with the flash chip */
			Chimera::SPI::Setup setup;			/**< SPI initialization settings */
			
			AT45xx_DeviceInfo info;
			
			#if defined(USING_FREERTOS)
			SemaphoreHandle_t processWakeup;
			#endif 

			uint8_t cmdBuffer[4];
			uint8_t emptyBuffer[264];
			
			void readCMD(uint8_t cmd, uint8_t* buff, size_t len);
			
			void executeCMD(uint8_t* cmd, size_t cmdLen, uint8_t* buff, size_t buffLen)
			{
				//size_t cmdLen = sizeof(cmd) / sizeof(uint8_t);
				//memset(cmdBuffer, 0, SIZE_OF_ARRAY(cmdBuffer));
				//memset(cmdBuffer, cmd, cmdLen);

				//Handling of nullptr and zero cmdlen

				/* Write the command data first */
				spi->write(cmd, cmdLen, false);

				/* */
				spi->write(cmdd+cmdLen, buff, buffLen, true);

				//Need to figure out how to handle the semaphore here...multiple increments and then
				//wait until it goes to zero again? I would need to own a lock on the SPI object and 
				//then push in transmissions.
			}


			void write8(uint8_t data, bool disableSS);
			void write32(uint32_t data, bool disableSS);
		};
		typedef boost::shared_ptr<AT45> AT45_sPtr;
	}
}
#endif /* AT45DB081_HPP */
