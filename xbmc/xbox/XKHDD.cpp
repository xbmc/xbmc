/*********************************
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
--------------------------------------------------------------------------------------------------------
Date: 01/12/2004
By: GeminiServer
Reason: more features for XBMC! [Code CleaUp and Removed No Need Features!]
--------------------------------------------------------------------------------------------------------*/
#include "../stdafx.h"
#include "XKHDD.h"
#include "XKRC4.h"
#include "XKSHA1.h"
#include <conio.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

XKHDD::XKHDD()
{}
XKHDD::~XKHDD(void)
{}
BYTE XKHDD::GetHddSmartTemp(UCHAR* IDEData)
{	
	// S.M.A.R.T Get HDD Temp ID: 194 OFFSET: 0x065 & 0x067
	BYTE retVal = (BYTE) *(IDEData+SMART_RED_HDD_TEMP2);
	return retVal;
}
WORD XKHDD::GetATABefehle(UCHAR* IDEData, BYTE IDEOffset)
{
	WORD retVal = (WORD) *(IDEData+IDEOffset);
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

void XKHDD::GetIDEFirmWare(UCHAR* IDEData, LPSTR ModelString, LPDWORD StrLen)
{
	UCHAR m_length = 0x8;		//IDE_FIRMWARE_OFFSET= 0x02e  //STart Adresse: Firmware: WORD:23-26 Hex:0x02e-0x035 DEZ:46-54
	m_length = CleanATAData((UCHAR*)ModelString, IDEData+IDE_FIRMWARE_OFFSET, m_length);
	*StrLen = m_length;
}
void XKHDD::GetIDESerial(UCHAR* IDEData, LPSTR SerialString, LPDWORD StrLen)
{
	UCHAR s_length = 0x14;
	s_length = CleanATAData((UCHAR*)SerialString, IDEData+HDD_SERIAL_OFFSET, s_length);
	*StrLen = s_length;
}
WORD XKHDD::GetIDESecurityStatus(UCHAR* IDEData)
{
	WORD retVal = (WORD) *(IDEData+HDD_SECURITY_STATUS_OFFSET);
	return retVal;
}
int XKHDD::CleanATAData(unsigned char *dst, unsigned char *src, int len)
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
	
	--dst;
	for(i=len; i>0; --i) 
	{
		if (dst[i] != ' ') break;
	}

	return i;
}

void XKHDD::GenerateHDDPwd(UCHAR* HDDKey, UCHAR* IDEData, UCHAR* HDDPass)
{
	XKSHA1 SHA1Obj;

	UCHAR serial[0x14];
	ZeroMemory(serial, 0x14);

	UCHAR model[0x28];
	ZeroMemory(model, 0x28); 
	
	UCHAR s_length = 0x14;
	UCHAR m_length = 0x28;

	s_length = CleanATAData(serial, IDEData+HDD_SERIAL_OFFSET, s_length);
	m_length = CleanATAData(model, IDEData+HDD_MODEL_OFFSET, m_length);


	SHA1Obj.HMAC_SHA1 (HDDPass, HDDKey, 0x10, model, m_length, serial, s_length);
}

void XKHDD::GetIDEModel(UCHAR* IDEData, LPSTR ModelString, LPDWORD StrLen)
{
	UCHAR m_length = 0x28;
	m_length = CleanATAData((UCHAR*)ModelString, IDEData+HDD_MODEL_OFFSET, m_length);
	*StrLen = m_length;
}
BOOL XKHDD::SendATACommand(WORD IDEPort, LPATA_COMMAND_OBJ ATACommandObj, UCHAR ReadWrite)
{
	//XBOX Sending ATA Commands..  
	BOOL retVal			= FALSE;
	UCHAR waitcount		= 10;
	WORD inVal			= 0;
	WORD SuccessRet		= 0x58;
	LPDWORD PIDEDATA	= (LPDWORD) &ATACommandObj->DATA_BUFFER ;
		
	//Write IDE Registers to IDE Port.. and in essence Execute the ATA Command..
	_outp(IDEPort + 1, ATACommandObj->IPReg.bFeaturesReg);		Sleep(10);
	_outp(IDEPort + 2, ATACommandObj->IPReg.bSectorCountReg); 	Sleep(10);
	_outp(IDEPort + 3, ATACommandObj->IPReg.bSectorNumberReg);	Sleep(10);
	_outp(IDEPort + 4, ATACommandObj->IPReg.bCylLowReg);		Sleep(10);
	_outp(IDEPort + 5, ATACommandObj->IPReg.bCylHighReg);		Sleep(10);
	_outp(IDEPort + 6, ATACommandObj->IPReg.bDriveHeadReg);		Sleep(10);
	_outp(IDEPort + 7, ATACommandObj->IPReg.bCommandReg);		Sleep(300);

	//Command Executed, Check Status.. If not success, wait a while..
	inVal = _inp(IDEPort+7); 
	while (((inVal & SuccessRet) != SuccessRet) && (waitcount > 0))
	{
		inVal = _inp(IDEPort+7); //Check Status..
		Sleep(300);
		waitcount--;
	}

	//Is this a IDE command that Requests Data, if so, Read the from IDE port ...
	if ((waitcount > 0) && (ReadWrite == IDE_COMMAND_READ))
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
		Sleep(10);

		//Now read a sector (512 Bytes) from the IDE Port
		ZeroMemory(ATACommandObj->DATA_BUFFER, 512);
		for (int i = 0; i < 128; i++)
		{
			PIDEDATA[i] = _inpd(IDEPort);
			Sleep(5);
		}

		retVal = TRUE;
	}

	//Is this a IDE command that Sends Data, if so, write the Data to IDE Port..
	if ((waitcount > 0) && (ATACommandObj->DATA_BUFFSIZE > 0) && (ReadWrite == IDE_COMMAND_WRITE))
	{
		//Read the command return output Registers
		ATACommandObj->OPReg.bErrorReg			= _inp(IDEPort + 1);
		ATACommandObj->OPReg.bSectorCountReg	= _inp(IDEPort + 2);
		ATACommandObj->OPReg.bSectorNumberReg	= _inp(IDEPort + 3);
		ATACommandObj->OPReg.bCylLowReg			= _inp(IDEPort + 4);
		ATACommandObj->OPReg.bCylHighReg		= _inp(IDEPort + 5);
		ATACommandObj->OPReg.bDriveHeadReg		= _inp(IDEPort + 6);
		ATACommandObj->OPReg.bStatusReg			= _inp(IDEPort + 7);
		Sleep(10);

		//Now Write a sector (512 Bytes) To the IDE Port
		for (int i = 0; i <  128; i++)
		{
			_outpd(IDEPort, PIDEDATA[i]);
				Sleep(5);
		}
		retVal = TRUE;
	}
	return retVal;
}