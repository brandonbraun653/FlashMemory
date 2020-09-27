/********************************************************************************
 *  File Name:
 *    at25_register.hpp
 *
 *  Description:
 *    Register definitions for the AT25xxx chips
 *
 *  2020 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

#pragma once
#ifndef ADESTO_AT25_REGISTER_HPP
#define ADESTO_AT25_REGISTER_HPP

/* STL Includes */
#include <cstddef>
#include <cstdint>

namespace Adesto::AT25::Register
{
  /*-------------------------------------------------------------------------------
  Status Register Byte 1
  -------------------------------------------------------------------------------*/
  static constexpr size_t SR_RDY_BUSY_POS = 0;
  static constexpr size_t SR_RDY_BUSY_MSK = 0x01;
  static constexpr size_t SR_RDY_BUSY     = SR_RDY_BUSY_MSK << SR_RDY_BUSY_POS;
}  // namespace Adesto::AT25

#endif  /* !ADESTO_AT25_REGISTER_HPP */
