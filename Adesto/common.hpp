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
}  // namespace Adesto

#endif  /* !ADESTO_COMMON_HPP */
