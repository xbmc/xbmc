/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

namespace XbmcThreads
{

  /**
   * This template will take any implementation of the "Lockable" concept
   * and allow it to be used as an "Exitable Lockable."
   *
   * Something that implements the "Lockable concept" simply means that
   * it has the three methods:
   *
   *   lock();
   *   try_lock();
   *   unlock();
   *
   * "Exitable" specifically means that, no matter how deep the recursion
   * on the mutex/critical section, we can exit from it and then restore
   * the state.
   *
   * This requires us to extend the Lockable so that we can keep track of the
   * number of locks that have been recursively acquired so that we can
   * undo it, and then restore that (See class CSingleExit).
   *
   * All xbmc code expects Lockables to be recursive.
   */
  template<class L> class CountingLockable
  {
    friend class ConditionVariable;

    CountingLockable(const CountingLockable&) = delete;
    CountingLockable& operator=(const CountingLockable&) = delete;
  protected:
    L mutex;
    unsigned int count = 0;

  public:
    inline CountingLockable() = default;

    // boost::thread Lockable concept
    inline void lock() { mutex.lock(); count++; }
    inline bool try_lock() { return mutex.try_lock() ? count++, true : false; }
    inline void unlock() { count--; mutex.unlock(); }

    /**
     * This implements the "exitable" behavior mentioned above.
     *
     * This can be used to ALMOST exit, but not quite, by passing
     *  the number of locks to leave. This is used in the windows
     *  ConditionVariable which requires that the lock be entered
     *  only once, and so it backs out ALMOST all the way, but
     *  leaves one still there.
     */
    inline unsigned int exit(unsigned int leave = 0)
    {
      // it's possible we don't actually own the lock
      // so we will try it.
      unsigned int ret = 0;
      if (try_lock())
      {
        if (leave < (count - 1))
        {
          ret = count - 1 - leave;  // The -1 is because we don't want
                                    //  to count the try_lock increment.
          // We must NOT compare "count" in this loop since
          // as soon as the last unlock is called another thread
          // can modify it.
          for (unsigned int i = 0; i < ret; i++)
            unlock();
        }
        unlock(); // undo the try_lock before returning
      }

      return ret;
    }

    /**
     * Restore a previous exit to the provided level.
     */
    inline void restore(unsigned int restoreCount)
    {
      for (unsigned int i = 0; i < restoreCount; i++)
        lock();
    }

    /**
     * Some implementations (see pthreads) require access to the underlying
     *  CCriticalSection, which is also implementation specific. This
     *  provides access to it through the same method on the guard classes
     *  UniqueLock, and SharedLock.
     *
     * There really should be no need for the users of the threading library
     *  to call this method.
     */
    inline L& get_underlying() { return mutex; }
  };


  /**
   * This template can be used to define the base implementation for any UniqueLock
   * (such as CSingleLock) that uses a Lockable as its mutex/critical section.
   */
  template<typename L> class UniqueLock
  {
    UniqueLock(const UniqueLock&) = delete;
    UniqueLock& operator=(const UniqueLock&) = delete;
  protected:
    L& mutex;
    bool owns;
    inline explicit UniqueLock(L& lockable) : mutex(lockable), owns(true) { mutex.lock(); }
    inline UniqueLock(L& lockable, bool try_to_lock_discrim ) : mutex(lockable) { owns = mutex.try_lock(); }
    inline ~UniqueLock() { if (owns) mutex.unlock(); }

  public:

    inline bool owns_lock() const { return owns; }

    //This also implements lockable
    inline void lock() { mutex.lock(); owns=true; }
    inline bool try_lock() { return (owns = mutex.try_lock()); }
    inline void unlock() { if (owns) { mutex.unlock(); owns=false; } }

    /**
     * See the note on the same method on CountingLockable
     */
    inline L& get_underlying() { return mutex; }
  };

  /**
   * This template can be used to define the base implementation for any SharedLock
   * (such as CSharedLock) that uses a Shared Lockable as its mutex/critical section.
   *
   * Something that implements the "Shared Lockable" concept has all of the methods
   * required by the Lockable concept and also:
   *
   * void lock_shared();
   * bool try_lock_shared();
   * void unlock_shared();
   */
  template<typename L> class SharedLock
  {
    SharedLock(const SharedLock&) = delete;
    SharedLock& operator=(const SharedLock&) = delete;
  protected:
    L& mutex;
    bool owns;
    inline explicit SharedLock(L& lockable) : mutex(lockable), owns(true) { mutex.lock_shared(); }
    inline ~SharedLock() { if (owns) mutex.unlock_shared(); }

    inline bool owns_lock() const { return owns; }
    inline void lock() { mutex.lock_shared(); owns = true; }
    inline bool try_lock() { return (owns = mutex.try_lock_shared()); }
    inline void unlock() { if (owns) mutex.unlock_shared(); owns = false; }

    /**
     * See the note on the same method on CountingLockable
     */
    inline L& get_underlying() { return mutex; }
  };


}
