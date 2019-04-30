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
  m_opaque_pool = [[NSAutoreleasePool alloc] init];
}
CCocoaAutoPool::~CCocoaAutoPool()
{
  [(NSAutoreleasePool*)m_opaque_pool release];
}

void* Cocoa_Create_AutoReleasePool(void)
{
  // Original Author: Elan Feingold
	// Create an autorelease pool (necessary to call Obj-C code from non-Obj-C code)
  NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
  return pool;
}

void Cocoa_Destroy_AutoReleasePool(void* aPool)
{
  // Original Author: Elan Feingold
  NSAutoreleasePool* pool = (NSAutoreleasePool* )aPool;
  [pool release];
}
