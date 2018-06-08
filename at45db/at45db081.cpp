/* Boost Includes */
#include <boost/make_shared.hpp>

#include "at45db081.hpp"

using namespace Chimera::SPI;

#define BYTE_LEN(x) (sizeof(x)/sizeof(uint8_t))

namespace Adesto
{
	namespace NORFlash
	{
		AT45::AT45(const int& spiChannel)
		{
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
			#endif
		}

		bool AT45::initialize(uint32_t clockFreq)
		{
			if (spi->begin(setup) != SPI_OK)
				return false;

			#if defined(USING_FREERTOS)
			spi->setMode(TXRX, DMA);
			spi->attachThreadTrigger(BUFFERED_TXRX_COMPLETE, &multiTXWakeup);
			spi->attachThreadTrigger(TXRX_COMPLETE, &singleTXWakeup);
			#else			spi->setMode(TXRX, BLOCKING);
			#endif

			/*--------------------------------------
			 * Check for a proper device connection  
			 *-------------------------------------*/
			//First check at low frequency (~1MHz) for stability reasons
			auto lowFreqInfo = getDeviceInfo();
			if (info.manufacturerID != JEDEC_CODE)
				return false;

			//Now try at the requested frequency 
			spi->updateClockFrequency(clockFreq);
			auto hiFreqInfo = getDeviceInfo();
			
			if (memcmp(&lowFreqInfo, &hiFreqInfo, sizeof(AT45xx_DeviceInfo)) != 0)
				return false;


			/*--------------------------------------
			* Initialize various other settings
			*-------------------------------------*/
			usePowerOf2FlashPageSize();

			return true;
		}

		uint16_t AT45::readStatusRegister()
		{
			uint8_t reg[2];

			cmdBuffer[0] = STATUS_REGISTER_READ;

			executeCMD(cmdBuffer, BYTE_LEN(STATUS_REGISTER_READ), reg, 2);

			return (uint16_t)((reg[0] << 8) | reg[1]);
		}

		AT45xx_DeviceInfo AT45::getDeviceInfo()
		{
			uint8_t data[3];
			memset(data, 0, SIZE_OF_ARRAY(data));
			memset(cmdBuffer, 0, SIZE_OF_ARRAY(cmdBuffer));

			cmdBuffer[0] = READ_DEVICE_INFO;
			executeCMD(cmdBuffer, BYTE_LEN(READ_DEVICE_INFO), data, 3);

			info.manufacturerID = data[0];
			info.familyCode = static_cast<FamilyCode>((uint8_t)(data[1] >> 5) & 0xFF);
			info.densityCode = static_cast<DensityCode>(data[1] & 0x1F);
			info.subCode = static_cast<SubCode>((data[2] >> 5) & 0xFF);
			info.productVariant = static_cast<ProductVariant>(data[2] & 0x1F);

			return info;
		}

		void AT45::executeCMD(uint8_t* cmd, size_t cmdLen, uint8_t* buff, size_t buffLen)
		{
			/** The spi transaction here is broken up into two blocks.
			 *		1) Send opcode to the chip. This can be multiple bytes wide.
			 *		2) Read back any data into the return buffer
			 *
			 *	On both accounts, the write commands expect valid pointers and any checking is
			 *	left up to the underlying Chimera spi driver. An interesting quirk worth mentioning here
			 *	is how the second write function works. When reading data in spi, there must also be
			 *	an outgoing "dummy" transmission so data can be clocked into the receive buffer. Since not all
			 *	microcontrollers allow automatic clocking of dummy bytes, it must get these bytes from
			 *  somewhere. Rather than create and use a large buffer of zeros, the spi will simply continue
			 *  reading from the end of the command buffer. This will spit out random data on the MOSI line, BUT
			 *	the flash chip isn't listening to that anyways.
			 *
			 *	Finally, if using FreeRTOS, the system is configured to use DMA transmissions by default.
			 *	In order to run asynchronously, the thread will check for a semaphore flag that signals
			 *	a transmission/reception is complete before continuing on, all without blocking other threads.
			 **/

			if (buff && buffLen > 0)
			{
				spi->write(cmd, cmdLen, false);
				spi->write(cmd + cmdLen, buff, buffLen, true);
				#if defined(USING_FREERTOS)
				xSemaphoreTake(multiTXWakeup, portMAX_DELAY);
				#endif
			}
			else
			{
				spi->write(cmd, cmdLen);
				#if defined(USING_FREERTOS)
				xSemaphoreTake(singleTXWakeup, portMAX_DELAY);
				#endif
			}
		}

		void AT45::usePowerOf2FlashPageSize()
		{
			uint32_t cmd = CFG_PWR_2_PAGE_SIZE;
			memcpy(cmdBuffer, (uint8_t*)&cmd, BYTE_LEN(cmd));

			executeCMD(cmdBuffer, BYTE_LEN(cmd));
		}
	}
}