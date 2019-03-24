/********************************************************************************
 * File Name:
 *	  test_at45db081_init.cpp
 *
 * Description:
 *	  Implements tests for the init function of the AT45DB081 driver
 *
 * 2019 | Brandon Braun | brandonbraun653@gmail.com
 ********************************************************************************/

/* Driver Includes */
#include "at45db081.hpp"

/* Testing Framework Includes */
#include <gtest/gtest.h>
#include <Chimera/spi.hpp>

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

using ErrCode = Chimera::Modules::Memory::Status;

TEST( AT45Initialize, SPIFailedInit )
{
  /*------------------------------------------------
  Setup Test Objects
  ------------------------------------------------*/
  NiceMock<Chimera::Mock::SPIMock> spi;
  Adesto::NORFlash::AT45 flash( &spi );

  Adesto::NORFlash::FlashChip chip = Adesto::NORFlash::FlashChip::AT45DB081E;

  /*------------------------------------------------
  Setup Mock Behavior
  ------------------------------------------------*/
  // clang-format off
  EXPECT_CALL( spi, init( _ ) ).Times( Exactly( 1 ) )
    .WillRepeatedly( Return( Chimera::SPI::Status::FAIL ) );

  // clang-format on
  /*------------------------------------------------
  Verify
  ------------------------------------------------*/
  EXPECT_EQ( ErrCode::FAILED_INIT, flash.init( chip ) );
}

TEST( AT45Initialize, BadDeviceInfoLowSpeed )
{
  /*------------------------------------------------
  Setup Test Objects
  ------------------------------------------------*/
  NiceMock<Chimera::Mock::SPIMock> spi;
  Adesto::NORFlash::AT45 flash( &spi );

  Adesto::NORFlash::FlashChip chip = Adesto::NORFlash::FlashChip::AT45DB081E;

  /*------------------------------------------------
  Setup Mock Behavior
  ------------------------------------------------*/
  // clang-format off

  EXPECT_CALL( spi, init( _ ) ).Times( Exactly( 1 ) )
    .WillRepeatedly( Return( Chimera::SPI::Status::OK ) );

  std::array<uint8_t, 3> fakeInfo = { 0u, 0u, 0u };
  EXPECT_CALL( spi, readBytes( _, _, _ ) )
    .WillOnce( DoAll( SetArrayArgument<0>( fakeInfo.data(), fakeInfo.data() + 3 ),
                      Return(Chimera::SPI::Status::OK)));

  // clang-format on
  /*------------------------------------------------
  Verify
  ------------------------------------------------*/
  EXPECT_EQ( ErrCode::UNKNOWN_JEDEC, flash.init( chip ) );
}

TEST( AT45Initialize, BadDeviceInfoHighSpeed )
{
  /*------------------------------------------------
  Setup Test Objects
  ------------------------------------------------*/
  NiceMock<Chimera::Mock::SPIMock> spi;
  Adesto::NORFlash::AT45 flash( &spi );

  Adesto::NORFlash::FlashChip chip = Adesto::NORFlash::FlashChip::AT45DB081E;
  std::array<uint8_t, 3> goodInfo = { 0x1F, 0x25, 0x00 };
  std::array<uint8_t, 3> fakeInfo = { 0u, 0u, 0u };

  /*------------------------------------------------
  Setup Mock Behavior
  ------------------------------------------------*/
  // clang-format off

  EXPECT_CALL( spi, init( _ ) ).Times( Exactly( 1 ) )
    .WillRepeatedly( Return( Chimera::SPI::Status::OK ) );

  EXPECT_CALL( spi, readBytes( _, _, _ ) )
    .WillOnce( DoAll( SetArrayArgument<0>( goodInfo.data(), goodInfo.data() + goodInfo.size() ),
                      Return(Chimera::SPI::Status::OK)))
    .WillOnce( DoAll( SetArrayArgument<0>( fakeInfo.data(), fakeInfo.data() + fakeInfo.size() ),
                      Return(Chimera::SPI::Status::OK)));

  // clang-format on
  /*------------------------------------------------
  Verify
  ------------------------------------------------*/
  EXPECT_EQ( ErrCode::HF_INIT_FAIL, flash.init( chip ) );
}

TEST( AT45Initialize, InitializationSuccess )
{
  /*------------------------------------------------
  Setup Test Objects
  ------------------------------------------------*/
  NiceMock<Chimera::Mock::SPIMock> spi;
  Adesto::NORFlash::AT45 flash( &spi );

  Adesto::NORFlash::FlashChip chip          = Adesto::NORFlash::FlashChip::AT45DB081E;

  uint8_t msk = 0xFF;
  uint16_t sr = Adesto::NORFlash::PAGE_SIZE_CONFIG_Pos;

  std::array<uint8_t, 3> goodInfo = { 0x1F, 0x25, 0x00 };
  std::array<uint8_t, 2> statusReg = { static_cast<uint8_t>( ( sr >> 8 ) & msk ),
                                       static_cast<uint8_t>( sr & msk ) };

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
                      Return(Chimera::SPI::Status::OK)));

  // clang-format on
  /*------------------------------------------------
  Verify
  ------------------------------------------------*/
  EXPECT_EQ( ErrCode::OK, flash.init( chip ) );
  EXPECT_EQ( true, flash.isInitialized() );
}

#endif /* GMOCK_TEST */

#if defined( HW_TEST )
#include "bus_pirate.hpp"

using ErrCode = Chimera::Modules::Memory::Status;

TEST( AT45InitializeHW, initSuccess )
{
  using namespace Chimera::SPI;

  std::string device = "COM7";
  HWInterface::BusPirate::Device bp(device);

  SPIClass_sPtr spi = std::make_shared<SPIClass>(bp);
  Adesto::NORFlash::AT45 flash( spi );

  Adesto::NORFlash::FlashChip chip = Adesto::NORFlash::FlashChip::AT45DB081E;
  auto result = flash.init(chip);

  EXPECT_EQ( ErrCode::OK, result );
}

#endif /* HW_TEST */
