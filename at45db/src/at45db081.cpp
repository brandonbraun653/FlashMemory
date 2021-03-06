/********************************************************************************
 *   File Name:
 *       at45db081.cpp
 *
 *   Description:
 *       Driver for the AT45DB NOR flash chip series from Adesto
 *
 *   2018-2019 | Brandon Braun | brandonbraun653@gmail.com
 ********************************************************************************/

/* C/C++ Includes */
#include <memory>

/* Driver Includes */
#include "at45db081.hpp"

using namespace Chimera::SPI;
using namespace Chimera::Modules::Memory;
using ErrCode = Chimera::Modules::Memory::Status;

#define BYTE_LEN( x ) ( sizeof( x ) / sizeof( uint8_t ) )

struct AddressDescriptions
{
  uint8_t dummyBitsMSB = 0;
  uint8_t addressBits  = 0;
  uint8_t dummyBitsLSB = 0;
};

struct AddressScheme
{
  AddressDescriptions standardSize;
  AddressDescriptions binarySize;
};

struct MemoryAddressFormat
{
  AddressScheme page;
  AddressScheme block;
  AddressScheme sector;
  AddressScheme sector0ab;
  const uint8_t numAddressBytes;
};

struct FlashDelay
{
  uint8_t pageEraseAndProgramming;
  uint8_t pageProgramming;
  uint8_t pageErase;
  uint8_t blockErase;
  uint16_t sectorErase;
  uint16_t chipErase;
};

struct FlashSizes
{
  uint32_t numSectors = 0;
  uint32_t numBlocks  = 0;
  uint32_t numPages   = 0;
};

namespace Adesto
{
  namespace NORFlash
  {
    /*------------------------------------------------
    Tracks the region sizes of each supported flash chip.
    This MUST be kept in the same order as FlashChip enum.
    ------------------------------------------------*/
    static constexpr FlashSizes chipSpecs[ static_cast<uint8_t>( FlashChip::NUM_SUPPORTED_CHIPS ) ] = {
      { 16, 512, 4096 }, /* AT45DB081E */
    };

    static constexpr MemoryAddressFormat addressFormat[ static_cast<uint8_t>( FlashChip::NUM_SUPPORTED_CHIPS ) ] = {
      // AT45DB081E: See datasheet pgs. 13-14
      {
          { { 3, 12, 9 }, { 4, 12, 8 } },  // Page
          { { 3, 9, 12 }, { 4, 9, 11 } },  // Block
          { { 3, 4, 17 }, { 4, 4, 16 } },  // Sector
          { { 3, 9, 12 }, { 4, 9, 11 } },  // Sector 0a, 0b
          3                                // Number of address bytes
      },
    };

    static constexpr FlashDelay chipDelay[ static_cast<uint8_t>( FlashChip::NUM_SUPPORTED_CHIPS ) ] = {
      // AT45DB081E: See datasheet pg.49
      {
          15,    // Page erase and programming
          2,     // Page programming
          12,    // Page erase
          30,    // Block erase
          700,   // Sector erase
          10000  // Chip erase
      },
    };

    Chimera::Status_t AT45::init( const FlashChip chip, const uint32_t clockFreq )
    {
      Chimera::Status_t initResult = Chimera::CommonStatusCodes::FAIL;

      device = chip;
      cmdBuffer.fill( 0 );

      /*------------------------------------------------
      Initialize the SPI device with the correct parameters
      ------------------------------------------------*/
      setup.clockFrequency = 1000000;
      setup.bitOrder       = BitOrder::MSB_FIRST;
      setup.clockMode      = ClockMode::MODE0;
      setup.dataSize       = DataSize::SZ_8BIT;
      setup.mode           = Mode::MASTER;

      if ( spi->init( setup ) != Chimera::SPI::Status::OK )
      {
        initResult = ErrCode::FAILED_INIT;
      }
      else
      {
        spi->setPeripheralMode( Chimera::SPI::SubPeripheral::TXRX, Chimera::SPI::SubPeripheralMode::BLOCKING );
        spi->setChipSelectControlMode( Chimera::SPI::ChipSelectMode::MANUAL );
        spiInitialized = true;

        /*------------------------------------------------
        Check for a proper device connection:
        1) Get the manufacturer id at low freq (~1MHz for stability)
        2) Retry again at the user requested frequency
        ------------------------------------------------*/
        AT45xx_DeviceInfo loFreqInfo;
        AT45xx_DeviceInfo hiFreqInfo;

        /* Set these to be different so we don't accidentally match blank data */
        loFreqInfo.densityCode = DENSITY_4MBIT;
        hiFreqInfo.densityCode = DENSITY_64MBIT;

        getDeviceInfo( loFreqInfo );
        if ( loFreqInfo.manufacturerID != JEDEC_CODE )
        {
          initResult = ErrCode::UNKNOWN_JEDEC;
        }
        else
        {
          spi->setClockFrequency( clockFreq, 0 );
          getDeviceInfo( hiFreqInfo );

          if ( memcmp( &loFreqInfo, &hiFreqInfo, sizeof( AT45xx_DeviceInfo ) ) != 0 )
          {
            initResult = ErrCode::HF_INIT_FAIL;
          }
          else
          {
            chipInfo       = hiFreqInfo;
            initResult     = useBinaryPageSize();
            clockFrequency = clockFreq;
          }
        }
      }

      chipInitialized = ( initResult == ErrCode::OK );

      return initResult;
    }

