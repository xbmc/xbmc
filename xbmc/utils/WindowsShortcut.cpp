/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

// WindowsShortcut.cpp: implementation of the CWindowsShortcut class.
//
//////////////////////////////////////////////////////////////////////

#include "sc.h"
#include "WindowsShortcut.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif


#define FLAG_SHELLITEMIDLIST   1
#define FLAG_FILEORDIRECTORY   2
#define FLAG_DESCRIPTION     4
#define FLAG_RELATIVEPATH     8
#define FLAG_WORKINGDIRECTORY   0x10
#define FLAG_ARGUMENTS          0x20
#define FLAG_ICON               0x40

#define VOLUME_LOCAL        1
#define VOLUME_NETWORK      2

using namespace std;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CWindowsShortcut::CWindowsShortcut()
{
}

CWindowsShortcut::~CWindowsShortcut()
{
}

bool CWindowsShortcut::GetShortcut(const string& strFileName, string& strFileOrDir)
{
  strFileOrDir = "";
  if (!IsShortcut(strFileName) ) return false;


  CFile file;
  if (!file.Open(strFileName.c_str(), CFile::typeBinary | CFile::modeRead)) return false;
  byte byHeader[2048];
  int iBytesRead = file.Read(byHeader, 2048);
  file.Close();

  DWORD dwFlags = *((DWORD*)(&byHeader[0x14]));

  int iPos = 0x4c;
  if (dwFlags & FLAG_SHELLITEMIDLIST)
  {
    WORD sLength = *((WORD*)(&byHeader[iPos]));
    iPos += sLength;
    iPos += 2;
  }

  // skip File Location Info
  DWORD dwLen = 0x1c;
  if (dwFlags & FLAG_SHELLITEMIDLIST)
  {
    dwLen = *((DWORD*)(&byHeader[iPos]));
  }


  DWORD dwVolumeFlags = *((DWORD*)(&byHeader[iPos + 0x8]));
  DWORD dwOffsetLocalVolumeInfo = *((DWORD*)(&byHeader[iPos + 0xc]));
  DWORD dwOffsetBasePathName = *((DWORD*)(&byHeader[iPos + 0x10]));
  DWORD dwOffsetNetworkVolumeInfo = *((DWORD*)(&byHeader[iPos + 0x14]));
  DWORD dwOffsetRemainingPathName = *((DWORD*)(&byHeader[iPos + 0x18]));


  if ((dwVolumeFlags & VOLUME_NETWORK) == 0) return false;

  strFileOrDir = "smb:";
  // share name
  iPos += dwOffsetNetworkVolumeInfo + 0x14;
  while (iPos < iBytesRead && byHeader[iPos] != 0)
  {
    if (byHeader[iPos] == '\\') byHeader[iPos] = '/';
    strFileOrDir += (char)byHeader[iPos];
    iPos++;
  }
  iPos++;
  // file/folder name
  strFileOrDir += '/';
  while (iPos < iBytesRead && byHeader[iPos] != 0)
  {
    if (byHeader[iPos] == '\\') byHeader[iPos] = '/';
    strFileOrDir += (char)byHeader[iPos];
    iPos++;
  }
  return true;
}

bool CWindowsShortcut::IsShortcut(const string& strFileName)
{
  CFile file;
  if (!file.Open(strFileName.c_str(), CFile::typeBinary | CFile::modeRead)) return false;
  byte byHeader[0x80];
  int iBytesRead = file.Read(byHeader, 0x80);
  file.Close();
  if (iBytesRead < 0x4c)
  {
    return false;
  }
  //long integer that is always set to 4Ch
  if (byHeader[0] != 0x4c) return false;
  if (byHeader[1] != 0x0 ) return false;
  if (byHeader[2] != 0x0 ) return false;
  if (byHeader[3] != 0x0 ) return false;

  //globally unique identifier GUID of the shell links
  if (byHeader[0x04] != 0x01) return false;
  if (byHeader[0x05] != 0x14) return false;
  if (byHeader[0x06] != 0x02) return false;
  if (byHeader[0x07] != 0x00) return false;
  if (byHeader[0x08] != 0x00) return false;
  if (byHeader[0x09] != 0x00) return false;
  if (byHeader[0x0a] != 0x00) return false;
  if (byHeader[0x0b] != 0x00) return false;
  if (byHeader[0x0c] != 0xc0) return false;
  if (byHeader[0x0d] != 0x00) return false;
  if (byHeader[0x0e] != 0x00) return false;
  if (byHeader[0x0f] != 0x00) return false;
  if (byHeader[0x10] != 0x00) return false;
  if (byHeader[0x11] != 0x00) return false;
  if (byHeader[0x12] != 0x00) return false;
  if (byHeader[0x13] != 0x46) return false;

  // 2dwords, always 0
  if (byHeader[0x44] != 0x0 ) return false;
  if (byHeader[0x45] != 0x0 ) return false;
  if (byHeader[0x46] != 0x0 ) return false;
  if (byHeader[0x47] != 0x0 ) return false;
  if (byHeader[0x48] != 0x0 ) return false;
  if (byHeader[0x49] != 0x0 ) return false;
  if (byHeader[0x4a] != 0x0 ) return false;
  if (byHeader[0x4b] != 0x0 ) return false;

  return true;
}
