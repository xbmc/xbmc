/*********************************
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
******************************************************************************************************
**	     XKHDD.CPP - Hard Disk Class Implementation      
******************************************************************************************************
**  This Class contains functionality to access a IDE HDD and query it by sending ATA Commands.
**  Member helper function included to parse ATA IDENTIFY data and get HDD Serial, Model etc.
******************************************************************************************************
**	CREDITS:
******************************************************************************************************
**	SPEEDBUMP:
**	----------
**		My utmost greatfullness and admiration goes towards SpeedBump for all his hard work..
**		I used most of his code and converted to C++ objects etc..
**
**	MRUEL:
**	------
**		Thanks for the pointing us to the Pascall Code that was used for Locking/Unlocking the disk..  
**		Great work on UnlockX.. keep it up !
**
**	MARTIN GERDES:
**	--------------
**		An editor of the german "c't" magazine. The article we referenced was published in 
**		c't Magazine 11/91 "Platten-Auslese - Konfiguration von AT-Bus-Platten lesen"
**		Thanks!, We ported this Pascall and ASM stuff to CPP to access the IDE Ports directly 
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
--------------------------------------------------------------------------------------------------------*/
#include "../stdafx.h"
#include "XKHDD.h"
#include "XKRC4.h"
#include "XKSHA1.h"
#include <conio.h>
#include "undocumented.h"


XKHDD::XKHDD()
{}
XKHDD::~XKHDD(void)
{}

// Extra wrapper function make 
#ifdef _XBOX
BYTE XKHDD::GetHddSmartTemp()
{
  ATA_COMMAND_OBJ hddcommand;
  ZeroMemory(&hddcommand, sizeof(XKHDD::ATA_COMMAND_OBJ));
  
  hddcommand.DATA_BUFFSIZE = 0;
  hddcommand.IPReg.bFeaturesReg     = 0xd0;    // SEND READ SMART VALUES
  hddcommand.IPReg.bSectorCountReg  = 1;
  hddcommand.IPReg.bSectorNumberReg = 1;
  hddcommand.IPReg.bCylLowReg       = 0x4f;    // SET SMART CYL LOW
  hddcommand.IPReg.bCylHighReg      = 0xc2;    // SET SMART CYL HI
  hddcommand.IPReg.bDriveHeadReg    = 0xa0;    // SET Device Where HDD is
  hddcommand.IPReg.bCommandReg      = 0xb0;    // SET ON IDE SEND SMART MODE;

  SendATACommand(IDE_PRIMARY_PORT, &hddcommand, 0x00);
  return GetHddSmartTemp((UCHAR *)&hddcommand.DATA_BUFFER);
}
#endif
BYTE XKHDD::GetHddSmartTemp(UCHAR* IDEData)
{	
	// S.M.A.R.T Get HDD Temp ID: 194 OFFSET: 0x065 & 0x067
	BYTE retVal = (BYTE) *(IDEData+SMART_RED_HDD_TEMP2);
	return retVal;
}

WORD XKHDD::GetIDENumOfCyls(UCHAR* IDEData)
{
	WORD retVal = (WORD) *(IDEData+HDD_NUM_OF_CYLS_OFFSET);
	return retVal;
}

WORD XKHDD::GetIDENumOfHeads(UCHAR* IDEData)
{
	WORD retVal = (WORD) *(IDEData+HDD_NUM_OF_HEADS_OFFSET);
	return retVal;
}

WORD XKHDD::GetIDESecPerTrack(UCHAR* IDEData)
{
	WORD retVal = (WORD) *(IDEData+HDD_SECTOR_PERTRACK_OFFSET);
	return retVal;
}

void XKHDD::GetIDEFirmWare(UCHAR* IDEData, LPSTR FirmwareString)
{
	CleanATAData((UCHAR*)FirmwareString, IDEData+IDE_FIRMWARE_OFFSET, HDD_FIRMWARE_LENGTH);
}

void XKHDD::GetIDESerial(UCHAR* IDEData, LPSTR SerialString)
{
	CleanATAData((UCHAR*)SerialString, IDEData+HDD_SERIAL_OFFSET, HDD_SERIAL_LENGTH);
}

void XKHDD::GetIDEModel(UCHAR* IDEData, LPSTR ModelString)
{
	CleanATAData((UCHAR*)ModelString, IDEData+HDD_MODEL_OFFSET, HDD_MODEL_LENGTH);
}

WORD XKHDD::GetIDESecurityStatus(UCHAR* IDEData)
{
	WORD retVal = (WORD) *(IDEData+HDD_SECURITY_STATUS_OFFSET);
	return retVal;
}

void XKHDD::CleanATAData(unsigned char *dst, unsigned char *src, int len)
{
	CleanATAData(dst, src, len, TRUE);
}

