/********************************************************************************
 *  File Name:
 *    test_fixtures_at45db081.hpp
 *
 *  Description:
 *    Provides several test fixtures used in testing the AT45DB081
 *
 *  2019 | Brandon Braun | brandonbraun653@gmail.com
 ********************************************************************************/

/* Driver Includes */
#include "at45db081.hpp"

/* Chimera Includes */
#include <Chimera/modules/memory/flash.hpp>

/* Test Driver Includes */
#include <gtest/gtest.h>


#if defined( GMOCK_TEST )
#include <gmock/gmock.h>
#include <Chimera/mock/spi.hpp>

using ::testing::NiceMock;

using ErrCode = Chimera::Modules::Memory::Status;

class VirtualFlash : public ::testing::Test
{
protected:
  NiceMock<Chimera::Mock::SPIMock> spi;
  Adesto::NORFlash::AT45 *flash;

  VirtualFlash()          = default;
  virtual ~VirtualFlash() = default;

  virtual void SetUp();

  virtual void TearDown();

  void testReset();

  void passInit();
};
#endif  /* GMOCK_TEST */

#if defined( HW_TEST )
#include <Chimera/spi.hpp>

#include <random>
#include <algorithm>
#include <functional>

using ErrCode = Chimera::Modules::Memory::Status;

class HardwareFlash : public ::testing::Test
{
protected:
  Chimera::SPI::SPIClass_sPtr spi;
  Adesto::NORFlash::AT45 *flash;

  HardwareFlash()         = default;
  virtual ~HardwareFlash() = default;

  virtual void SetUp();

  virtual void TearDown();

  void testReset();

  void passInit();

  template<std::size_t L>
  void randomFill( std::array<uint8_t, L> &data )
  {
    std::random_device rnd_device;
    std::mt19937 mersenne_engine{ rnd_device() };
    std::uniform_int_distribution<unsigned short> dist{ std::numeric_limits<uint8_t>::min(),
                                                        std::numeric_limits<uint8_t>::max() };
    auto gen = [&dist, &mersenne_engine]() { return static_cast<uint8_t>( dist( mersenne_engine ) ); };

    generate( data.begin(), data.end(), gen );
  }
};

#endif /* HW_TEST */
