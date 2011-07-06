#pragma once

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

#include "threads/CriticalSection.h"
#include "threads/Condition.h"
#include <boost/thread/shared_mutex.hpp>

/**
 * A CSharedSection is a mutex that satisfies the "shared lockable" concept
 * Something that implements the "Shared Lockable" concept has all of the methods
 * required by the Lockable concept and also:
 *
 * void lock_shared();
 * bool try_lock_shared();
 * void unlock_shared();
 *
 * In a nutshell a "shared lockable" satisfies the read/write lock semantics
 * where many readers can own the lock at the same time, but a writer must
 * have exclusive access. A reader would obtain "shared access" using a 
 * CSharedLock while a writer would obtain "exclusive access." using a 
 * CExclusiveLock.
 */
class CSharedSection
{
  CCriticalSection sec;
  XbmcThreads::TightConditionVariable<bool&> cond;

  unsigned int sharedCount;
  bool noShared;

public:
  inline CSharedSection() : cond(noShared), sharedCount(0), noShared(true) {}

  inline void lock() { CSingleLock l(sec); if (sharedCount) cond.wait(l); sec.lock(); }
  inline bool try_lock() { return (sec.try_lock() ? ((sharedCount == 0) ? true : (sec.unlock(), false)) : false); }
  inline void unlock() { sec.unlock(); }

  inline void lock_shared() { CSingleLock l(sec); sharedCount++; noShared = false; }
  inline bool try_lock_shared() { return (sec.try_lock() ? sharedCount++, noShared = false, sec.unlock(), true : false); }
  inline void unlock_shared() { CSingleLock l(sec); sharedCount--; if (!sharedCount) { noShared = true; cond.notifyAll(); } }
};

class CSharedLock : public boost::shared_lock<CSharedSection>
{
public:
  inline CSharedLock(CSharedSection& cs) : boost::shared_lock<CSharedSection>(cs) {}
  inline CSharedLock(const CSharedSection& cs) : boost::shared_lock<CSharedSection>((CSharedSection&)cs) {}

  inline bool IsOwner() const { return owns_lock(); }
  inline void Enter() { lock(); }
  inline void Leave() { unlock(); }
};

class CExclusiveLock : public boost::unique_lock<CSharedSection>
{
public:
  inline CExclusiveLock(CSharedSection& cs) : boost::unique_lock<CSharedSection>(cs) {}
  inline CExclusiveLock(const CSharedSection& cs) : boost::unique_lock<CSharedSection> ((CSharedSection&)cs) {}

  inline bool IsOwner() const { return owns_lock(); }
  inline void Leave() { unlock(); }
  inline void Enter() { lock(); }
};

