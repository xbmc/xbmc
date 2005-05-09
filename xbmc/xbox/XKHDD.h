/*
**********************************
**********************************
**      BROUGHT TO YOU BY:		**
**********************************
**********************************
**								**
**		  [TEAM ASSEMBLY]		**
**								**
**		www.team-assembly.com	**
**								**
******************************************************************************************************
* This is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
* 
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
******************************************************************************************************


********************************************************************************************************
**	     XKHDD.H - Hard Disk Class Header      
********************************************************************************************************
**
**  This is the Class Header, see the .CPP file for more comments and implementation details.
**  Member helper function included to parse ATA IDENTIFY data and get HDD Serial, Model etc.
**
**
********************************************************************************************************

********************************************************************************************************
**	CREDITS:
********************************************************************************************************
**	SPEEDBUMP:
**  ----------
**		My utmost greatfullness and admiration goes towards SpeedBump for all his hard work..
**		I used most of his code and converted to C++ objects etc..
**
**	MRUEL:
** -------
**		Thanks for the pointing us to the Pascall Code that was used for Locking/Unlocking the disk..  
**		Great work on UnlockX.. keep it up !
**
**	MARTIN GERDES:
**  -------------
**		An editor of the german "c't" magazine. The article we referenced was published in 
**		c't Magazine 11/91 "Platten-Auslese - Konfiguration von AT-Bus-Platten lesen"
**		Thanks!, We ported this Pascall and ASM stuff to CPP to access the IDE Ports directly 
**
********************************************************************************************************


********************************************************************************************************
**	References:
********************************************************************************************************
**
**  ATA Specification:
**  ------------------
**  This stuff helped us a quite a bit..  so go check it out !
**  REFERENCE URL: ftp://fission.dt.wdc.com/pub/standards/ata/ata-3/ata3-r6.doc
**
********************************************************************************************************

UPDATE LOG:
--------------------------------------------------------------------------------------------------------
Date: 02/18/2003
By: UNDEAD [team-assembly]
Reason: Prepared 0.2 for Public Release
--------------------------------------------------------------------------------------------------------
Date: 01/06/2003
By: UNDEAD [team-assembly]
Reason: Prepared for Public Release
--------------------------------------------------------------------------------------------------------

*/
#pragma once
#pragma message ("Compiling for XBOX: " __FILE__)
#include <xtl.h>

//S.M.A.R.T IDENTIFIY OFFSETS
//CUSTOM S.M.A.R.T READ Values Offsets
#define SMART_RED_HDD_TEMP1						0x065
#define SMART_RED_HDD_TEMP2						0x067



// Performs S.M.A.R.T cmd --> Requires valid bFeaturesReg, bCylLowReg, and bCylHighReg
#define	IDE_EXECUTE_SMART_FUNCTION				0xB0	

// Cylinder register values required when issuing SMART command
#define	SMART_CYL_LOW							0x4F
#define	SMART_CYL_HI							0xC2

// Feature register defines for S.M.A.R.T "sub commands"
#define SMART_READ_ATTRIBUTE_VALUES				0xD0	// S.M.A.R.T READ DATA
#define	SMART_READ_ATTRIBUTE_THRESHOLDS			0xD1	// Obsoleted in ATA4!
#define SMART_ENABLE_DISABLE_ATTRIBUTE_AUTOSAVE	0xD2
#define SMART_SAVE_ATTRIBUTE_VALUES				0xD3
#define	SMART_EXECUTE_OFFLINE_IMMEDIATE			0xD4	// ATA4
#define	SMART_ENABLE_SMART_OPERATIONS			0xD8	// Vendor specific command
#define	SMART_DISABLE_SMART_OPERATIONS			0xD9	// Vendor specific command
#define	SMART_RETURN_SMART_STATUS				0xDA	// Vendor specific command

