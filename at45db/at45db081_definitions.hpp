#pragma once
#ifndef AT45DB801_DEFINITIONS
#define AT45DB801_DEFINITIONS

#include <stdlib.h>
#include <stdint.h>

namespace AT45DBFlash
{
	const size_t numSectors = 16;
	const size_t numBlocks = 512;
	const size_t numPages = 4096;
	const size_t numBytesPerPage = 256;

	/** Available Read Operations
	* See: Datasheet Section 15
	*/
	enum ReadOp
	{
		MAIN_MEM_PAGE_READ	= 0xD2,		/* Main Memory Page Read */
		CONT_ARR_READ_LP	= 0x01,		/* Continuous Array Read (Low Power Mode) */
		CONT_ARR_READ_LF	= 0x03,		/* Continuous Array Read (Low Frequency) */
		CONT_ARR_READ_HF1	= 0x0B,		/* Continuous Array Read (High Frequency) */
		CONT_ARR_READ_HF2	= 0x1B,		/* Continuous Array Read (High Frequency) */
		CONT_ARR_READ_LEG	= 0xE8,		/* Continuous Array Read (Legacy -- Not Recommended) */
		BUFFER1_READ_LF		= 0xD1,		/* Buffer 1 Read (Low Frequency) */
		BUFFER2_READ_LF		= 0xD3,		/* Buffer 2 Read (Low Frequency) */
		BUFFER1_READ_HF		= 0xD4,		/* Buffer 1 Read (High Frequency) */
		BUFFER2_READ_HF		= 0xD6		/* Buffer 2 Read (High Frequency) */
	};

	/** AVailable Program & Erase Commands
	* See: Datasheet Section 15
	*/
	enum PgmEraseOp
	{
		BUFFER1_WRITE = 0x84,									/* Buffer 1 WRite */
		BUFFER2_WRITE = 0x87,									/* Buffer 2 WRite */
		BUFFER1_TO_MAIN_MEM_PAGE_PGM_W_ERASE = 0x83,			/* Buffer 1 to Main Memory Page Program with Built-In Erase */
		BUFFER2_TO_MAIN_MEM_PAGE_PGM_W_ERASE = 0x86,			/* Buffer 2 to Main Memory Page Program with Build-In Erase */
		BUFFER1_TO_MAIN_MEM_PAGE_PGM_WO_ERASE = 0x88,			/* Buffer 1 to Main Memory Page Program without Built-In Erase */
		BUFFER2_TO_MAIN_MEM_PAGE_PGM_WO_ERASE = 0x89,			/* Buffer 2 to Main Memory Page Program without Built-In Erase */
		MAIN_MEM_PAGE_PGM_THR_BUFFER1_W_ERASE = 0x82,			/* Main Memory Page Program through Buffer 1 with Built-In Erase */
		MAIN_MEM_PAGE_PGM_THR_BUFFER2_W_ERASE = 0x85,			/* Main Memory Page Program through Buffer 2 with Built-In Erase */
		MAIN_MEM_BP_PGM_THR_BUFFER1_WO_ERASE = 0x02,			/* Main Memory Byte/Page Program through Buffer 1 without Built-In Erase */
		PAGE_ERASE = 0x81,										/* Page Erase */
		BLOCK_ERASE = 0x50,										/* Block Erase */
		SECTOR_ERASE = 0x7C,									/* Sector Erase */
		CHIP_ERASE = (0xC7 | 0x94 | 0x80 | 0x9A),				/* Chip Erase */
		PGM_OR_ERASE_SUSPEND = 0xB0,							/* Program/Erase Suspend */
		PGM_OR_ERASE_RESUME = 0xD0,								/* Program/Erase Suspend */
		RMW_THR_BUFFER1 = 0x58,									/* Read-Modify-Write through Buffer 1 */
		RMW_THR_BUFFER2 = 0x59									/* Read-Modify-Write through Buffer 2 */
	};

	/** Protection & Security Commands
	* See: Datasheet Section 15
	*/
	enum SecurityOp
	{
		ENABLE_SECTOR_PROTECTION = (0x3D | 0x2A | 0x7F | 0xA9),			/* Enable Sector Protection */
		DISABLE_SECTOR_PROTECTION = (0x3D | 0x2A | 0x7F | 0x9A),		/* Disable Sector Protection */
		ERASE_SECTOR_PROTECTION_REG = (0x3D | 0x2A | 0x7F | 0xCF),		/* Erase Sector Protection Register */
		PGM_SECTOR_PROTECTION_REG = (0x3D | 0x2A | 0x7F | 0xFC),		/* Program Sector Protection Register */
		READ_SECTOR_PROTECTION_REG = 0x32,								/* Read Sector Protection Register */
		SECTOR_LOCKDOWN = (0x3D | 0x2A | 0x7F | 0x30),					/* Sector Lockdown */
		READ_SECTOR_LOCKDOWN_REG = 0x35,								/* Read Sector Lockdown Register */
		FREEZE_SECTOR_LOCKDOWN = (0x34 | 0x55 | 0xAA | 0x40),			/* Freeze Sector Lockdown */
		PGM_SECURITY_REGISTER = (0x9B | 0x00 | 0x00 | 0x00),			/* Program Security Register */
		READ_SECURITY_REGISTER = 0x77									/* Read Security Register */
	};

	/** Additional Commands
	* See: Datasheet Section 15
	*/
	enum ExtensionOp
	{
		MAIN_MEM_PAGE_TO_BUFFER1_TRANSFER = 0x53,					/* Main Memory Page to Buffer 1 Transfer */
		MAIN_MEM_PAGE_TO_BUFFER2_TRANSFER = 0x55,					/* Main Memory Page to Buffer 2 Transfer */
		MAIN_MEM_PAGE_TO_BUFFER1_COMPARE = 0x60,					/* Main Memory Page to Buffer 1 Compare */
		MAIN_MEM_PAGE_TO_BUFFER2_COMPARE = 0x61,					/* Main Memory Page to Buffer 2 Compare */
		AUTO_PAGE_REWRITE1 = 0x58,									/* Auto Page Rewrite */
		AUTO_PAGE_REWRITE2 = 0X59,									/* Auto Page Rewrite */
		DEEP_POWER_DOWN = 0xB9,										/* Deep Power Down */
		RESUME_FROM_DEEP_POWER_DOWN = 0xAB,							/* Resume From Deep Power Down */
		ULTRA_DEEP_POWER_DOWN = 0x79,								/* Ultra-Deep Power Down */
		STATUS_REGISTER_READ = 0xD7,								/* Status Register Read */
		READ_DEVICE_INFO = 0x9F,									/* Manufacturer and Device ID Read */
		CFG_PWR_2_PAGE_SIZE = (0x3D | 0x2A | 0x80 | 0xA6),			/* Configure "Power of 2" (Binary) Page Size */
		CFG_STD_FLASH_PAGE_SIZE = (0x3D | 0x2A | 0x80 | 0xA7),		/* Configure Standard DataFlash Page Size */
		SOFTWARE_RESET = (0xF0 | 0x00 | 0x00 | 0x00)				/* Software Reset */
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
}
#endif /* AT45DB801_DEFINITIONS */
