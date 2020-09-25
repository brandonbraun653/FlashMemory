/********************************************************************************
 *  File Name:
 *    at25_driver.cpp
 *
 *  Description:
 *    Adesto AT25 memory driver
 *
 *  2020 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

/* STL Includes */
#include <array>

/* Adesto Includes */
#include <Adesto/at25/at25_commands.hpp>
#include <Adesto/at25/at25_driver.hpp>
#include <Adesto/at25/at25_types.hpp>

/* Chimera Includes */
#include <Chimera/common>
#include <Chimera/spi>

namespace Adesto::AT25
{
  /*-------------------------------------------------------------------------------
  Constants
  -------------------------------------------------------------------------------*/
  /*-------------------------------------------------
  List of device identifier codes as they would appear
  shifted out in MSB mode.
  -------------------------------------------------*/
  static const std::array<uint32_t, 1> s_supported_devices =  {
    0x001F8501, // AT25SF081, 8Mb
  };

  /*-------------------------------------------------------------------------------
  Private Functions
  -------------------------------------------------------------------------------*/
  static bool device_supported( const uint32_t devID )
  {
    uint32_t lsb_endian_id = 0;
    uint32_t msb_endian_id = 0;

    for ( auto x = 0; x < s_supported_devices.size(); x++ )
    {
      /*-------------------------------------------------
      It's unknown which endianness the host device operates
      on, so compare against both possible options.
      -------------------------------------------------*/
      msb_endian_id = s_supported_devices[ x ];
      lsb_endian_id = ( ( msb_endian_id & 0x00FF0000 ) >> 16 )
                    | ( ( msb_endian_id & 0x0000FF00 ) >> 0  )
                    | ( ( msb_endian_id & 0x000000FF ) << 16 );

      if ( ( msb_endian_id == devID ) || ( lsb_endian_id == devID ) )
      {
        return true;
      }
    }

    return false;
  }

  /*-------------------------------------------------------------------------------
  Device Driver Implementation
  -------------------------------------------------------------------------------*/
  Driver::Driver()
  {

  }


  Driver::~Driver()
  {

  }

  /*-------------------------------------------------------------------------------
  Driver: Generic Memory Interface
  -------------------------------------------------------------------------------*/
  Aurora::Memory::Status Driver::open()
  {
    return Aurora::Memory::Status::ERR_UNKNOWN_JEDEC;
  }


  Aurora::Memory::Status Driver::close()
  {
    return Aurora::Memory::Status::ERR_UNKNOWN_JEDEC;
  }


  Aurora::Memory::Status Driver::write( const size_t address, const void *const data, const size_t length )
  {
    return Aurora::Memory::Status::ERR_UNKNOWN_JEDEC;
  }


  Aurora::Memory::Status Driver::read( const size_t address, void *const data, const size_t length )
  {
    /*-------------------------------------------------
    Initialize the command sequence. The high speed
    command works for all frequency ranges.
    -------------------------------------------------*/
    cmdBuffer[ 0 ] = Command::READ_ARRAY_HS;
    cmdBuffer[ 1 ] = ( address & ADDRESS_BYTE_3_MSK ) >> ADDRESS_BYTE_3_POS;
    cmdBuffer[ 2 ] = ( address & ADDRESS_BYTE_2_MSK ) >> ADDRESS_BYTE_2_POS;
    cmdBuffer[ 3 ] = ( address & ADDRESS_BYTE_1_MSK ) >> ADDRESS_BYTE_1_POS;
    cmdBuffer[ 4 ] = 0; // Dummy byte

    /*-------------------------------------------------
    Perform the SPI transaction
    -------------------------------------------------*/
    spi->lock();
    spi->setChipSelect( Chimera::GPIO::State::LOW );

    // Tell the hardware which address to read from
    spi->writeBytes( cmdBuffer.data(), Command::READ_ARRAY_HS_OPS_LEN );
    spi->await( Chimera::Event::Trigger::TRIGGER_WRITE_COMPLETE, Chimera::Threading::TIMEOUT_BLOCK );

    // Pull out all the data
    spi->readBytes( data, length );
    spi->await( Chimera::Event::Trigger::TRIGGER_READ_COMPLETE, Chimera::Threading::TIMEOUT_BLOCK );

    spi->setChipSelect( Chimera::GPIO::State::HIGH );
    spi->unlock();

    return Aurora::Memory::Status::ERR_OK;
  }


  Aurora::Memory::Status Driver::erase( const size_t address, const size_t length )
  {


    return Aurora::Memory::Status::ERR_UNKNOWN_JEDEC;
  }


  Aurora::Memory::Status Driver::erase( const Aurora::Memory::Chunk chunk, const size_t id )
  {
    return Aurora::Memory::Status::ERR_UNKNOWN_JEDEC;
  }


  Aurora::Memory::Status Driver::flush()
  {
    return Aurora::Memory::Status::ERR_OK;
  }


  Aurora::Memory::Status Driver::onEvent( const Aurora::Memory::Event event, void ( *func )( const size_t ) )
  {
    return Aurora::Memory::Status::ERR_UNKNOWN_JEDEC;
  }


  Aurora::Memory::Status Driver::writeProtect( const bool enable, const Aurora::Memory::Chunk chunk, const size_t id )
  {
    return Aurora::Memory::Status::ERR_UNKNOWN_JEDEC;
  }


