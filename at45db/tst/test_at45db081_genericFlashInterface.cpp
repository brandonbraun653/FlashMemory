/********************************************************************************
 * File Name:
 *	  test_at45db081_genericFlashInterface.cpp
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

/*------------------------------------------------
Write
------------------------------------------------*/
TEST_F( VirtualFlash, GFI_Write_PreInit )
{
  uint8_t data = 44;
  EXPECT_EQ( Chimera::CommonStatusCodes::NOT_INITIALIZED, flash->write( 0, &data, 1 ) );
}

TEST_F( VirtualFlash, GFI_Write_NullPtr )
{
  EXPECT_EQ( Chimera::CommonStatusCodes::INVAL_FUNC_PARAM, flash->write( 0, nullptr, 55 ) );
}

TEST_F( VirtualFlash, GFI_Write_TooMuchData )
{
  uint8_t data = 44;
  EXPECT_EQ( Chimera::Modules::Memory::Status::OVERRUN, flash->write( 0, &data, std::numeric_limits<uint32_t>::max() ) );
}

/*------------------------------------------------
Read
------------------------------------------------*/
TEST_F( VirtualFlash, GFI_Read_PreInit )
{
  uint8_t data = 44;
  EXPECT_EQ( Chimera::CommonStatusCodes::NOT_INITIALIZED, flash->read( 0, &data, 1 ) );
}

TEST_F( VirtualFlash, GFI_Read_NullPtr )
{
  EXPECT_EQ( Chimera::CommonStatusCodes::INVAL_FUNC_PARAM, flash->read( 0, nullptr, 55 ) );
}

TEST_F( VirtualFlash, GFI_Read_TooMuchData )
{
  uint8_t data = 44;
  EXPECT_EQ( Chimera::Modules::Memory::Status::OVERRUN, flash->read( 0, &data, std::numeric_limits<uint32_t>::max() ) );
}

/*------------------------------------------------
Erase
------------------------------------------------*/
TEST_F( VirtualFlash, GFI_Erase_PreInit )
{
  EXPECT_EQ( Chimera::CommonStatusCodes::NOT_INITIALIZED, flash->erase( 0, 0 ) );
}

TEST_F( VirtualFlash, GFI_Erase_RangeTooLarge )
{
  EXPECT_EQ( Chimera::CommonStatusCodes::NOT_INITIALIZED, flash->erase( 0, std::numeric_limits<uint32_t>::max() ) );
}

/*------------------------------------------------
Initialization Status
------------------------------------------------*/
TEST_F( VirtualFlash, GFI_IsInitialized_PreInit )
{
  EXPECT_EQ( false, flash->isInitialized() );
}

TEST_F( VirtualFlash, GFI_IsInitialized_PostInit )
{
  passInit();
  EXPECT_EQ( true, flash->isInitialized() );
}

#endif /* GMOCK_TEST */

#if defined( HW_TEST )
using namespace Adesto;
using namespace Adesto::NORFlash;

/*------------------------------------------------
Binary Page Size
------------------------------------------------*/

TEST_F( HardwareFlash, GFI_Erase_Binary_SinglePage )
{

}


/**
 *  Data Range: ***
 *  Don't Care: ---
 *  StartAddr:  p1
 *  EndAddr:    p2
 *  BlockBoundaries: a, b, c, d, e
 *
 * a/p1    p2    b          c          d          e
 *  |*******-----|----------|----------|----------|
 */
