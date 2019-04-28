/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#import "TVOSFileUtils.h"

#import "CompileInfo.h"
#import "utils/URIUtils.h"

#import <mutex>

#import <Foundation/Foundation.h>

const char* CTVOSFileUtils::GetUserHomeDirectory()
{
  static std::string appHomeFolder;
  static std::once_flag dir_flag;

  std::call_once(dir_flag, [] {
    appHomeFolder = URIUtils::AddFileToFolder(GetOSCachesDirectory(), CCompileInfo::GetAppName());
  });

  return appHomeFolder.c_str();
}

const char* CTVOSFileUtils::GetOSCachesDirectory()
{
  static std::string cacheFolder;
  static std::once_flag cache_flag;

  std::call_once(cache_flag, [] {
    NSString* cachePath =
        NSSearchPathForDirectoriesInDomains(NSCachesDirectory, NSUserDomainMask, YES).lastObject;
    cacheFolder = cachePath.UTF8String;
    URIUtils::RemoveSlashAtEnd(cacheFolder);
  });
  return cacheFolder.c_str();
}
