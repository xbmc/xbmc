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
#include <boost/thread/shared_mutex.hpp>

/**
 * A CSharedSection is a CountingLockable whose implementation is a boost
 *  shared_mutex.
 *
 * It implemented boost's shared Locakable concept which requires the 
 *  additional implementation of:
 *
 * lock_shared()
 * try_lock_shared()
 * unlock_shared()
 */
class CSharedSection : public CountingLockable<boost::shared_mutex>
{
public:

  inline void lock_shared() { mutex.lock_shared(); }
  inline bool try_lock_shared() { return mutex.try_lock_shared(); }
  inline void unlock_shared() { return mutex.unlock_shared(); }
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