    Chimera::Status_t AT45::sramLoad( const SRAMBuffer bufferNumber, const uint16_t offset, const uint8_t *const dataIn,
                                      const uint32_t len, Chimera::void_func_uint32_t onComplete )
    {
      Chimera::Status_t error = Chimera::CommonStatusCodes::FAIL;

      if ( !chipInitialized )
      {
        error = Chimera::CommonStatusCodes::NOT_INITIALIZED;
      }
      else if ( !dataIn )
      {
        error = Chimera::CommonStatusCodes::INVAL_FUNC_PARAM;
      }
      else
      {
        static constexpr uint8_t BUFFER_LOAD_CMD_LEN = 4; /**< CMD(1) + Address(3) */

        /*------------------------------------------------
        In the buildReadWriteCommand, the page number is 0 because we are only writing
        to SRAM buffers. The data isn't actually being written to a page yet.

        See: (6.1) Buffer Write in the device datasheet
        ------------------------------------------------*/
        cmdBuffer[ 0 ] = ( bufferNumber == SRAMBuffer::BUFFER1 ) ? BUFFER1_WRITE : BUFFER2_WRITE;
        buildReadWriteCommand( 0, offset );

        SPI_write( cmdBuffer.data(), BUFFER_LOAD_CMD_LEN, false );
        SPI_write( dataIn, len, true );

        if ( onComplete )
        {
          onComplete( 0 );
        }

        error = Chimera::CommonStatusCodes::OK;
      }

      return error;
    }

    Chimera::Status_t AT45::sramRead( const SRAMBuffer bufferNumber, const uint16_t offset, uint8_t *const dataOut,
                                      const uint32_t len, Chimera::void_func_uint32_t onComplete )
    {
      Chimera::Status_t error = Chimera::CommonStatusCodes::FAIL;

      if ( !chipInitialized )
      {
        error = Chimera::CommonStatusCodes::NOT_INITIALIZED;
      }
      else if ( !dataOut )
      {
        error = Chimera::CommonStatusCodes::INVAL_FUNC_PARAM;
      }
      else
      {
        uint32_t readCmdLen = 4; /**< CMD(1) + Address(3) */

        if ( clockFrequency > 50000000 ) /* 50 MHz Clock */
        {
          cmdBuffer[ 0 ] = ( bufferNumber == SRAMBuffer::BUFFER1 ) ? BUFFER1_READ_HF : BUFFER2_READ_HF;
          readCmdLen += 1; /**< + Init(1) */
        }
        else
        {
          cmdBuffer[ 0 ] = ( bufferNumber == SRAMBuffer::BUFFER1 ) ? BUFFER1_READ_LF : BUFFER2_READ_LF;
        }

        /*------------------------------------------------
        Load the cmdBuffer with the correct addressing bytes
        ------------------------------------------------*/
        buildReadWriteCommand( 0, offset );

        SPI_write( cmdBuffer.data(), readCmdLen, false );
        SPI_read( dataOut, len, true );

        if ( onComplete )
        {
          onComplete( 0 );
        }

        error = Chimera::CommonStatusCodes::OK;
      }

      return error;
    }

    Chimera::Status_t AT45::sramCommit( const SRAMBuffer bufferNumber, const uint16_t pageNumber, const bool erase,
                                        Chimera::void_func_uint32_t onComplete )
    {
      Chimera::Status_t error = Chimera::CommonStatusCodes::FAIL;

      if ( !chipInitialized )
      {
        error = Chimera::CommonStatusCodes::NOT_INITIALIZED;
      }
      else
      {
        static constexpr uint8_t SRAM_COMMIT_CMD_LEN = 4; /**< CMD(1) + Address(3) */

        if ( erase )
        {
          cmdBuffer[ 0 ] = ( bufferNumber == SRAMBuffer::BUFFER1 ) ? BUFFER1_TO_MAIN_MEM_PAGE_PGM_W_ERASE
                                                                   : BUFFER2_TO_MAIN_MEM_PAGE_PGM_W_ERASE;
        }
        else
        {
          cmdBuffer[ 0 ] = ( bufferNumber == SRAMBuffer::BUFFER1 ) ? BUFFER1_TO_MAIN_MEM_PAGE_PGM_WO_ERASE
                                                                   : BUFFER2_TO_MAIN_MEM_PAGE_PGM_WO_ERASE;
        }

        /*------------------------------------------------
        Only the page number is valid and then offset is ignored.
        See: (6.2) 'Buffer to Main Memory Page Program with/without Built-In Erase'
        ------------------------------------------------*/
        buildReadWriteCommand( pageNumber, 0x0000 );
        SPI_write( cmdBuffer.data(), SRAM_COMMIT_CMD_LEN, true );

        if ( onComplete )
        {
          onComplete( 0 );
        }

        error = Chimera::CommonStatusCodes::OK;
      }

      return error;
    }

    Chimera::Status_t AT45::directPageRead( const uint16_t pageNumber, const uint16_t pageOffset, uint8_t *const dataOut,
                                            const uint32_t len, Chimera::void_func_uint32_t onComplete )
    {
      Chimera::Status_t error = Chimera::CommonStatusCodes::FAIL;

      if ( !chipInitialized )
      {
        error = Chimera::CommonStatusCodes::NOT_INITIALIZED;
      }
      else if ( !dataOut )
      {
        error = Chimera::CommonStatusCodes::INVAL_FUNC_PARAM;
      }
      else
      {
        static constexpr uint8_t MAIN_MEM_PAGE_READ_CMD_LEN = 8;

        cmdBuffer[ 0 ] = MAIN_MEM_PAGE_READ;
        buildReadWriteCommand( pageNumber, pageOffset );

        /*------------------------------------------------
        The command is comprised of an opcode (1 byte), an address (3 bytes), and 4 dummy bytes.
        The dummy bytes are used to initialize the read operation.

        See: (5.6) Main Memory Page Read
        ------------------------------------------------*/
        SPI_write( cmdBuffer.data(), MAIN_MEM_PAGE_READ_CMD_LEN, false );
        SPI_read( dataOut, len, true );

        if ( onComplete )
        {
          onComplete( 0 );
        }

        error = ErrCode::OK;
      }

      return error;
    }

