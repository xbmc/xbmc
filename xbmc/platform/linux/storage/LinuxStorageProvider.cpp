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

#include "LinuxStorageProvider.h"
#include "guilib/LocalizeStrings.h"
#include "UDevProvider.h"
#ifdef HAS_DBUS
#include "UDisksProvider.h"
#include "UDisks2Provider.h"
#endif
#include "PosixMountProvider.h"

IStorageProvider* IStorageProvider::CreateInstance()
{
  return new CLinuxStorageProvider();
}

CLinuxStorageProvider::CLinuxStorageProvider()
{
  m_instance = NULL;

#ifdef HAS_DBUS
  if (CUDisks2Provider::HasUDisks2())
    m_instance = new CUDisks2Provider();
  else if (CUDisksProvider::HasUDisks())
    m_instance = new CUDisksProvider();
#endif
#ifdef HAVE_LIBUDEV
  if (m_instance == NULL)
    m_instance = new CUDevProvider();
#endif

  if (m_instance == NULL)
    m_instance = new CPosixMountProvider();
}

CLinuxStorageProvider::~CLinuxStorageProvider()
{
  delete m_instance;
}

void CLinuxStorageProvider::Initialize()
{
  m_instance->Initialize();
}

void CLinuxStorageProvider::Stop()
{
  m_instance->Stop();
}

void CLinuxStorageProvider::GetLocalDrives(VECSOURCES &localDrives)
{
  // Home directory
  CMediaSource share;
  share.strPath = getenv("HOME");
  share.strName = g_localizeStrings.Get(21440);
  share.m_ignore = true;
  share.m_iDriveType = CMediaSource::SOURCE_TYPE_LOCAL;
  localDrives.push_back(share);
  share.strPath = "/";
  share.strName = g_localizeStrings.Get(21453);
  localDrives.push_back(share);

  m_instance->GetLocalDrives(localDrives);
}

void CLinuxStorageProvider::GetRemovableDrives(VECSOURCES &removableDrives)
{
  m_instance->GetRemovableDrives(removableDrives);
}

bool CLinuxStorageProvider::Eject(const std::string& mountpath)
{
  return m_instance->Eject(mountpath);
}

std::vector<std::string> CLinuxStorageProvider::GetDiskUsage()
{
  return m_instance->GetDiskUsage();
}

bool CLinuxStorageProvider::PumpDriveChangeEvents(IStorageEventsCallback *callback)
{
  return m_instance->PumpDriveChangeEvents(callback);
}
