/*
**********************************
**********************************
**      BROUGHT TO YOU BY:      **
**********************************
**********************************
**                              **
**       [TEAM ASSEMBLY]        **
**                              **
**     www.team-assembly.com    **
**                              **
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
*/
#pragma once
#if defined (_WINDOWS)
	#pragma message ("Compiling for WINDOWS: " __FILE__)
//	#include <afxwin.h>         // MFC core and standard components
#elif defined (_XBOX)
	#pragma message ("Compiling for XBOX: " __FILE__)
	#include <xtl.h>
#else
	#error ERR: Have to Define _WINDOWS or _XBOX !!
#endif

// Performs S.M.A.R.T cmd --> Requires valid bFeaturesReg, bCylLowReg, and bCylHighReg
#define	ATA_SMART                 0xB0

// Cylinder register values required when issuing SMART command
#define SMART_CYL_LOW             0x4F
#define SMART_CYL_HI              0xC2

#define ATA_SMART_READ_VALUES       0xD0
#define	ATA_SMART_READ_THRESHOLDS   0xD1
#define ATA_SMART_AUTOSAVE          0xD2
#define ATA_SMART_SAVE_VALUES       0xD3
#define	ATA_SMART_IMMEDIATE_OFFLINE 0xD4
#define	ATA_SMART_ENABLE            0xD8
#define	ATA_SMART_DISABLE           0xD9
#define	ATA_SMART_STATUS            0xDA

#define SMART_ATTR_HDTEMP           0xBE // obsolete?
#define SMART_ATTR_HDATEMP          0xC2
#define SMART_ATTR_TEMP             0xE7

//ATA IDENTIFY Structure offsets..
//As per ATA Spec
#define HDD_CONFIG                  0x000 // 0
#define HDD_NUM_OF_CYLS_OFFSET      0x002 // 2
#define HDD_NUM_OF_HEADS_OFFSET     0x006 // 6
#define HDD_BYTE_PERTRACK_OFFSET    0x008 // 8
#define HDD_BYTE_PERSECTOR_OFFSET   0x00a // 10
#define HDD_SECTOR_PERTRACK_OFFSET  0x00c // 12
#define HDD_VENDOR_SPEC_1_OFFSET    0x00e // 14
#define HDD_SERIAL_OFFSET           0x014 // 20  ->ASCII
#define HDD_BUFFER_TYPE_OFFSET      0x028 // 40
#define HDD_BUFFER_SIZE_OFFSET      0x02A // 42
#define HDD_NUM_OF_ECC_BYTE_OFFSET  0x02c // 44
#define IDE_FIRMWARE_OFFSET         0x02e // 46 ->ASCII
#define HDD_MODEL_OFFSET            0x036 // 54 ->ASCII
#define HDD_MUL_SEC_PER_INT_OFFSET  0x05E // 94
#define HDD_DWIO_OFFSET             0x060 // 96
#define HDD_LBADMA_OFFSET           0x062 // 98
#define HDD_PIO_TIMING_MODE_OFFSET  0x066 // 102
#define HDD_DMA_TIMING_MODE_OFFSET  0x068 // 104
#define HDD_AP_NUM_OF_CYLS_OFFSET   0x06C // 108
#define HDD_AP_NUM_OF_HEADS_OFFSET  0x06E // 110
#define HDD_AP_SEC_PER_TRACK_OFFSET 0x070 // 112
#define HDD_CAPACITY_OFFSET         0x072 // 114
#define HDD_NUM_SEC_PER_INT_OFFSET  0x076 // 118
#define HDD_LBA_SECTORS_OFFSET      0x078 // 120
#define HDD_SIN_DMA_MODES_OFFSET    0x07C // 124
#define HDD_MUL_DMA_MODES_OFFSET    0x07E // 126
#define HDD_CMD_SUPPORT_OFFSET      0x0A4 // 164
#define HDD_CMD_ENABLED_OFFSET      0x0AA // 170
#define HDD_SECURITY_STATUS_OFFSET  0x100 // 256
#define HDD_RESERVED_5_OFFSET       0x140 // 320

#define HDD_FIRMWARE_LENGTH 0x08
#define HDD_SERIAL_LENGTH   0x14
#define HDD_MODEL_LENGTH    0x28

//IDE Port Addresses
#define IDE_PRIMARY_PORT        0x01F0
#define IDE_SECONDARY_PORT      0x0170

//Important ATA Register Values As per ATA Spec
#define IDE_DEVICE_MASTER       0xA0
#define IDE_DEVICE_SLAVE        0xB0

