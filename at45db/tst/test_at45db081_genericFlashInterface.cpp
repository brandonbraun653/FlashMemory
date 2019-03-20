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

#endif /* GMOCK_TEST */

#if defined( HW_TEST )
#endif /* HW_TEST */
