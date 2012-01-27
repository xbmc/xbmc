/*
 *      Copyright (C) 2005-2009 Team XBMC
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
#include "Win32StorageProvider.h"
#include "WIN32Util.h"
#include "guilib/LocalizeStrings.h"
#include "filesystem/SpecialProtocol.h"

bool CWin32StorageProvider::event = false;

void CWin32StorageProvider::GetLocalDrives(VECSOURCES &localDrives)
{
  CMediaSource share;
  share.strPath = _P("special://home");
  share.strName = g_localizeStrings.Get(21440);
  share.m_ignore = true;
  share.m_iDriveType = CMediaSource::SOURCE_TYPE_LOCAL;
  localDrives.push_back(share);

  CWIN32Util::GetDrivesByType(localDrives, LOCAL_DRIVES);
}

void CWin32StorageProvider::GetRemovableDrives(VECSOURCES &removableDrives)
{
  CWIN32Util::GetDrivesByType(removableDrives, REMOVABLE_DRIVES, true);
}

bool CWin32StorageProvider::Eject(CStdString mountpath)
{
  if (!mountpath.IsEmpty())
  {
    return CWIN32Util::EjectDrive(mountpath[0]);
  }
  return false;
}

std::vector<CStdString> CWin32StorageProvider::GetDiskUsage()
{
  return CWIN32Util::GetDiskUsage();
}

bool CWin32StorageProvider::PumpDriveChangeEvents(IStorageEventsCallback *callback)
{
  bool b = event;
  event = false;
  return b;
}
