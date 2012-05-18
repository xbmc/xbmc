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

#include "threads/Event.h"
#include "threads/Atomics.h"

#include "threads/test/TestHelpers.h"

#include <boost/shared_array.hpp>
#include <stdio.h>

using namespace XbmcThreads;

//=============================================================================
// Helper classes
//=============================================================================

class waiter
{
  CEvent& event;
public:
  bool& result;

  volatile bool waiting;

  waiter(CEvent& o, bool& flag) : event(o), result(flag), waiting(false) {}
  
  void operator()()
  {
    waiting = true;
    result = event.Wait();
    waiting = false;
  }
};

class timed_waiter
{
  CEvent& event;
  unsigned int waitTime;
public:
  int& result;

  volatile bool waiting;

  timed_waiter(CEvent& o, int& flag, int waitTimeMillis) : event(o), waitTime(waitTimeMillis), result(flag), waiting(false) {}
  
  void operator()()
  {
    waiting = true;
    result = 0;
    result = event.WaitMSec(waitTime) ? 1 : -1;
    waiting = false;
  }
};

class group_wait
{
  CEventGroup& event;
  int timeout;
public:
  CEvent* result;
  bool waiting;

  group_wait(CEventGroup& o) : event(o), timeout(-1), result(NULL), waiting(false) {}
  group_wait(CEventGroup& o, int timeout_) : event(o), timeout(timeout_), result(NULL), waiting(false) {}

  void operator()()
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

TEST(TestEventCase)
{
  CEvent event;
  bool result = false;
  waiter w1(event,result);
  thread waitThread(w1);

  CHECK(waitForWaiters(event,1,10000));

  CHECK(!result);

  event.Set();

  CHECK(waitThread.timed_join(10000));

  CHECK(result);
}

TEST(TestEvent2WaitsCase)
{
  CEvent event;
  bool result1 = false;
  bool result2 = false;
  waiter w1(event,result1);
  waiter w2(event,result2);
  thread waitThread1(w1);
  thread waitThread2(w2);

  CHECK(waitForWaiters(event,2,10000));

  CHECK(!result1);
  CHECK(!result2);

  event.Set();

  CHECK(waitThread1.timed_join(MILLIS(10000)));
  CHECK(waitThread2.timed_join(MILLIS(10000)));

  CHECK(result1);
  CHECK(result2);

}

TEST(TestEventTimedWaitsCase)
{
  CEvent event;
  int result1 = 10;
  timed_waiter w1(event,result1,100);
  thread waitThread1(w1);

  CHECK(waitForWaiters(event,1,10000));

  CHECK(result1 == 0);

  event.Set();

  CHECK(waitThread1.timed_join(MILLIS(10000)));

  CHECK(result1 == 1);
}

TEST(TestEventTimedWaitsTimeoutCase)
{
  CEvent event;
  int result1 = 10;
  timed_waiter w1(event,result1,50);
  thread waitThread1(w1);

  CHECK(waitForWaiters(event,1,100));

  CHECK(result1 == 0);

  CHECK(waitThread1.timed_join(MILLIS(10000)));

  CHECK(result1 == -1);
}

TEST(TestEventGroupCase)
{
  CEvent event1;
  CEvent event2;

  CEventGroup group(&event1,&event2,NULL);

  bool result1 = false;
  bool result2 = false;

  waiter w1(event1,result1);
  waiter w2(event2,result2);
  group_wait w3(group);

  thread waitThread1(ref(w1));
  thread waitThread2(ref(w2));
  thread waitThread3(ref(w3));

  CHECK(waitForWaiters(event1,1,10000));
  CHECK(waitForWaiters(event2,1,10000));
  CHECK(waitForWaiters(group,1,10000));

  CHECK(!result1);
  CHECK(!result2);

  CHECK(w3.waiting);
  CHECK(w3.result == NULL);

  event1.Set();

  CHECK(waitThread1.timed_join(MILLIS(10000)));
  CHECK(waitThread3.timed_join(MILLIS(10000)));
  SleepMillis(10);

  CHECK(result1);
  CHECK(!w1.waiting);
  CHECK(!result2);
  CHECK(w2.waiting);
  CHECK(!w3.waiting);
  CHECK(w3.result == &event1);

  event2.Set();

  CHECK(waitThread2.timed_join(MILLIS(10000)));

}

TEST(TestEventGroupLimitedGroupScopeCase)
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

    thread waitThread1(ref(w1));
    thread waitThread2(ref(w2));
    thread waitThread3(ref(w3));

