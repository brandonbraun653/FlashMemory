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
#include <Adesto/at25/at25_constants.hpp>
#include <Adesto/at25/at25_driver.hpp>
#include <Adesto/at25/at25_register.hpp>
#include <Adesto/at25/at25_types.hpp>

/* Chimera Includes */
#include <Chimera/common>
#include <Chimera/spi>

namespace Adesto::AT25
{
  /*-------------------------------------------------------------------------------
  Private Functions
  -------------------------------------------------------------------------------*/
  static bool device_supported( const uint32_t devID )
  {
    uint32_t lsb_endian_id = 0;
    uint32_t msb_endian_id = 0;

    for ( auto x = 0; x < SupportedDevices.size(); x++ )
    {
      /*-------------------------------------------------
      It's unknown which endianness the host device operates
      on, so compare against both possible options.
      -------------------------------------------------*/
      msb_endian_id = SupportedDevices[ x ];
      lsb_endian_id = ( ( msb_endian_id & 0x00FF0000 ) >> 16 ) | ( ( msb_endian_id & 0x0000FF00 ) >> 0 )
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
    return Aurora::Memory::Status::ERR_OK;
  }


  Aurora::Memory::Status Driver::close()
  {
    return Aurora::Memory::Status::ERR_OK;
  }


  Aurora::Memory::Status Driver::write( const size_t address, const void *const data, const size_t length )
  {
    /*-------------------------------------------------
    Input Protection: Writes greater than a page will
    have hard to debug side-effects.
    -------------------------------------------------*/
    if ( !data || !length || ( length > PAGE_SIZE ) )
    {
      return Aurora::Memory::Status::ERR_BAD_ARG;
    }

    /*-------------------------------------------------
    Acquire access to this driver
    -------------------------------------------------*/
    this->lock();

    /*-------------------------------------------------
    Per datasheet specs, the write enable command must
    be sent before issuing the actual data.
    -------------------------------------------------*/
    issueWriteEnable();

    /*-------------------------------------------------
    Initialize the command sequence
    -------------------------------------------------*/
    cmdBuffer[ 0 ] = Command::PAGE_PROGRAM;
    cmdBuffer[ 1 ] = ( address & ADDRESS_BYTE_3_MSK ) >> ADDRESS_BYTE_3_POS;
    cmdBuffer[ 2 ] = ( address & ADDRESS_BYTE_2_MSK ) >> ADDRESS_BYTE_2_POS;
    cmdBuffer[ 3 ] = ( address & ADDRESS_BYTE_1_MSK ) >> ADDRESS_BYTE_1_POS;

    /*-------------------------------------------------
    Perform the SPI transaction
    -------------------------------------------------*/
    // Acquire the SPI and enable the memory chip
    mSPI->lock();
    mSPI->setChipSelect( Chimera::GPIO::State::LOW );

    // Tell the hardware which address to write into
    mSPI->writeBytes( cmdBuffer.data(), Command::PAGE_PROGRAM_OPS_LEN );
    mSPI->await( Chimera::Event::Trigger::TRIGGER_TRANSFER_COMPLETE, Chimera::Threading::TIMEOUT_BLOCK );

    // Dump the data
    mSPI->writeBytes( data, length );
    mSPI->await( Chimera::Event::Trigger::TRIGGER_TRANSFER_COMPLETE, Chimera::Threading::TIMEOUT_BLOCK );

    // Release the SPI and disable the memory chip
    mSPI->setChipSelect( Chimera::GPIO::State::HIGH );
    mSPI->unlock();

    /*-------------------------------------------------
    Release access to this driver and exit
    -------------------------------------------------*/
    this->unlock();
    return Aurora::Memory::Status::ERR_OK;
  }


  Aurora::Memory::Status Driver::read( const size_t address, void *const data, const size_t length )
  {
    /*-------------------------------------------------
    Input Protection
    -------------------------------------------------*/
    if ( !data || !length )
    {
      return Aurora::Memory::Status::ERR_BAD_ARG;
    }

    /*-------------------------------------------------
    Acquire access to this driver
    -------------------------------------------------*/
    this->lock();

    /*-------------------------------------------------
    Initialize the command sequence. The high speed
    command works for all frequency ranges.
    -------------------------------------------------*/
    cmdBuffer[ 0 ] = Command::READ_ARRAY_HS;
    cmdBuffer[ 1 ] = ( address & ADDRESS_BYTE_3_MSK ) >> ADDRESS_BYTE_3_POS;
    cmdBuffer[ 2 ] = ( address & ADDRESS_BYTE_2_MSK ) >> ADDRESS_BYTE_2_POS;
    cmdBuffer[ 3 ] = ( address & ADDRESS_BYTE_1_MSK ) >> ADDRESS_BYTE_1_POS;
    cmdBuffer[ 4 ] = 0;  // Dummy byte

    /*-------------------------------------------------
    Perform the SPI transaction
    -------------------------------------------------*/
    mSPI->lock();
    mSPI->setChipSelect( Chimera::GPIO::State::LOW );

    // Tell the hardware which address to read from
    mSPI->writeBytes( cmdBuffer.data(), Command::READ_ARRAY_HS_OPS_LEN );
    mSPI->await( Chimera::Event::Trigger::TRIGGER_TRANSFER_COMPLETE, Chimera::Threading::TIMEOUT_BLOCK );

    // Pull out all the data
    mSPI->readBytes( data, length );
    mSPI->await( Chimera::Event::Trigger::TRIGGER_TRANSFER_COMPLETE, Chimera::Threading::TIMEOUT_BLOCK );

    mSPI->setChipSelect( Chimera::GPIO::State::HIGH );
    mSPI->unlock();

    /*-------------------------------------------------
    Release access to this driver and exit
    -------------------------------------------------*/
    this->unlock();
    return Aurora::Memory::Status::ERR_OK;
  }


  Aurora::Memory::Status Driver::erase( const size_t address, const size_t length )
  {
    /*-------------------------------------------------
    Input Protection: Length Validity
    -------------------------------------------------*/
    bool validSize = false;
    for ( auto idx = 0; idx < EraseChunks.size(); idx++ )
    {
      if ( length == EraseChunks[ idx ] )
      {
        validSize = true;
        break;
      }
    }

    if ( !validSize || !length )
    {
      return Aurora::Memory::Status::ERR_BAD_ARG;
    }

    /*-------------------------------------------------
    Input Protection: Address Alignment
    -------------------------------------------------*/
    if ( ( address % length ) != 0 )
    {
      return Aurora::Memory::Status::ERR_BAD_ARG;
    }

    /*-------------------------------------------------
    Acquire access to this driver
    -------------------------------------------------*/
    this->lock();

    /*-------------------------------------------------
    Per datasheet specs, the write enable command must
    be sent before issuing the actual data.
    -------------------------------------------------*/
    issueWriteEnable();

    /*-------------------------------------------------
    Determine the op-code to use based on the requested
    chunk size to erase.
    -------------------------------------------------*/
    size_t eraseOpsLen = Command::BLOCK_ERASE_OPS_LEN;
    switch ( length )
    {
      case CHUNK_SIZE_4K:
        cmdBuffer[ 0 ] = Command::BLOCK_ERASE_4K;
        break;

      case CHUNK_SIZE_32K:
        cmdBuffer[ 0 ] = Command::BLOCK_ERASE_32K;
        break;

      case CHUNK_SIZE_64K:
        cmdBuffer[ 0 ] = Command::BLOCK_ERASE_64K;
        break;

      default:
        /*-------------------------------------------------
        Whole chip erase?
        -------------------------------------------------*/
        if ( length == densityToBytes( mInfo.density ) )
        {
          cmdBuffer[ 0 ] = Command::CHIP_ERASE;
          eraseOpsLen    = Command::CHIP_ERASE_OPS_LEN;
          break;
        }

        /*-------------------------------------------------
        Should never get here...
        -------------------------------------------------*/
        Chimera::insert_debug_breakpoint();
        this->unlock();
        return Aurora::Memory::Status::ERR_UNSUPPORTED;
        break;
    }

    /*-------------------------------------------------
    Initialize the command sequence. If whole chip
    erase command, these bytes will be ignored anyways.
    -------------------------------------------------*/
    cmdBuffer[ 1 ] = ( address & ADDRESS_BYTE_3_MSK ) >> ADDRESS_BYTE_3_POS;
    cmdBuffer[ 2 ] = ( address & ADDRESS_BYTE_2_MSK ) >> ADDRESS_BYTE_2_POS;
    cmdBuffer[ 3 ] = ( address & ADDRESS_BYTE_1_MSK ) >> ADDRESS_BYTE_1_POS;

    /*-------------------------------------------------
    Perform the SPI transaction
    -------------------------------------------------*/
    auto spiResult = Chimera::Status::OK;

    mSPI->lock();
    spiResult |= mSPI->setChipSelect( Chimera::GPIO::State::LOW );
    spiResult |= mSPI->readWriteBytes( cmdBuffer.data(), cmdBuffer.data(), eraseOpsLen );
    spiResult |= mSPI->await( Chimera::Event::Trigger::TRIGGER_TRANSFER_COMPLETE, Chimera::Threading::TIMEOUT_BLOCK );
    spiResult |= mSPI->setChipSelect( Chimera::GPIO::State::HIGH );
    mSPI->unlock();

    /*-------------------------------------------------
    Release access to this driver
    -------------------------------------------------*/
    this->unlock();
    if ( spiResult == Chimera::Status::OK )
    {
      return Aurora::Memory::Status::ERR_OK;
    }
    else
    {
      return Aurora::Memory::Status::ERR_DRIVER_ERR;
    }
  }


  Aurora::Memory::Status Driver::erase( const Aurora::Memory::Chunk chunk, const size_t id )
  {
    /*-------------------------------------------------
    Get the size allocated to the chunk type
    -------------------------------------------------*/
    size_t chunkSize = 0;
    size_t maxIndex  = 0;

    auto props = getDeviceProperties();

    switch ( chunk )
    {
      case Aurora::Memory::Chunk::PAGE:
        chunkSize = PAGE_SIZE;
        maxIndex  = props.numPages;
        break;

      case Aurora::Memory::Chunk::BLOCK:
        chunkSize = BLOCK_SIZE;
        maxIndex  = props.numBlocks;
        break;

      case Aurora::Memory::Chunk::SECTOR:
        chunkSize = SECTOR_SIZE;
        maxIndex  = props.numSectors;
        break;

      default:
        return Aurora::Memory::Status::ERR_BAD_ARG;
        break;
    };

    /*-------------------------------------------------
    Is this even a valid index for the selected chunk?
    -------------------------------------------------*/
    if ( id >= maxIndex )
    {
      return Aurora::Memory::Status::ERR_BAD_ARG;
    }

    /*-------------------------------------------------
    Calculate the starting address and erase
    -------------------------------------------------*/
    size_t address = chunkSize * id;
    return erase( address, chunkSize );
  }


  Aurora::Memory::Status Driver::eraseChip()
  {
    /*-------------------------------------------------
    Per datasheet specs, the write enable command must
    be sent before issuing the actual data.
    -------------------------------------------------*/
    issueWriteEnable();

    /*-------------------------------------------------
    Perform the SPI transaction
    -------------------------------------------------*/
    auto spiResult = Chimera::Status::OK;

    mSPI->lock();
    spiResult |= mSPI->setChipSelect( Chimera::GPIO::State::LOW );
    spiResult |= mSPI->writeBytes( &Command::CHIP_ERASE, Command::CHIP_ERASE_OPS_LEN );
    spiResult |= mSPI->await( Chimera::Event::Trigger::TRIGGER_TRANSFER_COMPLETE, Chimera::Threading::TIMEOUT_BLOCK );
    spiResult |= mSPI->setChipSelect( Chimera::GPIO::State::HIGH );
    mSPI->unlock();

    /*-------------------------------------------------
    Release access to this driver
    -------------------------------------------------*/
    if ( spiResult == Chimera::Status::OK )
    {
      return Aurora::Memory::Status::ERR_OK;
    }
    else
    {
      return Aurora::Memory::Status::ERR_DRIVER_ERR;
    }
  }


  Aurora::Memory::Status Driver::flush()
  {
    return Aurora::Memory::Status::ERR_OK;
  }

  Aurora::Memory::Status Driver::pendEvent( const Aurora::Memory::Event event, const size_t timeout )
  {
    /*-------------------------------------------------
    Decide the bits used to indicate events occurred.
    -------------------------------------------------*/
    uint16_t eventBitMask = 0;  // Indicates bits to look at
    size_t pollDelay      = 0;  // How long to wait between checks

    switch( event )
    {
      case Aurora::Memory::Event::MEM_ERASE_COMPLETE:
      case Aurora::Memory::Event::MEM_READ_COMPLETE:
      case Aurora::Memory::Event::MEM_WRITE_COMPLETE:
        eventBitMask = Register::SR_RDY_BUSY;
        pollDelay    = Chimera::Threading::TIMEOUT_5MS;
        break;

      default:
        return Aurora::Memory::Status::ERR_UNSUPPORTED;
        break;
    };

    /*-------------------------------------------------
    For the AT25SF081, the device is busy when the
    RDY/BSY flag is set. Assuming this extends to other
    AT25 devices as well.

    See Table 10-1 of device datasheet.
    -------------------------------------------------*/
    uint16_t statusRegister = readStatusRegister();
    const size_t startTime  = Chimera::millis();

    while ( statusRegister & eventBitMask )
    {
      /*-------------------------------------------------
      Check for timeout, otherwise suspend this thread
      and allow others to do something.
      -------------------------------------------------*/
      if( ( Chimera::millis() - startTime ) > timeout )
      {
        return Aurora::Memory::Status::ERR_TIMEOUT;
        break;
      }
      else
      {
        Chimera::delayMilliseconds( pollDelay );
      }

      /*-------------------------------------------------
      Poll the latest info
      -------------------------------------------------*/
      statusRegister = readStatusRegister();
    };

    return Aurora::Memory::Status::ERR_OK;
  }


  Aurora::Memory::Status Driver::onEvent( const Aurora::Memory::Event event, void ( *func )( const size_t ) )
  {
    return Aurora::Memory::Status::ERR_UNSUPPORTED;
  }


  Aurora::Memory::Status Driver::writeProtect( const bool enable, const Aurora::Memory::Chunk chunk, const size_t id )
  {
    return Aurora::Memory::Status::ERR_UNSUPPORTED;
  }


  Aurora::Memory::Status Driver::readProtect( const bool enable, const Aurora::Memory::Chunk chunk, const size_t id )
  {
    return Aurora::Memory::Status::ERR_UNSUPPORTED;
  }


  Aurora::Memory::Properties Driver::getDeviceProperties()
  {
    /*-------------------------------------------------
    Deduce the device properties. Assumes configure()
    has already been called.
    -------------------------------------------------*/
    Aurora::Memory::Properties tmp;
    tmp.clear();

    if ( size_t deviceSize = densityToBytes( mInfo.density ); deviceSize )
    {
      tmp.pageSize = PAGE_SIZE;
      tmp.numPages = deviceSize / PAGE_SIZE;

      tmp.blockSize = BLOCK_SIZE;
      tmp.numBlocks = deviceSize / BLOCK_SIZE;

      tmp.sectorSize = SECTOR_SIZE;
      tmp.numSectors = deviceSize / SECTOR_SIZE;

      tmp.jedec = mInfo.mfgID;

      tmp.startAddress = 0;
      tmp.endAddress   = deviceSize;

      tmp.writeChunk = Aurora::Memory::Chunk::PAGE;
      tmp.readChunk  = Aurora::Memory::Chunk::PAGE;
      tmp.eraseChunk = Aurora::Memory::Chunk::BLOCK;
    }

    return tmp;
  }


  /*-------------------------------------------------------------------------------
  Driver: Adesto Interface
  -------------------------------------------------------------------------------*/
  bool Driver::configure( const Chimera::SPI::Channel channel )
  {
    /*-------------------------------------------------
    Acquire access to this driver
    -------------------------------------------------*/
    this->lock();

    /*-------------------------------------------------
    Try and acquire the SPI driver, then read out the
    device identifier details.
    -------------------------------------------------*/
    DeviceInfo tmp;
    mSPI = Chimera::SPI::getDriver( channel );

    /*-------------------------------------------------
    Release access to this driver
    -------------------------------------------------*/
    this->unlock();
    return ( mSPI != nullptr ) && readDeviceInfo( tmp );
  }


  bool Driver::readDeviceInfo( DeviceInfo &info )
  {
    /*-------------------------------------------------
    Acquire access to this driver
    -------------------------------------------------*/
    this->lock();

    /*-------------------------------------------------
    Initialize the command sequence
    -------------------------------------------------*/
    cmdBuffer.fill( 0 );
    cmdBuffer[ 0 ] = Command::READ_DEV_INFO;

    /*-------------------------------------------------
    Perform the SPI transaction
    -------------------------------------------------*/
    auto spiResult = Chimera::Status::OK;

    mSPI->lock();
    spiResult |= mSPI->setChipSelect( Chimera::GPIO::State::LOW );
    spiResult |= mSPI->readWriteBytes( cmdBuffer.data(), cmdBuffer.data(), Command::READ_DEV_INFO_OPS_LEN );
    spiResult |= mSPI->await( Chimera::Event::Trigger::TRIGGER_TRANSFER_COMPLETE, Chimera::Threading::TIMEOUT_BLOCK );
    spiResult |= mSPI->setChipSelect( Chimera::GPIO::State::HIGH );
    mSPI->unlock();

    /*-------------------------------------------------
    Reformat the read data properly. First returned byte
    is actually in the second position of the buffer.
    -------------------------------------------------*/
    mInfo.mfgID   = cmdBuffer[ 1 ] & MFR_MSK;
    mInfo.family  = static_cast<FamilyCode>( ( cmdBuffer[ 2 ] >> FAMILY_CODE_POS ) & FAMILY_CODE_MSK );
    mInfo.density = static_cast<DensityCode>( ( cmdBuffer[ 2 ] >> DENSITY_CODE_POS ) & DENSITY_CODE_MSK );
    mInfo.sub     = static_cast<SubCode>( ( cmdBuffer[ 3 ] >> SUB_CODE_POS ) & SUB_CODE_MSK );
    mInfo.variant = static_cast<ProductVariant>( ( cmdBuffer[ 3 ] >> PROD_VERSION_POS ) && PROD_VERSION_MSK );

    info = mInfo;

    /*-------------------------------------------------
    Validate the data
    -------------------------------------------------*/
    uint32_t fullID = 0;
    memcpy( &fullID, &cmdBuffer[ 1 ], Command::READ_DEV_INFO_RSP_LEN );

    /*-------------------------------------------------
    Release access to this driver
    -------------------------------------------------*/
    this->unlock();
    return device_supported( fullID ) && ( spiResult == Chimera::Status::OK );
  }


  uint16_t Driver::readStatusRegister()
  {
    /*-------------------------------------------------
    Acquire access to this driver
    -------------------------------------------------*/
    this->lock();

    /*-------------------------------------------------
    Initialize the command sequence
    -------------------------------------------------*/
    cmdBuffer.fill( 0 );
    uint16_t result = 0;

    /*-------------------------------------------------
    Perform the SPI transaction
    -------------------------------------------------*/
    mSPI->lock();

    // Read out byte 1
    cmdBuffer[ 0 ] = Command::READ_SR_BYTE1;
    mSPI->setChipSelect( Chimera::GPIO::State::LOW );
    mSPI->readWriteBytes( cmdBuffer.data(), cmdBuffer.data(), Command::READ_SR_BYTE1_OPS_LEN );
    mSPI->await( Chimera::Event::Trigger::TRIGGER_TRANSFER_COMPLETE, Chimera::Threading::TIMEOUT_BLOCK );
    mSPI->setChipSelect( Chimera::GPIO::State::HIGH );

    result |= cmdBuffer[ 1 ];

    // Read out byte 2
    cmdBuffer[ 0 ] = Command::READ_SR_BYTE2;
    cmdBuffer[ 1 ] = 0;
    mSPI->setChipSelect( Chimera::GPIO::State::LOW );
    mSPI->readWriteBytes( cmdBuffer.data(), cmdBuffer.data(), Command::READ_SR_BYTE2_OPS_LEN );
    mSPI->await( Chimera::Event::Trigger::TRIGGER_TRANSFER_COMPLETE, Chimera::Threading::TIMEOUT_BLOCK );
    mSPI->setChipSelect( Chimera::GPIO::State::HIGH );

    result |= ( cmdBuffer[ 1 ] << 8 );

    mSPI->unlock();

    /*-------------------------------------------------
    Release access to this driver
    -------------------------------------------------*/
    this->unlock();
    return result;
  }


  /*-------------------------------------------------------------------------------
  Driver: Private Interface
  -------------------------------------------------------------------------------*/
  void Driver::issueWriteEnable()
  {
    mSPI->lock();
    mSPI->setChipSelect( Chimera::GPIO::State::LOW );
    mSPI->writeBytes( &Command::WRITE_ENABLE, Command::WRITE_ENABLE_OPS_LEN );
    mSPI->await( Chimera::Event::TRIGGER_TRANSFER_COMPLETE, Chimera::Threading::TIMEOUT_BLOCK );
    mSPI->setChipSelect( Chimera::GPIO::State::HIGH );
    mSPI->unlock();
  }
}  // namespace Adesto::AT25