// bDriverError values
#define	SMART_NO_ERROR							0	// No error
#define	SMART_IDE_ERROR							1	// Error from IDE controller
#define	SMART_INVALID_FLAG						2	// Invalid command flag
#define	SMART_INVALID_COMMAND					3	// Invalid command byte
#define	SMART_INVALID_BUFFER					4	// Bad buffer (null, invalid addr..)
#define	SMART_INVALID_DRIVE						5	// Drive number not valid
#define	SMART_INVALID_IOCTL						6	// Invalid IOCTL
#define	SMART_ERROR_NO_MEM						7	// Could not lock user's buffer
#define	SMART_INVALID_REGISTER					8	// Some IDE Register not valid
#define	SMART_NOT_SUPPORTED						9	// Invalid cmd flag set
#define	SMART_NO_IDE_DEVICE						10	// Cmd issued to device not present although drive number is valid
// 11-255 reserved									


//ATA IDENTIFY Structure offsets..
//As per ATA Spec

#define HDD_CONFIG						0x000	// 0
#define HDD_NUM_OF_CYLS_OFFSET			0x002	// 2
#define HDD_RESERVED_1_OFFSET			0x004	// 4
#define HDD_NUM_OF_HEADS_OFFSET			0x006	// 6
#define HDD_BYTE_PERTRACK_OFFSET		0x008	// 8
#define HDD_BYTE_PERSECTOR_OFFSET		0x00a	// 10
#define HDD_SECTOR_PERTRACK_OFFSET		0x00c	// 12
#define HDD_VENDOR_SPEC_1_OFFSET		0x00e	// 14
#define HDD_SERIAL_OFFSET				0x014	// 20  ->ASCII
#define HDD_BUFFER_TYPE_OFFSET			0x028	// 40
#define HDD_BUFFER_SIZE_OFFSET			0x02A	// 42
#define HDD_NUM_OF_ECC_BYTE_OFFSET		0x02c	// 44
#define IDE_FIRMWARE_OFFSET				0x02e	// 46 ->ASCII
#define HDD_MODEL_OFFSET				0x036	// 54 ->ASCII
#define HDD_MUL_SEC_PER_INT_OFFSET		0x05E	// 94
#define HDD_DWIO_OFFSET					0x060	// 96
#define HDD_LBADMA_OFFSET				0x062	// 98
#define HDD_RESERVED_2_OFFSET			0x064	// 100
#define HDD_PIO_TIMING_MODE_OFFSET		0x066	// 102
#define HDD_DMA_TIMING_MODE_OFFSET		0x068	// 104
#define HDD_RESERVED_3_OFFSET			0x06A	// 106
#define HDD_AP_NUM_OF_CYLS_OFFSET		0x06C	// 108
#define HDD_AP_NUM_OF_HEADS_OFFSET		0x06E	// 110
#define HDD_AP_SEC_PER_TRACK_OFFSET		0x070	// 112
#define HDD_CAPACITY_OFFSET				0x072	// 114
#define HDD_NUM_SEC_PER_INT_OFFSET		0x076	// 118
#define HDD_LBA_SECTORS_OFFSET			0x078	// 120
#define HDD_SIN_DMA_MODES_OFFSET		0x07C	// 124
#define HDD_MUL_DMA_MODES_OFFSET		0x07E	// 126
#define HDD_RESERVED_4_OFFSET			0x080	// 128
#define HDD_SECURITY_STATUS_OFFSET		0x100	// 256
#define HDD_RESERVED_5_OFFSET			0x140	// 320


//IDE Port Addresses
#define IDE_PRIMARY_PORT				0x01F0
#define IDE_SECONDARY_PORT				0x0170

//Important ATA Register Values As per ATA Spec
#define IDE_DEVICE_MASTER				0x00A0
#define IDE_DEVICE_SLAVE				0x00B0

//Important ATA Command return register values As per ATA Spec
#define IDE_ERROR_SUCCESS				0x0000
#define IDE_ERROR_ABORT					0x0004

//Our SendATACommand needs this to figure our if we should read or write data to IDE registers..
#define	IDE_COMMAND_READ				0x00
#define	IDE_COMMAND_WRITE				0x01

