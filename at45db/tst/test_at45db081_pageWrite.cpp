/********************************************************************************
 * File Name:
 *	  test_at45db081_pageWrite.cpp
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

TEST_F( VirtualFlash, PageWrite_PreInit )
{
  uint8_t someData = 0u;
  EXPECT_EQ( Chimera::CommonStatusCodes::NOT_INITIALIZED,
             flash->pageWrite( Adesto::NORFlash::SRAMBuffer::BUFFER1, 0, 0, &someData, sizeof( someData ) ) );
  EXPECT_EQ( Chimera::CommonStatusCodes::NOT_INITIALIZED,
             flash->pageWrite( Adesto::NORFlash::SRAMBuffer::BUFFER2, 0, 0, &someData, sizeof( someData ) ) );
}

TEST_F( VirtualFlash, PageWrite_NullPtrInput )
{
  passInit();

  EXPECT_EQ( Chimera::CommonStatusCodes::INVAL_FUNC_PARAM, flash->pageWrite( Adesto::NORFlash::SRAMBuffer::BUFFER1, 0, 0, nullptr, 1 ) );
  EXPECT_EQ( Chimera::CommonStatusCodes::INVAL_FUNC_PARAM, flash->pageWrite( Adesto::NORFlash::SRAMBuffer::BUFFER1, 0, 0, NULL, 1 ) );

  EXPECT_EQ( Chimera::CommonStatusCodes::INVAL_FUNC_PARAM, flash->pageWrite( Adesto::NORFlash::SRAMBuffer::BUFFER2, 0, 0, nullptr, 1 ) );
  EXPECT_EQ( Chimera::CommonStatusCodes::INVAL_FUNC_PARAM, flash->pageWrite( Adesto::NORFlash::SRAMBuffer::BUFFER2, 0, 0, NULL, 1 ) );
}
#endif /* GMOCK_TEST */

#if defined( HW_TEST )
using namespace Adesto::NORFlash;

/*------------------------------------------------
Binary
------------------------------------------------*/
TEST_F( HardwareFlash, PageWrite_Binary_NullOffset_Buffer1 )
{
  static constexpr uint8_t len                = 10;
  static constexpr uint16_t offset            = 0;
  static constexpr uint32_t page              = 80;
  static constexpr uint16_t pageSize          = PAGE_SIZE_BINARY;
  static constexpr uint8_t uniquePageResetVal = 0xA3;
  static constexpr SRAMBuffer sramBuffer      = SRAMBuffer::BUFFER1;

  std::array<uint8_t, len> readData;
  std::array<uint8_t, pageSize> pageData;
  std::array<uint8_t, len> writeData;

  /*------------------------------------------------
  Initialize
  ------------------------------------------------*/
  readData.fill( 0 );
  randomFill( writeData );

  passInit();
  flash->useBinaryPageSize();
  ASSERT_EQ( pageSize, flash->getPageSize() );

  /* Clear out the SRAM buffer so we don't get random data written to memory */
  pageData.fill( uniquePageResetVal );
  flash->sramLoad( sramBuffer, 0, pageData.data(), pageSize );
  pageData.fill( 0 );

  /*------------------------------------------------
  Call FUT
  ------------------------------------------------*/
  EXPECT_EQ( Chimera::CommonStatusCodes::OK,
             flash->pageWrite( sramBuffer, offset, page, writeData.data(), len ) );

  while ( !flash->isDeviceReady() )
  {
    Chimera::delayMilliseconds( 10 );
  }

  /*------------------------------------------------
  Verify
  ------------------------------------------------*/
  flash->directPageRead( page, offset, readData.data(), len );
  flash->directPageRead( page, 0, pageData.data(), pageSize );

  /* Explicitly written data matches read data */
  EXPECT_EQ( 0, memcmp( readData.data(), writeData.data(), len ) );

  /* Bytes before data are erased */
  EXPECT_EQ( true, std::all_of( pageData.begin(), pageData.begin() + offset,
                                []( uint8_t val ) { return ( val == uniquePageResetVal ); } ) );

  /* Bytes after data are erased */
  EXPECT_EQ( true, std::all_of( pageData.begin() + offset + len, pageData.end(),
                                []( uint8_t val ) { return ( val == uniquePageResetVal ); } ) );

  /* No program or erase error occurred */
  EXPECT_EQ( Chimera::CommonStatusCodes::OK, flash->isErasePgmError() );
}

