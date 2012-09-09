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
class locker : public IRunnable
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
  
  void Run()
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

TEST(TestCritSection, General)
{
  CCriticalSection sec;

  CSingleLock l1(sec);
  CSingleLock l2(sec);
}

TEST(TestSharedSection, General)
{
  CSharedSection sec;

  CSharedLock l1(sec);
  CSharedLock l2(sec);
}

TEST(TestSharedSection, GetSharedLockWhileTryingExclusiveLock)
{
  volatile long mutex = 0;
  CEvent event;

  CSharedSection sec;

  CSharedLock l1(sec); // get a shared lock

  locker<CExclusiveLock> l2(sec,&mutex);
  thread waitThread1(l2); // try to get an exclusive lock

  EXPECT_TRUE(waitForThread(mutex,1,10000));
  SleepMillis(10);  // still need to give it a chance to move ahead

  EXPECT_TRUE(!l2.haslock);  // this thread is waiting ...
  EXPECT_TRUE(!l2.obtainedlock);  // this thread is waiting ...

  // now try and get a SharedLock
  locker<CSharedLock> l3(sec,&mutex,&event);
  thread waitThread3(l3); // try to get a shared lock
  EXPECT_TRUE(waitForThread(mutex,2,10000));
  SleepMillis(10);
  EXPECT_TRUE(l3.haslock);

  event.Set();
  EXPECT_TRUE(waitThread3.timed_join(MILLIS(10000)));

  // l3 should have released.
  EXPECT_TRUE(!l3.haslock);

  // but the exclusive lock should still not have happened
  EXPECT_TRUE(!l2.haslock);  // this thread is waiting ...
  EXPECT_TRUE(!l2.obtainedlock);  // this thread is waiting ...

  // let it go
  l1.Leave(); // the last shared lock leaves.

  EXPECT_TRUE(waitThread1.timed_join(MILLIS(10000)));

  EXPECT_TRUE(l2.obtainedlock);  // the exclusive lock was captured
  EXPECT_TRUE(!l2.haslock);  // ... but it doesn't have it anymore
}

TEST(TestSharedSection, TwoCase)
{
  CSharedSection sec;

  CEvent event;
  volatile long mutex = 0;

  locker<CSharedLock> l1(sec,&mutex,&event);

  {
    CSharedLock lock(sec);
    thread waitThread1(l1);

    EXPECT_TRUE(waitForWaiters(event,1,10000));
    EXPECT_TRUE(l1.haslock);

    event.Set();

    EXPECT_TRUE(waitThread1.timed_join(MILLIS(10000)));
  }

  locker<CSharedLock> l2(sec,&mutex,&event);
  {
    CExclusiveLock lock(sec); // get exclusive lock
    thread waitThread2(l2); // thread should block

    EXPECT_TRUE(waitForThread(mutex,1,10000));
    SleepMillis(10);

    EXPECT_TRUE(!l2.haslock);

    lock.Leave();

    EXPECT_TRUE(waitForWaiters(event,1,10000));
    SleepMillis(10);
    EXPECT_TRUE(l2.haslock);

    event.Set();
    
    EXPECT_TRUE(waitThread2.timed_join(MILLIS(10000)));
  }
}

TEST(TestMultipleSharedSection, General)
{
  CSharedSection sec;

  CEvent event;
  volatile long mutex = 0;

  locker<CSharedLock> l1(sec,&mutex, &event);

  {
    CSharedLock lock(sec);
    thread waitThread1(l1);

    EXPECT_TRUE(waitForThread(mutex,1,10000));
    SleepMillis(10);

    EXPECT_TRUE(l1.haslock);

    event.Set();

    EXPECT_TRUE(waitThread1.timed_join(MILLIS(10000)));
  }

  locker<CSharedLock> l2(sec,&mutex,&event);
  locker<CSharedLock> l3(sec,&mutex,&event);
  locker<CSharedLock> l4(sec,&mutex,&event);
  locker<CSharedLock> l5(sec,&mutex,&event);
  {
    CExclusiveLock lock(sec);
    thread waitThread1(l2);
    thread waitThread2(l3);
    thread waitThread3(l4);
    thread waitThread4(l5);

    EXPECT_TRUE(waitForThread(mutex,4,10000));
    SleepMillis(10);

    EXPECT_TRUE(!l2.haslock);
    EXPECT_TRUE(!l3.haslock);
    EXPECT_TRUE(!l4.haslock);
    EXPECT_TRUE(!l5.haslock);

    lock.Leave();

    EXPECT_TRUE(waitForWaiters(event,4,10000));

    EXPECT_TRUE(l2.haslock);
    EXPECT_TRUE(l3.haslock);
    EXPECT_TRUE(l4.haslock);
    EXPECT_TRUE(l5.haslock);

    event.Set();
    
    EXPECT_TRUE(waitThread1.timed_join(MILLIS(10000)));
    EXPECT_TRUE(waitThread2.timed_join(MILLIS(10000)));
    EXPECT_TRUE(waitThread3.timed_join(MILLIS(10000)));
    EXPECT_TRUE(waitThread4.timed_join(MILLIS(10000)));
  }
}