    Chimera::Status_t AT45::directArrayRead( const uint16_t pageNumber, const uint16_t pageOffset, uint8_t *const dataOut,
                                             const uint32_t len, Chimera::void_func_uint32_t onComplete )
    {
      Chimera::Status_t error = Chimera::CommonStatusCodes::FAIL;

      if ( !chipInitialized )
      {
        error = Chimera::CommonStatusCodes::NOT_INITIALIZED;
      }
      else if ( !dataOut )
      {
        error = Chimera::CommonStatusCodes::INVAL_FUNC_PARAM;
      }
      else
      {
        static constexpr uint8_t CONT_ARRAY_READ_CMD_LEN = 4;
        uint32_t numDummyBytes                           = 0;

        /*------------------------------------------------
        The command is comprised of an opcode (1 byte), an address (3 bytes), and X dummy bytes.
        The dummy bytes are used to initialize the read operation for higher frequencies.

        See: (5.2, 5.3, 5.4, 5.4) Continuous Array Read
        ------------------------------------------------*/
        if ( clockFrequency > 50000000 )  // TODO: Remove magic number
        {
          cmdBuffer[ 0 ] = CONT_ARR_READ_HF1;
        }
        else
        {
          cmdBuffer[ 0 ] = CONT_ARR_READ_LF;
        }

        switch ( cmdBuffer[ 0 ] )
        {
          case CONT_ARR_READ_HF1:
            numDummyBytes = 1;
            break;

          case CONT_ARR_READ_HF2:
            numDummyBytes = 2;
            break;

          default:
            numDummyBytes = 0;
            break;
        }

        buildReadWriteCommand( pageNumber, pageOffset );

        SPI_write( cmdBuffer.data(), CONT_ARRAY_READ_CMD_LEN + numDummyBytes, false );
        SPI_read( dataOut, len, true );

        if ( onComplete )
        {
          onComplete( 0 );
        }

        error = ErrCode::OK;
      }

      return error;
    }

    Chimera::Status_t AT45::byteWrite( const uint16_t pageNumber, const uint16_t pageOffset, const uint8_t *const dataIn,
                                       const uint32_t len, Chimera::void_func_uint32_t onComplete )
    {
      Chimera::Status_t error = Chimera::CommonStatusCodes::FAIL;

      if ( !chipInitialized )
      {
        error = Chimera::CommonStatusCodes::NOT_INITIALIZED;
      }
      else if ( !dataIn )
      {
        error = Chimera::CommonStatusCodes::INVAL_FUNC_PARAM;
      }
      else
      {
        static constexpr uint8_t MAIN_MEM_BYTE_PGM_CMD_LEN = 4;

        /*------------------------------------------------
        The command is comprised of an opcode (1 byte) and an address (3 bytes)

        See: (6.5) Main Memory Byte/Page Program through Buffer 1 WITHOUT Built-In Erase
        ------------------------------------------------*/
        cmdBuffer[ 0 ] = MAIN_MEM_BP_PGM_THR_BUFFER1_WO_ERASE;
        buildReadWriteCommand( pageNumber, pageOffset );

        SPI_write( cmdBuffer.data(), MAIN_MEM_BYTE_PGM_CMD_LEN, false );
        SPI_write( dataIn, len, true );

        if ( onComplete )
        {
          onComplete( 0 );
        }

        error = Chimera::CommonStatusCodes::OK;
      }

      return error;
    }

    Chimera::Status_t AT45::pageWrite( const SRAMBuffer bufferNumber, const uint16_t bufferOffset, const uint16_t pageNumber,
                                       const uint8_t *const dataIn, const uint32_t len, Chimera::void_func_uint32_t onComplete )
    {
      Chimera::Status_t error = Chimera::CommonStatusCodes::FAIL;

      if ( !chipInitialized )
      {
        error = Chimera::CommonStatusCodes::NOT_INITIALIZED;
      }
      else if ( !dataIn )
      {
        error = Chimera::CommonStatusCodes::INVAL_FUNC_PARAM;
      }
      else
      {
        static constexpr uint8_t MAIN_MEM_PAGE_PGM_CMD_LEN = 4;

        /*------------------------------------------------
        The command is comprised of an opcode (1 byte) and an address (3 bytes)

        See: (6.4) Main Memory Page Program through Buffer WITH Built-In Erase
        ------------------------------------------------*/
        cmdBuffer[ 0 ] = ( bufferNumber == SRAMBuffer::BUFFER1 ) ? MAIN_MEM_PAGE_PGM_THR_BUFFER1_W_ERASE
                                                                 : MAIN_MEM_PAGE_PGM_THR_BUFFER2_W_ERASE;
        buildReadWriteCommand( pageNumber, bufferOffset );

        SPI_write( cmdBuffer.data(), MAIN_MEM_PAGE_PGM_CMD_LEN, false );
        SPI_write( dataIn, len, true );

        if ( onComplete )
        {
          onComplete( 0 );
        }

        error = ErrCode::OK;
      }

      return error;
    }

