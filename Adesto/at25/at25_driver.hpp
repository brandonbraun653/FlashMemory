/********************************************************************************
 *  File Name:
 *    at25_driver.hpp
 *
 *  Description:
 *    Driver for the AT25 based memory chips
 *
 *  2020 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

#pragma once
#ifndef ADESTO_AT25_MEMORY_HPP
#define ADESTO_AT25_MEMORY_HPP

/* Aurora Includes */
#include <Aurora/memory>

/* Chimera Includes */
#include <Chimera/common>
#include <Chimera/spi>
#include <Chimera/thread>

/* Adesto Includes */
#include <Adesto/at25/at25_types.hpp>
#include <Adesto/at25/at25_commands.hpp>

namespace Adesto::AT25
{
  class Driver : public virtual Aurora::Memory::IGenericDevice, public Chimera::Threading::Lockable
  {
  public:
    Driver();
    ~Driver();

    /*-------------------------------------------------
    Generic Memory Device Interface
    -------------------------------------------------*/
    Aurora::Memory::Status open() final override;
    Aurora::Memory::Status close() final override;
    Aurora::Memory::Status write( const size_t address, const void *const data, const size_t length ) final override;
    Aurora::Memory::Status read( const size_t address, void *const data, const size_t length ) final override;
    Aurora::Memory::Status erase( const size_t address, const size_t length ) final override;
    Aurora::Memory::Status erase( const Aurora::Memory::Chunk chunk, const size_t id ) final override;
    Aurora::Memory::Status eraseChip() final override;
    Aurora::Memory::Status flush() final override;
    Aurora::Memory::Status pendEvent( const Aurora::Memory::Event event, const size_t timeout ) final override;
    Aurora::Memory::Status onEvent( const Aurora::Memory::Event event, void ( *func )( const size_t ) ) final override;
    Aurora::Memory::Status writeProtect( const bool enable, const Aurora::Memory::Chunk chunk, const size_t id ) final override;
    Aurora::Memory::Status readProtect( const bool enable, const Aurora::Memory::Chunk chunk, const size_t id ) final override;
    Aurora::Memory::Properties getDeviceProperties() final override;

    /*-------------------------------------------------
    Adesto Driver Interface
    -------------------------------------------------*/
    /**
     *  Configures the driver to use the correct settings. Note that
     *  the SPI instance must be pre-initialized.
     *
     *  @param[in]  channel     Which SPI channel to use.
     *  @return bool
     */
    bool configure( const Chimera::SPI::Channel channel );

    /**
     *  Reads the device configuration info
     *
     *  @param[out]  info       Device information results
     *  @return bool            If true, the output struct contains valid data
     */
    bool readDeviceInfo( DeviceInfo &info );

    /**
     *  Reads the status register bytes
     *
     *  @return uint16_t
     */
    uint16_t readStatusRegister();

  private:
    DeviceInfo mInfo;                                    /**< Device specific details */
    Chimera::SPI::Driver_sPtr mSPI;                      /**< SPI driver instance */
    std::array<uint8_t, Command::MAX_CMD_LEN> cmdBuffer; /**< Buffer for holding a command sequence */

    /*-------------------------------------------------------------------------------
    Private Functions
    -------------------------------------------------------------------------------*/
    void issueWriteEnable();
  };
}  // namespace Adesto::AT25

#endif  /* !ADESTO_AT25_MEMORY_HPP */