void XKHDD::CleanATAData(unsigned char *dst, unsigned char *src, int len, BOOL bClean)
{
	//Byte swap the Data that comes back from ATA command..  and clean out blanks etc.
	unsigned char tmp;
	int i;

	for(i=0; i < len; i+=2) 
	{
		tmp = src[i];		
		dst[i] = src[i+1];
		dst[i+1] = tmp;
	}
	
	for(i=len-1; i>0; --i) 
	{
		if (dst[i] != ' ') break;
	}

	if (bClean)
		dst[i + 1] = 0;
}

void XKHDD::GenerateHDDPwd(UCHAR* HDDKey, UCHAR* IDEData, UCHAR* HDDPass)
{
	XKSHA1 SHA1Obj;

	UCHAR serial[HDD_SERIAL_LENGTH];
	ZeroMemory(serial, HDD_SERIAL_LENGTH);

	UCHAR model[HDD_MODEL_LENGTH];
	ZeroMemory(model, HDD_MODEL_LENGTH); 

	CleanATAData(serial, IDEData+HDD_SERIAL_OFFSET, HDD_SERIAL_LENGTH, FALSE);
	CleanATAData(model, IDEData+HDD_MODEL_OFFSET, HDD_MODEL_LENGTH, FALSE);

	SHA1Obj.HMAC_SHA1 (HDDPass, HDDKey, 0x10, model, HDD_MODEL_LENGTH, serial, HDD_SERIAL_LENGTH);
}

