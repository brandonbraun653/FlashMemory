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

TEST_F( VirtualFlash, GFI_Write_PreInit )
{
  EXPECT_EQ( Chimera::CommonStatusCodes::NOT_INITIALIZED, flash->write( 0, nullptr, 0 ) );
}

TEST_F( VirtualFlash, GFI_Read_PreInit )
{
  EXPECT_EQ( Chimera::CommonStatusCodes::NOT_INITIALIZED, flash->read( 0, nullptr, 0 ) );
}

TEST_F( VirtualFlash, GFI_Erase_PreInit )
{
  EXPECT_EQ( Chimera::CommonStatusCodes::NOT_INITIALIZED, flash->erase( 0, 0 ) );
}

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

TEST_F( HardwareFlash, GFI_WriteRead_Binary_PageAligned_LessThanSinglePage )
{
  static constexpr uint8_t len         = 10;
  static constexpr uint32_t pageOffset = 0;
  static constexpr uint32_t address    = 0x00001200 + pageOffset;

  std::array<uint8_t, len> writeData;
  std::array<uint8_t, len> readData;

  randomFill( writeData );
  readData.fill( 0 );

  passInit();
  flash->useBinaryPageSize();

  EXPECT_EQ( Chimera::CommonStatusCodes::OK, flash->write( address, writeData.data(), len ) );
  EXPECT_EQ( Chimera::CommonStatusCodes::OK, flash->read( address, readData.data(), len ) );
  EXPECT_EQ( 0, memcmp( writeData.data(), readData.data(), len ) );
}

TEST_F( HardwareFlash, GFI_WriteRead_Binary_PageUnAligned_LessThanSinglePage )
{
  static constexpr uint8_t len         = 10;
  static constexpr uint32_t pageOffset = 53;
  static constexpr uint32_t address    = 0x00001200 + pageOffset;

  std::array<uint8_t, len> writeData;
  std::array<uint8_t, len> readData;

  randomFill( writeData );
  readData.fill( 0 );

  passInit();
  flash->useBinaryPageSize();

  EXPECT_EQ( Chimera::CommonStatusCodes::OK, flash->write( address, writeData.data(), len ) );
  EXPECT_EQ( Chimera::CommonStatusCodes::OK, flash->read( address, readData.data(), len ) );
  EXPECT_EQ( 0, memcmp( writeData.data(), readData.data(), len ) );
}

TEST_F( HardwareFlash, GFI_WriteRead_Binary_PageAligned_MultiPage )
{
  static constexpr uint32_t len         = PAGE_SIZE_BINARY * 2;
  static constexpr uint32_t pageOffset = 0;
  static constexpr uint32_t address    = 0x00001200 + pageOffset;

  std::array<uint8_t, len> writeData;
  std::array<uint8_t, len> readData;

  randomFill( writeData );
  readData.fill( 0 );

  passInit();
  flash->useBinaryPageSize();

  EXPECT_EQ( Chimera::CommonStatusCodes::OK, flash->write( address, writeData.data(), len ) );
  EXPECT_EQ( Chimera::CommonStatusCodes::OK, flash->read( address, readData.data(), len ) );
  EXPECT_EQ( 0, memcmp( writeData.data(), readData.data(), len ) );
}

TEST_F( HardwareFlash, GFI_WriteRead_Binary_PageUnAligned_MultiPage )
{
  static constexpr uint32_t len        = PAGE_SIZE_BINARY * 2;
  static constexpr uint32_t pageOffset = 153;
  static constexpr uint32_t address    = 0x00001200 + pageOffset;

  std::array<uint8_t, len> writeData;
  std::array<uint8_t, len> readData;

  randomFill( writeData );
  readData.fill( 0 );

  passInit();
  flash->useBinaryPageSize();

  EXPECT_EQ( Chimera::CommonStatusCodes::OK, flash->write( address, writeData.data(), len ) );
  EXPECT_EQ( Chimera::CommonStatusCodes::OK, flash->read( address, readData.data(), len ) );
  EXPECT_EQ( 0, memcmp( writeData.data(), readData.data(), len ) );
}


#endif /* HW_TEST */
