/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "LinuxStorageProvider.h"
#include "guilib/LocalizeStrings.h"
#include "UDevProvider.h"
#ifdef HAS_DBUS
#include "UDisksProvider.h"
#include "UDisks2Provider.h"
#endif
#include "platform/posix/PosixMountProvider.h"

std::unique_ptr<IStorageProvider> IStorageProvider::CreateInstance()
{
  return std::make_unique<CLinuxStorageProvider>();
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
