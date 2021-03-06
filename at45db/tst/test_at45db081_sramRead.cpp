/********************************************************************************
 * File Name:
 *	  test_at45db081_sramRead.cpp
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

TEST_F( VirtualFlash, BufferRead_PreInitialization )
{
  uint8_t someData[] = { 1, 2, 3, 4 };
  EXPECT_EQ( Chimera::CommonStatusCodes::NOT_INITIALIZED,
             flash->sramRead( Adesto::NORFlash::SRAMBuffer::BUFFER1, 0xFFFF, someData, 0xFFFF ) );
}

TEST_F( VirtualFlash, BufferRead_NullPointerInput )
{
  passInit();

  EXPECT_EQ( Chimera::CommonStatusCodes::INVAL_FUNC_PARAM,
             flash->sramRead( Adesto::NORFlash::SRAMBuffer::BUFFER1, 0xFFFF, nullptr, 0xFFFF ) );

  EXPECT_EQ( Chimera::CommonStatusCodes::INVAL_FUNC_PARAM,
             flash->sramRead( Adesto::NORFlash::SRAMBuffer::BUFFER1, 0xFFFF, NULL, 0xFFFF ) );
}

#endif /* GMOCK_TEST */

#if defined( HW_TEST )
/*------------------------------------------------
Due to hardware tests on read also requiring a load, the testing of both
bufferRead & bufferLoad occur in test_at45db081_bufferLoad.cpp.
------------------------------------------------*/
#endif /* HW_TEST */
