/********************************************************************************
 * File Name:
 *	  test_at45db081_readModifyWrite.cpp
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

TEST_F( VirtualFlash, ReadModifyWrite_PreInit )
{
  EXPECT_EQ( Chimera::CommonStatusCodes::NOT_INITIALIZED,
             flash->readModifyWrite( Adesto::SRAMBuffer::BUFFER1, 0, 0, nullptr, 0 ) );
}

TEST_F( VirtualFlash, ReadModifyWrite_Nullptr )
{
  passInit();

  EXPECT_EQ( Chimera::CommonStatusCodes::INVAL_FUNC_PARAM,
             flash->readModifyWrite( Adesto::SRAMBuffer::BUFFER1, 0, 0, nullptr, 0 ) );

  EXPECT_EQ( Chimera::CommonStatusCodes::INVAL_FUNC_PARAM,
             flash->readModifyWrite( Adesto::SRAMBuffer::BUFFER2, 0, 0, nullptr, 0 ) );
}

#endif /* GMOCK_TEST */

#if defined( HW_TEST )
using namespace Adesto;
using namespace Adesto::NORFlash;

TEST_F( HardwareFlash, RMW_Binary_Offset_Buffer1 )
{
  static constexpr uint8_t len                = 10;
  static constexpr uint16_t offset            = 85;
  static constexpr uint32_t page              = 66;
  static constexpr uint16_t pageSize          = PAGE_SIZE_BINARY;
  static constexpr SRAMBuffer sramBuffer      = SRAMBuffer::BUFFER1;

  std::array<uint8_t, len> readData;
  std::array<uint8_t, len> writeData;

  std::array<uint8_t, pageSize> pageFillData;
  std::array<uint8_t, pageSize> pageReadData;
  std::array<uint8_t, pageSize> pageExptData;

  std::array<unsigned short, pageSize> randomData1;
  std::array<unsigned short, pageSize> randomData2;

  /*------------------------------------------------
  Initialize
  ------------------------------------------------*/
  readData.fill( 0 );
  pageReadData.fill( 0 );

  /* Generate random data to write to subset of the page */
  randomFill( randomData1 );
  for ( uint8_t x = 0; x < len; x++ )
  {
    writeData[ x ] = static_cast<uint8_t>( randomData1[ x ] & 0xFF );
  }

  /* Generate random data to fill the page with */
  randomFill( randomData2 );
  for ( uint16_t x = 0; x < pageSize; x++ )
  {
    pageFillData[ x ] = static_cast<uint8_t>( randomData2[ x ] & 0xFF );
  }

  /* Pre-generate the expected data that should fill the page */
  pageExptData = pageFillData;
  for ( uint16_t x = 0; x < len; x++ )
  {
    pageExptData[ offset + x ] = writeData[ x ];
  }

  passInit();
  flash->useBinaryPageSize();
  ASSERT_EQ( pageSize, flash->getPageSize() );
  ASSERT_EQ( Chimera::CommonStatusCodes::OK, flash->pageWrite( sramBuffer, 0, page, pageFillData.data(), pageSize ) );

  /*------------------------------------------------
  Call FUT
  ------------------------------------------------*/
  EXPECT_EQ( Chimera::CommonStatusCodes::OK,
             flash->readModifyWrite( sramBuffer, page, offset, reinterpret_cast<uint8_t *>( writeData.data() ), len ) );

  while ( !flash->isDeviceReady() )
  {
    Chimera::delayMilliseconds( 10 );
  }

  /*------------------------------------------------
  Verify
  ------------------------------------------------*/
  flash->directPageRead( page, offset, readData.data(), len );
  flash->directPageRead( page, 0, pageReadData.data(), pageSize );

  /* Explicitly written data matches read data */
  EXPECT_EQ( 0, memcmp( readData.data(), writeData.data(), len ) );

  /* Entire page matches expected */
  EXPECT_EQ( 0, memcmp( pageReadData.data(), pageExptData.data(), pageSize ) );

  /* No program or erase error occurred */
  EXPECT_EQ( Chimera::CommonStatusCodes::OK, flash->isErasePgmError() );
}

