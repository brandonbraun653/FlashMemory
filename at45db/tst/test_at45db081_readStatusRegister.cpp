/********************************************************************************
 * File Name:
 *	  test_at45db081_readStatusRegister.cpp
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

TEST_F( VirtualFlash, ReadStatusRegister_PreInit )
{
  EXPECT_EQ( std::numeric_limits<uint16_t>::max(), flash->readStatusRegister( nullptr ) );
}

#endif /* GMOCK_TEST */

#if defined( HW_TEST )

TEST_F( HardwareFlash, ReadStatusRegister )
{
  passInit();
  EXPECT_NE( std::numeric_limits<uint16_t>::max(), flash->readStatusRegister( nullptr ) );
}

#endif /* HW_TEST */