//Important ATA Command return register values As per ATA Spec
#define ATA_ERROR_SUCCESS       0x0000
#define ATA_ERROR_ABORT         0x0004

//Our SendATACommand needs this to figure our if we should read or write data to IDE registers..
#define IDE_COMMAND_READ          0x00
#define IDE_COMMAND_WRITE         0x01
#define IDE_COMMAND_NONDATA       0x02

//Important ATA/ATAPI Commands As per ATA Spec
#define ATA_IDENTIFY_PACKET_DEVICE    0xA1
#define ATA_IDENTIFY_DEVICE           0xEC
#define ATA_READ_SECTORS              0x20
#define ATA_READ_BUFFER               0xE4
#define ATA_WRITE_SECTORS             0x30
#define ATA_WRITE_BUFFER              0xE8
#define ATA_EXECUTE_DEVICE_DIAGNOSTIC 0x90
#define ATA_COMMAND_POWERMODE1        0xE5
#define ATA_COMMAND_STANDBY           0xE0
#define ATA_COMMAND_ACTIVE            0xE1
#define ATA_COMMAND_SET_FEATURES      0xEF

#define ATA_FEATURE_ENABLE_APM        0x05
#define ATA_FEATURE_DISABLE_APM       0x85
#define ATA_FEATURE_ENABLE_AAM        0x42
#define ATA_FEATURE_DISABLE_AAM       0xC2

#define ATA_POWERSTATE_ACTIVE         0xFF
#define ATA_POWERSTATE_STANDBY        0x00 // 0x80=idle

#define	ATA_SECURITY_SET_PASSWORD     0xF1
#define ATA_SECURITY_UNLOCK           0xF2
#define	ATA_SECURITY_FREEZE_LOCK      0xF5
#define	ATA_SECURITY_DISABLE_PASSWORD 0xF6

//Important ATA IDENTIFY Data Structure values
//As per ATA Spec
#define ATA_SECURITY_SUPPORTED     0x0001
#define ATA_SECURITY_ENABLED       0x0002
#define ATA_SECURITY_PASSWORD_SET  0x0004
#define ATA_SECURITY_FROZEN        0x0008
#define ATA_SECURITY_COUNT_EXPIRED 0x0010
#define ATA_SECURITY_LEVEL_MAX     0x0100


// Registers
#define ATA_DATA_REGISTER          0x01F0
#define ATA_ERROR_REGISTER         0x01F1
#define ATA_FEATURE_REG            IDE_ERROR_REGISTER
#define ATA_SECTOR_COUNT_REGISTER  0x01F2
#define ATA_SECTOR_NUMBER_REGISTER 0x01F3
#define ATA_CYLINDER_LOW_REGISTER  0x01F4
#define ATA_CYLINDER_HIGH_REGISTER 0x01F5
#define ATA_DRIVE_HEAD_REGISTER    0x01F6
#define ATA_STATUS_REGISTER        0x01F7
#define ATA_COMMAND_REGISTER       0x01F7

// bits of status register 
#define ATA_STATUS_DRIVE_BUSY           0x80
#define ATA_STATUS_DRIVE_READY          0x40
#define ATA_STATUS_DRIVE_WRITE_FAULT    0x20
#define ATA_STATUS_DRIVE_SEEK_COMPLETE  0x10
#define ATA_STATUS_DRIVE_DATA_REQUEST   0x08
#define ATA_STATUS_DRIVE_CORRECTED_DATA 0x04
#define ATA_STATUS_DRIVE_INDEX          0x02
#define ATA_STATUS_DRIVE_ERROR          0x01

#define ATA_STATUS_WRITE_READY (ATA_STATUS_DRIVE_READY | ATA_STATUS_DRIVE_SEEK_COMPLETE | ATA_STATUS_DRIVE_DATA_REQUEST)
#define ATA_STATUS_READ_READY  (ATA_STATUS_DRIVE_READY | ATA_STATUS_DRIVE_SEEK_COMPLETE)

#define XBOX_IDE_CHANNEL_VECTOR  0x0E
#define XBOX_HIGHEST_IRQL        0x1A
#define XBOX_IDE_CHANNEL_IRQL (XBOX_HIGHEST_IRQL - XBOX_IDE_CHANNEL_VECTOR)

#ifdef _XBOX
#define XBOX_DEVICE_HDD    0
#define XBOX_DEVICE_DVDROM 1
#else
#define XBOX_DEVICE_HDD    0
#define XBOX_DEVICE_DVDROM -1
#endif

