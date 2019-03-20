/********************************************************************************
 * File Name:
 *	  test_at45db081_byteWrite.cpp
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

TEST_F( VirtualFlash, ByteWrite_PreInit )
{
  uint8_t someData = 0u;
  EXPECT_EQ( Chimera::CommonStatusCodes::NOT_INITIALIZED, flash->byteWrite(0, 0, &someData, sizeof( someData ) ) );
}

TEST_F( VirtualFlash, ByteWrite_NullPtrInput )
{
  passInit();

  EXPECT_EQ( Chimera::CommonStatusCodes::INVAL_FUNC_PARAM, flash->byteWrite( 0, 0, nullptr, 1 ) );
  EXPECT_EQ( Chimera::CommonStatusCodes::INVAL_FUNC_PARAM, flash->byteWrite( 0, 0, NULL, 1 ) );
}
#endif /* GMOCK_TEST */

#if defined( HW_TEST )
#endif /* HW_TEST */
