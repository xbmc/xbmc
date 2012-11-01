#ifndef __X_HANDLE__
#define __X_HANDLE__

/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#ifndef _WIN32

#include <list>

#include "PlatformDefs.h"
#include "XHandlePublic.h"
#include "threads/Condition.h"
#include "threads/CriticalSection.h"
#include "utils/StdString.h"

struct CXHandle {

public:
  typedef enum { HND_NULL = 0, HND_FILE, HND_EVENT, HND_MUTEX, HND_FIND_FILE } HandleType;

  CXHandle();
  CXHandle(HandleType nType);
  CXHandle(const CXHandle &src);

  virtual ~CXHandle();
  void Init();
  inline HandleType GetType() { return m_type; }
  void ChangeType(HandleType newType);

  XbmcThreads::ConditionVariable     *m_hCond;
  std::list<CXHandle*>  m_hParents;

  // simulate mutex and critical section
  CCriticalSection *m_hMutex;
  int       RecursionCount;  // for mutex - for compatibility with WIN32 critical section
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
  CCriticalSection *m_internalLock;

  static void DumpObjectTracker();

protected:
  HandleType  m_type;
  static int m_objectTracker[10];

};

#endif

#endif

