/********************************************************************************
 * File Name:
 *	  test_at45db081_getFlashSize.cpp
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

TEST_F( VirtualFlash, GetFlashSize_PreInit )
{
  EXPECT_EQ( 0, flash->getFlashCapacity() );
}

#endif /* GMOCK_TEST */

#if defined( HW_TEST )

TEST_F( HardwareFlash, GetFlashSize )
{
  passInit();

  /* 8-MBit*/
  EXPECT_EQ( 1048576, flash->getFlashCapacity() );
}

#endif /* HW_TEST */
