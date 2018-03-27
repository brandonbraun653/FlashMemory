#pragma once
#ifndef AT45DB081_HPP
#define AT45DB081_HPP

/* Standard C++ Includes */
#include <stdlib.h>

/* Boost Includes */
#include <boost/function.hpp>

/* Supporting Includes */
#include "at45db081_definitions.hpp"

namespace AT45DBFlash
{
	//I'm going to need to define some kind of binding for the really low level read/write functionality.
	//Do I want to give it a pointer reference that guarantees an interface or should I manually bind the 
	//functions? Manually binding would give me a bit more flexibility I think...but then how do those functions
	//know which object to use? Hmm...This begs the question of if I need some sort of highly generalized SPI interface
	//that is completely and utterly processor independent. 
	typedef boost::function<int, uint8_t*, uint8_t*, size_t> SPIReadWriteFunc;

	class AT45DB
	{
	public:


		AT45DB() = default;
		~AT45DB() = default;

	private:

		int pgmEraseOp(PgmEraseOp op); //Probably need to split this up
		int readOp(ReadOp op);
		int securityOp(SecurityOp op);
		int extOp(ExtensionOp op);
	};

}
#endif /* AT45DB081_HPP */
