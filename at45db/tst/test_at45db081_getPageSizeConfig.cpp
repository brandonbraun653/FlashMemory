/********************************************************************************
 * File Name:
 *	  test_at45db081_getPageSizeConfig.cpp
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

TEST_F( VirtualFlash, GetPageSizeConfig_PreInit )
{
  EXPECT_EQ( 0, flash->getPageSizeConfig() );
}

#endif /* GMOCK_TEST */

#if defined( HW_TEST )

TEST_F( HardwareFlash, GetPageSizeConfig )
{
  passInit();

  EXPECT_EQ( Chimera::CommonStatusCodes::OK, flash->useBinaryPageSize() );
  EXPECT_EQ( Adesto::NORFlash::PAGE_SIZE_BINARY, flash->getPageSizeConfig() );

  EXPECT_EQ( Chimera::CommonStatusCodes::OK, flash->useExtendedPageSize() );
  EXPECT_EQ( Adesto::NORFlash::PAGE_SIZE_EXTENDED, flash->getPageSizeConfig() );
}

#endif /* HW_TEST */
