/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "threads/Event.h"
#include "threads/IRunnable.h"
#include "threads/test/TestHelpers.h"

#include <memory>
#include <stdio.h>

using namespace XbmcThreads;
using namespace std::chrono_literals;

//=============================================================================
// Helper classes
//=============================================================================

class waiter : public IRunnable
{
  CEvent& event;
public:
  bool& result;

  volatile bool waiting = false;

  waiter(CEvent& o, bool& flag) : event(o), result(flag) {}

  void Run() override
  {
    waiting = true;
    result = event.Wait();
    waiting = false;
  }
};

class timed_waiter : public IRunnable
{
  CEvent& event;
  std::chrono::milliseconds waitTime;

public:
  int& result;

  volatile bool waiting = false;

  timed_waiter(CEvent& o, int& flag, std::chrono::milliseconds waitTimeMillis)
    : event(o), waitTime(waitTimeMillis), result(flag)
  {
  }

  void Run() override
  {
    waiting = true;
    result = 0;
    result = event.Wait(waitTime) ? 1 : -1;
    waiting = false;
  }
};

class group_wait : public IRunnable
{
  CEventGroup& event;
  std::chrono::milliseconds timeout;

public:
  CEvent* result;
  bool waiting;

  group_wait(CEventGroup& o) : event(o), timeout(-1ms), result(NULL), waiting(false) {}

  group_wait(CEventGroup& o, std::chrono::milliseconds timeout_)
    : event(o), timeout(timeout_), result(NULL), waiting(false)
  {
  }

  void Run() override
  {
    waiting = true;
    if (timeout == -1ms)
      result = event.wait();
    else
      result = event.wait(timeout);
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

  EXPECT_TRUE(waitForWaiters(event, 1, 10000ms));

  EXPECT_TRUE(!result);

  event.Set();

  EXPECT_TRUE(waitThread.timed_join(10000ms));

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

  EXPECT_TRUE(waitForWaiters(event, 2, 10000ms));

  EXPECT_TRUE(!result1);
  EXPECT_TRUE(!result2);

  event.Set();

  EXPECT_TRUE(waitThread1.timed_join(10000ms));
  EXPECT_TRUE(waitThread2.timed_join(10000ms));

  EXPECT_TRUE(result1);
  EXPECT_TRUE(result2);

}

TEST(TestEvent, TimedWaits)
{
  CEvent event;
  int result1 = 10;
  timed_waiter w1(event, result1, 100ms);
  thread waitThread1(w1);

  EXPECT_TRUE(waitForWaiters(event, 1, 10000ms));

  EXPECT_TRUE(result1 == 0);

  event.Set();

  EXPECT_TRUE(waitThread1.timed_join(10000ms));

  EXPECT_TRUE(result1 == 1);
}

TEST(TestEvent, TimedWaitsTimeout)
{
  CEvent event;
  int result1 = 10;
  timed_waiter w1(event, result1, 50ms);
  thread waitThread1(w1);

  EXPECT_TRUE(waitForWaiters(event, 1, 100ms));

  EXPECT_TRUE(result1 == 0);

  EXPECT_TRUE(waitThread1.timed_join(10000ms));

  EXPECT_TRUE(result1 == -1);
}

TEST(TestEvent, Group)
{
  CEvent event1;
  CEvent event2;

  CEventGroup group{&event1,&event2};

  bool result1 = false;
  bool result2 = false;

  waiter w1(event1,result1);
  waiter w2(event2,result2);
  group_wait w3(group);

  thread waitThread1(w1);
  thread waitThread2(w2);
  thread waitThread3(w3);

  EXPECT_TRUE(waitForWaiters(event1, 1, 10000ms));
  EXPECT_TRUE(waitForWaiters(event2, 1, 10000ms));
  EXPECT_TRUE(waitForWaiters(group, 1, 10000ms));

  EXPECT_TRUE(!result1);
  EXPECT_TRUE(!result2);

  EXPECT_TRUE(w3.waiting);
  EXPECT_TRUE(w3.result == NULL);

  event1.Set();

  EXPECT_TRUE(waitThread1.timed_join(10000ms));
  EXPECT_TRUE(waitThread3.timed_join(10000ms));
  std::this_thread::sleep_for(10ms);

  EXPECT_TRUE(result1);
  EXPECT_TRUE(!w1.waiting);
  EXPECT_TRUE(!result2);
  EXPECT_TRUE(w2.waiting);
  EXPECT_TRUE(!w3.waiting);
  EXPECT_TRUE(w3.result == &event1);

  event2.Set();

  EXPECT_TRUE(waitThread2.timed_join(10000ms));
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

    EXPECT_TRUE(waitForWaiters(event1,1,10000ms));
    EXPECT_TRUE(waitForWaiters(event2,1,10000ms));
    EXPECT_TRUE(waitForWaiters(group,1,10000ms));

    EXPECT_TRUE(!result1);
    EXPECT_TRUE(!result2);

    EXPECT_TRUE(w3.waiting);
    EXPECT_TRUE(w3.result == NULL);

    event1.Set();

    EXPECT_TRUE(waitThread1.timed_join(10000ms));
    EXPECT_TRUE(waitThread3.timed_join(10000ms));
    std::this_thread::sleep_for(10ms);

    EXPECT_TRUE(result1);
    EXPECT_TRUE(!w1.waiting);
    EXPECT_TRUE(!result2);
    EXPECT_TRUE(w2.waiting);
    EXPECT_TRUE(!w3.waiting);
    EXPECT_TRUE(w3.result == &event1);
  }

