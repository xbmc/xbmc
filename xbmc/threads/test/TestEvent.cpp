/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#include "threads/Event.h"
#include "threads/Atomics.h"

#include "threads/test/TestHelpers.h"

#include <memory>
#include <stdio.h>

using namespace XbmcThreads;

//=============================================================================
// Helper classes
//=============================================================================

class waiter : public IRunnable
{
  CEvent& event;
public:
  bool& result;

  volatile bool waiting;

  waiter(CEvent& o, bool& flag) : event(o), result(flag), waiting(false) {}
  
  void Run()
  {
    waiting = true;
    result = event.Wait();
    waiting = false;
  }
};

class timed_waiter : public IRunnable
{
  CEvent& event;
  unsigned int waitTime;
public:
  int& result;

  volatile bool waiting;

  timed_waiter(CEvent& o, int& flag, int waitTimeMillis) : event(o), waitTime(waitTimeMillis), result(flag), waiting(false) {}
  
  void Run()
  {
    waiting = true;
    result = 0;
    result = event.WaitMSec(waitTime) ? 1 : -1;
    waiting = false;
  }
};

class group_wait : public IRunnable
{
  CEventGroup& event;
  int timeout;
public:
  CEvent* result;
  bool waiting;

  group_wait(CEventGroup& o) : event(o), timeout(-1), result(NULL), waiting(false) {}
  group_wait(CEventGroup& o, int timeout_) : event(o), timeout(timeout_), result(NULL), waiting(false) {}

  void Run()
  {
    waiting = true;
    if (timeout == -1)
      result = event.wait();
    else
      result = event.wait((unsigned int)timeout);
    waiting = false;
  }
};

//=============================================================================

TEST(TestEvent, General)
{
  CEvent event;
  bool result = false;
  waiter w1(event,result);
  thread waitThread(w1);

  EXPECT_TRUE(waitForWaiters(event,1,10000));

  EXPECT_TRUE(!result);

  event.Set();

  EXPECT_TRUE(waitThread.timed_join(10000));

  EXPECT_TRUE(result);
}

TEST(TestEvent, TwoWaits)
{
  CEvent event;
  bool result1 = false;
  bool result2 = false;
  waiter w1(event,result1);
  waiter w2(event,result2);
  thread waitThread1(w1);
  thread waitThread2(w2);

  EXPECT_TRUE(waitForWaiters(event,2,10000));

  EXPECT_TRUE(!result1);
  EXPECT_TRUE(!result2);

  event.Set();

  EXPECT_TRUE(waitThread1.timed_join(MILLIS(10000)));
  EXPECT_TRUE(waitThread2.timed_join(MILLIS(10000)));

  EXPECT_TRUE(result1);
  EXPECT_TRUE(result2);

}

TEST(TestEvent, TimedWaits)
{
  CEvent event;
  int result1 = 10;
  timed_waiter w1(event,result1,100);
  thread waitThread1(w1);

  EXPECT_TRUE(waitForWaiters(event,1,10000));

  EXPECT_TRUE(result1 == 0);

  event.Set();

  EXPECT_TRUE(waitThread1.timed_join(MILLIS(10000)));

  EXPECT_TRUE(result1 == 1);
}

TEST(TestEvent, TimedWaitsTimeout)
{
  CEvent event;
  int result1 = 10;
  timed_waiter w1(event,result1,50);
  thread waitThread1(w1);

  EXPECT_TRUE(waitForWaiters(event,1,100));

  EXPECT_TRUE(result1 == 0);

  EXPECT_TRUE(waitThread1.timed_join(MILLIS(10000)));

  EXPECT_TRUE(result1 == -1);
}

