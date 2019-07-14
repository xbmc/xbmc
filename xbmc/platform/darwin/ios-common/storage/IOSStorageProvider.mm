/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "IOSStorageProvider.h"

#include "guilib/LocalizeStrings.h"
#include "utils/StringUtils.h"

#import <Foundation/Foundation.h>

IStorageProvider* IStorageProvider::CreateInstance()
{
  return new CIOSStorageProvider();
}

void CIOSStorageProvider::GetLocalDrives(VECSOURCES& localDrives)
{
  CMediaSource share;

  // User home folder
  share.strPath = "special://envhome/";
  share.strName = g_localizeStrings.Get(21440);
  share.m_ignore = true;
  localDrives.push_back(share);

  // iOS Inbox folder
  share.strPath = "special://envhome/Documents/Inbox";
  share.strName = "Inbox";
  share.m_ignore = true;
  localDrives.push_back(share);
}

std::vector<std::string> CIOSStorageProvider::GetDiskUsage()
{
  return {};
}
