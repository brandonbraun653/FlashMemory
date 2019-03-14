/********************************************************************************
 * File Name:
 *	  test_at45db081_read.cpp
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

using ::testing::_;
using ::testing::AtLeast;
using ::testing::DoAll;
using ::testing::Exactly;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::SetArgPointee;
using ::testing::SetArrayArgument;

TEST_F( VirtualFlash, DirectPageRead_PreInitialization )
{
  uint8_t fakeData[] = { 0, 0};
  EXPECT_EQ( Chimera::CommonStatusCodes::NOT_INITIALIZED, flash->directPageRead( 0, 0, fakeData, 0 ) );
}

TEST_F( VirtualFlash, DirectPageRead_NullPtrInput )
{
  passInit();

  EXPECT_EQ( Chimera::CommonStatusCodes::INVAL_FUNC_PARAM, flash->directPageRead( 0, 0, nullptr, 0 ) );
  EXPECT_EQ( Chimera::CommonStatusCodes::INVAL_FUNC_PARAM, flash->directPageRead( 0, 0, NULL, 0 ) );
}

#endif /* GMOCK_TEST */

#if defined( HW_TEST )
/*------------------------------------------------
The read functionality will be HW tested along with the write functions.
------------------------------------------------*/
#endif /* HW_TEST */