TEST_F( HardwareFlash, RMW_Binary_Offset_Buffer2 )
{
  static constexpr uint8_t len           = 23;
  static constexpr uint16_t offset       = 150;
  static constexpr uint32_t page         = 543;
  static constexpr uint16_t pageSize     = PAGE_SIZE_BINARY;
  static constexpr SRAMBuffer sramBuffer = SRAMBuffer::BUFFER2;

  std::array<uint8_t, len> readData;
  std::array<uint8_t, len> writeData;

  std::array<uint8_t, pageSize> pageFillData;
  std::array<uint8_t, pageSize> pageReadData;
  std::array<uint8_t, pageSize> pageExptData;

  std::array<unsigned short, pageSize> randomData1;
  std::array<unsigned short, pageSize> randomData2;

  /*------------------------------------------------
  Initialize
  ------------------------------------------------*/
  readData.fill( 0 );
  pageReadData.fill( 0 );

  /* Generate random data to write to subset of the page */
  randomFill( randomData1 );
  for ( uint8_t x = 0; x < len; x++ )
  {
    writeData[ x ] = static_cast<uint8_t>( randomData1[ x ] & 0xFF );
  }

  /* Generate random data to fill the page with */
  randomFill( randomData2 );
  for ( uint16_t x = 0; x < pageSize; x++ )
  {
    pageFillData[ x ] = static_cast<uint8_t>( randomData2[ x ] & 0xFF );
  }

  /* Pre-generate the expected data that should fill the page */
  pageExptData = pageFillData;
  for ( uint16_t x = 0; x < len; x++ )
  {
    pageExptData[ offset + x ] = writeData[ x ];
  }

  passInit();
  flash->useBinaryPageSize();
  ASSERT_EQ( pageSize, flash->getPageSize() );
  ASSERT_EQ( Chimera::CommonStatusCodes::OK, flash->pageWrite( sramBuffer, 0, page, pageFillData.data(), pageSize ) );

  /*------------------------------------------------
  Call FUT
  ------------------------------------------------*/
  EXPECT_EQ( Chimera::CommonStatusCodes::OK,
             flash->readModifyWrite( sramBuffer, page, offset, reinterpret_cast<uint8_t *>( writeData.data() ), len ) );

  while ( !flash->isDeviceReady() )
  {
    Chimera::delayMilliseconds( 10 );
  }

  /*------------------------------------------------
  Verify
  ------------------------------------------------*/
  flash->directPageRead( page, offset, readData.data(), len );
  flash->directPageRead( page, 0, pageReadData.data(), pageSize );

  /* Explicitly written data matches read data */
  EXPECT_EQ( 0, memcmp( readData.data(), writeData.data(), len ) );

  /* Entire page matches expected */
  EXPECT_EQ( 0, memcmp( pageReadData.data(), pageExptData.data(), pageSize ) );

  /* No program or erase error occurred */
  EXPECT_EQ( Chimera::CommonStatusCodes::OK, flash->isErasePgmError() );
}

TEST_F( HardwareFlash, RMW_Extended_Offset_Buffer1 )
{
  static constexpr uint8_t len           = 100;
  static constexpr uint16_t offset       = 10;
  static constexpr uint32_t page         = 15;
  static constexpr uint16_t pageSize     = PAGE_SIZE_EXTENDED;
  static constexpr SRAMBuffer sramBuffer = SRAMBuffer::BUFFER1;

  std::array<uint8_t, len> readData;
  std::array<uint8_t, len> writeData;

  std::array<uint8_t, pageSize> pageFillData;
  std::array<uint8_t, pageSize> pageReadData;
  std::array<uint8_t, pageSize> pageExptData;

  std::array<unsigned short, pageSize> randomData1;
  std::array<unsigned short, pageSize> randomData2;

  /*------------------------------------------------
  Initialize
  ------------------------------------------------*/
  readData.fill( 0 );
  pageReadData.fill( 0 );

  /* Generate random data to write to subset of the page */
  randomFill( randomData1 );
  for ( uint8_t x = 0; x < len; x++ )
  {
    writeData[ x ] = static_cast<uint8_t>( randomData1[ x ] & 0xFF );
  }

  /* Generate random data to fill the page with */
  randomFill( randomData2 );
  for ( uint16_t x = 0; x < pageSize; x++ )
  {
    pageFillData[ x ] = static_cast<uint8_t>( randomData2[ x ] & 0xFF );
  }

  /* Pre-generate the expected data that should fill the page */
  pageExptData = pageFillData;
  for ( uint16_t x = 0; x < len; x++ )
  {
    pageExptData[ offset + x ] = writeData[ x ];
  }

  passInit();
  flash->useExtendedPageSize();
  ASSERT_EQ( pageSize, flash->getPageSize() );
  ASSERT_EQ( Chimera::CommonStatusCodes::OK, flash->pageWrite( sramBuffer, 0, page, pageFillData.data(), pageSize ) );

  /*------------------------------------------------
  Call FUT
  ------------------------------------------------*/
  EXPECT_EQ( Chimera::CommonStatusCodes::OK,
             flash->readModifyWrite( sramBuffer, page, offset, reinterpret_cast<uint8_t *>( writeData.data() ), len ) );

  while ( !flash->isDeviceReady() )
  {
    Chimera::delayMilliseconds( 10 );
  }

  /*------------------------------------------------
  Verify
  ------------------------------------------------*/
  flash->directPageRead( page, offset, readData.data(), len );
  flash->directPageRead( page, 0, pageReadData.data(), pageSize );

  /* Explicitly written data matches read data */
  EXPECT_EQ( 0, memcmp( readData.data(), writeData.data(), len ) );

  /* Entire page matches expected */
  EXPECT_EQ( 0, memcmp( pageReadData.data(), pageExptData.data(), pageSize ) );

  /* No program or erase error occurred */
  EXPECT_EQ( Chimera::CommonStatusCodes::OK, flash->isErasePgmError() );
}

