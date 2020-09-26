/********************************************************************************
 *  File Name:
 *    at25_constants.hpp
 *
 *  Description:
 *    Constants describing the AT25 series devices
 *
 *  2020 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

#pragma once
#ifndef ADESTO_AT25_CONSTANTS_HPP
#define ADESTO_AT25_CONSTANTS_HPP

/* STL Includes */
#include <array>
#include <cstddef>

/* Adesto Includes */
#include <Adesto/common.hpp>

namespace Adesto::AT25
{
  /*-------------------------------------------------------------------------------
  Constants
  -------------------------------------------------------------------------------*/
  /*-------------------------------------------------
  Default selections for paging sizes. Originally
  configured for the AT25SF081.
  -------------------------------------------------*/
  static constexpr size_t PAGE_SIZE   = CHUNK_SIZE_256;
  static constexpr size_t BLOCK_SIZE  = CHUNK_SIZE_4K;
  static constexpr size_t SECTOR_SIZE = CHUNK_SIZE_32K;
  // There is also a 64k sector, but two 32kb sections can be erased if it's really a problem.

  /*-------------------------------------------------
  List of device identifier codes as they would appear
  shifted out in MSB mode.
  -------------------------------------------------*/
  static const std::array<uint32_t, 1> SupportedDevices = {
    0x001F8501,  // AT25SF081, 8Mb
  };

  /*-------------------------------------------------
  Erase chunk sizing supported
  -------------------------------------------------*/
  static constexpr std::array<size_t, 3> EraseChunks = {
    CHUNK_SIZE_4K,
    CHUNK_SIZE_32K,
    CHUNK_SIZE_64K
  };

}  // namespace Adesto::AT25

#endif  /* !ADESTO_AT25_CONSTANTS_HPP */
