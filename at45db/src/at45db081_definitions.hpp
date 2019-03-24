#pragma once
#ifndef AT45DB801_DEFINITIONS
#define AT45DB801_DEFINITIONS

#include <stdlib.h>
#include <stdint.h>

namespace Adesto
{
  const uint8_t JEDEC_CODE = 0x1F;

  /** Upper 3 MSB of Device ID Byte 1*/
  enum FamilyCode : uint8_t
  {
    AT45Dxxx = 0x01,
    // Add more as needed
  };

  /** Lower 5 LSB of Device ID Byte 1*/
  enum DensityCode : uint8_t
  {
    DENSITY_2MBIT  = 0x03,
    DENSITY_4MBIT  = 0x04,
    DENSITY_8MBIT  = 0x05,
    DENSITY_16MBIT = 0x06,
    DENSITY_32MBIT = 0x07,
    DENSITY_64MBIT = 0x08
  };

  /** Upper 3 MSB of Device ID Byte 2*/
  enum SubCode : uint8_t
  {
    STANDARD_SERIES = 0x00,
    // Add more as needed
  };

  /** Lower 5 LSB of Device ID Byte 2*/
  enum ProductVariant : uint8_t
  {
    DEFAULT = 0x00,
    // Add more as needed
  };

  namespace NORFlash
  {
    static constexpr uint8_t ERASE_RESET_VAL = 0xFF;

    static constexpr uint16_t PAGE_SIZE_BINARY   = 256u;
    static constexpr uint32_t BLOCK_SIZE_BINARY  = 2048u;
    static constexpr uint32_t SECTOR_SIZE_BINARY = 65536u;

    static constexpr uint16_t PAGE_SIZE_EXTENDED   = 264u;
    static constexpr uint32_t BLOCK_SIZE_EXTENDED  = 2112u;
    static constexpr uint32_t SECTOR_SIZE_EXTENDED = 67584u;

    /*------------------------------------------------
    Status Register Bits
    ------------------------------------------------*/
    static constexpr uint16_t READY_BUSY_Pos         = ( 1u << 15 );
    static constexpr uint16_t COMPARE_RESULT_Pos     = ( 1u << 14 );
    static constexpr uint16_t SECTOR_PROTECTION_Pos  = ( 1u << 9 );
    static constexpr uint16_t PAGE_SIZE_CONFIG_Pos   = ( 1u << 8 );
    static constexpr uint16_t ERASE_PGM_ERROR_Pos    = ( 1u << 5 );
    static constexpr uint16_t SECTOR_LOCKDOWN_EN_Pos = ( 1u << 3 );
    static constexpr uint16_t BUFF2_PGM_SUSPEND_Pos  = ( 1u << 2 );
    static constexpr uint16_t BUFF1_PGM_SUSPEND_Pos  = ( 1u << 1 );
    static constexpr uint16_t ERASE_SUSPEND_Pos      = ( 1u << 0 );

    /*------------------------------------------------
    Available Read Operations
    See: Datasheet Section 15
    ------------------------------------------------*/
    static constexpr uint8_t MAIN_MEM_PAGE_READ = 0xD2; /* Main Memory Page Read */
    static constexpr uint8_t CONT_ARR_READ_LP   = 0x01; /* Continuous Array Read (Low Power Mode) */
    static constexpr uint8_t CONT_ARR_READ_LF   = 0x03; /* Continuous Array Read (Low Frequency) */
    static constexpr uint8_t CONT_ARR_READ_HF1  = 0x0B; /* Continuous Array Read (High Frequency) */
    static constexpr uint8_t CONT_ARR_READ_HF2  = 0x1B; /* Continuous Array Read (High Frequency) */
    static constexpr uint8_t CONT_ARR_READ_LEG  = 0xE8; /* Continuous Array Read (Legacy -- Not Recommended) */
    static constexpr uint8_t BUFFER1_READ_LF    = 0xD1; /* Buffer 1 Read (Low Frequency) */
    static constexpr uint8_t BUFFER2_READ_LF    = 0xD3; /* Buffer 2 Read (Low Frequency) */
    static constexpr uint8_t BUFFER1_READ_HF    = 0xD4; /* Buffer 1 Read (High Frequency) */
    static constexpr uint8_t BUFFER2_READ_HF    = 0xD6; /* Buffer 2 Read (High Frequency) */