#define KIRQL UCHAR

class XKHDD
{
private:
public:
	//IDE ATA Input Registers Structure
	typedef struct IP_IDE_REG
	{
		BYTE bFeaturesReg;      // Used for specifying SMART "commands".
		BYTE bSectorCountReg;   // IDE sector count register
		BYTE bSectorNumberReg;  // IDE sector number register
		BYTE bCylLowReg;        // IDE low order cylinder value
		BYTE bCylHighReg;       // IDE high order cylinder value
		BYTE bDriveHeadReg;     // IDE drive/head register
		BYTE bCommandReg;       // Actual IDE command.
		BYTE bReserved;         // reserved for future use.  Must be zero.
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

	// Defines and Data Structures for Windows 2000 and XP IOCTL And ATA PassThrough commands
	// **************************************************************************************************************
	#define FILE_DEVICE_CONTROLLER      0x00000004
	#define IOCTL_SCSI_BASE             FILE_DEVICE_CONTROLLER

	#define METHOD_BUFFERED             0

	#define FILE_ANY_ACCESS         0
	#define IOCTL_SCSI_RESCAN_BUS   CTL_CODE(IOCTL_SCSI_BASE, 0x0407, METHOD_BUFFERED, FILE_ANY_ACCESS)

#if defined (_WINDOWS)
	typedef struct _IDEREG
	{
		BYTE bFeaturesReg;    // +0
		BYTE bSectorCountReg; // +1
		BYTE bSectorNumberReg;// +2
		BYTE bCylLowReg;      // +3
		BYTE bCylHighReg;     // +4
		BYTE bDriveHeadReg;   // +5
		BYTE bCommandReg;     // +6
	} IDEREG;
	typedef struct ATA_PASS_THROUGH
	{
		IDEREG IdeReg;
		ULONG  DataBufferSize;
		UCHAR  DataBuffer[1];
	};
	typedef ATA_PASS_THROUGH* LPATA_PASS_THROUGH;
#else
	typedef struct _IDEREG
	{
		BYTE bFeaturesReg;    // +0
		BYTE bSectorCountReg; // +1
		BYTE bSectorNumberReg;// +2
		BYTE bCylLowReg;      // +3
		BYTE bCylHighReg;     // +4
		// note: Xbox does not use drive select register in ATA pass through.
		BYTE bDriveHeadReg;   // +5
		BYTE bCommandReg;     // +6
		BYTE bHostSendsData;  // +7
	} IDEREG;
	typedef struct ATA_PASS_THROUGH
	{
		IDEREG IdeReg;
		ULONG  DataBufferSize;// +8
		BYTE * DataBuffer;    // +0c
	};
	typedef ATA_PASS_THROUGH* LPATA_PASS_THROUGH;
#endif
	// **************************************************************************************************************


	static BOOL	SendATACommand(UCHAR DeviceNum, LPATA_COMMAND_OBJ ATACommandObj, UCHAR ReadWrite);

	static BOOL EnableSMART();
	static signed char GetHddSmartTemp();
	static signed char GetHddSmartTemp(UCHAR* IDEData);

	// automatic acoustic management
	static BOOL SetAAMLevel(BYTE bLevel);
	// automatic power management
	static BOOL SetAPMLevel(BYTE bLevel);

	static KIRQL RaiseIRQLToIDEChannelIRQL();
	static VOID SpindownHarddisk(bool bSpinDown=true);

	//Helper Functions to Parse Data from ATA IDENTIFY Command
	static void	GetIDEModel(UCHAR* IDEData, LPSTR ModelString);
	static void	GetIDESerial(UCHAR* IDEData, LPSTR SerialString);
	static void	GetIDEFirmWare(UCHAR* IDEData, LPSTR FirmwareString);
	static WORD	GetIDESecurityStatus(UCHAR* IDEData);

	static BOOL IsSmartSupported(UCHAR * IDEData);
	static BOOL IsSmartEnabled  (UCHAR * IDEData);

	static void	CleanATAData(unsigned char *dst, unsigned char *src, int len);
	static void CleanATAData(unsigned char *dst, unsigned char *src, int len, BOOL bClean);

	//Given a XBOX HDDKey and ATA Identify data structure, this function calucates
	//Password the xbox will use when trying to unlock a drive..
	static void	GenerateHDDPwd(UCHAR* HDDKey, UCHAR* IDEData, UCHAR* HDDPass);
};