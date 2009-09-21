#ifndef _XCRITICAL_SECTION_H_
#define _XCRITICAL_SECTION_H_

/*
 *      Copyright (C) 2005-2008 Team XBMC
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

#ifdef _LINUX
#include "PlatformDefs.h"
#include "linux/XSyncUtils.h"
#endif

class XCriticalSection
{
 public:

  // Constructor/destructor.
  XCriticalSection();
  virtual ~XCriticalSection();

  // Initialize a critical section.
  void Initialize();

  // Destroy a critical section.
  void Destroy();

  // Enter a critical section.
  void Enter();

  // Leave a critical section.
  void Leave();

  // Checks if current thread owns the critical section.
  BOOL Owning();

  // Exits a critical section.
  DWORD Exit();

  // Restores critical section count.
  void Restore(DWORD count);

 private:

  ThreadIdentifier  m_ownerThread;
  pthread_mutex_t   m_mutex;
  pthread_mutex_t   m_countMutex;
  int               m_count;
  bool              m_isDestroyed;
  bool              m_isInitialized;
};

// Define the C API.
void  InitializeCriticalSection(XCriticalSection* section);
void  DeleteCriticalSection(XCriticalSection* section);
BOOL  OwningCriticalSection(XCriticalSection* section);
DWORD ExitCriticalSection(XCriticalSection* section);
void  RestoreCriticalSection(XCriticalSection* section, DWORD count);
void  EnterCriticalSection(XCriticalSection* section);
void  LeaveCriticalSection(XCriticalSection* section);

#endif