    Chimera::Status_t AT45::readModifyWrite( const SRAMBuffer bufferNumber, const uint16_t pageNumber,
                                             const uint16_t pageOffset, const uint8_t *const dataIn, const uint32_t len,
                                             Chimera::void_func_uint32_t onComplete )
    {
      Chimera::Status_t error = Chimera::CommonStatusCodes::FAIL;

      if ( !chipInitialized )
      {
        error = Chimera::CommonStatusCodes::NOT_INITIALIZED;
      }
      else if ( !dataIn )
      {
        error = Chimera::CommonStatusCodes::INVAL_FUNC_PARAM;
      }
      else
      {
        static constexpr uint8_t READ_MODIFY_WRITE_CMD_LEN = 4;

        /*------------------------------------------------
        The command is comprised of an opcode (1 byte) and an address (3 bytes)

        See: (6.6) Read-Modify-Write
        ------------------------------------------------*/
        cmdBuffer[ 0 ] = ( bufferNumber == SRAMBuffer::BUFFER1 ) ? AUTO_PAGE_REWRITE1 : AUTO_PAGE_REWRITE2;
        buildReadWriteCommand( pageNumber, pageOffset );

        SPI_write( cmdBuffer.data(), READ_MODIFY_WRITE_CMD_LEN, false );
        SPI_write( dataIn, len, true );

        if ( onComplete )
        {
          onComplete( 0 );
        }

        error = Chimera::CommonStatusCodes::OK;
      }

      return error;
    }

    Chimera::Status_t AT45::erasePage( const uint32_t page )
    {
      Chimera::Status_t error = Chimera::CommonStatusCodes::FAIL;

      if ( !chipInitialized )
      {
        error = Chimera::CommonStatusCodes::NOT_INITIALIZED;
      }
      else if ( page >= chipSpecs[ static_cast<uint8_t>( device ) ].numPages )
      {
        error = Chimera::CommonStatusCodes::INVAL_FUNC_PARAM;
      }
      else
      {
        cmdBuffer[ 0 ] = PAGE_ERASE;
        buildEraseCommand( Section_t::PAGE, page );

        SPI_write( cmdBuffer.data(),
                   ( BYTE_LEN( PAGE_ERASE ) + addressFormat[ static_cast<uint8_t>( device ) ].numAddressBytes ), true );
        error = Chimera::CommonStatusCodes::OK;
      }

      return error;
    }

    Chimera::Status_t AT45::eraseBlock( const uint32_t block )
    {
      Chimera::Status_t error = Chimera::CommonStatusCodes::FAIL;

      if ( !chipInitialized )
      {
        error = Chimera::CommonStatusCodes::NOT_INITIALIZED;
      }
      else if ( block >= chipSpecs[ static_cast<uint8_t>( device ) ].numBlocks )
      {
        error = Chimera::CommonStatusCodes::INVAL_FUNC_PARAM;
      }
      else
      {
        cmdBuffer[ 0 ] = BLOCK_ERASE;
        buildEraseCommand( Section_t::BLOCK, block );

        SPI_write( cmdBuffer.data(),
                   ( BYTE_LEN( BLOCK_ERASE ) + addressFormat[ static_cast<uint8_t>( device ) ].numAddressBytes ), true );
        error = Chimera::CommonStatusCodes::OK;
      }

      return error;
    }

    Chimera::Status_t AT45::eraseSector( const uint32_t sector )
    {
      Chimera::Status_t error = Chimera::CommonStatusCodes::FAIL;

      if ( !chipInitialized )
      {
        error = Chimera::CommonStatusCodes::NOT_INITIALIZED;
      }
      else if ( sector >= chipSpecs[ static_cast<uint8_t>( device ) ].numSectors )
      {
        error = Chimera::CommonStatusCodes::INVAL_FUNC_PARAM;
      }
      else
      {
        cmdBuffer[ 0 ] = SECTOR_ERASE;
        buildEraseCommand( Section_t::SECTOR, sector );

        SPI_write( cmdBuffer.data(),
                   ( BYTE_LEN( SECTOR_ERASE ) + addressFormat[ static_cast<uint8_t>( device ) ].numAddressBytes ), true );
        error = Chimera::CommonStatusCodes::OK;
      }

      return error;
    }

    Chimera::Status_t AT45::eraseChip()
    {
      Chimera::Status_t error = Chimera::CommonStatusCodes::FAIL;

      if ( !chipInitialized )
      {
        error = Chimera::CommonStatusCodes::NOT_INITIALIZED;
      }
      else
      {
        uint32_t cmd = CHIP_ERASE;
        memcpy( cmdBuffer.data(), ( uint8_t * )&cmd, sizeof( cmd ) );

        SPI_write( cmdBuffer.data(), BYTE_LEN( CHIP_ERASE ), true );
        error = Chimera::CommonStatusCodes::OK;
      }

      return error;
    }

    uint16_t AT45::getPageSizeConfig()
    {
      uint16_t retVal = 0u;

      if ( spiInitialized )
      {
        retVal = ( readStatusRegister() & PAGE_SIZE_CONFIG_Pos ) ? PAGE_SIZE_BINARY : PAGE_SIZE_EXTENDED;
      }

      return retVal;
    }

