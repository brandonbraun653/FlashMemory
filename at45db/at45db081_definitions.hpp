#pragma once
#ifndef AT45DB801_DEFINITIONS
#define AT45DB801_DEFINITIONS

#include <stdlib.h>
#include <stdint.h>

namespace Adesto
{
	const uint8_t JEDEC_CODE = 0x1F;
	
	/** Upper 3 MSB of Device ID Byte 1*/
	enum FamilyCode : uint8_t
	{
		AT45Dxxx = 0x01,
		// Add more as needed
	};
	
	/** Lower 5 LSB of Device ID Byte 1*/
	enum DensityCode : uint8_t
	{
		DENSITY_8MBIT = 0x05,
		// Add more as needed 
	};
	
	/** Upper 3 MSB of Device ID Byte 2*/
	enum SubCode : uint8_t 
	{
		STANDARD_SERIES = 0x00,
		// Add more as needed
	};
	
	/** Lower 5 LSB of Device ID Byte 2*/
	enum ProductVariant : uint8_t
	{
		DEFAULT = 0x00,
		// Add more as needed
	};
	
	namespace NORFlash
	{
		/** Available Read Operations
		 *	See: Datasheet Section 15
		 **/
		enum AT45DB_ReadOp_uint8_t : uint8_t
		{
			MAIN_MEM_PAGE_READ						= 0xD2,			/* Main Memory Page Read */
			CONT_ARR_READ_LP						= 0x01,			/* Continuous Array Read (Low Power Mode) */
			CONT_ARR_READ_LF						= 0x03,			/* Continuous Array Read (Low Frequency) */
			CONT_ARR_READ_HF1						= 0x0B,			/* Continuous Array Read (High Frequency) */
			CONT_ARR_READ_HF2						= 0x1B,			/* Continuous Array Read (High Frequency) */
			CONT_ARR_READ_LEG						= 0xE8,			/* Continuous Array Read (Legacy -- Not Recommended) */
			BUFFER1_READ_LF							= 0xD1,			/* Buffer 1 Read (Low Frequency) */
			BUFFER2_READ_LF							= 0xD3,			/* Buffer 2 Read (Low Frequency) */
			BUFFER1_READ_HF							= 0xD4,			/* Buffer 1 Read (High Frequency) */
			BUFFER2_READ_HF							= 0xD6			/* Buffer 2 Read (High Frequency) */
		};

		/** AVailable Program & Erase Commands
		 *	See: Datasheet Section 15
		 **/
		enum AT45DB_PgmEraseOp_uint8_t : uint8_t
		{
			BUFFER1_WRITE							= 0x84,			/* Buffer 1 Write */
			BUFFER2_WRITE							= 0x87,			/* Buffer 2 Write */
			BUFFER1_TO_MAIN_MEM_PAGE_PGM_W_ERASE	= 0x83,			/* Buffer 1 to Main Memory Page Program with Built-In Erase */
			BUFFER2_TO_MAIN_MEM_PAGE_PGM_W_ERASE	= 0x86,			/* Buffer 2 to Main Memory Page Program with Build-In Erase */
			BUFFER1_TO_MAIN_MEM_PAGE_PGM_WO_ERASE	= 0x88,			/* Buffer 1 to Main Memory Page Program without Built-In Erase */
			BUFFER2_TO_MAIN_MEM_PAGE_PGM_WO_ERASE	= 0x89,			/* Buffer 2 to Main Memory Page Program without Built-In Erase */
			MAIN_MEM_PAGE_PGM_THR_BUFFER1_W_ERASE	= 0x82,			/* Main Memory Page Program through Buffer 1 with Built-In Erase */
			MAIN_MEM_PAGE_PGM_THR_BUFFER2_W_ERASE	= 0x85,			/* Main Memory Page Program through Buffer 2 with Built-In Erase */
			MAIN_MEM_BP_PGM_THR_BUFFER1_WO_ERASE	= 0x02,			/* Main Memory Byte/Page Program through Buffer 1 without Built-In Erase */
			PAGE_ERASE								= 0x81,			/* Page Erase */
			BLOCK_ERASE								= 0x50,			/* Block Erase */
			SECTOR_ERASE							= 0x7C,			/* Sector Erase */
			PGM_OR_ERASE_SUSPEND					= 0xB0,			/* Program/Erase Suspend */
			PGM_OR_ERASE_RESUME						= 0xD0,			/* Program/Erase Suspend */
			RMW_THR_BUFFER1							= 0x58,			/* Read-Modify-Write through Buffer 1 */
			RMW_THR_BUFFER2							= 0x59			/* Read-Modify-Write through Buffer 2 */
		};

