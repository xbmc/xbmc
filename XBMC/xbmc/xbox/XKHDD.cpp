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
******************************************************************************************************
*/
#include "stdafx.h"
#include "XKHDD.h"
#include "XKRC4.h"
#include "XKSHA1.h"
#include <conio.h>
#include "undocumented.h"


XKHDD::XKHDD()
{}
XKHDD::~XKHDD(void)
{}

BOOL XKHDD::EnableSMART()
{
	ATA_COMMAND_OBJ hddcommand;
	ZeroMemory(&hddcommand, sizeof(XKHDD::ATA_COMMAND_OBJ));

	hddcommand.IPReg.bFeaturesReg     = ATA_SMART_ENABLE;
	hddcommand.IPReg.bCylLowReg       = SMART_CYL_LOW;
	hddcommand.IPReg.bCylHighReg      = SMART_CYL_HI;
	hddcommand.IPReg.bCommandReg      = ATA_SMART;

	return SendATACommand(XBOX_DEVICE_HDD, &hddcommand, IDE_COMMAND_NONDATA);
}

signed char XKHDD::GetHddSmartTemp()
{
	ATA_COMMAND_OBJ hddcommand;
	ZeroMemory(&hddcommand, sizeof(XKHDD::ATA_COMMAND_OBJ));

	hddcommand.DATA_BUFFSIZE          = 0x200;
	hddcommand.IPReg.bFeaturesReg     = ATA_SMART_READ_VALUES;
	hddcommand.IPReg.bCylLowReg       = SMART_CYL_LOW; 
	hddcommand.IPReg.bCylHighReg      = SMART_CYL_HI;
	hddcommand.IPReg.bCommandReg      = ATA_SMART;

	if (SendATACommand(XBOX_DEVICE_HDD, &hddcommand, IDE_COMMAND_READ))
		return GetHddSmartTemp((UCHAR *)&hddcommand.DATA_BUFFER);
	else
		return 0;
}
// Automatic Acoustic Management: 0xFE = fast, 0x80 = quiet
BOOL XKHDD::SetAAMLevel(BYTE bLevel)
{
	ATA_COMMAND_OBJ hddcommand;
	ZeroMemory(&hddcommand, sizeof(XKHDD::ATA_COMMAND_OBJ));

	hddcommand.IPReg.bFeaturesReg     = ATA_FEATURE_ENABLE_AAM;
	hddcommand.IPReg.bSectorCountReg  = bLevel;
	hddcommand.IPReg.bCommandReg      = ATA_COMMAND_SET_FEATURES;

	return SendATACommand(XBOX_DEVICE_HDD, &hddcommand, IDE_COMMAND_NONDATA);
}
// Automatic Power Management: 0xFE = fast, 0x80 = quiet
BOOL XKHDD::SetAPMLevel(BYTE bLevel)
{
	ATA_COMMAND_OBJ hddcommand;
	ZeroMemory(&hddcommand, sizeof(XKHDD::ATA_COMMAND_OBJ));

	hddcommand.IPReg.bFeaturesReg     = ATA_FEATURE_ENABLE_APM;
	hddcommand.IPReg.bSectorCountReg  = bLevel;
	hddcommand.IPReg.bCommandReg      = ATA_COMMAND_SET_FEATURES;

	return SendATACommand(XBOX_DEVICE_HDD, &hddcommand, IDE_COMMAND_NONDATA);
}