TEST_F( HardwareFlash, GFI_WriteRead_Binary_PageAligned_LessThanSinglePage )
{
  static constexpr uint32_t len        = 10;
  static constexpr uint32_t page       = 35;
  static constexpr uint32_t pageSize   = PAGE_SIZE_BINARY;
  static constexpr uint32_t pageOffset = 0;
  static constexpr uint32_t address    = ( page * pageSize ) + pageOffset;

  std::array<uint8_t, len> writeData;
  std::array<uint8_t, len> readData;

  randomFill( writeData );
  readData.fill( 0 );

  passInit();
  flash->useBinaryPageSize();
  ASSERT_EQ( pageSize, flash->getPageSize() );


  EXPECT_EQ( Chimera::CommonStatusCodes::OK, flash->write( address, writeData.data(), len ) );
  EXPECT_EQ( Chimera::CommonStatusCodes::OK, flash->read( address, readData.data(), len ) );
  EXPECT_EQ( 0, memcmp( writeData.data(), readData.data(), len ) );
}

/**
 *  Data Range: ***
 *  Don't Care: ---
 *  StartAddr:  p1
 *  EndAddr:    p2
 *  BlockBoundaries: a, b, c, d, e
 *
 * a  p1     p2  b          c          d          e
 *  |--*******---|----------|----------|----------|
 */
TEST_F( HardwareFlash, GFI_WriteRead_Binary_PageUnAligned_LessThanSinglePage )
{
  static constexpr uint32_t len        = 10;
  static constexpr uint32_t page       = 35;
  static constexpr uint32_t pageSize   = PAGE_SIZE_BINARY;
  static constexpr uint32_t pageOffset = 84;
  static constexpr uint32_t address    = ( page * pageSize ) + pageOffset;

  std::array<uint8_t, len> writeData;
  std::array<uint8_t, len> readData;

  randomFill( writeData );
  readData.fill( 0 );

  passInit();
  flash->useBinaryPageSize();
  ASSERT_EQ( pageSize, flash->getPageSize() );

  EXPECT_EQ( Chimera::CommonStatusCodes::OK, flash->write( address, writeData.data(), len ) );
  EXPECT_EQ( Chimera::CommonStatusCodes::OK, flash->read( address, readData.data(), len ) );
  EXPECT_EQ( 0, memcmp( writeData.data(), readData.data(), len ) );
}

/**
 *  Data Range: ***
 *  Don't Care: ---
 *  StartAddr:  p1
 *  EndAddr:    p2
 *  BlockBoundaries: a, b, c, d, e
 *
 *  a      p1  b   p2     c          d          e
 *  |------****|****------|----------|----------|
 */
TEST_F( HardwareFlash, GFI_WriteRead_Binary_PageSpan_LessThanSinglePage )
{
  static constexpr uint32_t len        = 30;
  static constexpr uint32_t page       = 754;
  static constexpr uint32_t pageSize   = PAGE_SIZE_BINARY;
  static constexpr uint32_t pageOffset = pageSize - ( len / 2 );
  static constexpr uint32_t address    = ( page * pageSize ) + pageOffset;

  std::array<uint8_t, len> writeData;
  std::array<uint8_t, len> readData;

  randomFill( writeData );
  readData.fill( 0 );

  passInit();
  flash->useBinaryPageSize();
  ASSERT_EQ( pageSize, flash->getPageSize() );

  EXPECT_EQ( Chimera::CommonStatusCodes::OK, flash->write( address, writeData.data(), len ) );
  EXPECT_EQ( Chimera::CommonStatusCodes::OK, flash->read( address, readData.data(), len ) );
  EXPECT_EQ( 0, memcmp( writeData.data(), readData.data(), len ) );
}

/**
 *  Data Range: ***
 *  Don't Care: ---
 *  StartAddr:  p1
 *  EndAddr:    p2
 *  BlockBoundaries: a, b, c, d, e
 *
 *  a      p1  b          c     p2   d          e
 *  |------****|**********|******----|----------|
 */