TEST(TestEvent, Group)
{
  CEvent event1;
  CEvent event2;

  CEventGroup group(&event1,&event2,NULL);

  bool result1 = false;
  bool result2 = false;

  waiter w1(event1,result1);
  waiter w2(event2,result2);
  group_wait w3(group);

  thread waitThread1(w1);
  thread waitThread2(w2);
  thread waitThread3(w3);

  EXPECT_TRUE(waitForWaiters(event1,1,10000));
  EXPECT_TRUE(waitForWaiters(event2,1,10000));
  EXPECT_TRUE(waitForWaiters(group,1,10000));

  EXPECT_TRUE(!result1);
  EXPECT_TRUE(!result2);

  EXPECT_TRUE(w3.waiting);
  EXPECT_TRUE(w3.result == NULL);

  event1.Set();

  EXPECT_TRUE(waitThread1.timed_join(MILLIS(10000)));
  EXPECT_TRUE(waitThread3.timed_join(MILLIS(10000)));
  SleepMillis(10);

  EXPECT_TRUE(result1);
  EXPECT_TRUE(!w1.waiting);
  EXPECT_TRUE(!result2);
  EXPECT_TRUE(w2.waiting);
  EXPECT_TRUE(!w3.waiting);
  EXPECT_TRUE(w3.result == &event1);

  event2.Set();

  EXPECT_TRUE(waitThread2.timed_join(MILLIS(10000)));

}

/* Test disabled for now, because it deadlocks
TEST(TestEvent, GroupLimitedGroupScope)
{
  CEvent event1;
  CEvent event2;

  {
    CEventGroup group(&event1,&event2,NULL);

    bool result1 = false;
    bool result2 = false;

    waiter w1(event1,result1);
    waiter w2(event2,result2);
    group_wait w3(group);

    thread waitThread1(w1);
    thread waitThread2(w2);
    thread waitThread3(w3);

    EXPECT_TRUE(waitForWaiters(event1,1,10000));
    EXPECT_TRUE(waitForWaiters(event2,1,10000));
    EXPECT_TRUE(waitForWaiters(group,1,10000));

    EXPECT_TRUE(!result1);
    EXPECT_TRUE(!result2);

    EXPECT_TRUE(w3.waiting);
    EXPECT_TRUE(w3.result == NULL);

    event1.Set();

    EXPECT_TRUE(waitThread1.timed_join(MILLIS(10000)));
    EXPECT_TRUE(waitThread3.timed_join(MILLIS(10000)));
    SleepMillis(10);

    EXPECT_TRUE(result1);
    EXPECT_TRUE(!w1.waiting);
    EXPECT_TRUE(!result2);
    EXPECT_TRUE(w2.waiting);
    EXPECT_TRUE(!w3.waiting);
    EXPECT_TRUE(w3.result == &event1);
  }

  event2.Set();

  SleepMillis(50); // give thread 2 a chance to exit
}*/

TEST(TestEvent, TwoGroups)
{
  CEvent event1;
  CEvent event2;

  CEventGroup group1(2, &event1,&event2);
  CEventGroup group2(&event1,&event2,NULL);

  bool result1 = false;
  bool result2 = false;

  waiter w1(event1,result1);
  waiter w2(event2,result2);
  group_wait w3(group1);
  group_wait w4(group2);

  thread waitThread1(w1);
  thread waitThread2(w2);
  thread waitThread3(w3);
  thread waitThread4(w4);

  EXPECT_TRUE(waitForWaiters(event1,1,10000));
  EXPECT_TRUE(waitForWaiters(event2,1,10000));
  EXPECT_TRUE(waitForWaiters(group1,1,10000));
  EXPECT_TRUE(waitForWaiters(group2,1,10000));

  EXPECT_TRUE(!result1);
  EXPECT_TRUE(!result2);

  EXPECT_TRUE(w3.waiting);
  EXPECT_EQ(w3.result,(void*)NULL);
  EXPECT_TRUE(w4.waiting);
  EXPECT_EQ(w4.result,(void*)NULL);

  event1.Set();

  EXPECT_TRUE(waitThread1.timed_join(MILLIS(10000)));
  EXPECT_TRUE(waitThread3.timed_join(MILLIS(10000)));
  EXPECT_TRUE(waitThread4.timed_join(MILLIS(10000)));
  SleepMillis(10);

  EXPECT_TRUE(result1);
  EXPECT_TRUE(!w1.waiting);
  EXPECT_TRUE(!result2);
  EXPECT_TRUE(w2.waiting);
  EXPECT_TRUE(!w3.waiting);
  EXPECT_TRUE(w3.result == &event1);
  EXPECT_TRUE(!w4.waiting);
  EXPECT_TRUE(w4.result == &event1);

  event2.Set();

  EXPECT_TRUE(waitThread2.timed_join(MILLIS(10000)));
}

