/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "threads/Event.h"
#include "threads/IRunnable.h"
#include "threads/SharedSection.h"
#include "threads/SingleLock.h"
#include "threads/test/TestHelpers.h"

#include <stdio.h>

using namespace std::chrono_literals;

//=============================================================================
// Helper classes
//=============================================================================

template<class L>
class locker : public IRunnable
{
  CSharedSection& sec;
  CEvent* wait;

  std::atomic<long>* mutex;
public:
  volatile bool haslock;
  volatile bool obtainedlock;

  inline locker(CSharedSection& o, std::atomic<long>* mutex_ = NULL, CEvent* wait_ = NULL) :
    sec(o), wait(wait_), mutex(mutex_), haslock(false), obtainedlock(false) {}

  inline locker(CSharedSection& o, CEvent* wait_ = NULL) :
    sec(o), wait(wait_), mutex(NULL), haslock(false), obtainedlock(false) {}

  void Run() override
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
  std::atomic<long> mutex(0L);
  CEvent event;

  CSharedSection sec;

  CSharedLock l1(sec); // get a shared lock

  locker<CExclusiveLock> l2(sec,&mutex);
  thread waitThread1(l2); // try to get an exclusive lock

  EXPECT_TRUE(waitForThread(mutex, 1, 10000ms));
  std::this_thread::sleep_for(10ms); // still need to give it a chance to move ahead

  EXPECT_TRUE(!l2.haslock);  // this thread is waiting ...
  EXPECT_TRUE(!l2.obtainedlock);  // this thread is waiting ...

  // now try and get a SharedLock
  locker<CSharedLock> l3(sec,&mutex,&event);
  thread waitThread3(l3); // try to get a shared lock
  EXPECT_TRUE(waitForThread(mutex, 2, 10000ms));
  std::this_thread::sleep_for(10ms);
  EXPECT_TRUE(l3.haslock);

  event.Set();
  EXPECT_TRUE(waitThread3.timed_join(10000ms));

  // l3 should have released.
  EXPECT_TRUE(!l3.haslock);

  // but the exclusive lock should still not have happened
  EXPECT_TRUE(!l2.haslock);  // this thread is waiting ...
  EXPECT_TRUE(!l2.obtainedlock);  // this thread is waiting ...

  // let it go
  l1.Leave(); // the last shared lock leaves.

  EXPECT_TRUE(waitThread1.timed_join(10000ms));

  EXPECT_TRUE(l2.obtainedlock);  // the exclusive lock was captured
  EXPECT_TRUE(!l2.haslock);  // ... but it doesn't have it anymore
}

TEST(TestSharedSection, TwoCase)
{
  CSharedSection sec;

  CEvent event;
  std::atomic<long> mutex(0L);

  locker<CSharedLock> l1(sec,&mutex,&event);

  {
    CSharedLock lock(sec);
    thread waitThread1(l1);

    EXPECT_TRUE(waitForWaiters(event, 1, 10000ms));
    EXPECT_TRUE(l1.haslock);

    event.Set();

    EXPECT_TRUE(waitThread1.timed_join(10000ms));
  }

  locker<CSharedLock> l2(sec,&mutex,&event);
  {
    CExclusiveLock lock(sec); // get exclusive lock
    thread waitThread2(l2); // thread should block

    EXPECT_TRUE(waitForThread(mutex, 1, 10000ms));
    std::this_thread::sleep_for(10ms);

    EXPECT_TRUE(!l2.haslock);

    lock.Leave();

    EXPECT_TRUE(waitForWaiters(event, 1, 10000ms));
    std::this_thread::sleep_for(10ms);
    EXPECT_TRUE(l2.haslock);

    event.Set();

    EXPECT_TRUE(waitThread2.timed_join(10000ms));
  }
}

TEST(TestMultipleSharedSection, General)
{
  CSharedSection sec;

  CEvent event;
  std::atomic<long> mutex(0L);

  locker<CSharedLock> l1(sec,&mutex, &event);

  {
    CSharedLock lock(sec);
    thread waitThread1(l1);

    EXPECT_TRUE(waitForThread(mutex, 1, 10000ms));
    std::this_thread::sleep_for(10ms);

    EXPECT_TRUE(l1.haslock);

    event.Set();

    EXPECT_TRUE(waitThread1.timed_join(10000ms));
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

    EXPECT_TRUE(waitForThread(mutex, 4, 10000ms));
    std::this_thread::sleep_for(10ms);

    EXPECT_TRUE(!l2.haslock);
    EXPECT_TRUE(!l3.haslock);
    EXPECT_TRUE(!l4.haslock);
    EXPECT_TRUE(!l5.haslock);

    lock.Leave();

    EXPECT_TRUE(waitForWaiters(event, 4, 10000ms));

    EXPECT_TRUE(l2.haslock);
    EXPECT_TRUE(l3.haslock);
    EXPECT_TRUE(l4.haslock);
    EXPECT_TRUE(l5.haslock);

    event.Set();

    EXPECT_TRUE(waitThread1.timed_join(10000ms));
    EXPECT_TRUE(waitThread2.timed_join(10000ms));
    EXPECT_TRUE(waitThread3.timed_join(10000ms));
    EXPECT_TRUE(waitThread4.timed_join(10000ms));
  }
}

