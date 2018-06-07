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
		}

		bool AT45::initialize(uint32_t clockFreq)
		{
			if (spi->begin(setup) != SPI_OK)
				return false;
			
			
			uint8_t OPdevID[5] = { 0x9f, 0x00, 0x00, 0x00, 0x00 };
			uint8_t data[5] = { 0x00, 0x00, 0x00, 0x00, 0x00 };
			
			spi->write(OPdevID, data, 5);
			
			memset(data, 0, 5);
			
			return true;
		}
	}
}
