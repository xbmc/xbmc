/*
 *  Copyright (C) 2009-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#if defined(TARGET_DARWIN_EMBEDDED)
  #import <Foundation/Foundation.h>
#else
  #import <Cocoa/Cocoa.h>
#endif

#import "platform/darwin/AutoPool.h"

CCocoaAutoPool::CCocoaAutoPool()
{
}
CCocoaAutoPool::~CCocoaAutoPool()
{
}

void* Cocoa_Create_AutoReleasePool(void)
{
  return nullptr;
}

void Cocoa_Destroy_AutoReleasePool(void* aPool)
{
}