  event2.Set();

  std::this_thread::sleep_for(50ms); // give thread 2 a chance to exit
}*/

TEST(TestEvent, TwoGroups)
{
  CEvent event1;
  CEvent event2;

  CEventGroup group1{&event1,&event2};
  CEventGroup group2{&event1,&event2};

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

  EXPECT_TRUE(waitForWaiters(event1, 1, 10000ms));
  EXPECT_TRUE(waitForWaiters(event2, 1, 10000ms));
  EXPECT_TRUE(waitForWaiters(group1, 1, 10000ms));
  EXPECT_TRUE(waitForWaiters(group2, 1, 10000ms));

  EXPECT_TRUE(!result1);
  EXPECT_TRUE(!result2);

  EXPECT_TRUE(w3.waiting);
  EXPECT_EQ(w3.result,(void*)NULL);
  EXPECT_TRUE(w4.waiting);
  EXPECT_EQ(w4.result,(void*)NULL);

  event1.Set();

  EXPECT_TRUE(waitThread1.timed_join(10000ms));
  EXPECT_TRUE(waitThread3.timed_join(10000ms));
  EXPECT_TRUE(waitThread4.timed_join(10000ms));
  std::this_thread::sleep_for(10ms);

  EXPECT_TRUE(result1);
  EXPECT_TRUE(!w1.waiting);
  EXPECT_TRUE(!result2);
  EXPECT_TRUE(w2.waiting);
  EXPECT_TRUE(!w3.waiting);
  EXPECT_TRUE(w3.result == &event1);
  EXPECT_TRUE(!w4.waiting);
  EXPECT_TRUE(w4.result == &event1);

  event2.Set();

  EXPECT_TRUE(waitThread2.timed_join(10000ms));
}

TEST(TestEvent, AutoResetBehavior)
{
  CEvent event;

  EXPECT_TRUE(!event.Wait(1ms));

  event.Set(); // event will remain signaled if there are no waits

  EXPECT_TRUE(event.Wait(1ms));
}

