/*
 * XBoxMediaPlayer
 * Copyright (c) 2002 d7o3g4q and RUNTiME
 * Portions Copyright (c) by the authors of ffmpeg and xvid
 *
 * This program is free software; you can redistribute it and/or modify
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
 */

// IoSupport.cpp: implementation of the CIoSupport class.
//
//////////////////////////////////////////////////////////////////////

#include "IoSupport.h"
#include "Undocumented.h"
#include "stdstring.h"

#define CTLCODE(DeviceType, Function, Method, Access) ( ((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method)  ) 
#define FSCTL_DISMOUNT_VOLUME  CTLCODE( FILE_DEVICE_FILE_SYSTEM, 0x08, METHOD_BUFFERED, FILE_ANY_ACCESS )


typedef struct 
{
	char szDriveLetter;
	char* szDevice;
} stDriveMapping;

stDriveMapping driveMapping[]=
{
	{ 'C', "Harddisk0\\Partition2"},
	{ 'D', "Cdrom0"},
	{ 'E', "Harddisk0\\Partition1"},
	{ 'F', "Harddisk0\\Partition6"},
	{ 'X', "Harddisk0\\Partition3"},
	{ 'Y', "Harddisk0\\Partition4"},
	{ 'Z', "Harddisk0\\Partition5"},
};
#define NUM_OF_DRIVES ( sizeof( driveMapping) / sizeof( driveMapping[0] ) )

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CIoSupport::CIoSupport()
{
	m_dwLastTrayState=0;

	m_gmXferBuffer = GlobalAlloc(GPTR, RAW_SECTOR_SIZE);
	m_rawXferBuffer = NULL;
	if( m_gmXferBuffer )
		m_rawXferBuffer = GlobalLock(m_gmXferBuffer);
}

CIoSupport::CIoSupport(CIoSupport& other)
{
	m_dwTrayState = other.m_dwTrayState;
	m_dwTrayCount = other.m_dwTrayCount;
	m_dwLastTrayState = other.m_dwLastTrayState;

	m_gmXferBuffer = GlobalAlloc(GPTR, RAW_SECTOR_SIZE);
	m_rawXferBuffer = NULL;
	if( m_gmXferBuffer )
		m_rawXferBuffer = GlobalLock(m_gmXferBuffer);
}

CIoSupport::~CIoSupport()
{
	if( m_gmXferBuffer )
	{
		GlobalUnlock(m_gmXferBuffer);
		GlobalFree(m_gmXferBuffer);
	}
}

// szDrive e.g. "D:"
// szDevice e.g. "Cdrom0" or "Harddisk0\Partition6"

HRESULT CIoSupport::Mount(const char* szDrive, char* szDevice)
{
	char szSourceDevice[256];
	char szDestinationDrive[16];

	sprintf(szSourceDevice,"\\Device\\%s",szDevice);
	sprintf(szDestinationDrive,"\\??\\%s",szDrive);

	STRING DeviceName =
	{
		strlen(szSourceDevice),
		strlen(szSourceDevice) + 1,
		szSourceDevice
	};

	STRING LinkName =
	{
		strlen(szDestinationDrive),
		strlen(szDestinationDrive) + 1,
		szDestinationDrive
	};

	IoCreateSymbolicLink(&LinkName, &DeviceName);

	return S_OK;
}



// szDrive e.g. "D:"

HRESULT CIoSupport::Unmount(const char* szDrive)
{
	char szDestinationDrive[16];
	sprintf(szDestinationDrive,"\\??\\%s",szDrive);

	STRING LinkName =
	{
		strlen(szDestinationDrive),
		strlen(szDestinationDrive) + 1,
		szDestinationDrive
	};

	IoDeleteSymbolicLink(&LinkName);
	
	return S_OK;
}