    uint16_t AT45::readStatusRegister( StatusRegister *const reg )
    {
      uint16_t retVal = std::numeric_limits<uint16_t>::max();

      if ( spiInitialized )
      {
        uint8_t val[ 2 ];

        cmdBuffer[ 0 ] = STATUS_REGISTER_READ;
        SPI_write( cmdBuffer.data(), BYTE_LEN( STATUS_REGISTER_READ ), false );
        SPI_read( val, 2, true );

        retVal = ( uint16_t )( ( val[ 0 ] << 8 ) | val[ 1 ] );

        if ( reg )
        {
          reg->compareResult          = retVal & COMPARE_RESULT_Pos;
          reg->deviceReady            = retVal & READY_BUSY_Pos;
          reg->eraseProgramError      = retVal & ERASE_PGM_ERROR_Pos;
          reg->eraseSuspend           = retVal & ERASE_SUSPEND_Pos;
          reg->pageSizeConfig         = retVal & PAGE_SIZE_CONFIG_Pos;
          reg->pgmSuspendStatusB1     = retVal & BUFF1_PGM_SUSPEND_Pos;
          reg->pgmSuspendStatusB2     = retVal & BUFF2_PGM_SUSPEND_Pos;
          reg->sectorLockdownEnabled  = retVal & SECTOR_LOCKDOWN_EN_Pos;
          reg->sectorProtectionStatus = retVal & SECTOR_PROTECTION_Pos;
        }
      }

      return retVal;
    }

    Chimera::Status_t AT45::isDeviceReady()
    {
      Chimera::Status_t error = Chimera::CommonStatusCodes::FAIL;

      if ( !spiInitialized )
      {
        error = Chimera::CommonStatusCodes::NOT_INITIALIZED;
      }
      else if ( readStatusRegister( nullptr ) & READY_BUSY_Pos )
      {
        error = Chimera::CommonStatusCodes::OK;
      }

      return error;
    }

    Chimera::Status_t AT45::isErasePgmError()
    {
      Chimera::Status_t error = Chimera::CommonStatusCodes::OK;

      if ( !spiInitialized )
      {
        error = Chimera::CommonStatusCodes::NOT_INITIALIZED;
      }
      else if ( readStatusRegister( nullptr ) & ERASE_PGM_ERROR_Pos )
      {
        error = Chimera::CommonStatusCodes::FAIL;
      }

      return error;
    }

    Chimera::Status_t AT45::useBinaryPageSize()
    {
      Chimera::Status_t error = Chimera::CommonStatusCodes::FAIL;

      if ( !spiInitialized )
      {
        error = Chimera::CommonStatusCodes::NOT_INITIALIZED;
      }
      else
      {
#if defined( SW_SIM )
        pageSize   = PAGE_SIZE_BINARY;
        blockSize  = BLOCK_SIZE_BINARY;
        sectorSize = SECTOR_SIZE_BINARY;

        error = Chimera::CommonStatusCodes::OK;
#else
        /*------------------------------------------------
        Instruct the chip to switch over to binary sizing
        ------------------------------------------------*/
        uint32_t cmd = CFG_PWR_2_PAGE_SIZE;
        memcpy( cmdBuffer.data(), &cmd, BYTE_LEN( cmd ) );
        SPI_write( cmdBuffer.data(), BYTE_LEN( cmd ), true );

        /*------------------------------------------------
        Wait until the chip signals it has completed
        ------------------------------------------------*/
        while ( !isDeviceReady() )
        {
          Chimera::delayMilliseconds( 10 );
        }

        /*------------------------------------------------
        Update our knowledge of the flash sizing
        ------------------------------------------------*/
        if ( getPageSizeConfig() == PAGE_SIZE_BINARY )
        {
          pageSize   = PAGE_SIZE_BINARY;
          blockSize  = BLOCK_SIZE_BINARY;
          sectorSize = SECTOR_SIZE_BINARY;

          error = Chimera::CommonStatusCodes::OK;
        }
#endif /* !SW_SIM */
      }

      return error;
    }

    Chimera::Status_t AT45::useExtendedPageSize()
    {
      Chimera::Status_t error = Chimera::CommonStatusCodes::FAIL;

      if ( !spiInitialized )
      {
        error = Chimera::CommonStatusCodes::NOT_INITIALIZED;
      }
      else
      {
#if defined( SW_SIM )
        pageSize   = PAGE_SIZE_EXTENDED;
        blockSize  = BLOCK_SIZE_EXTENDED;
        sectorSize = SECTOR_SIZE_EXTENDED;

        error = Chimera::CommonStatusCodes::OK;
#else
        /*------------------------------------------------
        Instruct the chip to switch over to alternate sizing
        ------------------------------------------------*/
        uint32_t cmd = CFG_STD_FLASH_PAGE_SIZE;
        memcpy( cmdBuffer.data(), &cmd, BYTE_LEN( cmd ) );
        SPI_write( cmdBuffer.data(), BYTE_LEN( cmd ), true );

        /*------------------------------------------------
        Wait until the chip signals it has completed
        ------------------------------------------------*/
        while ( !isDeviceReady() )
        {
          Chimera::delayMilliseconds( 10 );
        }

        /*------------------------------------------------
        Update our knowledge of the flash sizing
        ------------------------------------------------*/
        if ( getPageSizeConfig() == PAGE_SIZE_EXTENDED )
        {
          pageSize   = PAGE_SIZE_EXTENDED;
          blockSize  = BLOCK_SIZE_EXTENDED;
          sectorSize = SECTOR_SIZE_EXTENDED;

          error = Chimera::CommonStatusCodes::OK;
        }
#endif /* !SW_SIM */
      }

      return error;
    }

    Chimera::Status_t AT45::getDeviceInfo( AT45xx_DeviceInfo &info )
    {
      Chimera::Status_t error = Chimera::CommonStatusCodes::FAIL;

      if ( !spiInitialized )
      {
        error = Chimera::CommonStatusCodes::NOT_INITIALIZED;
      }
      else
      {
        std::array<uint8_t, 3> data;
        data.fill( 0 );
        cmdBuffer.fill( 0 );

        cmdBuffer[ 0 ] = READ_DEVICE_INFO;
        SPI_write( cmdBuffer.data(), BYTE_LEN( READ_DEVICE_INFO ), false );
        SPI_read( data.data(), static_cast<uint32_t>( data.size() ), true );

        info.manufacturerID = data[ 0 ];
        info.familyCode     = static_cast<FamilyCode>( ( uint8_t )( data[ 1 ] >> 5 ) & 0xFF );
        info.densityCode    = static_cast<DensityCode>( data[ 1 ] & 0x1F );
        info.subCode        = static_cast<SubCode>( ( data[ 2 ] >> 5 ) & 0xFF );
        info.productVariant = static_cast<ProductVariant>( data[ 2 ] & 0x1F );

        error = Chimera::CommonStatusCodes::OK;
      }

      return error;
    }