		enum AT45DB_PgmEraseOp_uint32_t : uint32_t
		{
			//Note: These values are reversed to preserve bit order 
			CHIP_ERASE								= 0x9A8094C7,	/* Chip Erase */
		};

		/** Protection & Security Commands
		 *  See: Datasheet Section 15
		 **/
		enum AT45DB_SecurityOp_uint8_t : uint8_t
		{	
			READ_SECTOR_PROTECTION_REG				= 0x32,			/* Read Sector Protection Register */
			READ_SECTOR_LOCKDOWN_REG				= 0x35,			/* Read Sector Lockdown Register */
			READ_SECURITY_REGISTER					= 0x77			/* Read Security Register */
		};

		enum AT45DB_SecurityOp_uint32_t : uint32_t
		{
			//Note: These values are reversed to preserve bit order 
			ENABLE_SECTOR_PROTECTION				= 0xA97F2A3D,	/* Enable Sector Protection */
			DISABLE_SECTOR_PROTECTION				= 0x9A7F2A3D,	/* Disable Sector Protection */
			ERASE_SECTOR_PROTECTION_REG				= 0xCF7F2A3D,	/* Erase Sector Protection Register */
			PGM_SECTOR_PROTECTION_REG				= 0xFC7F2A3D,	/* Program Sector Protection Register */
			SECTOR_LOCKDOWN							= 0x307F2A3D,	/* Sector Lockdown */
			FREEZE_SECTOR_LOCKDOWN					= 0x40AA5534,	/* Freeze Sector Lockdown */
			PGM_SECURITY_REGISTER					= 0x0000009B,	/* Program Security Register */
		};

		/** Additional Commands
		 *  See: Datasheet Section 15
		 **/
		enum AT45DB_ExtensionOp_uint8_t : uint8_t
		{
			MAIN_MEM_PAGE_TO_BUFFER1_TRANSFER		= 0x53,			/* Main Memory Page to Buffer 1 Transfer */
			MAIN_MEM_PAGE_TO_BUFFER2_TRANSFER		= 0x55,			/* Main Memory Page to Buffer 2 Transfer */
			MAIN_MEM_PAGE_TO_BUFFER1_COMPARE		= 0x60,			/* Main Memory Page to Buffer 1 Compare */
			MAIN_MEM_PAGE_TO_BUFFER2_COMPARE		= 0x61,			/* Main Memory Page to Buffer 2 Compare */
			AUTO_PAGE_REWRITE1						= 0x58,			/* Auto Page Rewrite */
			AUTO_PAGE_REWRITE2						= 0x59,			/* Auto Page Rewrite */
			DEEP_POWER_DOWN							= 0xB9,			/* Deep Power Down */
			RESUME_FROM_DEEP_POWER_DOWN				= 0xAB,			/* Resume From Deep Power Down */
			ULTRA_DEEP_POWER_DOWN					= 0x79,			/* Ultra-Deep Power Down */
			STATUS_REGISTER_READ					= 0xD7,			/* Status Register Read */
			READ_DEVICE_INFO						= 0x9F,			/* Manufacturer and Device ID Read */
		};

		enum AT45DB_ExtensionOp_uint32_t : uint32_t
		{
			//Note: These values are reversed to preserve bit order 
			CFG_PWR_2_PAGE_SIZE		= 0xA6802A3D,	/* Configure "Power of 2" (Binary) Page Size */
			CFG_STD_FLASH_PAGE_SIZE = 0xA7802A3D,	/* Configure Standard DataFlash Page Size */
			SOFTWARE_RESET			= 0x000000F0	/* Software Reset */
		};

		/** Informs the user about the type of chip in use
		 *  See: Datasheet Section 12
		 **/
		struct AT45xx_DeviceInfo
		{
			uint8_t manufacturerID;
			FamilyCode familyCode;
			DensityCode densityCode;
			SubCode subCode;
			ProductVariant productVariant;
		};
	}
}
#endif /* AT45DB801_DEFINITIONS */