HRESULT CIoSupport::Remount(LPCSTR szDrive, LPSTR szDevice)
{
	char szSourceDevice[48];
	sprintf(szSourceDevice,"\\Device\\%s",szDevice);

	Unmount(szDrive);
	
	ANSI_STRING filename;
	OBJECT_ATTRIBUTES attributes;
	IO_STATUS_BLOCK status;
	HANDLE hDevice;
	NTSTATUS error;
	DWORD dummy;

	RtlInitAnsiString(&filename, szSourceDevice);
	InitializeObjectAttributes(&attributes, &filename, OBJ_CASE_INSENSITIVE, NULL);

	if (NT_SUCCESS(error = NtCreateFile(&hDevice, GENERIC_READ |
		SYNCHRONIZE | FILE_READ_ATTRIBUTES, &attributes, &status, NULL, 0,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, FILE_OPEN,
		FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT)))
	{

		if (!DeviceIoControl(hDevice, FSCTL_DISMOUNT_VOLUME, NULL, 0, NULL, 0, &dummy, NULL))
		{
			CloseHandle(hDevice);
			return E_FAIL;
		}

		CloseHandle(hDevice);
	}
	
	Mount(szDrive,szDevice);

	return S_OK;
}

HRESULT CIoSupport::Remap(char* szMapping)
{
	char szMap[32];
	strcpy(szMap, szMapping );

	char* pComma = strstr(szMap,",");
	if (pComma)
	{
		*pComma = 0;
		
		// map device to drive letter
		Unmount(szMap);
		Mount(szMap,&pComma[1]);
		return S_OK;
	}

	return E_FAIL;
}


HRESULT CIoSupport::EjectTray()
{
	HalWriteSMBusValue(0x20, 0x0C, FALSE, 0);  // eject tray
	return S_OK;
}

HRESULT CIoSupport::CloseTray()
{
	HalWriteSMBusValue(0x20, 0x0C, FALSE, 1);  // close tray
	return S_OK;
}

DWORD CIoSupport::GetTrayState()
{
	HalReadSMCTrayState(&m_dwTrayState,&m_dwTrayCount);
	return m_dwTrayState;

	if(m_dwTrayState == TRAY_CLOSED_MEDIA_PRESENT) 
	{
		if (m_dwLastTrayState != TRAY_CLOSED_MEDIA_PRESENT)
		{
			m_dwLastTrayState = m_dwTrayState;
			return DRIVE_CLOSED_MEDIA_PRESENT;
		}
		else
		{
			return DRIVE_READY;
		}
	}
	else if(m_dwTrayState == TRAY_CLOSED_NO_MEDIA)
	{
		m_dwLastTrayState = m_dwTrayState;
		return DRIVE_CLOSED_NO_MEDIA;
	}
	else if(m_dwTrayState == TRAY_OPEN)
	{
		m_dwLastTrayState = m_dwTrayState;
		return DRIVE_OPEN;
	}
	else
	{
		m_dwLastTrayState = m_dwTrayState;
	}

	return DRIVE_NOT_READY;
}

HRESULT CIoSupport::Shutdown()
{
	// fails assertion on debug bios (symptom lockup unless running dr watson
	// so you can continue past the failed assertion).
	if (IsDebug())
		return E_FAIL;

		HalInitiateShutdown();

	return S_OK;
}


VOID CIoSupport::RemountDrive(LPCSTR szDrive)
{
	// ugly, but it works ;-)
	for (int i=0; i < NUM_OF_DRIVES; i++)
	{
		if (szDrive[0]== driveMapping[i].szDriveLetter)
		{
			Remount(szDrive, driveMapping[i].szDevice);
		}
	}
}

VOID CIoSupport::GetPartition(LPCSTR strFilename, LPSTR strPartition)
{
	strcpy(strPartition,"");
	for (int i=0; i < NUM_OF_DRIVES; i++)
	{
		if ( toupper(strFilename[0]) == driveMapping[i].szDriveLetter)
		{
			strcpy(strPartition, driveMapping[i].szDevice);
			return;
		}
	}
}

string CIoSupport::GetDrive(const string &szPartition)
{
	static string strDrive="E:";
	for (int i=0; i < NUM_OF_DRIVES; i++)
	{
		CStdString strMap=driveMapping[i].szDevice;
		CStdString strSearch=szPartition;
		CStdString strSearchLeft=strSearch.Left( strMap.size() );
		strMap.ToLower();
		strSearchLeft.ToLower();
		if ( strMap == strSearchLeft )
		{
			char szDrive[3];
			szDrive[0]=driveMapping[i].szDriveLetter;
			szDrive[1]=0;
			strDrive=szDrive;
			return  strDrive;
		}
	}
	return  strDrive;
}