    uint32_t AT45::getFlashCapacity()
    {
      uint32_t retVal = 0u;

      if ( chipInitialized )
      {
        switch ( chipInfo.densityCode )
        {
          case DENSITY_2MBIT:
            retVal = 262144;
            break;

          case DENSITY_4MBIT:
            retVal = 524288;
            break;

          case DENSITY_8MBIT:
            retVal = 1048576;
            break;

          case DENSITY_16MBIT:
            retVal = 2097152;
            break;

          case DENSITY_32MBIT:
            retVal = 4194304;
            break;

          case DENSITY_64MBIT:
            retVal = 8388608;
            break;

          default:
            break;
        }
      }

      return retVal;
    }

    uint32_t AT45::getPageSize()
    {
      return pageSize;
    }

    uint32_t AT45::getBlockSize()
    {
      return blockSize;
    }

    uint32_t AT45::getSectorSize()
    {
      return sectorSize;
    }

    BlockStatus AT45::DiskOpen( const uint8_t volNum, BlockMode openMode )
    {
      return BlockStatus::BLOCK_DEV_ENOSYS;
    }

    BlockStatus AT45::DiskClose( const uint8_t volNum )
    {
      return BlockStatus::BLOCK_DEV_ENOSYS;
    }

    BlockStatus AT45::DiskRead( const uint8_t volNum, const uint64_t sectorStart, const uint32_t sectorCount,
                                void *const readBuffer )
    {
      return BlockStatus::BLOCK_DEV_ENOSYS;
    }

    BlockStatus AT45::DiskWrite( const uint8_t volNum, const uint64_t sectorStart, const uint32_t sectorCount,
                                 const void *const writeBuffer )
    {
      return BlockStatus::BLOCK_DEV_ENOSYS;
    }

    BlockStatus AT45::DiskFlush( const uint8_t volNum )
    {
      return BlockStatus::BLOCK_DEV_ENOSYS;
    }

    bool AT45::isInitialized()
    {
      return chipInitialized;
    }

    Chimera::Status_t AT45::write( const uint32_t address, const uint8_t *const dataIn, const uint32_t len )
    {
      Chimera::Status_t error = Chimera::CommonStatusCodes::FAIL;

      if ( !chipInitialized )
      {
        error = Chimera::CommonStatusCodes::NOT_INITIALIZED;
      }
      else if ( !dataIn )
      {
        error = Chimera::CommonStatusCodes::INVAL_FUNC_PARAM;
      }
      else if ( ( address + len ) >= getFlashCapacity() )
      {
        error = ErrCode::OVERRUN;
      }
      else
      {
        error = Chimera::CommonStatusCodes::OK;

        Chimera::Modules::Memory::MemoryBlockRange dataRange( address, address + len, pageSize );

        uint32_t bytesWritten = 0;
        uint32_t bytesLeft    = len;
        uint32_t currentBlock = dataRange.startBlock();
        uint32_t startOffset  = dataRange.startOffset();
        uint32_t endOffset    = dataRange.endOffset();

        /*------------------------------------------------
        Write the first partial page (if there is one)
        ------------------------------------------------*/
        if ( startOffset != std::numeric_limits<uint32_t>::max() )
        {
          const uint32_t partialWriteSize = dataRange.startBytes();
          error = readModifyWrite( SRAMBuffer::BUFFER1, currentBlock, startOffset, dataIn, partialWriteSize );

          /*------------------------------------------------
          Wait for the chip to be finished with this operation. Thankfully
          this is a non-blocking operation if the Chimera backend implements
          the delay mechanism properly.
          ------------------------------------------------*/
          while ( !isDeviceReady() )
          {
            Chimera::delayMilliseconds( chipDelay[ static_cast<uint8_t>( device ) ].pageEraseAndProgramming );
          }

          /*------------------------------------------------
          Check if the readModifyWrite failed or the chip signaled some error
          ------------------------------------------------*/
          if ( ( error != Chimera::CommonStatusCodes::OK ) || ( isErasePgmError() != Chimera::CommonStatusCodes::OK ) )
          {
            error = Chimera::CommonStatusCodes::FAILED_WRITE;
          }
          else
          {
            bytesLeft -= partialWriteSize;
            bytesWritten += partialWriteSize;
            currentBlock += 1u;
          }
        }

        /*------------------------------------------------
        Write consecutive, fully spanned pages next
        ------------------------------------------------*/
        if ( error == Chimera::CommonStatusCodes::OK )
        {
          while ( bytesLeft >= pageSize )
          {
            error = pageWrite( SRAMBuffer::BUFFER1, 0, currentBlock, dataIn + bytesWritten, pageSize );

            while ( !isDeviceReady() )
            {
              Chimera::delayMilliseconds( chipDelay[ static_cast<uint8_t>( device ) ].pageEraseAndProgramming );
            }

            if ( error == Chimera::CommonStatusCodes::OK )
            {
              bytesLeft -= pageSize;
              bytesWritten += pageSize;
              currentBlock += 1u;
            }
            else
            {
              break;
            }
          }
        }

        /*------------------------------------------------
        Write the last partial page (if there is one)
        ------------------------------------------------*/
        if ( ( error == Chimera::CommonStatusCodes::OK ) && bytesLeft && ( endOffset != std::numeric_limits<uint32_t>::max() ) )
        {
          error = readModifyWrite( SRAMBuffer::BUFFER1, currentBlock, 0u, dataIn + bytesWritten, endOffset );

          /*------------------------------------------------
          Wait for the chip to be finished with this operation. Thankfully
          this is a non-blocking operation if the Chimera backend implements
          the delay mechanism properly.
          ------------------------------------------------*/
          while ( !isDeviceReady() )
          {
            Chimera::delayMilliseconds( chipDelay[ static_cast<uint8_t>( device ) ].pageEraseAndProgramming );
          }

          /*------------------------------------------------
          Check if the readModifyWrite failed or the chip signaled some error
          ------------------------------------------------*/
          if ( ( error != Chimera::CommonStatusCodes::OK ) || ( isErasePgmError() != Chimera::CommonStatusCodes::OK ) )
          {
            error = Chimera::CommonStatusCodes::FAILED_WRITE;
          }
        }
      }

      return error;
    }