TEST_F( HardwareFlash, GFI_WriteRead_Binary_PageSpan_MoreThanSinglePage )
{
  static constexpr uint32_t len        = 30 + PAGE_SIZE_BINARY;
  static constexpr uint32_t page       = 833;
  static constexpr uint32_t pageSize   = PAGE_SIZE_BINARY;
  static constexpr uint32_t pageOffset = pageSize - 15;
  static constexpr uint32_t address    = ( page * pageSize ) + pageOffset;

  std::array<uint8_t, len> writeData;
  std::array<uint8_t, len> readData;

  randomFill( writeData );
  readData.fill( 0 );

  passInit();
  flash->useBinaryPageSize();
  ASSERT_EQ( pageSize, flash->getPageSize() );

  EXPECT_EQ( Chimera::CommonStatusCodes::OK, flash->write( address, writeData.data(), len ) );
  EXPECT_EQ( Chimera::CommonStatusCodes::OK, flash->read( address, readData.data(), len ) );
  EXPECT_EQ( 0, memcmp( writeData.data(), readData.data(), len ) );
}

/**
 *  Data Range: ***
 *  Don't Care: ---
 *  StartAddr:  p1
 *  EndAddr:    p2
 *  BlockBoundaries: a, b, c, d, e
 *
 *  a         b/p1        c         d/p2        e
 *  |----------|**********|**********|----------|
 */
TEST_F( HardwareFlash, GFI_WriteRead_Binary_MultiPageSpan )
{
  static constexpr uint32_t len        = PAGE_SIZE_BINARY * 2;
  static constexpr uint32_t page       = 1024;
  static constexpr uint32_t pageSize   = PAGE_SIZE_BINARY;
  static constexpr uint32_t pageOffset = 0;
  static constexpr uint32_t address    = ( page * pageSize ) + pageOffset;

  std::array<uint8_t, len> writeData;
  std::array<uint8_t, len> readData;

  randomFill( writeData );
  readData.fill( 0 );

  passInit();
  flash->useBinaryPageSize();
  ASSERT_EQ( pageSize, flash->getPageSize() );

  EXPECT_EQ( Chimera::CommonStatusCodes::OK, flash->write( address, writeData.data(), len ) );
  EXPECT_EQ( Chimera::CommonStatusCodes::OK, flash->read( address, readData.data(), len ) );
  EXPECT_EQ( 0, memcmp( writeData.data(), readData.data(), len ) );
}

/**
 *  Data Range: ***
 *  Don't Care: ---
 *  StartAddr:  p1
 *  EndAddr:    p2
 *  BlockBoundaries: a, b, c, d, e
 *
 *  a       p1 b          c          d  p2      e
 *  |-------***|**********|**********|***-------|
 */
TEST_F( HardwareFlash, GFI_WriteRead_Binary_OffsetMultiPageSpan )
{
  static constexpr uint32_t len        = 30 + ( PAGE_SIZE_BINARY * 2 );
  static constexpr uint32_t page       = 1324;
  static constexpr uint32_t pageSize   = PAGE_SIZE_BINARY;
  static constexpr uint32_t pageOffset = pageSize - 15;
  static constexpr uint32_t address    = ( page * pageSize ) + pageOffset;

  std::array<uint8_t, len> writeData;
  std::array<uint8_t, len> readData;

  randomFill( writeData );
  readData.fill( 0 );

  passInit();
  flash->useBinaryPageSize();
  ASSERT_EQ( pageSize, flash->getPageSize() );

  EXPECT_EQ( Chimera::CommonStatusCodes::OK, flash->write( address, writeData.data(), len ) );
  EXPECT_EQ( Chimera::CommonStatusCodes::OK, flash->read( address, readData.data(), len ) );
  EXPECT_EQ( 0, memcmp( writeData.data(), readData.data(), len ) );
}


/*------------------------------------------------
Extended Page Size
------------------------------------------------*/

/**
 *  Data Range: ***
 *  Don't Care: ---
 *  StartAddr:  p1
 *  EndAddr:    p2
 *  BlockBoundaries: a, b, c, d, e
 *
 * a/p1    p2    b          c          d          e
 *  |*******-----|----------|----------|----------|
 */