HANDLE CIoSupport::OpenCDROM()
{
	ANSI_STRING filename;
	OBJECT_ATTRIBUTES attributes;
	IO_STATUS_BLOCK status;
	HANDLE hDevice;
	NTSTATUS error;


	Remount("D:","Cdrom0");
	Unmount("D:");

	if( !m_rawXferBuffer )
		return NULL;

	RtlInitAnsiString(&filename,"\\Device\\Cdrom0");
	InitializeObjectAttributes(&attributes, &filename, OBJ_CASE_INSENSITIVE, NULL);
	if (!NT_SUCCESS(error = NtCreateFile(&hDevice, 
																			GENERIC_READ |SYNCHRONIZE | FILE_READ_ATTRIBUTES, 
																			&attributes, 
																			&status, 
																			NULL, 
																			0,
																			FILE_SHARE_READ,
																			FILE_OPEN,	
																			FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT)))
	{
		return NULL;
	}
	return hDevice;
}

INT CIoSupport::ReadSector(HANDLE hDevice, DWORD dwSector, LPSTR lpczBuffer)

{
	DWORD dwRead;
	DWORD dwSectorSize = 2048;
	LARGE_INTEGER Displacement;

	Displacement.QuadPart = ((INT64)dwSector) * dwSectorSize;

	for (int i=0; i < 5; i++)
	{
	SetFilePointer(hDevice, Displacement.LowPart, &Displacement.HighPart, FILE_BEGIN);

	if (ReadFile(hDevice, m_rawXferBuffer, dwSectorSize, &dwRead, NULL))
		{
	memcpy(lpczBuffer, m_rawXferBuffer, dwSectorSize);
	return dwRead;
		}
	}

	OutputDebugString("CD Read error\n");
	return -1;
}


INT CIoSupport::ReadSectorMode2(HANDLE hDevice, DWORD dwSector, LPSTR lpczBuffer)
{
	DWORD dwBytesReturned;
	RAW_READ_INFO rawRead;

	// Oddly enough, DiskOffset uses the Red Book sector size
	rawRead.DiskOffset.QuadPart = 2048 * dwSector;
	rawRead.SectorCount = 1;
	rawRead.TrackMode = XAForm2;

	for (int i=0; i < 5; i++)
	{
		if( DeviceIoControl( hDevice,
			IOCTL_CDROM_RAW_READ,
			&rawRead,
			sizeof(RAW_READ_INFO),
			m_rawXferBuffer,
			sizeof(RAW_SECTOR_SIZE),
			&dwBytesReturned,
			NULL ) != 0 )
		{
			memcpy(lpczBuffer, (byte*)m_rawXferBuffer + MODE2_DATA_START, MODE2_DATA_SIZE);
			return MODE2_DATA_SIZE;
		}
	}
	return -1;
}

INT CIoSupport::ReadSectorCDDA(HANDLE hDevice, DWORD dwSector, LPSTR lpczBuffer)
{
	DWORD dwBytesReturned;
	RAW_READ_INFO rawRead;

	// Oddly enough, DiskOffset uses the Red Book sector size
	rawRead.DiskOffset.QuadPart = 2048 * dwSector;
	rawRead.SectorCount = 1;
	rawRead.TrackMode = CDDA;

	for (int i=0; i < 5; i++)
	{
		if( DeviceIoControl( hDevice,
			IOCTL_CDROM_RAW_READ,
			&rawRead,
			sizeof(RAW_READ_INFO),
			m_rawXferBuffer,
			sizeof(RAW_SECTOR_SIZE),
			&dwBytesReturned,
			NULL ) != 0 )
		{
			memcpy(lpczBuffer, m_rawXferBuffer, RAW_SECTOR_SIZE);
			return RAW_SECTOR_SIZE;
		}
	}
	return -1;
}

VOID CIoSupport::CloseCDROM(HANDLE hDevice)
{
	CloseHandle(hDevice);
}