TEST(TestEvent, ManualReset)
{
  CEvent event(true);
  bool result = false;
  waiter w1(event,result);
  thread waitThread(w1);

  EXPECT_TRUE(waitForWaiters(event, 1, 10000ms));

  EXPECT_TRUE(!result);

  event.Set();

  EXPECT_TRUE(waitThread.timed_join(10000ms));

  EXPECT_TRUE(result);

  // with manual reset, the state should remain signaled
  EXPECT_TRUE(event.Wait(1ms));

  event.Reset();

  EXPECT_TRUE(!event.Wait(1ms));
}

TEST(TestEvent, InitVal)
{
  CEvent event(false,true);
  EXPECT_TRUE(event.Wait(50ms));
}

TEST(TestEvent, SimpleTimeout)
{
  CEvent event;
  EXPECT_TRUE(!event.Wait(50ms));
}

TEST(TestEvent, GroupChildSet)
{
  CEvent event1(true);
  CEvent event2;

  event1.Set();
  CEventGroup group{&event1,&event2};

  bool result1 = false;
  bool result2 = false;

  waiter w1(event1,result1);
  waiter w2(event2,result2);
  group_wait w3(group);

  thread waitThread1(w1);
  thread waitThread2(w2);
  thread waitThread3(w3);

  EXPECT_TRUE(waitForWaiters(event2, 1, 10000ms));
  EXPECT_TRUE(waitThread1.timed_join(10000ms));
  EXPECT_TRUE(waitThread3.timed_join(10000ms));
  std::this_thread::sleep_for(10ms);

  EXPECT_TRUE(result1);
  EXPECT_TRUE(!result2);

  EXPECT_TRUE(!w3.waiting);
  EXPECT_TRUE(w3.result == &event1);

  event2.Set();

  EXPECT_TRUE(waitThread2.timed_join(10000ms));
}

TEST(TestEvent, GroupChildSet2)
{
  CEvent event1(true,true);
  CEvent event2;

  CEventGroup group{&event1,&event2};

  bool result1 = false;
  bool result2 = false;

  waiter w1(event1,result1);
  waiter w2(event2,result2);
  group_wait w3(group);

  thread waitThread1(w1);
  thread waitThread2(w2);
  thread waitThread3(w3);

  EXPECT_TRUE(waitForWaiters(event2, 1, 10000ms));
  EXPECT_TRUE(waitThread1.timed_join(10000ms));
  EXPECT_TRUE(waitThread3.timed_join(10000ms));
  std::this_thread::sleep_for(10ms);

  EXPECT_TRUE(result1);
  EXPECT_TRUE(!result2);

  EXPECT_TRUE(!w3.waiting);
  EXPECT_TRUE(w3.result == &event1);

  event2.Set();

  EXPECT_TRUE(waitThread2.timed_join(10000ms));
}

TEST(TestEvent, GroupWaitResetsChild)
{
  CEvent event1;
  CEvent event2;

  CEventGroup group{&event1,&event2};

  group_wait w3(group);

  thread waitThread3(w3);

  EXPECT_TRUE(waitForWaiters(group, 1, 10000ms));

  EXPECT_TRUE(w3.waiting);
  EXPECT_TRUE(w3.result == NULL);

  event2.Set();

  EXPECT_TRUE(waitThread3.timed_join(10000ms));

  EXPECT_TRUE(!w3.waiting);
  EXPECT_TRUE(w3.result == &event2);
  // event2 should have been reset.
  EXPECT_TRUE(event2.Wait(1ms) == false);
}

