/*
* XBMC Media Center
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

// IoSupport.h: interface for the CIoSupport class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_IOSUPPORT_H__F084A488_BD6E_49D5_8CD3_0BE62149DB40__INCLUDED_)
#define AFX_IOSUPPORT_H__F084A488_BD6E_49D5_8CD3_0BE62149DB40__INCLUDED_

#include "system.h" // for Win32 types

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define MODE1_DATA_SIZE    2048 // Mode1 sector has 2048 bytes of data

#define MODE2_DATA_START   24   // Mode2 raw sector has 24 bytes before the data payload
#define MODE2_DATA_SIZE    2324 // And has 2324 usable bytes
#define RAW_SECTOR_SIZE    2352 // Raw sector size

class CIoSupport
{
public:
  static void AllocReadBuffer();
  static void FreeReadBuffer();

  static HANDLE OpenCDROM();
  static INT ReadSector(HANDLE hDevice, DWORD dwSector, LPSTR lpczBuffer);
  static INT ReadSectorMode2(HANDLE hDevice, DWORD dwSector, LPSTR lpczBuffer);
  static INT ReadSectorCDDA(HANDLE hDevice, DWORD dwSector, LPSTR lpczBuffer);
  static VOID CloseCDROM(HANDLE hDevice);

private:
  static PVOID m_rawXferBuffer;
};

#endif // !defined(AFX_IOSUPPORT_H__F084A488_BD6E_49D5_8CD3_0BE62149DB40__INCLUDED_)