    CHECK(waitForWaiters(event1,1,10000));
    CHECK(waitForWaiters(event2,1,10000));
    CHECK(waitForWaiters(group,1,10000));

    CHECK(!result1);
    CHECK(!result2);

    CHECK(w3.waiting);
    CHECK(w3.result == NULL);

    event1.Set();

    CHECK(waitThread1.timed_join(MILLIS(10000)));
    CHECK(waitThread3.timed_join(MILLIS(10000)));
    SleepMillis(10);

    CHECK(result1);
    CHECK(!w1.waiting);
    CHECK(!result2);
    CHECK(w2.waiting);
    CHECK(!w3.waiting);
    CHECK(w3.result == &event1);
  }

  event2.Set();

  SleepMillis(50); // give thread 2 a chance to exit
}

TEST(TestEvent2GroupsCase)
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

  thread waitThread1(ref(w1));
  thread waitThread2(ref(w2));
  thread waitThread3(ref(w3));
  thread waitThread4(ref(w4));

  CHECK(waitForWaiters(event1,1,10000));
  CHECK(waitForWaiters(event2,1,10000));
  CHECK(waitForWaiters(group1,1,10000));
  CHECK(waitForWaiters(group2,1,10000));

  CHECK(!result1);
  CHECK(!result2);

  CHECK(w3.waiting);
  CHECK_EQUAL(w3.result,(void*)NULL);
  CHECK(w4.waiting);
  CHECK_EQUAL(w4.result,(void*)NULL);

  event1.Set();

  CHECK(waitThread1.timed_join(MILLIS(10000)));
  CHECK(waitThread3.timed_join(MILLIS(10000)));
  CHECK(waitThread4.timed_join(MILLIS(10000)));
  SleepMillis(10);

  CHECK(result1);
  CHECK(!w1.waiting);
  CHECK(!result2);
  CHECK(w2.waiting);
  CHECK(!w3.waiting);
  CHECK(w3.result == &event1);
  CHECK(!w4.waiting);
  CHECK(w4.result == &event1);

  event2.Set();

  CHECK(waitThread2.timed_join(MILLIS(10000)));
}

TEST(TestEventAutoResetBehavior)
{
  CEvent event;

  CHECK(!event.WaitMSec(1));

  event.Set(); // event will remain signaled if there are no waits

  CHECK(event.WaitMSec(1));
}

TEST(TestEventManualResetCase)
{
  CEvent event(true);
  bool result = false;
  waiter w1(event,result);
  thread waitThread(w1);

  CHECK(waitForWaiters(event,1,10000));

  CHECK(!result);

  event.Set();

  CHECK(waitThread.timed_join(MILLIS(10000)));

  CHECK(result);

  // with manual reset, the state should remain signaled
  CHECK(event.WaitMSec(1));

  event.Reset();

  CHECK(!event.WaitMSec(1));
}

TEST(TestEventInitValCase)
{
  CEvent event(false,true);
  CHECK(event.WaitMSec(50));
}

TEST(TestEventSimpleTimeoutCase)
{
  CEvent event;
  CHECK(!event.WaitMSec(50));
}

TEST(TestEventGroupChildSet)
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

  thread waitThread1(ref(w1));
  thread waitThread2(ref(w2));
  thread waitThread3(ref(w3));

  CHECK(waitForWaiters(event2,1,10000));
  CHECK(waitThread1.timed_join(MILLIS(10000)));
  CHECK(waitThread3.timed_join(MILLIS(10000)));
  SleepMillis(10);

  CHECK(result1);
  CHECK(!result2);

  CHECK(!w3.waiting);
  CHECK(w3.result == &event1);

  event2.Set();

  CHECK(waitThread2.timed_join(MILLIS(10000)));
}

TEST(TestEventGroupChildSet2)
{
  CEvent event1(true,true);
  CEvent event2;

  CEventGroup group(&event1,&event2,NULL);

  bool result1 = false;
  bool result2 = false;

  waiter w1(event1,result1);
  waiter w2(event2,result2);
  group_wait w3(group);

  thread waitThread1(ref(w1));
  thread waitThread2(ref(w2));
  thread waitThread3(ref(w3));

  CHECK(waitForWaiters(event2,1,10000));
  CHECK(waitThread1.timed_join(MILLIS(10000)));
  CHECK(waitThread3.timed_join(MILLIS(10000)));
  SleepMillis(10);

  CHECK(result1);
  CHECK(!result2);

  CHECK(!w3.waiting);
  CHECK(w3.result == &event1);

  event2.Set();

  CHECK(waitThread2.timed_join(MILLIS(10000)));
}