    /*------------------------------------------------
    Available Program & Erase Commands
    See: Datasheet Section 15
    ------------------------------------------------*/
    static constexpr uint8_t BUFFER1_WRITE                         = 0x84; /* Buffer 1 Write */
    static constexpr uint8_t BUFFER2_WRITE                         = 0x87; /* Buffer 2 Write */
    static constexpr uint8_t BUFFER1_TO_MAIN_MEM_PAGE_PGM_W_ERASE  = 0x83; /* Buffer 1 to Main Memory Page Program with Built-In Erase */
    static constexpr uint8_t BUFFER2_TO_MAIN_MEM_PAGE_PGM_W_ERASE  = 0x86; /* Buffer 2 to Main Memory Page Program with Build-In Erase */
    static constexpr uint8_t BUFFER1_TO_MAIN_MEM_PAGE_PGM_WO_ERASE = 0x88; /* Buffer 1 to Main Memory Page Program without Built-In Erase */
    static constexpr uint8_t BUFFER2_TO_MAIN_MEM_PAGE_PGM_WO_ERASE = 0x89; /* Buffer 2 to Main Memory Page Program without Built-In Erase */
    static constexpr uint8_t MAIN_MEM_PAGE_PGM_THR_BUFFER1_W_ERASE = 0x82; /* Main Memory Page Program through Buffer 1 with Built-In Erase */
    static constexpr uint8_t MAIN_MEM_PAGE_PGM_THR_BUFFER2_W_ERASE = 0x85; /* Main Memory Page Program through Buffer 2 with Built-In Erase */
    static constexpr uint8_t MAIN_MEM_BP_PGM_THR_BUFFER1_WO_ERASE  = 0x02; /* Main Memory Byte/Page Program through Buffer 1 without Built-In Erase */
    static constexpr uint8_t PAGE_ERASE                            = 0x81; /* Page Erase */
    static constexpr uint8_t BLOCK_ERASE                           = 0x50; /* Block Erase */
    static constexpr uint8_t SECTOR_ERASE                          = 0x7C; /* Sector Erase */
    static constexpr uint8_t PGM_OR_ERASE_SUSPEND                  = 0xB0; /* Program/Erase Suspend */
    static constexpr uint8_t PGM_OR_ERASE_RESUME                   = 0xD0; /* Program/Erase Suspend */
    static constexpr uint8_t RMW_THR_BUFFER1                       = 0x58; /* Read-Modify-Write through Buffer 1 */
    static constexpr uint8_t RMW_THR_BUFFER2                       = 0x59; /* Read-Modify-Write through Buffer 2 */

    // Note: These values are reversed to preserve bit order
    static constexpr uint32_t CHIP_ERASE = 0x9A8094C7; /* Chip Erase */

    /*------------------------------------------------
    Protection & Security Commands
    See: Datasheet Section 15
    ------------------------------------------------*/
    static constexpr uint8_t READ_SECTOR_PROTECTION_REG = 0x32; /* Read Sector Protection Register */
    static constexpr uint8_t READ_SECTOR_LOCKDOWN_REG   = 0x35; /* Read Sector Lockdown Register */
    static constexpr uint8_t READ_SECURITY_REGISTER     = 0x77; /* Read Security Register */

    // Note: These values are reversed to preserve bit order
    static constexpr uint32_t ENABLE_SECTOR_PROTECTION    = 0xA97F2A3D; /* Enable Sector Protection */
    static constexpr uint32_t DISABLE_SECTOR_PROTECTION   = 0x9A7F2A3D; /* Disable Sector Protection */
    static constexpr uint32_t ERASE_SECTOR_PROTECTION_REG = 0xCF7F2A3D; /* Erase Sector Protection Register */
    static constexpr uint32_t PGM_SECTOR_PROTECTION_REG   = 0xFC7F2A3D; /* Program Sector Protection Register */
    static constexpr uint32_t SECTOR_LOCKDOWN             = 0x307F2A3D; /* Sector Lockdown */
    static constexpr uint32_t FREEZE_SECTOR_LOCKDOWN      = 0x40AA5534; /* Freeze Sector Lockdown */
    static constexpr uint32_t PGM_SECURITY_REGISTER       = 0x0000009B; /* Program Security Register */

    /*------------------------------------------------
    Additional Commands
    See: Datasheet Section 15
    ------------------------------------------------*/
    static constexpr uint8_t MAIN_MEM_PAGE_TO_BUFFER1_TRANSFER = 0x53; /* Main Memory Page to Buffer 1 Transfer */
    static constexpr uint8_t MAIN_MEM_PAGE_TO_BUFFER2_TRANSFER = 0x55; /* Main Memory Page to Buffer 2 Transfer */
    static constexpr uint8_t MAIN_MEM_PAGE_TO_BUFFER1_COMPARE  = 0x60; /* Main Memory Page to Buffer 1 Compare */
    static constexpr uint8_t MAIN_MEM_PAGE_TO_BUFFER2_COMPARE  = 0x61; /* Main Memory Page to Buffer 2 Compare */
    static constexpr uint8_t AUTO_PAGE_REWRITE1                = 0x58; /* Auto Page Rewrite */
    static constexpr uint8_t AUTO_PAGE_REWRITE2                = 0x59; /* Auto Page Rewrite */
    static constexpr uint8_t DEEP_POWER_DOWN                   = 0xB9; /* Deep Power Down */
    static constexpr uint8_t RESUME_FROM_DEEP_POWER_DOWN       = 0xAB; /* Resume From Deep Power Down */
    static constexpr uint8_t ULTRA_DEEP_POWER_DOWN             = 0x79; /* Ultra-Deep Power Down */
    static constexpr uint8_t STATUS_REGISTER_READ              = 0xD7; /* Status Register Read */
    static constexpr uint8_t READ_DEVICE_INFO                  = 0x9F; /* Manufacturer and Device ID Read */

    // Note: These values are reversed to preserve bit order
    static constexpr uint32_t CFG_PWR_2_PAGE_SIZE     = 0xA6802A3D; /* Configure "Power of 2" (Binary) Page Size */
    static constexpr uint32_t CFG_STD_FLASH_PAGE_SIZE = 0xA7802A3D; /* Configure Standard DataFlash Page Size */
    static constexpr uint32_t SOFTWARE_RESET          = 0x000000F0; /* Software Reset */



  }  // namespace NORFlash
}  // namespace Adesto
#endif /* AT45DB801_DEFINITIONS */
