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

TEST_F( VirtualFlash, BufferLoad_PreInitialization )
{
  const uint8_t someData[] = { 1, 2, 3, 4 };
  EXPECT_EQ( Chimera::CommonStatusCodes::NOT_INITIALIZED,
             flash->bufferLoad( Adesto::SRAMBuffer::BUFFER1, 0xFFFF, someData, 0xFFFF ) );
}

TEST_F(VirtualFlash, BufferLoad_NullPointerInput)
{
  passInit();

  EXPECT_EQ( Chimera::CommonStatusCodes::INVAL_FUNC_PARAM,
             flash->bufferLoad( Adesto::SRAMBuffer::BUFFER1, 0xFFFF, nullptr, 0xFFFF ) );

  EXPECT_EQ( Chimera::CommonStatusCodes::INVAL_FUNC_PARAM,
             flash->bufferLoad( Adesto::SRAMBuffer::BUFFER1, 0xFFFF, NULL, 0xFFFF ) );
}

#endif /* GMOCK_TEST */

#if defined( HW_TEST )

TEST_F(HardwareFlash, BufferLoadRead_OffsetBeginning)
{
  using namespace Adesto;

  auto offset = 0;

  const uint8_t arraySize = 10;
  std::array<uint8_t, arraySize> loadData = { 0x00, 0x22, 0x44, 0x66, 0x88, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE };
  std::array<uint8_t, arraySize> readData = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

  passInit();

  EXPECT_EQ( ErrCode::OK, flash->bufferLoad( SRAMBuffer::BUFFER1, offset, loadData.data(), loadData.size() ) );
  EXPECT_EQ( ErrCode::OK, flash->bufferRead( SRAMBuffer::BUFFER1, offset, readData.data(), readData.size() ) );
  EXPECT_EQ( 0, memcmp( loadData.data(), readData.data(), arraySize ) );

}

#endif /* HW_TEST */