TEST(TestEventGroupWaitResetsChild)
{
  CEvent event1;
  CEvent event2;

  CEventGroup group(&event1,&event2,NULL);

  group_wait w3(group);

  thread waitThread3(ref(w3));

  CHECK(waitForWaiters(group,1,10000));

  CHECK(w3.waiting);
  CHECK(w3.result == NULL);

  event2.Set();

  CHECK(waitThread3.timed_join(MILLIS(10000)));

  CHECK(!w3.waiting);
  CHECK(w3.result == &event2);
  // event2 should have been reset.
  CHECK(event2.WaitMSec(1) == false); 
}

TEST(TestEventGroupTimedWait)
{
  CEvent event1;
  CEvent event2;
  CEventGroup group(&event1,&event2,NULL);

  bool result1 = false;
  bool result2 = false;

  waiter w1(event1,result1);
  waiter w2(event2,result2);

  thread waitThread1(ref(w1));
  thread waitThread2(ref(w2));

  CHECK(waitForWaiters(event1,1,10000));
  CHECK(waitForWaiters(event2,1,10000));

  CHECK(group.wait(20) == NULL); // waited ... got nothing

  group_wait w3(group,50);
  thread waitThread3(ref(w3));

  CHECK(waitForWaiters(group,1,10000));

  CHECK(!result1);
  CHECK(!result2);

  CHECK(w3.waiting);
  CHECK(w3.result == NULL);

  // this should end given the wait is for only 50 millis
  CHECK(waitThread3.timed_join(MILLIS(100)));

  CHECK(!w3.waiting);
  CHECK(w3.result == NULL);

  group_wait w4(group,50);
  thread waitThread4(ref(w4));

  CHECK(waitForWaiters(group,1,10000));

  CHECK(w4.waiting);
  CHECK(w4.result == NULL);

  event1.Set();

  CHECK(waitThread1.timed_join(MILLIS(10000)));
  CHECK(waitThread4.timed_join(MILLIS(10000)));
  SleepMillis(10);

  CHECK(result1);
  CHECK(!result2);

  CHECK(!w4.waiting);
  CHECK(w4.result == &event1);

  event2.Set();

  CHECK(waitThread2.timed_join(MILLIS(10000)));
}

#define TESTNUM 100000l
#define NUMTHREADS 100l

CEvent* g_event = NULL;
volatile long g_mutex;

class mass_waiter
{
public:
  CEvent& event;
  bool result;

  volatile bool waiting;

  mass_waiter() : event(*g_event), waiting(false) {}
  
  void operator()()
  {
    waiting = true;
    AtomicGuard g(&g_mutex);
    result = event.Wait();
    waiting = false;
  }
};

class poll_mass_waiter
{
public:
  CEvent& event;
  bool result;

  volatile bool waiting;

  poll_mass_waiter() : event(*g_event), waiting(false) {}
  
  void operator()()
  {
    waiting = true;
    AtomicGuard g(&g_mutex);
    while ((result = event.WaitMSec(0)) == false);
    waiting = false;
  }
};

template <class W> void RunMassEventTest(boost::shared_array<W>& m, bool canWaitOnEvent)
{
  boost::shared_array<thread> t;
  t.reset(new thread[NUMTHREADS]);
  for(size_t i=0; i<NUMTHREADS; i++)
    t[i] = thread(ref(m[i]));

  CHECK(waitForThread(g_mutex,NUMTHREADS,10000));
  if (canWaitOnEvent)
  {
    CHECK(waitForWaiters(*g_event,NUMTHREADS,10000));
  }

  SleepMillis(100);// give them a little more time

  for(size_t i=0; i<NUMTHREADS; i++)
  {
    CHECK(m[i].waiting);
  }

  g_event->Set();

  for(size_t i=0; i<NUMTHREADS; i++)
  {
    CHECK(t[i].timed_join(MILLIS(10000)));
  }

  for(size_t i=0; i<NUMTHREADS; i++)
  {
    CHECK(!m[i].waiting);
    CHECK(m[i].result);
  }
}


TEST(TestMassEvent)
{
  g_event = new CEvent();

  boost::shared_array<mass_waiter> m;
  m.reset(new mass_waiter[NUMTHREADS]);

  RunMassEventTest(m,true);
  delete g_event;
}

TEST(TestMassEventPolling)
{
  g_event = new CEvent(true); // polling needs to avoid the auto-reset

  boost::shared_array<poll_mass_waiter> m;
  m.reset(new poll_mass_waiter[NUMTHREADS]);

  RunMassEventTest(m,false);
  delete g_event;
}

