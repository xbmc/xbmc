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
#if defined (_WINDOWS)
	#pragma message ("Compiling for WINDOWS: " __FILE__)
	#include <afxwin.h>         // MFC core and standard components
#elif defined (_XBOX)
	#pragma message ("Compiling for XBOX: " __FILE__)
	#include <xtl.h>
#else
	#error ERR: Have to Define _WINDOWS or _XBOX !!
#endif

//Important ATA IDENTIFY Structure offsets..
//As per ATA Spec
#define HDD_SERIAL_OFFSET				0x014
#define HDD_MODEL_OFFSET				0x036
#define HDD_SECURITY_STATUS_OFFSET		0x100

//IDE Port Addresses
#define IDE_PRIMARY_PORT				0x01F0
#define IDE_SECONDARY_PORT				0x0170

//Important ATA Register Values
//As per ATA Spec
#define IDE_DEVICE_MASTER				0x00A0
#define IDE_DEVICE_SLAVE				0x00B0

//Important ATA/ATAPI Commands
//As per ATA Spec
#define IDE_ATAPI_IDENTIFY				0xA1  
#define IDE_ATA_IDENTIFY				0xEC  

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

//Important ATA Command return register values
//As per ATA Spec
#define IDE_ERROR_SUCCESS				0x0000
#define IDE_ERROR_ABORT					0x0004


//Our SendATACommand needs this to figure our if we should 
//read or write data to IDE registers..
#define	IDE_COMMAND_READ				0x00
#define	IDE_COMMAND_WRITE				0x01



class XKHDD
{
private:

public:


	//IDE ATA Input Registers Structure
	typedef struct IP_IDE_REG
	{
		BYTE bFeaturesReg;
		BYTE bSectorCountReg;
		BYTE bSectorNumberReg;
		BYTE bCylLowReg;
		BYTE bCylHighReg;
		BYTE bDriveHeadReg;
		BYTE bCommandReg;
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



#if defined (_WINDOWS)
	// Defines and Data Structures for Windows 2000 and XP IOCTL And ATA PassThrough commands
	// **************************************************************************************************************
	#define FILE_DEVICE_CONTROLLER      0x00000004
	#define IOCTL_SCSI_BASE             FILE_DEVICE_CONTROLLER
	#define FILE_READ_ACCESS            0x0001
	#define FILE_WRITE_ACCESS           0x0002
	#define METHOD_BUFFERED             0

	#define CTL_CODE(DeviceType, Function, Method, Access) (((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method))

	#define IOCTL_IDE_PASS_THROUGH  CTL_CODE(IOCTL_SCSI_BASE, 0x040A, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
	#define FILE_ANY_ACCESS         0
	#define IOCTL_SCSI_RESCAN_BUS   CTL_CODE(IOCTL_SCSI_BASE, 0x0407, METHOD_BUFFERED, FILE_ANY_ACCESS)
	
	typedef struct ATA_PASS_THROUGH
	{
		BYTE		IdeReg[7];
		ULONG		DataBufferSize;
		UCHAR		DataBuffer[1];
	};
	typedef ATA_PASS_THROUGH* LPATA_PASS_THROUGH;
	// **************************************************************************************************************
#endif

	
	
	//Default Constructor/Destructor..
	XKHDD();
	virtual ~XKHDD(void);


//This is the Function for Sending ATA Commands to the HDD..
//Right now we use Standard IOCTL stuff for Windows and direct 
//port access for XBOX..  later version will include Windows Driver 
//for direct port access in Windows so we can lock/unlock.  
#if defined (_WINDOWS)
	static BOOL	SendATACommand(UCHAR DeviceNum, LPATA_COMMAND_OBJ ATACommandObj, UCHAR ReadWrite);
#elif defined (_XBOX)
	static BOOL	SendATACommand(WORD IDEPort, LPATA_COMMAND_OBJ ATACommandObj, UCHAR ReadWrite);
#endif


	//Helper Functions to Parse Data from ATA IDENTIFY Command
	static void	GetIDEModel(UCHAR* IDEData, LPSTR ModelString, LPDWORD StrLen);
	static void	GetIDESerial(UCHAR* IDEData, LPSTR SerialString, LPDWORD StrLen);
	static WORD	GetIDESecurityStatus(UCHAR* IDEData);
	static int	CleanATAData(unsigned char *dst, unsigned char *src, int len);

	//Given a XBOX HDDKey and ATA Identify data structure, this function calucates
	//Password the xbox will use when trying to unlock a drive..
	static void	GenerateHDDPwd(UCHAR* HDDKey, UCHAR* IDEData, UCHAR* HDDPass);


};
