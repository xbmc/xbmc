/*
 *      Copyright (C) 2005-2011 Team XBMC
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

#include "threads/SharedSection.h"
#include "threads/SingleLock.h"
#include "threads/Event.h"
#include "threads/Atomics.h"
#include "threads/test/TestHelpers.h"

#include <stdio.h>

//=============================================================================
// Helper classes
//=============================================================================

template<class L>
class locker
{
  CSharedSection& sec;
  CEvent* wait;

  volatile long* mutex;
public:
  volatile bool haslock;
  volatile bool obtainedlock;

  inline locker(CSharedSection& o, volatile long* mutex_ = NULL, CEvent* wait_ = NULL) : 
    sec(o), wait(wait_), mutex(mutex_), haslock(false), obtainedlock(false) {}
  
  inline locker(CSharedSection& o, CEvent* wait_ = NULL) : 
    sec(o), wait(wait_), mutex(NULL), haslock(false), obtainedlock(false) {}
  
  void operator()()
  {
    AtomicGuard g(mutex);
    L lock(sec);
    haslock = true;
    obtainedlock = true;
    if (wait)
      wait->Wait();
    haslock = false;
  }
};

TEST(TestCritSectionCase)
{
  CCriticalSection sec;

  CSingleLock l1(sec);
  CSingleLock l2(sec);
}

TEST(TestSharedSectionCase)
{
  CSharedSection sec;

  CSharedLock l1(sec);
  CSharedLock l2(sec);
}

TEST(TestGetSharedLockWhileTryingExclusiveLock)
{
  volatile long mutex = 0;
  CEvent event;

  CSharedSection sec;

  CSharedLock l1(sec); // get a shared lock

  locker<CExclusiveLock> l2(sec,&mutex);
  thread waitThread1(ref(l2)); // try to get an exclusive lock

  CHECK(waitForThread(mutex,1,10000));
  SleepMillis(10);  // still need to give it a chance to move ahead

  CHECK(!l2.haslock);  // this thread is waiting ...
  CHECK(!l2.obtainedlock);  // this thread is waiting ...

  // now try and get a SharedLock
  locker<CSharedLock> l3(sec,&mutex,&event);
  thread waitThread3(ref(l3)); // try to get a shared lock
  CHECK(waitForThread(mutex,2,10000));
  SleepMillis(10);
  CHECK(l3.haslock);

  event.Set();
  CHECK(waitThread3.timed_join(MILLIS(10000)));

  // l3 should have released.
  CHECK(!l3.haslock);

  // but the exclusive lock should still not have happened
  CHECK(!l2.haslock);  // this thread is waiting ...
  CHECK(!l2.obtainedlock);  // this thread is waiting ...

  // let it go
  l1.Leave(); // the last shared lock leaves.

  CHECK(waitThread1.timed_join(MILLIS(10000)));

  CHECK(l2.obtainedlock);  // the exclusive lock was captured
  CHECK(!l2.haslock);  // ... but it doesn't have it anymore
}

TEST(TestSharedSection2Case)
{
  CSharedSection sec;

  CEvent event;
  volatile long mutex = 0;

  locker<CSharedLock> l1(sec,&mutex,&event);

  {
    CSharedLock lock(sec);
    thread waitThread1(ref(l1));

    CHECK(waitForWaiters(event,1,10000));
    CHECK(l1.haslock);

    event.Set();

    CHECK(waitThread1.timed_join(MILLIS(10000)));
  }

  locker<CSharedLock> l2(sec,&mutex,&event);
  {
    CExclusiveLock lock(sec); // get exclusive lock
    thread waitThread2(ref(l2)); // thread should block

    CHECK(waitForThread(mutex,1,10000));
    SleepMillis(10);

    CHECK(!l2.haslock);

    lock.Leave();

    CHECK(waitForWaiters(event,1,10000));
    SleepMillis(10);
    CHECK(l2.haslock);

    event.Set();
    
    CHECK(waitThread2.timed_join(MILLIS(10000)));
  }
}

TEST(TestMultipleSharedSectionCase)
{
  CSharedSection sec;

  CEvent event;
  volatile long mutex = 0;

  locker<CSharedLock> l1(sec,&mutex, &event);

  {
    CSharedLock lock(sec);
    thread waitThread1(ref(l1));

    CHECK(waitForThread(mutex,1,10000));
    SleepMillis(10);

    CHECK(l1.haslock);

    event.Set();

    CHECK(waitThread1.timed_join(MILLIS(10000)));
  }

  locker<CSharedLock> l2(sec,&mutex,&event);
  locker<CSharedLock> l3(sec,&mutex,&event);
  locker<CSharedLock> l4(sec,&mutex,&event);
  locker<CSharedLock> l5(sec,&mutex,&event);
  {
    CExclusiveLock lock(sec);
    thread waitThread1(ref(l2));
    thread waitThread2(ref(l3));
    thread waitThread3(ref(l4));
    thread waitThread4(ref(l5));

    CHECK(waitForThread(mutex,4,10000));
    SleepMillis(10);

    CHECK(!l2.haslock);
    CHECK(!l3.haslock);
    CHECK(!l4.haslock);
    CHECK(!l5.haslock);

    lock.Leave();

    CHECK(waitForWaiters(event,4,10000));

    CHECK(l2.haslock);
    CHECK(l3.haslock);
    CHECK(l4.haslock);
    CHECK(l5.haslock);

    event.Set();
    
    CHECK(waitThread1.timed_join(MILLIS(10000)));
    CHECK(waitThread2.timed_join(MILLIS(10000)));
    CHECK(waitThread3.timed_join(MILLIS(10000)));
    CHECK(waitThread4.timed_join(MILLIS(10000)));
  }
}

