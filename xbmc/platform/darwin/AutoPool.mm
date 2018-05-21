/*
 *      Copyright (C) 2009-present Team Kodi
 *      http://xbmc.org
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#if defined(TARGET_DARWIN)
#if defined(TARGET_DARWIN_IOS)
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
#endif
