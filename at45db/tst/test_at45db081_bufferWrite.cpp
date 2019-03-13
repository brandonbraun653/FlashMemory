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

TEST_F( VirtualFlash, BufferWrite_PreInitialization )
{
  EXPECT_EQ( Chimera::CommonStatusCodes::NOT_INITIALIZED,
             flash->bufferWrite( Adesto::SRAMBuffer::BUFFER1, 0xFFFF, false ) );
}

#endif /* GMOCK_TEST */

#if defined( HW_TEST )
#include "bus_pirate.hpp"

#endif /* HW_TEST */