TEST_F( HardwareFlash, PageWrite_Binary_NullOffset_Buffer2 )
{
  static constexpr uint8_t len                = 10;
  static constexpr uint16_t offset            = 0;
  static constexpr uint32_t page              = 8;
  static constexpr uint16_t pageSize          = PAGE_SIZE_BINARY;
  static constexpr uint8_t uniquePageResetVal = 0x33;
  static constexpr SRAMBuffer sramBuffer      = SRAMBuffer::BUFFER2;

  std::array<uint8_t, len> readData;
  std::array<uint8_t, pageSize> pageData;
  std::array<uint8_t, len> writeData;

  /*------------------------------------------------
  Initialize
  ------------------------------------------------*/
  readData.fill( 0 );
  randomFill( writeData );

  passInit();
  flash->useBinaryPageSize();
  ASSERT_EQ( pageSize, flash->getPageSize() );

  /* Clear out the SRAM buffer so we don't get random data written to memory */
  pageData.fill( uniquePageResetVal );
  flash->sramLoad( sramBuffer, 0, pageData.data(), pageSize );
  pageData.fill( 0 );

  /*------------------------------------------------
  Call FUT
  ------------------------------------------------*/
  EXPECT_EQ( Chimera::CommonStatusCodes::OK,
             flash->pageWrite( sramBuffer, offset, page, writeData.data(), len ) );

  while ( !flash->isDeviceReady() )
  {
    Chimera::delayMilliseconds( 10 );
  }

  /*------------------------------------------------
  Verify
  ------------------------------------------------*/
  flash->directPageRead( page, offset, readData.data(), len );
  flash->directPageRead( page, 0, pageData.data(), pageSize );

  /* Explicitly written data matches read data */
  EXPECT_EQ( 0, memcmp( readData.data(), writeData.data(), len ) );

  /* Bytes before data are erased */
  EXPECT_EQ( true, std::all_of( pageData.begin(), pageData.begin() + offset,
                                []( uint8_t val ) { return ( val == uniquePageResetVal ); } ) );

  /* Bytes after data are erased */
  EXPECT_EQ( true, std::all_of( pageData.begin() + offset + len, pageData.end(),
                                []( uint8_t val ) { return ( val == uniquePageResetVal ); } ) );

  /* No program or erase error occurred */
  EXPECT_EQ( Chimera::CommonStatusCodes::OK, flash->isErasePgmError() );
}

TEST_F( HardwareFlash, PageWrite_Binary_PositiveOffset_Buffer1 )
{
  static constexpr uint8_t len                = 10;
  static constexpr uint16_t offset            = 63;
  static constexpr uint32_t page              = 24;
  static constexpr uint16_t pageSize          = PAGE_SIZE_BINARY;
  static constexpr uint8_t uniquePageResetVal = 0x27;
  static constexpr SRAMBuffer sramBuffer      = SRAMBuffer::BUFFER1;

  std::array<uint8_t, len> readData;
  std::array<uint8_t, pageSize> pageData;
  std::array<uint8_t, len> writeData;

  /*------------------------------------------------
  Initialize
  ------------------------------------------------*/
  readData.fill( 0 );
  randomFill( writeData );

  passInit();
  flash->useBinaryPageSize();
  ASSERT_EQ( pageSize, flash->getPageSize() );

  /* Clear out the SRAM buffer so we don't get random data written to memory */
  pageData.fill( uniquePageResetVal );
  flash->sramLoad( sramBuffer, 0, pageData.data(), pageSize );
  pageData.fill( 0 );

  /*------------------------------------------------
  Call FUT
  ------------------------------------------------*/
  EXPECT_EQ( Chimera::CommonStatusCodes::OK,
             flash->pageWrite( sramBuffer, offset, page, writeData.data(), len ) );

  while ( !flash->isDeviceReady() )
  {
    Chimera::delayMilliseconds( 10 );
  }

  /*------------------------------------------------
  Verify
  ------------------------------------------------*/
  flash->directPageRead( page, offset, readData.data(), len );
  flash->directPageRead( page, 0, pageData.data(), pageSize );

  /* Explicitly written data matches read data */
  EXPECT_EQ( 0, memcmp( readData.data(), writeData.data(), len ) );

  /* Bytes before data are erased */
  EXPECT_EQ( true, std::all_of( pageData.begin(), pageData.begin() + offset,
                                []( uint8_t val ) { return ( val == uniquePageResetVal ); } ) );

  /* Bytes after data are erased */
  EXPECT_EQ( true, std::all_of( pageData.begin() + offset + len, pageData.end(),
                                []( uint8_t val ) { return ( val == uniquePageResetVal ); } ) );

  /* No program or erase error occurred */
  EXPECT_EQ( Chimera::CommonStatusCodes::OK, flash->isErasePgmError() );
}