TEST_F( HardwareFlash, RMW_Extended_Offset_Buffer2 )
{
  static constexpr uint8_t len           = 180;
  static constexpr uint16_t offset       = 10;
  static constexpr uint32_t page         = 789;
  static constexpr uint16_t pageSize     = PAGE_SIZE_EXTENDED;
  static constexpr SRAMBuffer sramBuffer = SRAMBuffer::BUFFER2;

  std::array<uint8_t, len> readData;
  std::array<uint8_t, len> writeData;

  std::array<uint8_t, pageSize> pageFillData;
  std::array<uint8_t, pageSize> pageReadData;
  std::array<uint8_t, pageSize> pageExptData;

  std::array<unsigned short, pageSize> randomData1;
  std::array<unsigned short, pageSize> randomData2;

  /*------------------------------------------------
  Initialize
  ------------------------------------------------*/
  readData.fill( 0 );
  pageReadData.fill( 0 );

  /* Generate random data to write to subset of the page */
  randomFill( randomData1 );
  for ( uint8_t x = 0; x < len; x++ )
  {
    writeData[ x ] = static_cast<uint8_t>( randomData1[ x ] & 0xFF );
  }

  /* Generate random data to fill the page with */
  randomFill( randomData2 );
  for ( uint16_t x = 0; x < pageSize; x++ )
  {
    pageFillData[ x ] = static_cast<uint8_t>( randomData2[ x ] & 0xFF );
  }

  /* Pre-generate the expected data that should fill the page */
  pageExptData = pageFillData;
  for ( uint16_t x = 0; x < len; x++ )
  {
    pageExptData[ offset + x ] = writeData[ x ];
  }

  passInit();
  flash->useExtendedPageSize();
  ASSERT_EQ( pageSize, flash->getPageSize() );
  ASSERT_EQ( Chimera::CommonStatusCodes::OK, flash->pageWrite( sramBuffer, 0, page, pageFillData.data(), pageSize ) );

  /*------------------------------------------------
  Call FUT
  ------------------------------------------------*/
  EXPECT_EQ( Chimera::CommonStatusCodes::OK,
             flash->readModifyWrite( sramBuffer, page, offset, reinterpret_cast<uint8_t *>( writeData.data() ), len ) );

  while ( !flash->isDeviceReady() )
  {
    Chimera::delayMilliseconds( 10 );
  }

  /*------------------------------------------------
  Verify
  ------------------------------------------------*/
  flash->directPageRead( page, offset, readData.data(), len );
  flash->directPageRead( page, 0, pageReadData.data(), pageSize );

  /* Explicitly written data matches read data */
  EXPECT_EQ( 0, memcmp( readData.data(), writeData.data(), len ) );

  /* Entire page matches expected */
  EXPECT_EQ( 0, memcmp( pageReadData.data(), pageExptData.data(), pageSize ) );

  /* No program or erase error occurred */
  EXPECT_EQ( Chimera::CommonStatusCodes::OK, flash->isErasePgmError() );
}

#endif /* HW_TEST */