//Important ATA/ATAPI Commands As per ATA Spec
#define IDE_ATAPI_IDENTIFY				0xA1  
#define IDE_ATA_IDENTIFY				0xEC
#define IDE_ATA_COMMAND_SEEK			0x70
#define IDE_ATA_COMMAND_READ_SECTORS	0x21
#define IDE_ATA_COMMAND_READ_BUFFER		0xE4
#define IDE_ATA_COMMAND_WRITE_SECTORS	0x30
#define IDE_ATA_COMMAND_WRITE_BUFFER	0xE8
#define IDE_ATA_COMMAND_DIAGNOSTIC		0x90

#define	IDE_ATA_SECURITY_SETPASSWORD	0xF1
#define IDE_ATA_SECURITY_UNLOCK			0xF2
#define	IDE_ATA_SECURITY_FREEZE			0xF5
#define	IDE_ATA_SECURITY_DISABLE		0xF6 

//Important ATA IDENTIFY Data Structure values
//As per ATA Spec
#define IDE_SECURITY_SUPPORTED			0x0001
#define IDE_SECURITY_ENABLED			0x0002
#define IDE_SECURITY_PASSWORD_SET		0x0004
#define IDE_SECURITY_FROZEN				0x0008
#define IDE_SECURITY_COUNT_EXPIRED		0x0010
#define IDE_SECURITY_LEVEL_MAX			0x0100




// Registers
#define IDE_DATA_REGISTER				0x01F0
#define IDE_ERROR_REGISTER				0x01F1
#define IDE_FEATURE_REG					IDE_ERROR_REGISTER
#define IDE_SECTOR_COUNT_REGISTER		0x01F2
#define IDE_SECTOR_NUMBER_REGISTER		0x01F3
#define IDE_CYLINDER_LOW_REGISTER		0x01F4
#define IDE_CYLINDER_HIGH_REGISTER		0x01F5
#define IDE_DRIVE_HEAD_REGISTER			0x01F6
#define IDE_STATUS_REGISTER				0x01F7
#define IDE_COMMAND_REGISTER			0x01F7

//Drives
#define IDE_DRIVE_0						0xa0
#define IDE_DRIVE_1						0xb0

// Commands
#define IDE_COMMAND_IDENTIFY_DRIVE		0xEC
#define IDE_COMMAND_SEEK				0x70
#define IDE_COMMAND_READ_SECTORS		0x21
#define IDE_COMMAND_READ_BUFFER			0xE4
#define IDE_COMMAND_WRITE_SECTORS		0x30
#define IDE_COMMAND_WRITE_BUFFER		0xE8
#define IDE_COMMAND_DIAGNOSTIC			0x90
#define IDE_COMMAND_ATAPI_IDENT_DRIVE	0xA1

// Results words from Identify Drive Request
#define	IDE_INDENTIFY_NUM_CYLINDERS		0x01 
#define	IDE_INDENTIFY_NUM_HEADS			0x03 
#define	IDE_INDENTIFY_NUM_BYTES_TRACK	0x04 
#define	IDE_INDENTIFY_NUM_BYTES_SECTOR	0x05 
#define	IDE_INDENTIFY_NUM_SECTORS_TRACK	0x06 


// bits of Status Register 
#define IDE_STATUS_DRIVE_BUSY			0x80
#define IDE_STATUS_DRIVE_READY			0x40
#define IDE_STATUS_DRIVE_WRITE_FAULT	0x20
#define IDE_STATUS_DRIVE_SEEK_COMPLETE	0x10
#define IDE_STATUS_DRIVE_DATA_REQUEST	0x08
#define IDE_STATUS_DRIVE_CORRECTED_DATA	0x04
#define IDE_STATUS_DRIVE_INDEX			0x02
#define IDE_STATUS_DRIVE_ERROR			0x01

