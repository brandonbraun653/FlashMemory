/********************************************************************************
 * File Name:
 *	  test_at45db081_isDeviceReady.cpp
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

TEST_F( VirtualFlash, IsDeviceReady_PreInit )
{
  EXPECT_EQ( Chimera::CommonStatusCodes::NOT_INITIALIZED, flash->isDeviceReady() );
}

#endif /* GMOCK_TEST */

#if defined( HW_TEST )
#endif /* HW_TEST */