TEST(TestEvent, AutoResetBehavior)
{
  CEvent event;

  EXPECT_TRUE(!event.WaitMSec(1));

  event.Set(); // event will remain signaled if there are no waits

  EXPECT_TRUE(event.WaitMSec(1));
}

TEST(TestEvent, ManualReset)
{
  CEvent event(true);
  bool result = false;
  waiter w1(event,result);
  thread waitThread(w1);

  EXPECT_TRUE(waitForWaiters(event,1,10000));

  EXPECT_TRUE(!result);

  event.Set();

  EXPECT_TRUE(waitThread.timed_join(MILLIS(10000)));

  EXPECT_TRUE(result);

  // with manual reset, the state should remain signaled
  EXPECT_TRUE(event.WaitMSec(1));

  event.Reset();

  EXPECT_TRUE(!event.WaitMSec(1));
}

TEST(TestEvent, InitVal)
{
  CEvent event(false,true);
  EXPECT_TRUE(event.WaitMSec(50));
}

TEST(TestEvent, SimpleTimeout)
{
  CEvent event;
  EXPECT_TRUE(!event.WaitMSec(50));
}

TEST(TestEvent, GroupChildSet)
{
  CEvent event1(true);
  CEvent event2;

  event1.Set();
  CEventGroup group(&event1,&event2,NULL);

  bool result1 = false;
  bool result2 = false;

  waiter w1(event1,result1);
  waiter w2(event2,result2);
  group_wait w3(group);

  thread waitThread1(w1);
  thread waitThread2(w2);
  thread waitThread3(w3);

  EXPECT_TRUE(waitForWaiters(event2,1,10000));
  EXPECT_TRUE(waitThread1.timed_join(MILLIS(10000)));
  EXPECT_TRUE(waitThread3.timed_join(MILLIS(10000)));
  SleepMillis(10);

  EXPECT_TRUE(result1);
  EXPECT_TRUE(!result2);

  EXPECT_TRUE(!w3.waiting);
  EXPECT_TRUE(w3.result == &event1);

  event2.Set();

  EXPECT_TRUE(waitThread2.timed_join(MILLIS(10000)));
}

TEST(TestEvent, GroupChildSet2)
{
  CEvent event1(true,true);
  CEvent event2;

  CEventGroup group(&event1,&event2,NULL);

  bool result1 = false;
  bool result2 = false;

  waiter w1(event1,result1);
  waiter w2(event2,result2);
  group_wait w3(group);

  thread waitThread1(w1);
  thread waitThread2(w2);
  thread waitThread3(w3);

  EXPECT_TRUE(waitForWaiters(event2,1,10000));
  EXPECT_TRUE(waitThread1.timed_join(MILLIS(10000)));
  EXPECT_TRUE(waitThread3.timed_join(MILLIS(10000)));
  SleepMillis(10);

  EXPECT_TRUE(result1);
  EXPECT_TRUE(!result2);

  EXPECT_TRUE(!w3.waiting);
  EXPECT_TRUE(w3.result == &event1);

  event2.Set();

  EXPECT_TRUE(waitThread2.timed_join(MILLIS(10000)));
}

TEST(TestEvent, GroupWaitResetsChild)
{
  CEvent event1;
  CEvent event2;

  CEventGroup group(&event1,&event2,NULL);

  group_wait w3(group);

  thread waitThread3(w3);

  EXPECT_TRUE(waitForWaiters(group,1,10000));

  EXPECT_TRUE(w3.waiting);
  EXPECT_TRUE(w3.result == NULL);

  event2.Set();

  EXPECT_TRUE(waitThread3.timed_join(MILLIS(10000)));

  EXPECT_TRUE(!w3.waiting);
  EXPECT_TRUE(w3.result == &event2);
  // event2 should have been reset.
  EXPECT_TRUE(event2.WaitMSec(1) == false);
}