TEST_F( HardwareFlash, GFI_WriteRead_Extended_PageAligned_LessThanSinglePage )
{
  static constexpr uint32_t len        = 10;
  static constexpr uint32_t page       = 35;
  static constexpr uint32_t pageSize   = PAGE_SIZE_EXTENDED;
  static constexpr uint32_t pageOffset = 0;
  static constexpr uint32_t address    = ( page * pageSize ) + pageOffset;

  std::array<uint8_t, len> writeData;
  std::array<uint8_t, len> readData;

  randomFill( writeData );
  readData.fill( 0 );

  passInit();
  flash->useExtendedPageSize();
  ASSERT_EQ( pageSize, flash->getPageSize() );


  EXPECT_EQ( Chimera::CommonStatusCodes::OK, flash->write( address, writeData.data(), len ) );
  EXPECT_EQ( Chimera::CommonStatusCodes::OK, flash->read( address, readData.data(), len ) );
  EXPECT_EQ( 0, memcmp( writeData.data(), readData.data(), len ) );
}

/**
 *  Data Range: ***
 *  Don't Care: ---
 *  StartAddr:  p1
 *  EndAddr:    p2
 *  BlockBoundaries: a, b, c, d, e
 *
 * a  p1     p2  b          c          d          e
 *  |--*******---|----------|----------|----------|
 */
TEST_F( HardwareFlash, GFI_WriteRead_Extended_PageUnAligned_LessThanSinglePage )
{
  static constexpr uint32_t len        = 10;
  static constexpr uint32_t page       = 35;
  static constexpr uint32_t pageSize   = PAGE_SIZE_EXTENDED;
  static constexpr uint32_t pageOffset = 84;
  static constexpr uint32_t address    = ( page * pageSize ) + pageOffset;

  std::array<uint8_t, len> writeData;
  std::array<uint8_t, len> readData;

  randomFill( writeData );
  readData.fill( 0 );

  passInit();
  flash->useExtendedPageSize();
  ASSERT_EQ( pageSize, flash->getPageSize() );

  EXPECT_EQ( Chimera::CommonStatusCodes::OK, flash->write( address, writeData.data(), len ) );
  EXPECT_EQ( Chimera::CommonStatusCodes::OK, flash->read( address, readData.data(), len ) );
  EXPECT_EQ( 0, memcmp( writeData.data(), readData.data(), len ) );
}

/**
 *  Data Range: ***
 *  Don't Care: ---
 *  StartAddr:  p1
 *  EndAddr:    p2
 *  BlockBoundaries: a, b, c, d, e
 *
 *  a      p1  b   p2     c          d          e
 *  |------****|****------|----------|----------|
 */
TEST_F( HardwareFlash, GFI_WriteRead_Extended_PageSpan_LessThanSinglePage )
{
  static constexpr uint32_t len        = 30;
  static constexpr uint32_t page       = 754;
  static constexpr uint32_t pageSize   = PAGE_SIZE_EXTENDED;
  static constexpr uint32_t pageOffset = pageSize - ( len / 2 );
  static constexpr uint32_t address    = ( page * pageSize ) + pageOffset;

  std::array<uint8_t, len> writeData;
  std::array<uint8_t, len> readData;

  randomFill( writeData );
  readData.fill( 0 );

  passInit();
  flash->useExtendedPageSize();
  ASSERT_EQ( pageSize, flash->getPageSize() );

  EXPECT_EQ( Chimera::CommonStatusCodes::OK, flash->write( address, writeData.data(), len ) );
  EXPECT_EQ( Chimera::CommonStatusCodes::OK, flash->read( address, readData.data(), len ) );
  EXPECT_EQ( 0, memcmp( writeData.data(), readData.data(), len ) );
}

/**
 *  Data Range: ***
 *  Don't Care: ---
 *  StartAddr:  p1
 *  EndAddr:    p2
 *  BlockBoundaries: a, b, c, d, e
 *
 *  a      p1  b          c     p2   d          e
 *  |------****|**********|******----|----------|
 */
