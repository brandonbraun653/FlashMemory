/********************************************************************************
 *  File Name:
 *    test_fixtures_at45db081.cpp
 *  
 *  Description:
 *    Provides several test fixtures used in testing the AT45DB081
 *  
 *  2019 | Brandon Braun | brandonbraun653@gmail.com
 ********************************************************************************/

#include "test_fixtures_at45db081.hpp"

/* Test Driver Includes */
#include <gtest/gtest.h>


#if defined( GMOCK_TEST )
using ::testing::_;
using ::testing::AtLeast;
using ::testing::DoAll;
using ::testing::Exactly;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::SetArgPointee;
using ::testing::SetArrayArgument;

void VirtualFlash::SetUp()
{
  flash = new Adesto::NORFlash::AT45( &spi );
}

void VirtualFlash::TearDown()
{
  delete flash;
}

void VirtualFlash::passInit()
{
  Adesto::FlashChip chip = Adesto::FlashChip::AT45DB081E;

  uint8_t msk = 0xFF;
  uint16_t sr = Adesto::NORFlash::StatusRegisterBitPos::PAGE_SIZE_CONFIG_Pos;

  std::array<uint8_t, 3> goodInfo  = { 0x1F, 0x25, 0x00 };
  std::array<uint8_t, 2> statusReg = { static_cast<uint8_t>( ( sr >> 8 ) & msk ), static_cast<uint8_t>( sr & msk ) };

  /*------------------------------------------------
  Setup Mock Behavior
  ------------------------------------------------*/
  // clang-format off

  EXPECT_CALL( spi, init( _ ) )
    .Times( Exactly( 1 ) )
    .WillRepeatedly( Return( Chimera::SPI::Status::OK ) );

  EXPECT_CALL( spi, readBytes( _, _, _ ) )
    .WillOnce( DoAll( SetArrayArgument<0>( goodInfo.data(), goodInfo.data() + goodInfo.size() ),
                      Return(Chimera::SPI::Status::OK)))
    .WillOnce( DoAll( SetArrayArgument<0>( goodInfo.data(), goodInfo.data() + goodInfo.size() ),
                      Return(Chimera::SPI::Status::OK)))
    .WillOnce( DoAll( SetArrayArgument<0>( statusReg.data(), statusReg.data() + statusReg.size() ),
                      Return(Chimera::SPI::Status::OK)));;

  // clang-format on
  /*------------------------------------------------
  Verify
  ------------------------------------------------*/
  EXPECT_EQ( ErrCode::OK, flash->init( chip ) );
  EXPECT_EQ( true, flash->isInitialized() );
}

void VirtualFlash::testReset()
{
  
}
#endif 
