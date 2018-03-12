#pragma once
#ifndef AT45DB801_DEFINITIONS
#define AT45DB801_DEFINITIONS

#include <stdlib.h>
#include <stdint.h>

/** Available Read Operations 
* See: Datasheet Section 15
*/
enum AT45DB_ReadOp
{
	MAIN_MEM_PAGE_READ	= 0xD2,		/* Main Memory Page Read */
	CONT_ARR_READ_LP	= 0x01,		/* Continuous Array Read (Low Power Mode) */
	CONT_ARR_READ_LF	= 0x03,		/* Continuous Array Read (Low Frequency) */
	CONT_ARR_READ_HF1	= 0x0B,		/* Continuous Array Read (High Frequency) */
	CONT_ARR_READ_HF2	= 0x1B,		/* Continuous Array Read (High Frequency) */
	CONT_ARR_READ_LEG	= 0xE8,		/* Continuous Array Read (Legacy -- Not Recommended) */
	BUFF1_READ_LF		= 0xD1,		/* Buffer 1 Read (Low Frequency) */
	BUFF2_READ_LF		= 0xD3,		/* Buffer 2 Read (Low Frequency) */
	BUFF1_READ_HF		= 0xD4,		/* Buffer 1 Read (High Frequency) */
	BUFF2_READ_HF		= 0xD6		/* Buffer 2 Read (High Frequency) */
};



/** Informs the user about the type of chip in use
* See: Datasheet Section 12
*/
struct AT45DB_DeviceID
{
	uint8_t manufacturerID = 0;
	uint16_t deviceID = 0;			
	uint8_t EDI = 0;
	uint8_t familyCode = 0;
	uint8_t densityCode = 0;
	uint8_t subCode = 0;
	uint8_t productVariant = 0;
};

#endif /* AT45DB801_DEFINITIONS */
