/********************************************************************************
 * File Name:
 *	  test_at45db081_read.cpp
 *
 * Description:
 *	  Implements tests for the AT45DB081 driver
 *
 * 2019 | Brandon Braun | brandonbraun653@gmail.com
 ********************************************************************************/

/* Driver Includes */
#include "at45db081.hpp"

/* Testing Framework Includes */
#include <gtest/gtest.h>
#include <Chimera/spi.hpp>

#if defined( GMOCK_TEST )
/* Mock Includes */
#include <Chimera/mock/spi.hpp>
#include <gmock/gmock.h>

using ::testing::_;
using ::testing::AtLeast;
using ::testing::DoAll;
using ::testing::Exactly;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::SetArgPointee;
using ::testing::SetArrayArgument;

//For GMOCK tests, only test the interface for strange inputs. Use the HW_TEST to actually
//test functionality.

//Also, perhaps it might be wise to define a common flash interface for this to inherit from? We want 
//mock-able behavior applicable to lots of tests. Leave the nitty gritty details to the user, but there
//should at least be some fairly simple common behaviors that can be defined.

#endif /* GMOCK_TEST */

#if defined( HW_TEST )
#include "bus_pirate.hpp"

//Test real behavior here. Read and write are going to need to be developed concurrently.

#endif /* HW_TEST */
