/* Boost Includes */
#include <boost/make_shared.hpp>

#include "at45db081.hpp"

using namespace Chimera::SPI;

namespace Adesto
{
	namespace NORFlash
	{
		AT45::AT45(const int& spiChannel)
		{
			spi = boost::make_shared<SPIClass>(spiChannel);

			/* These settings are hard coded for reliable communications. The user will
			 * only be able to control which spi channel is used and its clock frequency. */
			setup.clockFrequency = 1000000;
			setup.bitOrder = MSB_FIRST;
			setup.clockMode = MODE0;
			setup.dataSize = DATASIZE_8BIT;
			setup.mode = MASTER;


			memset(cmdBuffer, 0, SIZE_OF_ARRAY(cmdBuffer));
		}

		bool AT45::initialize(uint32_t clockFreq)
		{
			if (spi->begin(setup) != SPI_OK)
				return false;

			uint8_t data[30];
			memset(data, 0, SIZE_OF_ARRAY(data));
			executeCMD(READ_DEVICE_INFO, data, 30);

			
			
			#if defined(USING_FREERTOS)
			spi->setMode(TXRX, BLOCKING);
			//spi->attachThreadTrigger(TX_COMPLETE, &processWakeup);
			#else			spi->setMode(TXRX, BLOCKING);
			#endif
			
			
			
			return true;
		}
		
		AT45xx_DeviceInfo AT45::getDeviceInfo()
		{
			return info;
		}
		
		
		void AT45::readCMD(uint8_t cmd, uint8_t* buff, size_t len)
		{
			
		}
		
		void AT45::write8(uint8_t data, bool disableSS)
		{
			spi->write(&data, 1, disableSS);
		}
		
		void AT45::write32(uint32_t data, bool disableSS)
		{
			//spi->write(&data, 4, disableSS);
		}
	}
}
