/********************************************************************************
 * File Name:
 *	  test_at45db081_read.cpp
 *
 * Description:
 *	  Implements tests for the AT45DB081 driver
 *
 * 2019 | Brandon Braun | brandonbraun653@gmail.com
 ********************************************************************************/

/* Driver Includes */
#include "at45db081.hpp"

/* Testing Framework Includes */
#include <gtest/gtest.h>
#include <Chimera/spi.hpp>
#include "test_fixtures_at45db081.hpp"

/* C++ Includes */
#include <random>
#include <algorithm>
#include <functional>

#if defined( GMOCK_TEST )
/* Mock Includes */
#include <Chimera/mock/spi.hpp>
#include <gmock/gmock.h>

using ::testing::_;
using ::testing::AtLeast;
using ::testing::DoAll;
using ::testing::Exactly;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::SetArgPointee;
using ::testing::SetArrayArgument;

TEST_F( VirtualFlash, SRAMCommit_PreInitialization )
{
  EXPECT_EQ( Chimera::CommonStatusCodes::NOT_INITIALIZED,
             flash->sramCommit( Adesto::SRAMBuffer::BUFFER1, 0xFFFF, false ) );
}

#endif /* GMOCK_TEST */

#if defined( HW_TEST )

TEST_F( HardwareFlash, SRAMCommit_ReadWrite_Buffer1 )
{
  using namespace Adesto;

  const uint8_t arraySize                 = 10;
  std::array<uint8_t, arraySize> loadData = { 0x00, 0x22, 0x44, 0x66, 0x88, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE };
  std::array<uint8_t, arraySize> readData = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

  passInit();

  /*------------------------------------------------
  Start from the beginning
  ------------------------------------------------*/
  auto pageNumber = 0;
  auto pageOffset = 0;
  auto erase      = true;
  readData.fill(0);

  EXPECT_EQ( ErrCode::OK, flash->sramLoad( SRAMBuffer::BUFFER1, pageOffset, loadData.data(), loadData.size() ) );
  EXPECT_EQ( ErrCode::OK, flash->sramCommit( SRAMBuffer::BUFFER1, pageNumber, erase ) );

  while ( !flash->isDeviceReady() )
  {
    Chimera::delayMilliseconds( 100 );
  }

  EXPECT_EQ( ErrCode::OK, flash->directPageRead( pageNumber, pageOffset, readData.data(), arraySize ) );

  /*------------------------------------------------
  Let's try another page, just to be sure
  ------------------------------------------------*/
  pageNumber = 23;
  pageOffset = 0;
  erase      = true;
  readData.fill(0);

  EXPECT_EQ( ErrCode::OK, flash->sramLoad( SRAMBuffer::BUFFER1, pageOffset, loadData.data(), loadData.size() ) );
  EXPECT_EQ( ErrCode::OK, flash->sramCommit( SRAMBuffer::BUFFER1, pageNumber, erase ) );

  while ( !flash->isDeviceReady() )
  {
    Chimera::delayMilliseconds( 100 );
  }

  EXPECT_EQ( ErrCode::OK, flash->directPageRead( pageNumber, pageOffset, readData.data(), arraySize ) );

  /*------------------------------------------------
  Now combine page and offset
  ------------------------------------------------*/
  pageNumber = 71;
  pageOffset = 100;
  erase      = true;
  readData.fill( 0 );

  EXPECT_EQ( ErrCode::OK, flash->sramLoad( SRAMBuffer::BUFFER1, pageOffset, loadData.data(), loadData.size() ) );
  EXPECT_EQ( ErrCode::OK, flash->sramCommit( SRAMBuffer::BUFFER1, pageNumber, erase ) );

  while ( !flash->isDeviceReady() )
  {
    Chimera::delayMilliseconds( 100 );
  }

  EXPECT_EQ( ErrCode::OK, flash->directPageRead( pageNumber, pageOffset, readData.data(), arraySize ) );
}