  Aurora::Memory::Status Driver::readProtect( const bool enable, const Aurora::Memory::Chunk chunk, const size_t id )
  {
    return Aurora::Memory::Status::ERR_UNKNOWN_JEDEC;
  }


  Aurora::Memory::Properties Driver::getDeviceProperties()
  {
    return Aurora::Memory::Properties();
  }


  /*-------------------------------------------------------------------------------
  Driver: Adesto Interface
  -------------------------------------------------------------------------------*/
  bool Driver::configure( const Chimera::SPI::Channel channel )
  {
    spi = Chimera::SPI::getDriver( channel );
    return spi != nullptr;
  }


  bool Driver::readDeviceInfo( DeviceInfo &info )
  {
    /*-------------------------------------------------
    Initialize the command sequence
    -------------------------------------------------*/
    cmdBuffer.fill( 0 );
    cmdBuffer[ 0 ] = Command::READ_DEV_INFO;

    /*-------------------------------------------------
    Perform the SPI transaction
    -------------------------------------------------*/
    spi->lock();
    spi->setChipSelect( Chimera::GPIO::State::LOW );
    spi->readWriteBytes( cmdBuffer.data(), cmdBuffer.data(), Command::READ_DEV_INFO_OPS_LEN );
    spi->await( Chimera::Event::Trigger::TRIGGER_WRITE_COMPLETE, Chimera::Threading::TIMEOUT_BLOCK );
    spi->setChipSelect( Chimera::GPIO::State::HIGH );
    spi->unlock();

    /*-------------------------------------------------
    Reformat the read data properly. First returned byte
    is actually in the second position of the buffer.
    -------------------------------------------------*/
    info.mfgID   = cmdBuffer[ 1 ] & MFR_MSK;
    info.family  = static_cast<FamilyCode>( ( cmdBuffer[ 2 ] >> FAMILY_CODE_POS ) & FAMILY_CODE_MSK );
    info.density = static_cast<DensityCode>( ( cmdBuffer[ 2 ] >> DENSITY_CODE_POS ) & DENSITY_CODE_MSK );
    info.sub     = static_cast<SubCode>( ( cmdBuffer[ 3 ] >> SUB_CODE_POS ) & SUB_CODE_MSK );
    info.variant = static_cast<ProductVariant>( ( cmdBuffer[ 3 ] >> PROD_VERSION_POS ) && PROD_VERSION_MSK );

    /*-------------------------------------------------
    Validate the data
    -------------------------------------------------*/
    uint32_t fullID = 0;
    memcpy( &fullID, &cmdBuffer[ 1 ], Command::READ_DEV_INFO_RSP_LEN );
    return device_supported( fullID );
  }


  uint16_t Driver::readStatusRegister()
  {
    /*-------------------------------------------------
    Initialize the command sequence
    -------------------------------------------------*/
    cmdBuffer.fill( 0 );
    uint16_t result = 0;

    /*-------------------------------------------------
    Perform the SPI transaction
    -------------------------------------------------*/
    spi->lock();

    // Read out byte 1
    cmdBuffer[ 0 ] = Command::READ_SR_BYTE1;
    spi->setChipSelect( Chimera::GPIO::State::LOW );
    spi->readWriteBytes( cmdBuffer.data(), cmdBuffer.data(), Command::READ_SR_BYTE1_OPS_LEN );
    spi->await( Chimera::Event::Trigger::TRIGGER_WRITE_COMPLETE, Chimera::Threading::TIMEOUT_BLOCK );
    spi->setChipSelect( Chimera::GPIO::State::HIGH );

    result |= cmdBuffer[ 1 ];

    // Read out byte 2
    cmdBuffer[ 0 ] = Command::READ_SR_BYTE2;
    cmdBuffer[ 1 ] = 0;
    spi->setChipSelect( Chimera::GPIO::State::LOW );
    spi->readWriteBytes( cmdBuffer.data(), cmdBuffer.data(), Command::READ_SR_BYTE2_OPS_LEN );
    spi->await( Chimera::Event::Trigger::TRIGGER_WRITE_COMPLETE, Chimera::Threading::TIMEOUT_BLOCK );
    spi->setChipSelect( Chimera::GPIO::State::HIGH );

    result |= ( cmdBuffer[ 1 ] << 8 );

    spi->unlock();

    return result;
  }


  /*-------------------------------------------------------------------------------
  Driver: Private Functions
  -------------------------------------------------------------------------------*/
  void Driver::SPI_write( const void *const cmdBuffer, const size_t len, const bool activeCS )
  {
    spi->setChipSelect( Chimera::GPIO::State::LOW );
    spi->writeBytes( cmdBuffer, len );
    spi->await(Chimera::Event::Trigger::TRIGGER_WRITE_COMPLETE, Chimera::Threading::TIMEOUT_BLOCK );

    if( !activeCS )
    {
      spi->setChipSelect( Chimera::GPIO::State::HIGH );
    }
  }


  void Driver::SPI_read( void *const cmdBuffer, const size_t len, const bool activeCS )
  {
    spi->setChipSelect( Chimera::GPIO::State::LOW );
    spi->readBytes( cmdBuffer, len );
    spi->await(Chimera::Event::Trigger::TRIGGER_READ_COMPLETE, Chimera::Threading::TIMEOUT_BLOCK );

    if( !activeCS )
    {
      spi->setChipSelect( Chimera::GPIO::State::HIGH );
    }
  }

}  // namespace Adesto::AT25
