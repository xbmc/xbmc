/*
*  Copyright (C) 2010-2020 Team Kodi
*  This file is part of Kodi - https://kodi.tv
*
*  SPDX-License-Identifier: GPL-2.0-or-later
*  See LICENSES/README.md for more information.
*/

#import "DarwinEmbedUtils.h"

#include <mutex>
#include <string>

#import <Foundation/Foundation.h>

const char* CDarwinEmbedUtils::GetAppRootFolder(void)
{
  static std::string rootFolder;
  static std::once_flag flag;
  std::call_once(flag, [] {
    if (IsIosSandboxed())
    {
#ifdef TARGET_DARWIN_TVOS
      // writing to Documents is prohibited, more info:
      // https://developer.apple.com/library/archive/documentation/General/Conceptual/AppleTV_PG/index.html#//apple_ref/doc/uid/TP40015241-CH12-SW5
      // https://forums.developer.apple.com/thread/89008
      rootFolder = "Library/Caches";
#else
      // when we are sandbox make documents our root
      // so that user can access everything he needs
      // via itunes sharing
      rootFolder = "Documents";
#endif
    }
    else
    {
      rootFolder = "Library/Preferences";
    }
  });
  return rootFolder.c_str();
}

bool CDarwinEmbedUtils::IsIosSandboxed(void)
{
  static bool ret;
  static std::once_flag flag;
  std::call_once(flag, [] {
    auto const sandboxPrefixPath = @"/private/var/containers/Bundle/";
    ret = [NSBundle.mainBundle.executablePath hasPrefix:sandboxPrefixPath];
  });
  return ret;
}
