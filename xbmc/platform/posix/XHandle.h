/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <list>
#include <string>
#include <vector>

#include "PlatformDefs.h"
#include "XHandlePublic.h"
#include "threads/Condition.h"
#include "threads/CriticalSection.h"

struct CXHandle {

public:
  typedef enum { HND_NULL = 0, HND_FILE, HND_EVENT, HND_MUTEX, HND_FIND_FILE } HandleType;

  CXHandle();
  explicit CXHandle(HandleType nType);
  CXHandle(const CXHandle &src);

  virtual ~CXHandle();
  void Init();
  inline HandleType GetType() { return m_type; }
  void ChangeType(HandleType newType);

  XbmcThreads::ConditionVariable     *m_hCond;
  std::list<CXHandle*>  m_hParents;

  // simulate mutex and critical section
  CCriticalSection *m_hMutex;
  int       RecursionCount;  // for mutex - for compatibility with TARGET_WINDOWS critical section
  int       fd;
  bool      m_bManualEvent;
  time_t    m_tmCreation;
  std::vector<std::string> m_FindFileResults;
  int              m_nFindFileIterator;
  std::string      m_FindFileDir;
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
