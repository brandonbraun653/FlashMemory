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

			

		private:
			Chimera::SPI::SPIClass_sPtr spi;	/**< SPI object used for talking with the flash chip */
			Chimera::SPI::Setup setup;			/**< SPI initialization settings */
		};
		typedef boost::shared_ptr<AT45> AT45_sPtr;
	}
}
#endif /* AT45DB081_HPP */
