/*
 *  Copyright (c) 2002 Frodo
 *      Portions Copyright (c) by the authors of ffmpeg and xvid
 *  Copyright (C) 2002-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

// SingleLock.h: interface for the CSingleLock class.
//
//////////////////////////////////////////////////////////////////////

#include "threads/CriticalSection.h"

#include <mutex>

/**
 * This implements a "guard" pattern for a CCriticalSection that
 *  borrows most of it's functionality from boost's unique_lock.
 */
class CSingleLock : public std::unique_lock<CCriticalSection>
{
public:
  inline explicit CSingleLock(CCriticalSection& cs) : std::unique_lock<CCriticalSection>(cs) {}

  inline void Leave() { unlock(); }
  inline void Enter() { lock(); }

private:
  CSingleLock(const CSingleLock&) = delete;
  CSingleLock& operator=(const CSingleLock&) = delete;
};


/**
 * This implements a "guard" pattern for exiting all locks
 *  currently being held by the current thread and restoring
 *  those locks on destruction.
 *
 * This class can be used on a CCriticalSection that isn't owned
 *  by this thread in which case it will do nothing.
 */
class CSingleExit
{
  CCriticalSection& sec;
  unsigned int count;
public:
  inline explicit CSingleExit(CCriticalSection& cs) : sec(cs), count(cs.exit()) { }
  inline ~CSingleExit() { sec.restore(count); }
};