VOID XKHDD::SpindownHarddisk(bool bSpinDown)
{
#ifdef _XBOX
	int status;
	int iPowerCode = ATA_POWERSTATE_ACTIVE; //assume active mode

	KIRQL oldIrql = XKHDD::RaiseIRQLToIDEChannelIRQL();
	_outp(ATA_DRIVE_HEAD_REGISTER, 0xA0 );
	//Ask drive for powerstate
	_outp(ATA_COMMAND_REGISTER, ATA_COMMAND_POWERMODE1);
	//Get status of the command
	status = _inp(ATA_STATUS_REGISTER);
	int i = 0;
	while ( (i < 2000) && ((status & ATA_STATUS_DRIVE_BUSY) == ATA_STATUS_DRIVE_BUSY) )
	{
		//wait for drive to process the command
		status = _inp(ATA_STATUS_REGISTER);
		i++;
	};
	//if it's busy or not responding as we expected don't tell it to standby
	if ( (i < 2000) && (!(status & ATA_STATUS_DRIVE_ERROR)) )
	{
		iPowerCode = _inp(ATA_SECTOR_COUNT_REGISTER);
		if (bSpinDown && iPowerCode != ATA_POWERSTATE_STANDBY)
		{
			_outp(ATA_DRIVE_HEAD_REGISTER, 0xA0 );
			_outp(ATA_COMMAND_REGISTER, ATA_COMMAND_STANDBY);
		}
		else if (!bSpinDown && iPowerCode == ATA_POWERSTATE_STANDBY)
		{
			_outp(ATA_DRIVE_HEAD_REGISTER, 0xA0 );
			_outp(ATA_COMMAND_REGISTER, ATA_COMMAND_ACTIVE);
		}
	}
	KeLowerIrql(oldIrql);
#endif
}

