#ifndef __X_HANDLE__
#define __X_HANDLE__

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

#ifndef _WIN32

#include <list>
#include <pthread.h>

#include "PlatformDefs.h"
#include "XHandlePublic.h"
#include "threads/Semaphore.hpp"
#include "threads/XBMC_mutex.h"
#include "utils/StdString.h"

struct CXHandle {

public:
  typedef enum { HND_NULL = 0, HND_FILE, HND_EVENT, HND_MUTEX, HND_THREAD, HND_FIND_FILE } HandleType;

  CXHandle();
  CXHandle(HandleType nType);
  CXHandle(const CXHandle &src);

  virtual ~CXHandle();
  void Init();
  inline HandleType GetType() { return m_type; }
  void ChangeType(HandleType newType);

  CSemaphore            *m_pSem;
  ThreadIdentifier      m_hThread;
  bool                  m_threadValid;
  SDL_cond              *m_hCond;
  std::list<CXHandle*>  m_hParents;

#ifdef __APPLE__
  // Save the Mach thrad port, I don't think it can be obtained from
  // the pthread_t. We'll use it for querying timer information.
  //
  mach_port_t m_machThreadPort;
#endif

  // simulate mutex and critical section
  SDL_mutex *m_hMutex;
  int       RecursionCount;  // for mutex - for compatibility with WIN32 critical section
  pthread_t OwningThread;
  int       fd;
  bool      m_bManualEvent;
  time_t    m_tmCreation;
  CStdStringArray  m_FindFileResults;
  int              m_nFindFileIterator;
  CStdString       m_FindFileDir;
  off64_t          m_iOffset;
  bool             m_bCDROM;
  bool             m_bEventSet;
  int              m_nRefCount;
  SDL_mutex *m_internalLock;

  static void DumpObjectTracker();

protected:
  HandleType  m_type;
  static int m_objectTracker[10];

};

#endif

#endif

