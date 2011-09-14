// SingleLock.h: interface for the CSingleLock class.
//
//////////////////////////////////////////////////////////////////////

/*
 * XBMC Media Center
 * Copyright (c) 2002 Frodo
 * Portions Copyright (c) by the authors of ffmpeg and xvid
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#pragma once

#include "threads/CriticalSection.h"
#include "threads/Lockables.h"

/**
 * This implements a "guard" pattern for a CCriticalSection that
 *  borrows most of it's functionality from boost's unique_lock.
 */
class CSingleLock : public XbmcThreads::UniqueLock<CCriticalSection>
{
public:
  inline CSingleLock(CCriticalSection& cs) : XbmcThreads::UniqueLock<CCriticalSection>(cs) {}
  inline CSingleLock(const CCriticalSection& cs) : XbmcThreads::UniqueLock<CCriticalSection> ((CCriticalSection&)cs) {}

  inline void Leave() { unlock(); }
  inline void Enter() { lock(); }
protected:
  inline CSingleLock(CCriticalSection& cs, bool dicrim) : XbmcThreads::UniqueLock<CCriticalSection>(cs,true) {}
};

/**
 * This implements a "guard" pattern for a CCriticalSection that
 *  works like a CSingleLock but only "try"s the lock and so
 *  it's possible it doesn't actually get it..
 */
class CSingleTryLock : public CSingleLock
{
public:
  inline CSingleTryLock(CCriticalSection& cs) : CSingleLock(cs,true) {}

  inline bool IsOwner() const { return owns_lock(); }
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
  inline CSingleExit(CCriticalSection& cs) : sec(cs), count(cs.exit()) { }
  inline ~CSingleExit() { sec.restore(count); }
};