TEST_F( HardwareFlash, SRAMCommit_ReadWrite_Buffer2 )
{
  using namespace Adesto;

  const uint8_t arraySize                 = 10;
  std::array<uint8_t, arraySize> loadData = { 0x00, 0x22, 0x44, 0x66, 0x88, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE };
  std::array<uint8_t, arraySize> readData = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

  passInit();

  /*------------------------------------------------
  Start from the beginning
  ------------------------------------------------*/
  auto pageNumber = 0;
  auto pageOffset = 0;
  auto erase      = true;
  readData.fill( 0 );

  EXPECT_EQ( ErrCode::OK, flash->sramLoad( SRAMBuffer::BUFFER2, pageOffset, loadData.data(), loadData.size() ) );
  EXPECT_EQ( ErrCode::OK, flash->sramCommit( SRAMBuffer::BUFFER2, pageNumber, erase ) );

  while ( !flash->isDeviceReady() )
  {
    Chimera::delayMilliseconds( 100 );
  }

  EXPECT_EQ( ErrCode::OK, flash->directPageRead( pageNumber, pageOffset, readData.data(), arraySize ) );

  /*------------------------------------------------
  Let's try another page, just to be sure
  ------------------------------------------------*/
  pageNumber = 84;
  pageOffset = 0;
  erase      = true;
  readData.fill( 0 );

  EXPECT_EQ( ErrCode::OK, flash->sramLoad( SRAMBuffer::BUFFER2, pageOffset, loadData.data(), loadData.size() ) );
  EXPECT_EQ( ErrCode::OK, flash->sramCommit( SRAMBuffer::BUFFER2, pageNumber, erase ) );

  while ( !flash->isDeviceReady() )
  {
    Chimera::delayMilliseconds( 100 );
  }

  EXPECT_EQ( ErrCode::OK, flash->directPageRead( pageNumber, pageOffset, readData.data(), arraySize ) );

  /*------------------------------------------------
  Now combine page and offset
  ------------------------------------------------*/
  pageNumber = 123;
  pageOffset = 65;
  erase      = true;
  readData.fill( 0 );

  EXPECT_EQ( ErrCode::OK, flash->sramLoad( SRAMBuffer::BUFFER2, pageOffset, loadData.data(), loadData.size() ) );
  EXPECT_EQ( ErrCode::OK, flash->sramCommit( SRAMBuffer::BUFFER2, pageNumber, erase ) );

  while ( !flash->isDeviceReady() )
  {
    Chimera::delayMilliseconds( 100 );
  }

  EXPECT_EQ( ErrCode::OK, flash->directPageRead( pageNumber, pageOffset, readData.data(), arraySize ) );
}

TEST_F( HardwareFlash, SRAMCommit_FullPage )
{
  using namespace Adesto;

  std::random_device rnd_device;
  std::mt19937 mersenne_engine{ rnd_device() };
  std::uniform_int_distribution<unsigned short> dist{ 0x00, 0xFF };

  auto gen = [&dist, &mersenne_engine]() { return dist( mersenne_engine ); };

  const uint8_t arraySize = 125;
  std::array<uint8_t, arraySize> loadData;
  std::array<uint8_t, arraySize> readData;

  std::generate(loadData.begin(), loadData.end(), gen);

  passInit();

  /*------------------------------------------------
  Start from the beginning
  ------------------------------------------------*/
  auto pageNumber = 33;
  auto pageOffset = 0;
  auto erase      = true;
  readData.fill( 0 );

  EXPECT_EQ( ErrCode::OK, flash->sramLoad( SRAMBuffer::BUFFER2, pageOffset, loadData.data(), loadData.size() ) );
  EXPECT_EQ( ErrCode::OK, flash->sramCommit( SRAMBuffer::BUFFER2, pageNumber, erase ) );

  while ( !flash->isDeviceReady() )
  {
    Chimera::delayMilliseconds( 100 );
  }

  EXPECT_EQ( ErrCode::OK, flash->directPageRead( pageNumber, pageOffset, readData.data(), arraySize ) );
}
#endif /* HW_TEST */