// Return codes from various IDE_* functions
#define	IDE_ERROR_NO_ERROR				0
#define	IDE_ERROR_BAD_DRIVE				-1
#define	IDE_ERROR_INVALID_BLOCK			-2
#define	IDE_ERROR_DRIVE_ERROR			-3

#define LOW_BYTE(x)	(x & 0xff)
#define HIGH_BYTE(x)	((x >> 8) & 0xff)




class XKHDD
{
private:

public:


	//IDE ATA Input Registers Structure
	typedef struct IP_IDE_REG
	{
		BYTE bFeaturesReg;			// Used for specifying SMART "commands".
		BYTE bSectorCountReg;		// IDE sector count register
		BYTE bSectorNumberReg;		// IDE sector number register
		BYTE bCylLowReg;			// IDE low order cylinder value
		BYTE bCylHighReg;			// IDE high order cylinder value
		BYTE bDriveHeadReg;			// IDE drive/head register
		BYTE bCommandReg;			// Actual IDE command.
		BYTE bReserved;				// reserved for future use.  Must be zero.
	};
	typedef IP_IDE_REG* LPIP_IDE_REG;

	//IDE ATA Output Registers Structure
	typedef struct OP_IDE_REG
	{
		BYTE bErrorReg;
		BYTE bSectorCountReg;
		BYTE bSectorNumberReg;
		BYTE bCylLowReg;
		BYTE bCylHighReg;
		BYTE bDriveHeadReg;
		BYTE bStatusReg;
	};
	typedef OP_IDE_REG* LPOP_IDE_REG;

	//Our own object for issuing commands..
	//Includes in/ou register objects and 1 Sector of HDD Data
	typedef struct ATA_COMMAND_OBJ
	{
		IP_IDE_REG	IPReg;
		OP_IDE_REG	OPReg;
		BYTE		DATA_BUFFER[512];
		ULONG		DATA_BUFFSIZE;
	};
	typedef ATA_COMMAND_OBJ* LPATA_COMMAND_OBJ;
	//Default Constructor/Destructor..
	XKHDD();
	virtual ~XKHDD(void);


//This is the Function for Sending ATA Commands to the HDD..
//Right now we use Standard IOCTL stuff for Windows and direct 
//port access for XBOX..  later version will include Windows Driver 
//for direct port access in Windows so we can lock/unlock.  
	static BOOL	SendATACommand(WORD IDEPort, LPATA_COMMAND_OBJ ATACommandObj, UCHAR ReadWrite);

	static BOOL	SendATACommandOP(WORD IDEPort, LPOP_IDE_REG OP_IDE_REG, UCHAR ReadWrite);
	static BOOL	SendATACommandIP(WORD IDEPort, LPIP_IDE_REG IP_IDE_REG, UCHAR ReadWrite);

	//Helper Functions to Parse Data from ATA IDENTIFY Command
	static void	GetIDEModel(UCHAR* IDEData, LPSTR ModelString, LPDWORD StrLen);
	static void	GetIDESerial(UCHAR* IDEData, LPSTR SerialString, LPDWORD StrLen);
	static void	GetIDEFirmWare(UCHAR* IDEData, LPSTR SerialString, LPDWORD StrLen);
	static WORD	GetIDESecurityStatus(UCHAR* IDEData);
	
	static WORD	GetIDENumOfCyls(UCHAR* IDEData);
	static WORD	GetIDENumOfHeads(UCHAR* IDEData);
	static WORD	GetIDESecPerTrack(UCHAR* IDEData);
	static WORD GetATABefehle(UCHAR* IDEData, BYTE IDEOffset);

	static int	CleanATAData(unsigned char *dst, unsigned char *src, int len);

	//Given a XBOX HDDKey and ATA Identify data structure, this function calucates
	//Password the xbox will use when trying to unlock a drive..
	static void	GenerateHDDPwd(UCHAR* HDDKey, UCHAR* IDEData, UCHAR* HDDPass);


	static BYTE	GetHddSmartTemp(UCHAR* IDEData);


};