TEST(TestEvent, GroupTimedWait)
{
  CEvent event1;
  CEvent event2;
  CEventGroup group(&event1,&event2,NULL);

  bool result1 = false;
  bool result2 = false;

  waiter w1(event1,result1);
  waiter w2(event2,result2);

  thread waitThread1(w1);
  thread waitThread2(w2);

  EXPECT_TRUE(waitForWaiters(event1,1,10000));
  EXPECT_TRUE(waitForWaiters(event2,1,10000));

  EXPECT_TRUE(group.wait(20) == NULL); // waited ... got nothing

  group_wait w3(group,50);
  thread waitThread3(w3);

  EXPECT_TRUE(waitForWaiters(group,1,10000));

  EXPECT_TRUE(!result1);
  EXPECT_TRUE(!result2);

  EXPECT_TRUE(w3.waiting);
  EXPECT_TRUE(w3.result == NULL);

  // this should end given the wait is for only 50 millis
  EXPECT_TRUE(waitThread3.timed_join(MILLIS(100)));

  EXPECT_TRUE(!w3.waiting);
  EXPECT_TRUE(w3.result == NULL);

  group_wait w4(group,50);
  thread waitThread4(w4);

  EXPECT_TRUE(waitForWaiters(group,1,10000));

  EXPECT_TRUE(w4.waiting);
  EXPECT_TRUE(w4.result == NULL);

  event1.Set();

  EXPECT_TRUE(waitThread1.timed_join(MILLIS(10000)));
  EXPECT_TRUE(waitThread4.timed_join(MILLIS(10000)));
  SleepMillis(10);

  EXPECT_TRUE(result1);
  EXPECT_TRUE(!result2);

  EXPECT_TRUE(!w4.waiting);
  EXPECT_TRUE(w4.result == &event1);

  event2.Set();

  EXPECT_TRUE(waitThread2.timed_join(MILLIS(10000)));
}

#define TESTNUM 100000l
#define NUMTHREADS 100l

CEvent* g_event = NULL;
volatile long g_mutex;

class mass_waiter : public IRunnable
{
public:
  CEvent& event;
  bool result;

  volatile bool waiting;

  mass_waiter() : event(*g_event), waiting(false) {}
  
  void Run()
  {
    waiting = true;
    AtomicGuard g(&g_mutex);
    result = event.Wait();
    waiting = false;
  }
};

class poll_mass_waiter : public IRunnable
{
public:
  CEvent& event;
  bool result;

  volatile bool waiting;

  poll_mass_waiter() : event(*g_event), waiting(false) {}
  
  void Run()
  {
    waiting = true;
    AtomicGuard g(&g_mutex);
    while ((result = event.WaitMSec(0)) == false);
    waiting = false;
  }
};

template <class W> void RunMassEventTest(std::shared_ptr<W>& m, bool canWaitOnEvent)
{
  std::shared_ptr<thread> t;
  t.reset(new thread[NUMTHREADS], std::default_delete<thread[]>());
  for(size_t i=0; i<NUMTHREADS; i++)
    t[i] = thread(m[i]);

  EXPECT_TRUE(waitForThread(g_mutex,NUMTHREADS,10000));
  if (canWaitOnEvent)
  {
    EXPECT_TRUE(waitForWaiters(*g_event,NUMTHREADS,10000));
  }

  SleepMillis(100);// give them a little more time

  for(size_t i=0; i<NUMTHREADS; i++)
  {
    EXPECT_TRUE(m[i].waiting);
  }

  g_event->Set();

  for(size_t i=0; i<NUMTHREADS; i++)
  {
    EXPECT_TRUE(t[i].timed_join(MILLIS(10000)));
  }

  for(size_t i=0; i<NUMTHREADS; i++)
  {
    EXPECT_TRUE(!m[i].waiting);
    EXPECT_TRUE(m[i].result);
  }
}


TEST(TestMassEvent, General)
{
  g_event = new CEvent();

  std::shared_ptr<mass_waiter> m;
  m.reset(new mass_waiter[NUMTHREADS], std::default_delete<mass_waiter[]>());

  RunMassEventTest(m,true);
  delete g_event;
}

TEST(TestMassEvent, Polling)
{
  g_event = new CEvent(true); // polling needs to avoid the auto-reset

  std::shared_ptr<poll_mass_waiter> m;
  m.reset(new poll_mass_waiter[NUMTHREADS], std::default_delete<poll_mass_waiter[]>());

  RunMassEventTest(m,false);
  delete g_event;
}

