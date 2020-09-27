/********************************************************************************
 *  File Name:
 *    common.hpp
 *
 *  Description:
 *    Common Adesto properties
 *
 *  2020 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

#pragma once
#ifndef ADESTO_COMMON_HPP
#define ADESTO_COMMON_HPP

/* STL Includes */
#include <cstdint>
#include <cstddef>

namespace Adesto
{
  /*-------------------------------------------------------------------------------
  Aliases
  -------------------------------------------------------------------------------*/
  using Jedec_t   = uint8_t;
  using Command_t = uint8_t;

  /*-------------------------------------------------------------------------------
  Constants
  -------------------------------------------------------------------------------*/
  static constexpr Jedec_t JEDEC_CODE = 0x1F;

  /*-------------------------------------------------
  Addressing Constants
  -------------------------------------------------*/
  static constexpr size_t ADDRESS_BYTE_1_POS = 0;
  static constexpr size_t ADDRESS_BYTE_1_MSK = 0x000000FF;

  static constexpr size_t ADDRESS_BYTE_2_POS = 8;
  static constexpr size_t ADDRESS_BYTE_2_MSK = 0x0000FF00;

  static constexpr size_t ADDRESS_BYTE_3_POS = 16;
  static constexpr size_t ADDRESS_BYTE_3_MSK = 0x00FF0000;


  /*-------------------------------------------------
  Manufacturer & Device ID Bit Masks
  -------------------------------------------------*/
  static constexpr uint8_t MFR_MSK = 0xFF;

  static constexpr uint8_t FAMILY_CODE_POS = 5;
  static constexpr uint8_t FAMILY_CODE_MSK = 0x07;

  static constexpr uint8_t DENSITY_CODE_POS = 0;
  static constexpr uint8_t DENSITY_CODE_MSK = 0x1F;

  static constexpr uint8_t SUB_CODE_POS = 5;
  static constexpr uint8_t SUB_CODE_MSK = 0x07;

  static constexpr uint8_t PROD_VERSION_POS = 0;
  static constexpr uint8_t PROD_VERSION_MSK = 0x1F;

  /*-------------------------------------------------
  Common Block Sizes
  -------------------------------------------------*/
  static constexpr size_t CHUNK_SIZE_256 = 256;
  static constexpr size_t CHUNK_SIZE_4K  = 4 * 1024;
  static constexpr size_t CHUNK_SIZE_32K = 32 * 1024;
  static constexpr size_t CHUNK_SIZE_64K = 64 * 1024;

  /*-------------------------------------------------------------------------------
  Enumerations
  -------------------------------------------------------------------------------*/
  enum FamilyCode : uint8_t
  {
    AT45Dxxx  = 0x01,
    AT25SFxxx = 0x04,
  };

  enum DensityCode : uint8_t
  {
    DENSITY_2MBIT  = 0x03,
    DENSITY_4MBIT  = 0x04,
    DENSITY_8MBIT  = 0x05,
    DENSITY_16MBIT = 0x06,
    DENSITY_32MBIT = 0x07,
    DENSITY_64MBIT = 0x08
  };

  enum SubCode : uint8_t
  {
    STANDARD_SERIES = 0x00,
    // Add more as needed
  };

  enum ProductVariant : uint8_t
  {
    DEFAULT  = 0x00,
    VERSION1 = 0x01,
    // Add more as needed
  };

  /*-------------------------------------------------------------------------------
  Public Functions
  -------------------------------------------------------------------------------*/
  /**
   *  Converts supported density codes into the appropriate
   *  byte amounts.
   *
   *  @param[in]  density     Which density to look up
   *  @return size_t
   */
  size_t densityToBytes( const DensityCode density );

}  // namespace Adesto

#endif /* !ADESTO_COMMON_HPP */
