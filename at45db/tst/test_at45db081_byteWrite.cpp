/********************************************************************************
 * File Name:
 *	  test_at45db081_byteWrite.cpp
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

#if defined( GMOCK_TEST )
/* Mock Includes */
#include <Chimera/mock/spi.hpp>
#include <gmock/gmock.h>

TEST_F( VirtualFlash, ByteWrite_PreInit )
{
  uint8_t someData = 0u;
  EXPECT_EQ( Chimera::CommonStatusCodes::NOT_INITIALIZED, flash->byteWrite( 0, 0, &someData, sizeof( someData ) ) );
}

TEST_F( VirtualFlash, ByteWrite_NullPtrInput )
{
  passInit();

  EXPECT_EQ( Chimera::CommonStatusCodes::INVAL_FUNC_PARAM, flash->byteWrite( 0, 0, nullptr, 1 ) );
  EXPECT_EQ( Chimera::CommonStatusCodes::INVAL_FUNC_PARAM, flash->byteWrite( 0, 0, NULL, 1 ) );
}
#endif /* GMOCK_TEST */

#if defined( HW_TEST )
using namespace Adesto::NORFlash;

TEST_F( HardwareFlash, ByteWrite_Binary_NullOffset )
{
  static constexpr uint8_t len     = 10;
  static constexpr uint16_t offset = 0;
  static constexpr uint32_t page   = 80;

  std::array<uint8_t, len> writeData;
  std::array<uint8_t, len> readData;

  /*------------------------------------------------
  Initialize
  ------------------------------------------------*/
  readData.fill( 0 );
  randomFill( writeData );

  passInit();
  flash->useBinaryPageSize();
  ASSERT_EQ( PAGE_SIZE_BINARY, flash->getPageSize() );

  flash->erasePage( page );
  while ( !flash->isDeviceReady() )
  {
    Chimera::delayMilliseconds( 10 );
  }

  /*------------------------------------------------
  Call FUT
  ------------------------------------------------*/
  EXPECT_EQ( Chimera::CommonStatusCodes::OK,
             flash->byteWrite( page, offset, writeData.data(), len ) );

  while ( !flash->isDeviceReady() )
  {
    Chimera::delayMilliseconds( 10 );
  }

  /*------------------------------------------------
  Verify
  ------------------------------------------------*/
  flash->directPageRead( page, offset, readData.data(), len );
  EXPECT_EQ( 0, memcmp( readData.data(), writeData.data(), len ) );
  EXPECT_EQ( Chimera::CommonStatusCodes::OK, flash->isErasePgmError() );
}

TEST_F( HardwareFlash, ByteWrite_Binary_PositiveOffset )
{
  static constexpr uint8_t len     = 54;
  static constexpr uint16_t offset = 23;
  static constexpr uint32_t page   = 82;

  std::array<uint8_t, len> writeData;
  std::array<uint8_t, len> readData;

  /*------------------------------------------------
  Initialize
  ------------------------------------------------*/
  readData.fill( 0 );
  randomFill( writeData );

  passInit();
  flash->useBinaryPageSize();
  ASSERT_EQ( PAGE_SIZE_BINARY, flash->getPageSize() );

  flash->erasePage( page );
  while ( !flash->isDeviceReady() )
  {
    Chimera::delayMilliseconds( 10 );
  }

  /*------------------------------------------------
  Call FUT
  ------------------------------------------------*/
  EXPECT_EQ( Chimera::CommonStatusCodes::OK,
             flash->byteWrite( page, offset, writeData.data(), len ) );

  while ( !flash->isDeviceReady() )
  {
    Chimera::delayMilliseconds( 10 );
  }

  /*------------------------------------------------
  Verify
  ------------------------------------------------*/
  flash->directPageRead( page, offset, readData.data(), len );
  EXPECT_EQ( 0, memcmp( readData.data(), writeData.data(), len ) );
  EXPECT_EQ( Chimera::CommonStatusCodes::OK, flash->isErasePgmError() );
}

TEST_F( HardwareFlash, ByteWrite_Extended_NullOffset )
{
  static constexpr uint8_t len     = 10;
  static constexpr uint16_t offset = 0;
  static constexpr uint32_t page   = 33;

  std::array<uint8_t, len> writeData;
  std::array<uint8_t, len> readData;

  /*------------------------------------------------
  Initialize
  ------------------------------------------------*/
  readData.fill( 0 );
  randomFill( writeData );

  passInit();
  flash->useExtendedPageSize();
  ASSERT_EQ( PAGE_SIZE_EXTENDED, flash->getPageSize() );

  flash->erasePage( page );
  while ( !flash->isDeviceReady() )
  {
    Chimera::delayMilliseconds( 10 );
  }

  /*------------------------------------------------
  Call FUT
  ------------------------------------------------*/
  EXPECT_EQ( Chimera::CommonStatusCodes::OK,
             flash->byteWrite( page, offset, writeData.data(), len ) );

  while ( !flash->isDeviceReady() )
  {
    Chimera::delayMilliseconds( 10 );
  }

  /*------------------------------------------------
  Verify
  ------------------------------------------------*/
  flash->directPageRead( page, offset, readData.data(), len );
  EXPECT_EQ( 0, memcmp( readData.data(), writeData.data(), len ) );
  EXPECT_EQ( Chimera::CommonStatusCodes::OK, flash->isErasePgmError() );
}

TEST_F( HardwareFlash, ByteWrite_Extended_PositiveOffset )
{
  static constexpr uint8_t len     = 83;
  static constexpr uint16_t offset = 62;
  static constexpr uint32_t page   = 94;

  std::array<uint8_t, len> writeData;
  std::array<uint8_t, len> readData;

  /*------------------------------------------------
  Initialize
  ------------------------------------------------*/
  readData.fill( 0 );
  randomFill( writeData );

  passInit();
  flash->useExtendedPageSize();
  ASSERT_EQ( PAGE_SIZE_EXTENDED, flash->getPageSize() );

  flash->erasePage( page );
  while ( !flash->isDeviceReady() )
  {
    Chimera::delayMilliseconds( 10 );
  }

  /*------------------------------------------------
  Call FUT
  ------------------------------------------------*/
  EXPECT_EQ( Chimera::CommonStatusCodes::OK,
             flash->byteWrite( page, offset, writeData.data(), len ) );

  while ( !flash->isDeviceReady() )
  {
    Chimera::delayMilliseconds( 10 );
  }

  /*------------------------------------------------
  Verify
  ------------------------------------------------*/
  flash->directPageRead( page, offset, readData.data(), len );
  EXPECT_EQ( 0, memcmp( readData.data(), writeData.data(), len ) );
  EXPECT_EQ( Chimera::CommonStatusCodes::OK, flash->isErasePgmError() );
}
#endif /* HW_TEST */
