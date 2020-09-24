/********************************************************************************
 *  File Name:
 *    at25_driver.cpp
 *
 *  Description:
 *    Adesto AT25 memory driver
 *
 *  2020 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

/* Adesto Includes */
#include <Adesto/at25/at25_driver.hpp>
#include <Adesto/at25/at25_types.hpp>

/* Chimera Includes */
#include <Chimera/common>
#include <Chimera/spi>

namespace Adesto::AT25
{
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
    return Aurora::Memory::Status::ERR_UNKNOWN_JEDEC;
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
    return Aurora::Memory::Status::ERR_UNKNOWN_JEDEC;
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

    std::array<uint8_t, 3> data;
    data.fill( 0 );
    cmdBuffer.fill( 0 );

    cmdBuffer[ 0 ] = READ_DEVICE_INFO;
    SPI_write( cmdBuffer.data(), 1, true );
    SPI_read( data.data(), static_cast<uint32_t>( data.size() ), false );

    /*-------------------------------------------------
    Reformat the data properly
    -------------------------------------------------*/
    info.mfgID   = data[ 0 ];
    info.family  = static_cast<FamilyCode>( ( uint8_t )( data[ 1 ] >> 5 ) & 0xFF );
    info.density = static_cast<DensityCode>( data[ 1 ] & 0x1F );
    info.sub     = static_cast<SubCode>( ( data[ 2 ] >> 5 ) & 0xFF );
    info.variant = static_cast<ProductVariant>( data[ 2 ] & 0x1F );

    return info.mfgID == JEDEC_CODE;
  }


  /*-------------------------------------------------------------------------------
  Driver: Private Functions
  -------------------------------------------------------------------------------*/
  void Driver::SPI_write( const void *const data, const size_t len, const bool activeCS )
  {
    spi->lock();
    spi->setChipSelect( Chimera::GPIO::State::LOW );
    spi->writeBytes( data, len );
    spi->await(Chimera::Event::Trigger::TRIGGER_WRITE_COMPLETE, Chimera::Threading::TIMEOUT_BLOCK );

    if( !activeCS )
    {
      spi->setChipSelect( Chimera::GPIO::State::HIGH );
    }

    spi->unlock();
  }

  void Driver::SPI_read( void *const data, const size_t len, const bool activeCS )
  {
    spi->lock();
    spi->setChipSelect( Chimera::GPIO::State::LOW );
    spi->readBytes( data, len );
    spi->await(Chimera::Event::Trigger::TRIGGER_READ_COMPLETE, Chimera::Threading::TIMEOUT_BLOCK );

    if( !activeCS )
    {
      spi->setChipSelect( Chimera::GPIO::State::HIGH );
    }

    spi->unlock();
  }

}  // namespace Adesto::AT25