VOID CIoSupport::UpdateDvdrom()
{
	OutputDebugString("Starting Dvdrom update.\n");
	BOOL bClosingTray = false;
	BOOL bShouldHaveClosed = false;

	// if the tray is open, close it
	DWORD dwCurrentState;
	do
	{
		dwCurrentState = GetTrayState();
		switch(dwCurrentState)
		{
			case DRIVE_OPEN:

				// drive is open		
				if (!bClosingTray)
				{
					bClosingTray = true;

					OutputDebugString("Drive open, closing tray...\n");
					CloseTray();
				}
				else if (bShouldHaveClosed)
				{
					// the operation failed, we cannot stay in this loop
					OutputDebugString("Dvdrom ended (failed to retract tray).\n");
					return;
				}

				break;
			case DRIVE_NOT_READY:
				// drive is not ready (closing, opening)
				OutputDebugString("Drive transition.\n");			
				bShouldHaveClosed = bClosingTray;
				Sleep(6000);
				break;
			case DRIVE_READY:
				// drive is ready
				OutputDebugString("Drive ready.\n");
				break;
			case DRIVE_CLOSED_NO_MEDIA:
				// nothing in there...
				OutputDebugString("Drive closed no media.\n");
				break;
			case DRIVE_CLOSED_MEDIA_PRESENT:
				// drive has been closed and is ready
				OutputDebugString("Drive closed media present, remounting...\n");
				Remount("D:","Cdrom0");
				break;
		}

	} while (dwCurrentState<DRIVE_READY);

	OutputDebugString("Dvdrom updated.\n");
}


// returns true if this is a debug machine
BOOL CIoSupport::IsDebug()
{
	// TODO: add the export to enable the following code to work!
	// return (XboxKrnlVersion->Qfe & 0x8000);
	return FALSE;
}

VOID CIoSupport::IdexWritePortUchar(USHORT port, UCHAR data) 
{
	_asm
	{
		mov dx, port
		mov al, data
		out dx, al
	}
}

VOID CIoSupport::SpindownHarddisk()
{
	#define IDE_DEVICE_SELECT_REGISTER	0x01F6
	#define IDE_COMMAND_REGISTER		0x01F7
	#define IDE_COMMAND_STANDBY			0xE0

	Sleep(2000);
	KIRQL oldIrql = KeRaiseIrqlToDpcLevel();
	IdexWritePortUchar(IDE_DEVICE_SELECT_REGISTER, 0xA0 );
	IdexWritePortUchar(IDE_COMMAND_REGISTER, IDE_COMMAND_STANDBY);
	KeLowerIrql(oldIrql);
}


VOID CIoSupport::GetXbePath(char* szDest)
{
	//Function to get the XBE Path like:
	//E:\DevKit\xbplayer\xbplayer.xbe

	PANSI_STRING pImageFileName = (PANSI_STRING)XeImageFileName;
	CIoSupport helper;
	char szDevicePath[1024+1];
	char szHomeDrive[3];
	char szPartition[50];
  memset(szDevicePath,0,sizeof(szDevicePath));
  memset(szHomeDrive,0,sizeof(szHomeDrive));
  memset(szPartition,0,sizeof(szPartition));
	strncpy(szDevicePath,
		    &pImageFileName->Buffer[8],
		    pImageFileName->Length - 7 );	//get xbepath (without \Device\ prefix)
	

	strcpy(szHomeDrive, helper.GetDrive(szDevicePath).c_str());	//get driveletter of xbepath
	if (szHomeDrive[0])
	{
		//OutputDebugString("homedrive:");OutputDebugString(szHomeDrive);OutputDebugString("\n");

		helper.GetPartition(szHomeDrive,szPartition);		//get patition (ie: Hardisk0\Partition1)
//		OutputDebugString("partition:");OutputDebugString(szPartition);OutputDebugString("\n");
    strcpy(szDest, szHomeDrive);						//copy drive letter
		strcat(szDest, ":");										//copy drive letter
		strncat(szDest,													//concat the path
			    &szDevicePath[strlen(szPartition)],
			    strlen(szDevicePath) - strlen(szPartition));		


	} 
	else 
	{
		OutputDebugString("cant get xbe path\n");
		szDest[0]=0;	//somehow we cant get the xbepath :(
	}
}