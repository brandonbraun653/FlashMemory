/********************************************************************************
 *  File Name:
 *    common.cpp
 *
 *  Description:
 *    Common functions for Adesto memory drivers
 *
 *  2020 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

/* STL Includes */
#include <cstdint>
#include <cstddef>

/* Adesto Includes */
#include <Adesto/common.hpp>

#define MEGA ( 1000000 )
#define BITS_IN_BYTE ( 8 )

namespace Adesto
{
  size_t densityToBytes( const DensityCode density )
  {
    switch( density )
    {
      case DensityCode::DENSITY_2MBIT:
        return ( 2 * MEGA ) / BITS_IN_BYTE;
        break;

      case DensityCode::DENSITY_4MBIT:
        return ( 4 * MEGA ) / BITS_IN_BYTE;
        break;

      case DensityCode::DENSITY_8MBIT:
        return ( 8 * MEGA ) / BITS_IN_BYTE;
        break;

      case DensityCode::DENSITY_16MBIT:
        return ( 16 * MEGA ) / BITS_IN_BYTE;
        break;

      case DensityCode::DENSITY_32MBIT:
        return ( 32 * MEGA ) / BITS_IN_BYTE;
        break;

      case DensityCode::DENSITY_64MBIT:
        return ( 64 * MEGA ) / BITS_IN_BYTE;
        break;

      default:
        return 0;
        break;
    };
  }
}  // namespace Adesto