TEST(TestEvent, GroupTimedWait)
{
  CEvent event1;
  CEvent event2;
  CEventGroup group{&event1,&event2};

  bool result1 = false;
  bool result2 = false;

  waiter w1(event1,result1);
  waiter w2(event2,result2);

  thread waitThread1(w1);
  thread waitThread2(w2);

  EXPECT_TRUE(waitForWaiters(event1, 1, 10000ms));
  EXPECT_TRUE(waitForWaiters(event2, 1, 10000ms));

  EXPECT_TRUE(group.wait(20ms) == NULL); // waited ... got nothing

  group_wait w3(group, 50ms);
  thread waitThread3(w3);

  EXPECT_TRUE(waitForWaiters(group, 1, 10000ms));

  EXPECT_TRUE(!result1);
  EXPECT_TRUE(!result2);

  EXPECT_TRUE(w3.waiting);
  EXPECT_TRUE(w3.result == NULL);

  // this should end given the wait is for only 50 millis
  EXPECT_TRUE(waitThread3.timed_join(200ms));

  EXPECT_TRUE(!w3.waiting);
  EXPECT_TRUE(w3.result == NULL);

  group_wait w4(group, 50ms);
  thread waitThread4(w4);

  EXPECT_TRUE(waitForWaiters(group, 1, 10000ms));

  EXPECT_TRUE(w4.waiting);
  EXPECT_TRUE(w4.result == NULL);

  event1.Set();

  EXPECT_TRUE(waitThread1.timed_join(10000ms));
  EXPECT_TRUE(waitThread4.timed_join(10000ms));
  std::this_thread::sleep_for(10ms);

  EXPECT_TRUE(result1);
  EXPECT_TRUE(!result2);

  EXPECT_TRUE(!w4.waiting);
  EXPECT_TRUE(w4.result == &event1);

  event2.Set();

  EXPECT_TRUE(waitThread2.timed_join(10000ms));
}

#define TESTNUM 100000l
#define NUMTHREADS 100l

CEvent* g_event = NULL;
std::atomic<long> g_mutex;

class mass_waiter : public IRunnable
{
public:
  CEvent& event;
  bool result;

  volatile bool waiting = false;

  mass_waiter() : event(*g_event) {}

  void Run() override
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

  volatile bool waiting = false;

  poll_mass_waiter() : event(*g_event) {}

  void Run() override
  {
    waiting = true;
    AtomicGuard g(&g_mutex);
    while ((result = event.Wait(0ms)) == false)
      ;
    waiting = false;
  }
};

template <class W> void RunMassEventTest(std::vector<std::shared_ptr<W>>& m, bool canWaitOnEvent)
{
  std::vector<std::shared_ptr<thread>> t(NUMTHREADS);
  for(size_t i=0; i<NUMTHREADS; i++)
    t[i] = std::make_shared<thread>(*m[i]);

  EXPECT_TRUE(waitForThread(g_mutex, NUMTHREADS, 10000ms));
  if (canWaitOnEvent)
  {
    EXPECT_TRUE(waitForWaiters(*g_event, NUMTHREADS, 10000ms));
  }

  std::this_thread::sleep_for(100ms); // give them a little more time

  for(size_t i=0; i<NUMTHREADS; i++)
  {
    EXPECT_TRUE(m[i]->waiting);
  }

  g_event->Set();

  for(size_t i=0; i<NUMTHREADS; i++)
  {
    EXPECT_TRUE(t[i]->timed_join(10000ms));
  }

  for(size_t i=0; i<NUMTHREADS; i++)
  {
    EXPECT_TRUE(!m[i]->waiting);
    EXPECT_TRUE(m[i]->result);
  }
}


TEST(TestMassEvent, General)
{
  g_event = new CEvent();

  std::vector<std::shared_ptr<mass_waiter>> m(NUMTHREADS);
  for(size_t i=0; i<NUMTHREADS; i++)
    m[i] = std::make_shared<mass_waiter>();

  RunMassEventTest(m,true);
  delete g_event;
}

TEST(TestMassEvent, Polling)
{
  g_event = new CEvent(true); // polling needs to avoid the auto-reset

  std::vector<std::shared_ptr<poll_mass_waiter>> m(NUMTHREADS);
  for(size_t i=0; i<NUMTHREADS; i++)
    m[i] = std::make_shared<poll_mass_waiter>();

  RunMassEventTest(m,false);
  delete g_event;
}