#if defined (_WINDOWS)
//Windows Version of sending ATA Commands..  This is normal IOCTL stuff...
BOOL XKHDD::SendATACommand(UCHAR DeviceNum, LPATA_COMMAND_OBJ ATACommandObj, UCHAR ReadWrite)
{
	BOOL retVal = FALSE;

	char tmp[128];
    sprintf(tmp, "\\\\.\\PhysicalDrive%u", DeviceNum);
	//Open HDD and get handle to Open Device...
    HANDLE Device = CreateFile(tmp, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

	if (Device == INVALID_HANDLE_VALUE)
		return FALSE;
  
	//Set up ATA Pass through structures..
	unsigned int Size = sizeof(ATA_PASS_THROUGH) + ATACommandObj->DATA_BUFFSIZE;
	LPATA_PASS_THROUGH pAPT = (LPATA_PASS_THROUGH) VirtualAlloc(NULL, Size, MEM_COMMIT, PAGE_READWRITE);
	ZeroMemory (pAPT, Size);
		
	if (pAPT)
	{
		memset(pAPT, 0, Size);
		pAPT->DataBufferSize = ATACommandObj->DATA_BUFFSIZE;
		pAPT->IdeReg[0] = ATACommandObj->IPReg.bFeaturesReg;
		pAPT->IdeReg[1] = ATACommandObj->IPReg.bSectorCountReg;
		pAPT->IdeReg[2] = ATACommandObj->IPReg.bSectorNumberReg;
		pAPT->IdeReg[3] = ATACommandObj->IPReg.bCylLowReg;
		pAPT->IdeReg[4] = ATACommandObj->IPReg.bCylHighReg;
		pAPT->IdeReg[5] = ATACommandObj->IPReg.bDriveHeadReg;
		pAPT->IdeReg[6] = ATACommandObj->IPReg.bCommandReg;

		if (ATACommandObj->IPReg.bCommandReg == 0xEC)  
		{
			// For some Reason Win2K/XP sometimes needs to rescan bus before we get IDENTIFY info..
			DWORD BytesReturned = 0;
			(void)DeviceIoControl(Device, IOCTL_SCSI_RESCAN_BUS, NULL, 0, NULL, 0, &BytesReturned, FALSE);
			Sleep(500);    
		}

		//Copy the data IN buffer so we can send ATA Command..
		memcpy(pAPT->DataBuffer, ATACommandObj->DATA_BUFFER, ATACommandObj->DATA_BUFFSIZE);

		DWORD BytesReturned = 0;
		//Send the ATA/IDE Pass through command..
		BOOL Status = DeviceIoControl(Device, IOCTL_IDE_PASS_THROUGH, pAPT, Size, pAPT, Size, &BytesReturned, FALSE);
		
		//Get the Error and Status registers of IDE command return
		ATACommandObj->OPReg.bErrorReg =  pAPT->IdeReg[0];
		ATACommandObj->OPReg.bStatusReg = pAPT->IdeReg[6];

		//If the command was successfull, copy the ATA structure's data into the ouptut object... 
		if (Status)
		{
			if (ATACommandObj->DATA_BUFFER)
			{
				if (ATACommandObj->DATA_BUFFSIZE > 0) 
					ZeroMemory(ATACommandObj->DATA_BUFFER, ATACommandObj->DATA_BUFFSIZE);
				if (BytesReturned > sizeof(ATA_PASS_THROUGH))
				{
					unsigned int ReturnedSize = BytesReturned - sizeof(ATA_PASS_THROUGH);
					ATACommandObj->DATA_BUFFSIZE = ReturnedSize;
					memcpy(ATACommandObj->DATA_BUFFER, pAPT->DataBuffer, ReturnedSize);
				}
			}
			
			retVal = TRUE;
		}
	}
	else
		retVal = FALSE;
		
	VirtualFree(pAPT, Size, MEM_RELEASE);
	CloseHandle(Device);
	return retVal;
}


#else
//XBOX Version of Sending ATA Commands..  
//Since XBOX runs everything in Kernel Mode, we use direct port access :)
BOOL XKHDD::SendATACommand(WORD IDEPort, LPATA_COMMAND_OBJ ATACommandObj, UCHAR ReadWrite)
{
	//XBOX Sending ATA Commands..  
	BOOL retVal			= FALSE;
	WORD waitcount;
	LPDWORD PIDEDATA	= (LPDWORD) &ATACommandObj->DATA_BUFFER;
  BYTE old_irql;

  // Raise IRQL to DPC level to prevent interrupts during ATA operation
  old_irql = KeRaiseIrqlToDpcLevel();

  // Wait for device to be ready
	for (waitcount = 10000; waitcount > 0; waitcount--)
	{
		if ((_inp(IDEPort + 7) & IDE_STATUS_DRIVE_BUSY) == 0)
			break;
		Sleep(1);
	}
	if (waitcount == 0)
  {
    KfLowerIrql(old_irql);
		return FALSE;
  }

	//Write IDE Registers to IDE Port.. and in essence Execute the ATA Command..
	_outp(IDEPort + 1, ATACommandObj->IPReg.bFeaturesReg);
	_outp(IDEPort + 2, ATACommandObj->IPReg.bSectorCountReg);
	_outp(IDEPort + 3, ATACommandObj->IPReg.bSectorNumberReg);
	_outp(IDEPort + 4, ATACommandObj->IPReg.bCylLowReg);
	_outp(IDEPort + 5, ATACommandObj->IPReg.bCylHighReg);
	_outp(IDEPort + 6, ATACommandObj->IPReg.bDriveHeadReg);
	_outp(IDEPort + 7, ATACommandObj->IPReg.bCommandReg);

	//Command Executed, Check Status.. If not success, wait a while.
	for (waitcount = 10000; waitcount > 0; waitcount--)
	{
		if ((_inp(IDEPort + 7) & IDE_STATUS_READY) == IDE_STATUS_READY)
			break;
		Sleep(1);
	}
	if (waitcount == 0)
  {
    KfLowerIrql(old_irql);
		return FALSE;
  }

	//Is this a IDE command that Requests Data, if so, read the from IDE port ...
	if (ReadWrite == IDE_COMMAND_READ)
	{
		//Read the command return output Registers
		ATACommandObj->OPReg.bErrorReg =		_inp(IDEPort + 1);
		ATACommandObj->OPReg.bSectorCountReg =	_inp(IDEPort + 2);
		ATACommandObj->OPReg.bSectorNumberReg =	_inp(IDEPort + 3);
		ATACommandObj->OPReg.bCylLowReg =		_inp(IDEPort + 4);
		ATACommandObj->OPReg.bCylHighReg =		_inp(IDEPort + 5);
		ATACommandObj->OPReg.bDriveHeadReg =	_inp(IDEPort + 6);
		ATACommandObj->OPReg.bStatusReg =		_inp(IDEPort + 7);

		ATACommandObj->DATA_BUFFSIZE = 512;

		//Now read a sector (512 Bytes) from the IDE Port
		ZeroMemory(ATACommandObj->DATA_BUFFER, 512);
		for (int i = 0; i < 128; i++)
			PIDEDATA[i] = _inpd(IDEPort);

		retVal = TRUE;
	}
	//Is this a IDE command that Sends Data, if so, write the Data to IDE Port..
	else if ((ReadWrite == IDE_COMMAND_WRITE) && (ATACommandObj->DATA_BUFFSIZE > 0))
	{
		//Read the command return output Registers
		ATACommandObj->OPReg.bErrorReg			= _inp(IDEPort + 1);
		ATACommandObj->OPReg.bSectorCountReg	= _inp(IDEPort + 2);
		ATACommandObj->OPReg.bSectorNumberReg	= _inp(IDEPort + 3);
		ATACommandObj->OPReg.bCylLowReg			= _inp(IDEPort + 4);
		ATACommandObj->OPReg.bCylHighReg		= _inp(IDEPort + 5);
		ATACommandObj->OPReg.bDriveHeadReg		= _inp(IDEPort + 6);
		ATACommandObj->OPReg.bStatusReg			= _inp(IDEPort + 7);

		//Now Write a sector (512 Bytes) To the IDE Port
		for (int i = 0; i <  128; i++)
			_outpd(IDEPort, PIDEDATA[i]);
		retVal = TRUE;
	}

  KfLowerIrql(old_irql);

	return retVal;
}
#endif