TEST_F( HardwareFlash, GFI_WriteRead_Extended_PageSpan_MoreThanSinglePage )
{
  static constexpr uint32_t len        = 30 + PAGE_SIZE_EXTENDED;
  static constexpr uint32_t page       = 833;
  static constexpr uint32_t pageSize   = PAGE_SIZE_EXTENDED;
  static constexpr uint32_t pageOffset = pageSize - 15;
  static constexpr uint32_t address    = ( page * pageSize ) + pageOffset;

  std::array<uint8_t, len> writeData;
  std::array<uint8_t, len> readData;

  randomFill( writeData );
  readData.fill( 0 );

  passInit();
  flash->useExtendedPageSize();
  ASSERT_EQ( pageSize, flash->getPageSize() );

  EXPECT_EQ( Chimera::CommonStatusCodes::OK, flash->write( address, writeData.data(), len ) );
  EXPECT_EQ( Chimera::CommonStatusCodes::OK, flash->read( address, readData.data(), len ) );
  EXPECT_EQ( 0, memcmp( writeData.data(), readData.data(), len ) );
}

/**
 *  Data Range: ***
 *  Don't Care: ---
 *  StartAddr:  p1
 *  EndAddr:    p2
 *  BlockBoundaries: a, b, c, d, e
 *
 *  a         b/p1        c         d/p2        e
 *  |----------|**********|**********|----------|
 */
TEST_F( HardwareFlash, GFI_WriteRead_Extended_MultiPageSpan )
{
  static constexpr uint32_t len        = PAGE_SIZE_EXTENDED * 2;
  static constexpr uint32_t page       = 1024;
  static constexpr uint32_t pageSize   = PAGE_SIZE_EXTENDED;
  static constexpr uint32_t pageOffset = 0;
  static constexpr uint32_t address    = ( page * pageSize ) + pageOffset;

  std::array<uint8_t, len> writeData;
  std::array<uint8_t, len> readData;

  randomFill( writeData );
  readData.fill( 0 );

  passInit();
  flash->useExtendedPageSize();
  ASSERT_EQ( pageSize, flash->getPageSize() );

  EXPECT_EQ( Chimera::CommonStatusCodes::OK, flash->write( address, writeData.data(), len ) );
  EXPECT_EQ( Chimera::CommonStatusCodes::OK, flash->read( address, readData.data(), len ) );
  EXPECT_EQ( 0, memcmp( writeData.data(), readData.data(), len ) );
}

/**
 *  Data Range: ***
 *  Don't Care: ---
 *  StartAddr:  p1
 *  EndAddr:    p2
 *  BlockBoundaries: a, b, c, d, e
 *
 *  a       p1 b          c          d  p2      e
 *  |-------***|**********|**********|***-------|
 */
TEST_F( HardwareFlash, GFI_WriteRead_Extended_OffsetMultiPageSpan )
{
  static constexpr uint32_t len        = 30 + ( PAGE_SIZE_EXTENDED * 2 );
  static constexpr uint32_t page       = 1324;
  static constexpr uint32_t pageSize   = PAGE_SIZE_EXTENDED;
  static constexpr uint32_t pageOffset = pageSize - 15;
  static constexpr uint32_t address    = ( page * pageSize ) + pageOffset;

  std::array<uint8_t, len> writeData;
  std::array<uint8_t, len> readData;

  randomFill( writeData );
  readData.fill( 0 );

  passInit();
  flash->useExtendedPageSize();
  ASSERT_EQ( pageSize, flash->getPageSize() );

  EXPECT_EQ( Chimera::CommonStatusCodes::OK, flash->write( address, writeData.data(), len ) );
  EXPECT_EQ( Chimera::CommonStatusCodes::OK, flash->read( address, readData.data(), len ) );
  EXPECT_EQ( 0, memcmp( writeData.data(), readData.data(), len ) );
}

#endif /* HW_TEST */
