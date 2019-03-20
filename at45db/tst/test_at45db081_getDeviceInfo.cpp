/********************************************************************************
 * File Name:
 *	  test_at45db081_getDeviceInfo.cpp
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

TEST_F( VirtualFlash, GetDeviceInfo_PreInit )
{
  Adesto::NORFlash::AT45xx_DeviceInfo dummyInfo;
  EXPECT_EQ( Chimera::CommonStatusCodes::NOT_INITIALIZED, flash->getDeviceInfo( dummyInfo ) );
}

#endif /* GMOCK_TEST */

#if defined( HW_TEST )

TEST_F( HardwareFlash, GetDeviceInfo )
{
  passInit();

  Adesto::NORFlash::AT45xx_DeviceInfo testInfo;

  EXPECT_EQ( Chimera::CommonStatusCodes::OK, flash->getDeviceInfo( testInfo ) );
  EXPECT_EQ( 0x1F, testInfo.manufacturerID );
  EXPECT_EQ( Adesto::FamilyCode::AT45Dxxx, testInfo.familyCode );
  EXPECT_EQ( Adesto::DENSITY_8MBIT, testInfo.densityCode );
  EXPECT_EQ( Adesto::SubCode::STANDARD_SERIES, testInfo.subCode );
  EXPECT_EQ( Adesto::ProductVariant::DEFAULT, testInfo.productVariant );
}

#endif /* HW_TEST */
