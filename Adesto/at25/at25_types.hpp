/********************************************************************************
 *  File Name:
 *    at25_types.hpp
 *
 *  Description:
 *    Types and constants associated with the Adesto AT25 series memory devices
 *
 *  2020 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

#pragma once
#ifndef ADESTO_AT25_TYPES_HPP
#define ADESTO_AT25_TYPES_HPP

/* STL Includes */
#include <cstddef>
#include <memory>

/* Adesto Includes */
#include <Adesto/common.hpp>

namespace Adesto::AT25
{
  /*-------------------------------------------------------------------------------
  Forward Declarations
  -------------------------------------------------------------------------------*/
  class Driver;

  /*-------------------------------------------------------------------------------
  Aliases
  -------------------------------------------------------------------------------*/
  using Driver_sPtr = std::shared_ptr<Driver>;

  /*-------------------------------------------------------------------------------
  Enumerations
  -------------------------------------------------------------------------------*/


  /*-------------------------------------------------------------------------------
  Structures
  -------------------------------------------------------------------------------*/
  struct DeviceInfo
  {
    Jedec_t mfgID;
    FamilyCode family;
    DensityCode density;
    SubCode sub;
    ProductVariant variant;
  };
}  // namespace Adesto::AT25

#endif /* !ADESTO_AT25_TYPES_HPP */