TEST_F( HardwareFlash, PageWrite_Binary_PositiveOffset_Buffer2 )
{
  static constexpr uint8_t len                = 17;
  static constexpr uint16_t offset            = 133;
  static constexpr uint32_t page              = 115;
  static constexpr uint16_t pageSize          = PAGE_SIZE_BINARY;
  static constexpr uint8_t uniquePageResetVal = 0xAA;
  static constexpr SRAMBuffer sramBuffer      = SRAMBuffer::BUFFER2;

  std::array<uint8_t, len> readData;
  std::array<uint8_t, pageSize> pageData;
  std::array<uint8_t, len> writeData;

  /*------------------------------------------------
  Initialize
  ------------------------------------------------*/
  readData.fill( 0 );
  randomFill( writeData );

  passInit();
  flash->useBinaryPageSize();
  ASSERT_EQ( pageSize, flash->getPageSize() );

  /* Clear out the SRAM buffer so we don't get random data written to memory */
  pageData.fill( uniquePageResetVal );
  flash->sramLoad( sramBuffer, 0, pageData.data(), pageSize );
  pageData.fill( 0 );

  /*------------------------------------------------
  Call FUT
  ------------------------------------------------*/
  EXPECT_EQ( Chimera::CommonStatusCodes::OK,
             flash->pageWrite( sramBuffer, offset, page, writeData.data(), len ) );

  while ( !flash->isDeviceReady() )
  {
    Chimera::delayMilliseconds( 10 );
  }

  /*------------------------------------------------
  Verify
  ------------------------------------------------*/
  flash->directPageRead( page, offset, readData.data(), len );
  flash->directPageRead( page, 0, pageData.data(), pageSize );

  /* Explicitly written data matches read data */
  EXPECT_EQ( 0, memcmp( readData.data(), writeData.data(), len ) );

  /* Bytes before data are erased */
  EXPECT_EQ( true, std::all_of( pageData.begin(), pageData.begin() + offset,
                                []( uint8_t val ) { return ( val == uniquePageResetVal ); } ) );

  /* Bytes after data are erased */
  EXPECT_EQ( true, std::all_of( pageData.begin() + offset + len, pageData.end(),
                                []( uint8_t val ) { return ( val == uniquePageResetVal ); } ) );

  /* No program or erase error occurred */
  EXPECT_EQ( Chimera::CommonStatusCodes::OK, flash->isErasePgmError() );
}