    Chimera::Status_t AT45::read( const uint32_t address, uint8_t *const dataOut, const uint32_t len )
    {
      Chimera::Status_t error = Chimera::CommonStatusCodes::FAIL;

      if ( !chipInitialized )
      {
        error = Chimera::CommonStatusCodes::NOT_INITIALIZED;
      }
      else if ( !dataOut )
      {
        error = Chimera::CommonStatusCodes::INVAL_FUNC_PARAM;
      }
      else if ( ( address + len ) >= getFlashCapacity() )
      {
        error = ErrCode::OVERRUN;
      }
      else
      {
        Chimera::Modules::Memory::MemoryBlockRange dataRange( address, address + len, pageSize );
        error = directArrayRead( dataRange.startBlock(), dataRange.startOffset(), dataOut, len );
      }

      return error;
    }

    Chimera::Status_t AT45::erase( const uint32_t address, const uint32_t len )
    {
      Chimera::Status_t error = Chimera::CommonStatusCodes::FAIL;

      if ( !chipInitialized )
      {
        error = Chimera::CommonStatusCodes::NOT_INITIALIZED;
      }
      else if ( ( address + len ) >= getFlashCapacity() )
      {
        error = ErrCode::OVERRUN;
      }
      else if ( ( address % pageSize != 0 ) || ( len % pageSize != 0 ) )
      {
        error = ErrCode::UNALIGNED_MEM;
      }
      else
      {
        DeviceDescriptor dev{ pageSize, blockSize, sectorSize };
        FlashUtilities util( dev );

        auto range = util.getCompositeSections( address, len );
        error      = eraseRanges( range );
      }

      return error;
    }

    Chimera::Status_t AT45::writeCompleteCallback( const Chimera::void_func_uint32_t func )
    {
      return Chimera::CommonStatusCodes::NOT_SUPPORTED;
    }

    Chimera::Status_t AT45::readCompleteCallback( const Chimera::void_func_uint32_t func )
    {
      return Chimera::CommonStatusCodes::NOT_SUPPORTED;
    }

    Chimera::Status_t AT45::eraseCompleteCallback( const Chimera::void_func_uint32_t func )
    {
      return Chimera::CommonStatusCodes::NOT_SUPPORTED;
    }

    Chimera::Status_t AT45::eraseRanges( const SectionList &range )
    {
      Chimera::Status_t error = Chimera::CommonStatusCodes::OK;

      /*------------------------------------------------
      Sectors
      ------------------------------------------------*/
      for ( size_t i = 0; i < range.sectors.size(); i++ )
      {
        error = eraseSector( range.sectors[ i ] );

        while ( isDeviceReady() != Chimera::CommonStatusCodes::OK )
        {
          Chimera::delayMilliseconds( chipDelay[ static_cast<uint8_t>( device ) ].sectorErase );
        }

        if ( isErasePgmError() != Chimera::CommonStatusCodes::OK )
        {
          error = ErrCode::FAILED_ERASE;
        }
      }

      /*------------------------------------------------
      Blocks
      ------------------------------------------------*/
      for ( size_t i = 0; i < range.blocks.size(); i++ )
      {
        error = eraseBlock( range.blocks[ i ] );

        while ( isDeviceReady() != Chimera::CommonStatusCodes::OK )
        {
          Chimera::delayMilliseconds( chipDelay[ static_cast<uint8_t>( device ) ].blockErase );
        }

        if ( isErasePgmError() != Chimera::CommonStatusCodes::OK )
        {
          error = ErrCode::FAILED_ERASE;
        }
      }

      /*------------------------------------------------
      Pages
      ------------------------------------------------*/
      for ( size_t i = 0; i < range.pages.size(); i++ )
      {
        error = erasePage( range.pages[ i ] );

        while ( isDeviceReady() != Chimera::CommonStatusCodes::OK )
        {
          Chimera::delayMilliseconds( chipDelay[ static_cast<uint8_t>( device ) ].pageErase );
        }

        if ( isErasePgmError() != Chimera::CommonStatusCodes::OK )
        {
          error = ErrCode::FAILED_ERASE;
        }
      }

      return error;
    }

