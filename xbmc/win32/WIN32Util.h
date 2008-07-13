#pragma once

/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "URL.h"

class CWIN32Util
{
public:
  CWIN32Util(void);
  virtual ~CWIN32Util(void);

  static const CStdString GetNextFreeDriveLetter();
  static CStdString MountShare(const CStdString &smbPath, const CStdString &strUser, const CStdString &strPass, DWORD *dwError=NULL);
  static CStdString MountShare(const CStdString &strPath, DWORD *dwError=NULL);
  static DWORD UmountShare(const CStdString &strPath);
  static CStdString URLEncode(const CURL &url);
  static CStdString CWIN32Util::GetLocalPath(const CStdString &strPath);
  static char CWIN32Util::FirstDriveFromMask (ULONG unitmask);
  static int CWIN32Util::GetDriveStatus(const CStdString &strPath);
  static void UpdateDriveMask();
  static CStdString GetChangedDrive();

private:
  static DWORD dwDriveMask;

};
