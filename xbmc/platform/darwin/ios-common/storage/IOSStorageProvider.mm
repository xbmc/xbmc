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

std::unique_ptr<IStorageProvider> IStorageProvider::CreateInstance()
{
  return std::make_unique<CIOSStorageProvider>();
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
  std::vector<std::string> result;
  auto fileSystemAttributes =
      [NSFileManager.defaultManager attributesOfFileSystemForPath:NSHomeDirectory() error:nil];

  auto formatter = [NSByteCountFormatter new];
  formatter.includesActualByteCount = YES;
  formatter.zeroPadsFractionDigits = YES;

  for (const auto& pair :
       {std::make_pair(NSFileSystemFreeSize, 160), std::make_pair(NSFileSystemSize, 20161)})
    if (auto sizeStr = [formatter stringForObjectValue:fileSystemAttributes[pair.first]])
      result.push_back(
          StringUtils::Format("{}: {}", g_localizeStrings.Get(pair.second), sizeStr.UTF8String));

  return result;
}
