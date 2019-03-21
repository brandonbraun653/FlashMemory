/********************************************************************************
 * File Name:
 *	  test_at45db081_erase.cpp
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
Whole Chip
------------------------------------------------*/
TEST_F( VirtualFlash, EraseChip_PreInit )
{
  EXPECT_EQ( Chimera::CommonStatusCodes::NOT_INITIALIZED, flash->eraseChip() );
}

/*------------------------------------------------
Page
------------------------------------------------*/
TEST_F( VirtualFlash, ErasePage_PreInit )
{
  EXPECT_EQ( Chimera::CommonStatusCodes::NOT_INITIALIZED, flash->erasePage( 0 ) );
}

TEST_F( VirtualFlash, ErasePage_InvalidRegion )
{
  passInit();
  EXPECT_EQ( Chimera::CommonStatusCodes::INVAL_FUNC_PARAM, flash->erasePage( std::numeric_limits<uint32_t>::max() ) );
}

/*------------------------------------------------
Block
------------------------------------------------*/
TEST_F( VirtualFlash, EraseBlock_PreInit )
{
  EXPECT_EQ( Chimera::CommonStatusCodes::NOT_INITIALIZED, flash->eraseBlock( 0 ) );
}

TEST_F( VirtualFlash, EraseBlock_InvalidRegion )
{
  passInit();
  EXPECT_EQ( Chimera::CommonStatusCodes::INVAL_FUNC_PARAM, flash->eraseBlock( std::numeric_limits<uint32_t>::max() ) );
}

/*------------------------------------------------
Sector
------------------------------------------------*/
TEST_F( VirtualFlash, EraseSector_PreInit )
{
  EXPECT_EQ( Chimera::CommonStatusCodes::NOT_INITIALIZED, flash->eraseSector( 0 ) );
}

TEST_F( VirtualFlash, EraseSector_InvalidRegion )
{
  passInit();
  EXPECT_EQ( Chimera::CommonStatusCodes::INVAL_FUNC_PARAM, flash->eraseSector( std::numeric_limits<uint32_t>::max() ) );
}


#endif /* GMOCK_TEST */

#if defined( HW_TEST )
using namespace Adesto::NORFlash;

TEST_F( HardwareFlash, ErasePage_Binary )
{
  std::array<uint8_t, PAGE_SIZE_BINARY> pageData;
  pageData.fill( 0 );
  static constexpr uint8_t page = 15;

  passInit();
  flash->useBinaryPageSize();

  EXPECT_EQ( Chimera::CommonStatusCodes::OK, flash->erasePage( page ) );

  while ( !flash->isDeviceReady() )
  {
    Chimera::delayMilliseconds( 10 );
  }
  flash->directPageRead( page, 0u, pageData.data(), PAGE_SIZE_BINARY );

  EXPECT_EQ( true, std::all_of( pageData.begin(), pageData.end(), []( uint8_t val ) { return ( val == ERASE_RESET_VAL ); } ) );
}

TEST_F( HardwareFlash, ErasePage_Extended )
{
  std::array<uint8_t, PAGE_SIZE_EXTENDED> pageData;
  pageData.fill( 0 );
  static constexpr uint8_t page = 63;

  passInit();
  flash->useExtendedPageSize();

  EXPECT_EQ( Chimera::CommonStatusCodes::OK, flash->erasePage( page ) );

  while ( !flash->isDeviceReady() )
  {
    Chimera::delayMilliseconds( 10 );
  }
  flash->directPageRead( page, 0u, pageData.data(), PAGE_SIZE_EXTENDED );

  EXPECT_EQ( true, std::all_of( pageData.begin(), pageData.end(), []( uint8_t val ) { return ( val == ERASE_RESET_VAL ); } ) );
}


#endif /* HW_TEST */