/*------------------------------------------------
Extended
------------------------------------------------*/
TEST_F( HardwareFlash, PageWrite_Extended_NullOffset_Buffer1 )
{
  static constexpr uint8_t len                = 154;
  static constexpr uint16_t offset            = 0;
  static constexpr uint32_t page              = 4000;
  static constexpr uint16_t pageSize          = PAGE_SIZE_EXTENDED;
  static constexpr uint8_t uniquePageResetVal = 0x99;
  static constexpr SRAMBuffer sramBuffer      = SRAMBuffer::BUFFER1;

  std::array<uint8_t, len> readData;
  std::array<uint8_t, pageSize> pageData;
  std::array<uint8_t, len> writeData;

  /*------------------------------------------------
  Initialize
  ------------------------------------------------*/
  readData.fill( 0 );
  randomFill( writeData );

  passInit();
  flash->useExtendedPageSize();
  ASSERT_EQ( pageSize, flash->getPageSize() );

  /* Clear out the SRAM buffer so we don't get random data written to memory */
  pageData.fill( uniquePageResetVal );
  flash->sramLoad( sramBuffer, 0, pageData.data(), pageSize );
  pageData.fill( 0 );

  /*------------------------------------------------
  Call FUT
  ------------------------------------------------*/
  EXPECT_EQ( Chimera::CommonStatusCodes::OK,
             flash->pageWrite( sramBuffer, offset, page, writeData.data(), len ) );

  while ( !flash->isDeviceReady() )
  {
    Chimera::delayMilliseconds( 10 );
  }

  /*------------------------------------------------
  Verify
  ------------------------------------------------*/
  flash->directPageRead( page, offset, readData.data(), len );
  flash->directPageRead( page, 0, pageData.data(), pageSize );

  /* Explicitly written data matches read data */
  EXPECT_EQ( 0, memcmp( readData.data(), writeData.data(), len ) );

  /* Bytes before data are erased */
  EXPECT_EQ( true, std::all_of( pageData.begin(), pageData.begin() + offset,
                                []( uint8_t val ) { return ( val == uniquePageResetVal ); } ) );

  /* Bytes after data are erased */
  EXPECT_EQ( true, std::all_of( pageData.begin() + offset + len, pageData.end(),
                                []( uint8_t val ) { return ( val == uniquePageResetVal ); } ) );

  /* No program or erase error occurred */
  EXPECT_EQ( Chimera::CommonStatusCodes::OK, flash->isErasePgmError() );
}

TEST_F( HardwareFlash, PageWrite_Extended_NullOffset_Buffer2 )
{
  static constexpr uint8_t len                = 43;
  static constexpr uint16_t offset            = 0;
  static constexpr uint32_t page              = 1234;
  static constexpr uint16_t pageSize          = PAGE_SIZE_EXTENDED;
  static constexpr uint8_t uniquePageResetVal = 0xDD;
  static constexpr SRAMBuffer sramBuffer      = SRAMBuffer::BUFFER2;

  std::array<uint8_t, len> readData;
  std::array<uint8_t, pageSize> pageData;
  std::array<uint8_t, len> writeData;

  /*------------------------------------------------
  Initialize
  ------------------------------------------------*/
  readData.fill( 0 );
  randomFill( writeData );

  passInit();
  flash->useExtendedPageSize();
  ASSERT_EQ( pageSize, flash->getPageSize() );

  /* Clear out the SRAM buffer so we don't get random data written to memory */
  pageData.fill( uniquePageResetVal );
  flash->sramLoad( sramBuffer, 0, pageData.data(), pageSize );
  pageData.fill( 0 );

  /*------------------------------------------------
  Call FUT
  ------------------------------------------------*/
  EXPECT_EQ( Chimera::CommonStatusCodes::OK,
             flash->pageWrite( sramBuffer, offset, page, writeData.data(), len ) );

  while ( !flash->isDeviceReady() )
  {
    Chimera::delayMilliseconds( 10 );
  }

  /*------------------------------------------------
  Verify
  ------------------------------------------------*/
  flash->directPageRead( page, offset, readData.data(), len );
  flash->directPageRead( page, 0, pageData.data(), pageSize );

  /* Explicitly written data matches read data */
  EXPECT_EQ( 0, memcmp( readData.data(), writeData.data(), len ) );

  /* Bytes before data are erased */
  EXPECT_EQ( true, std::all_of( pageData.begin(), pageData.begin() + offset,
                                []( uint8_t val ) { return ( val == uniquePageResetVal ); } ) );

  /* Bytes after data are erased */
  EXPECT_EQ( true, std::all_of( pageData.begin() + offset + len, pageData.end(),
                                []( uint8_t val ) { return ( val == uniquePageResetVal ); } ) );

  /* No program or erase error occurred */
  EXPECT_EQ( Chimera::CommonStatusCodes::OK, flash->isErasePgmError() );
}