    void AT45::buildReadWriteCommand( const uint16_t pageNumber, const uint16_t offset )
    {
      /*------------------------------------------------
      Grab the correct page configuration size. This informs the code
      how much bit shifting to apply when building the command.
      ------------------------------------------------*/
      const AddressDescriptions *config;
      if ( pageSize == PAGE_SIZE_EXTENDED )
      {
        config = &addressFormat[ static_cast<uint8_t>( device ) ].page.standardSize;
      }
      else
      {
        config = &addressFormat[ static_cast<uint8_t>( device ) ].page.binarySize;
      }

      /*------------------------------------------------
      Generate masks of the correct bit width to clean up the input variables
      ------------------------------------------------*/
      uint32_t addressBitMask = ( 1u << config->addressBits ) - 1u;
      uint32_t offsetBitMask  = ( 1u << config->dummyBitsLSB ) - 1u;

      /*------------------------------------------------
       The full address is really only 3 bytes wide. They are set up as
       follows, with 'a' == address bit, 'o' == offset bit and 'x' == don't care.
       This is the exact order in which it must be transmitted. (ie MSB first)

                               Byte 1 | Byte 2 | Byte 3
      For 264 byte page size: xxxaaaaa|aaaaaaao|oooooooo
      For 256 byte page size: xxxxaaaa|aaaaaaaa|oooooooo
      ------------------------------------------------*/
      uint32_t fullAddress = ( ( pageNumber & addressBitMask ) << config->dummyBitsLSB ) | ( offsetBitMask & offset );

      /*------------------------------------------------
      Note: Cannot use memcpy because it reverses the byte order expected by the flash chip.
      For example, if the value of 'fullAddress' were 0xAABBCC, the memcpy would put the values into the cmdBuffer as
       *0xCCBBAA. This is correct as far as the MCU is concerned, but the flash chip needs the data exactly as calculated:
       *0xAABBCC
      ------------------------------------------------*/
      cmdBuffer[ 1 ] = ( fullAddress & 0xFF0000 ) >> 16;
      cmdBuffer[ 2 ] = ( fullAddress & 0x00FF00 ) >> 8;
      cmdBuffer[ 3 ] = ( fullAddress & 0x0000FF );
    }

    void AT45::buildEraseCommand( const Chimera::Modules::Memory::Section_t section, const uint32_t sectionNumber )
    {
      uint32_t fullAddress              = 0u;
      const AddressDescriptions *config = nullptr;

      switch ( section )
      {
        case Section_t::PAGE:
          if ( pageSize == PAGE_SIZE_EXTENDED )
          {
            config = &addressFormat[ static_cast<uint8_t>( device ) ].page.standardSize;
          }
          else
          {
            config = &addressFormat[ static_cast<uint8_t>( device ) ].page.binarySize;
          }
          break;

        case Section_t::BLOCK:
          if ( pageSize == PAGE_SIZE_EXTENDED )
          {
            config = &addressFormat[ static_cast<uint8_t>( device ) ].block.standardSize;
          }
          else
          {
            config = &addressFormat[ static_cast<uint8_t>( device ) ].block.binarySize;
          }
          break;

        case Section_t::SECTOR:
          if ( sectionNumber == 0 )
          {
            if ( pageSize == PAGE_SIZE_EXTENDED )
            {
              config = &addressFormat[ static_cast<uint8_t>( device ) ].sector0ab.standardSize;
            }
            else
            {
              config = &addressFormat[ static_cast<uint8_t>( device ) ].sector0ab.binarySize;
            }
          }
          else
          {
            if ( pageSize == PAGE_SIZE_EXTENDED )
            {
              config = &addressFormat[ static_cast<uint8_t>( device ) ].sector.standardSize;
            }
            else
            {
              config = &addressFormat[ static_cast<uint8_t>( device ) ].sector.binarySize;
            }
          }
          break;

        default:
          break;
      };

      if ( !config )
      {
        return;
      }

      /*------------------------------------------------
      This ignores Sector 0a for simplicity reasons. The full address below
      directly corresponds to Sector 0b, and the format seems common across
      all AT45 chips. Use Block 0 to get the address for Sector 0a.
      ------------------------------------------------*/
      if ( ( section == Section_t::SECTOR ) && ( sectionNumber == 0 ) )
      {
        fullAddress = 1u << config->dummyBitsLSB;
      }
      else
      {
        uint32_t bitMask = ( 1u << config->addressBits ) - 1u;
        fullAddress      = ( sectionNumber & bitMask ) << config->dummyBitsLSB;
      }

      /*------------------------------------------------
      Note: Cannot use memcpy because it reverses the byte order expected by the flash chip.
      For example, if the value of 'fullAddress' were 0xAABBCC, the memcpy would put the values
      into the cmdBuffer as 0xCCBBAA. This is correct as far as the MCU is concerned, but the
      flash chip needs the data exactly as calculated: 0xAABBCC
      ------------------------------------------------*/
      cmdBuffer[ 1 ] = ( fullAddress & 0xFF0000 ) >> 16;
      cmdBuffer[ 2 ] = ( fullAddress & 0x00FF00 ) >> 8;
      cmdBuffer[ 3 ] = fullAddress & 0x0000FF;
    }

    void AT45::SPI_write( const uint8_t *const data, const uint32_t len, const bool disableSS )
    {
      spi->setChipSelect( Chimera::GPIO::State::LOW );
      spi->writeBytes( data, len, 10 );

      if ( disableSS )
      {
        spi->setChipSelect( Chimera::GPIO::State::HIGH );
      }
    }

    void AT45::SPI_read( uint8_t *const data, const uint32_t len, const bool disableSS )
    {
      spi->setChipSelect( Chimera::GPIO::State::LOW );
      spi->readBytes( data, len, 10 );

      if ( disableSS )
      {
        spi->setChipSelect( Chimera::GPIO::State::HIGH );
      }
    }

  }  // namespace NORFlash
}  // namespace Adesto
