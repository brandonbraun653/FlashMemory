/********************************************************************************
 *  File Name:
 *    test_fixtures_at45db081.hpp
 *
 *  Description:
 *    Provides several test fixtures used in testing the AT45DB081
 *
 *  2019 | Brandon Braun | brandonbraun653@gmail.com
 ********************************************************************************/

/* Driver Includes */
#include "at45db081.hpp"

/* Chimera Includes */
#include <Chimera/mock/spi.hpp>
#include <Chimera/modules/memory/flash.hpp>

/* Test Driver Includes */
#include <gtest/gtest.h>


#if defined( GMOCK_TEST )
#include <gmock/gmock.h>

using ::testing::NiceMock;

using ErrCode = Chimera::Modules::Memory::Status;

class VirtualFlash : public ::testing::Test
{
protected:
  NiceMock<Chimera::Mock::SPIMock> spi;
  Adesto::NORFlash::AT45 *flash;

  VirtualFlash() = default;
  virtual ~VirtualFlash() = default;

  virtual void SetUp();

  virtual void TearDown();

  void testReset();

  void passInit();
};
#endif