TEST_F( HardwareFlash, PageWrite_Extended_PositiveOffset_Buffer1 )
{
  static constexpr uint8_t len                = 180;
  static constexpr uint16_t offset            = 10;
  static constexpr uint32_t page              = 32;
  static constexpr uint16_t pageSize          = PAGE_SIZE_EXTENDED;
  static constexpr uint8_t uniquePageResetVal = 0x44;
  static constexpr SRAMBuffer sramBuffer      = SRAMBuffer::BUFFER1;

  std::array<uint8_t, len> readData;
  std::array<uint8_t, pageSize> pageData;
  std::array<uint8_t, len> writeData;

  /*------------------------------------------------
  Initialize
  ------------------------------------------------*/
  readData.fill( 0 );
  randomFill( writeData );

  passInit();
  flash->useExtendedPageSize();
  ASSERT_EQ( pageSize, flash->getPageSize() );

  /* Clear out the SRAM buffer so we don't get random data written to memory */
  pageData.fill( uniquePageResetVal );
  flash->sramLoad( sramBuffer, 0, pageData.data(), pageSize );
  pageData.fill( 0 );

  /*------------------------------------------------
  Call FUT
  ------------------------------------------------*/
  EXPECT_EQ( Chimera::CommonStatusCodes::OK,
             flash->pageWrite( sramBuffer, offset, page, writeData.data(), len ) );

  while ( !flash->isDeviceReady() )
  {
    Chimera::delayMilliseconds( 10 );
  }

  /*------------------------------------------------
  Verify
  ------------------------------------------------*/
  flash->directPageRead( page, offset, readData.data(), len );
  flash->directPageRead( page, 0, pageData.data(), pageSize );

  /* Explicitly written data matches read data */
  EXPECT_EQ( 0, memcmp( readData.data(), writeData.data(), len ) );

  /* Bytes before data are erased */
  EXPECT_EQ( true, std::all_of( pageData.begin(), pageData.begin() + offset,
                                []( uint8_t val ) { return ( val == uniquePageResetVal ); } ) );

  /* Bytes after data are erased */
  EXPECT_EQ( true, std::all_of( pageData.begin() + offset + len, pageData.end(),
                                []( uint8_t val ) { return ( val == uniquePageResetVal ); } ) );

  /* No program or erase error occurred */
  EXPECT_EQ( Chimera::CommonStatusCodes::OK, flash->isErasePgmError() );
}

TEST_F( HardwareFlash, PageWrite_Extended_PositiveOffset_Buffer2 )
{
  static constexpr uint8_t len                = 43;
  static constexpr uint16_t offset            = 16;
  static constexpr uint32_t page              = 1000;
  static constexpr uint16_t pageSize          = PAGE_SIZE_EXTENDED;
  static constexpr uint8_t uniquePageResetVal = 0xAA;
  static constexpr SRAMBuffer sramBuffer      = SRAMBuffer::BUFFER2;

  std::array<uint8_t, len> readData;
  std::array<uint8_t, pageSize> pageData;
  std::array<uint8_t, len> writeData;

  /*------------------------------------------------
  Initialize
  ------------------------------------------------*/
  readData.fill( 0 );
  randomFill( writeData );

  passInit();
  flash->useExtendedPageSize();
  ASSERT_EQ( pageSize, flash->getPageSize() );

  /* Clear out the SRAM buffer so we don't get random data written to memory */
  pageData.fill( uniquePageResetVal );
  flash->sramLoad( sramBuffer, 0, pageData.data(), pageSize );
  pageData.fill( 0 );

  /*------------------------------------------------
  Call FUT
  ------------------------------------------------*/
  EXPECT_EQ( Chimera::CommonStatusCodes::OK,
             flash->pageWrite( sramBuffer, offset, page, writeData.data(), len ) );

  while ( !flash->isDeviceReady() )
  {
    Chimera::delayMilliseconds( 10 );
  }

  /*------------------------------------------------
  Verify
  ------------------------------------------------*/
  flash->directPageRead( page, offset, readData.data(), len );
  flash->directPageRead( page, 0, pageData.data(), pageSize );

  /* Explicitly written data matches read data */
  EXPECT_EQ( 0, memcmp( readData.data(), writeData.data(), len ) );

  /* Bytes before data are erased */
  EXPECT_EQ( true, std::all_of( pageData.begin(), pageData.begin() + offset,
                                []( uint8_t val ) { return ( val == uniquePageResetVal ); } ) );

  /* Bytes after data are erased */
  EXPECT_EQ( true, std::all_of( pageData.begin() + offset + len, pageData.end(),
                                []( uint8_t val ) { return ( val == uniquePageResetVal ); } ) );

  /* No program or erase error occurred */
  EXPECT_EQ( Chimera::CommonStatusCodes::OK, flash->isErasePgmError() );
}

#endif /* HW_TEST */