signed char XKHDD::GetHddSmartTemp(UCHAR* IDEData)
{
	int i;
	signed char retval = 0;
	// Scan the SMART attributes returned by the HDD for the HDDTemp attribute.
	for (i=2; i < (2+(12*29)); i+=12)
	{
		// SMART ATTR structure: AA FF FF CC TT RR RR RR RR ... (raw values 8 bytes long)
		// AA = attribute, FF = flags, CC = "current value", TT = threshould, RR = raw
		// current value and threshold are invalid for temperature, raw provides a valid value.
		if (IDEData[i] == SMART_ATTR_HDATEMP)
		{
			retval = (signed char) IDEData[i+5];
			break;
		}
	}
	return retval;
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

BOOL XKHDD::IsSmartSupported(UCHAR * IDEData)
{
	return ((WORD)*(IDEData + HDD_CMD_SUPPORT_OFFSET)) & 1;
}

BOOL XKHDD::IsSmartEnabled(UCHAR * IDEData)
{
	return ((WORD)*(IDEData + HDD_CMD_ENABLED_OFFSET)) & 1;
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

KIRQL XKHDD::RaiseIRQLToIDEChannelIRQL()
{
	return KfRaiseIrql(XBOX_IDE_CHANNEL_IRQL);
}

BOOL XKHDD::SendATACommand(UCHAR DeviceNum, LPATA_COMMAND_OBJ ATACommandObj, UCHAR ReadWrite)
{
	BOOL retVal = FALSE;

#if defined (_WINDOWS)
	char tmp[128];
	sprintf(tmp, "\\\\.\\PhysicalDrive%u", DeviceNum);
#else
	ANSI_STRING tmp;
	if (DeviceNum == XBOX_DEVICE_HDD)
		RtlInitAnsiString(&tmp, "\\Device\\Harddisk0\\Partition0");
	else if (DeviceNum == XBOX_DEVICE_DVDROM)
		RtlInitAnsiString(&tmp, "\\Device\\Cdrom0");
	else
		return FALSE;
#endif
	//Open HDD and get handle to Open Device...
#if defined (_WINDOWS)
	HANDLE Device = CreateFile(tmp, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	if (Device == INVALID_HANDLE_VALUE)
		return FALSE;
#else
	HANDLE Device;
	IO_STATUS_BLOCK iosb;
	OBJECT_ATTRIBUTES oa;
	NTSTATUS status;

	ZeroMemory (&oa, sizeof(OBJECT_ATTRIBUTES));
	oa.ObjectName = &tmp;
	oa.Attributes = 0x40; // case insensitive

	status = NtOpenFile(&Device, SYNCHRONIZE | GENERIC_READ | GENERIC_WRITE, &oa, &iosb, FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_SYNCHRONOUS_IO_ALERT);

	if (status < 0) // check for success
		return FALSE;
#endif

	//Set up ATA Pass through structures..
#if defined (_WINDOWS)
	unsigned int Size = sizeof(ATA_PASS_THROUGH) + ATACommandObj->DATA_BUFFSIZE;
#else
	unsigned int Size = sizeof(ATA_PASS_THROUGH);
#endif
	LPATA_PASS_THROUGH pAPT = (LPATA_PASS_THROUGH) VirtualAlloc(NULL, Size, MEM_COMMIT, PAGE_READWRITE);
	ZeroMemory (pAPT, Size);

	if (pAPT)
	{
		memset(pAPT, 0, Size);
		pAPT->DataBufferSize          = ATACommandObj->DATA_BUFFSIZE;
		pAPT->IdeReg.bFeaturesReg     = ATACommandObj->IPReg.bFeaturesReg;
		pAPT->IdeReg.bSectorCountReg  = ATACommandObj->IPReg.bSectorCountReg;
		pAPT->IdeReg.bSectorNumberReg = ATACommandObj->IPReg.bSectorNumberReg;
		pAPT->IdeReg.bCylLowReg       = ATACommandObj->IPReg.bCylLowReg;
		pAPT->IdeReg.bCylHighReg      = ATACommandObj->IPReg.bCylHighReg;
		pAPT->IdeReg.bDriveHeadReg    = ATACommandObj->IPReg.bDriveHeadReg;
		pAPT->IdeReg.bCommandReg      = ATACommandObj->IPReg.bCommandReg;

#if defined (_WINDOWS)
		if (ATACommandObj->IPReg.bCommandReg == 0xEC)  
		{
			// For some Reason Win2K/XP sometimes needs to rescan bus before we get IDENTIFY info..
			DWORD BytesReturned = 0;
			(void)DeviceIoControl(Device, IOCTL_SCSI_RESCAN_BUS, NULL, 0, NULL, 0, &BytesReturned, FALSE);
			Sleep(500);    
		}

		//Copy the data IN buffer so we can send ATA Command..
		memcpy(pAPT->DataBuffer, ATACommandObj->DATA_BUFFER, ATACommandObj->DATA_BUFFSIZE);
#else
		pAPT->DataBuffer = (BYTE *) &ATACommandObj->DATA_BUFFER;
		pAPT->IdeReg.bHostSendsData = ReadWrite == IDE_COMMAND_WRITE ? TRUE : FALSE;
#endif

		DWORD BytesReturned = 0;
		//Send the ATA/IDE Pass through command..
		BOOL Status = DeviceIoControl(Device, IOCTL_IDE_PASS_THROUGH, pAPT, Size, pAPT, Size, &BytesReturned, FALSE);
		
		//Get the Error and Status registers of IDE command return
		ATACommandObj->OPReg.bErrorReg =  pAPT->IdeReg.bFeaturesReg;
		ATACommandObj->OPReg.bStatusReg = pAPT->IdeReg.bCommandReg;

		//If the command was successful, copy the ATA structure's data into the ouptut object... 
		if (Status)
		{
#if defined (_WINDOWS)
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
#endif
			retVal = TRUE;
		}
	}
	else
		retVal = FALSE;

#if !defined(_WINDOWS)
		// On the Xbox, if this is the DVD-ROM device, make sure we poll the DVD-ROM device for readiness
		// after the command is sent.
		if (DeviceNum == XBOX_DEVICE_DVDROM)
		{
			for (int errors = 0; errors < 5; errors++)
			{
				if (DeviceIoControl(Device, IOCTL_CDROM_CHECK_VERIFY, 0, 0, 0, 0, 0, 0))
					break;
				Sleep(200);
			}
		}
#endif

	VirtualFree(pAPT, Size, MEM_RELEASE);
	CloseHandle(Device);
	return retVal;
}
