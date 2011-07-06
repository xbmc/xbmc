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

#include <boost/test/unit_test.hpp>

#include "threads/Event.h"

#include <boost/thread/thread.hpp>
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

static void Sleep(unsigned int millis) { boost::thread::sleep(boost::get_system_time() + boost::posix_time::milliseconds(millis)); }

//=============================================================================

BOOST_AUTO_TEST_CASE(TestEventCase)
{
  CEvent event;
  bool result = false;
  waiter w1(event,result);
  boost::thread waitThread(w1);

  Sleep(50);

  BOOST_CHECK(!result);

  event.Set();

  Sleep(10);

  BOOST_CHECK(result);
}

BOOST_AUTO_TEST_CASE(TestEvent2WaitsCase)
{
  CEvent event;
  bool result1 = false;
  bool result2 = false;
  waiter w1(event,result1);
  waiter w2(event,result2);
  boost::thread waitThread1(w1);
  boost::thread waitThread2(w2);

  Sleep(50);

  BOOST_CHECK(!result1);
  BOOST_CHECK(!result2);

  event.Set();

  Sleep(10);

  BOOST_CHECK(result1);
  BOOST_CHECK(result2);
}

BOOST_AUTO_TEST_CASE(TestEventTimedWaitsCase)
{
  CEvent event;
  int result1 = 10;
  timed_waiter w1(event,result1,100);
  boost::thread waitThread1(w1);

  Sleep(10);

  BOOST_CHECK(result1 == 0);

  event.Set();

  Sleep(10);

  BOOST_CHECK(result1 == 1);
}

BOOST_AUTO_TEST_CASE(TestEventTimedWaitsTimeoutCase)
{
  CEvent event;
  int result1 = 10;
  timed_waiter w1(event,result1,50);
  boost::thread waitThread1(w1);

  Sleep(10);

  BOOST_CHECK(result1 == 0);

  Sleep(80);

  BOOST_CHECK(result1 == -1);
}

BOOST_AUTO_TEST_CASE(TestEventGroupCase)
{
  CEvent event1;
  CEvent event2;

  CEventGroup group(&event1,&event2,NULL);

  bool result1 = false;
  bool result2 = false;

  waiter w1(event1,result1);
  waiter w2(event2,result2);
  group_wait w3(group);

  boost::thread waitThread1(boost::ref(w1));
  boost::thread waitThread2(boost::ref(w2));
  boost::thread waitThread3(boost::ref(w3));

  Sleep(10);

  BOOST_CHECK(!result1);
  BOOST_CHECK(!result2);

  BOOST_CHECK(w3.waiting);
  BOOST_CHECK(w3.result == NULL);

  event1.Set();

  Sleep(10);

  BOOST_CHECK(result1);
  BOOST_CHECK(!w1.waiting);
  BOOST_CHECK(!result2);
  BOOST_CHECK(w2.waiting);
  BOOST_CHECK(!w3.waiting);
  BOOST_CHECK(w3.result == &event1);

  event2.Set();

  Sleep(50);
}

BOOST_AUTO_TEST_CASE(TestEventGroupLimitedGroupScopeCase)
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

    boost::thread waitThread1(boost::ref(w1));
    boost::thread waitThread2(boost::ref(w2));
    boost::thread waitThread3(boost::ref(w3));

    Sleep(10);

    BOOST_CHECK(!result1);
    BOOST_CHECK(!result2);

    BOOST_CHECK(w3.waiting);
    BOOST_CHECK(w3.result == NULL);

    event1.Set();

    Sleep(10);

    BOOST_CHECK(result1);
    BOOST_CHECK(!w1.waiting);
    BOOST_CHECK(!result2);
    BOOST_CHECK(w2.waiting);
    BOOST_CHECK(!w3.waiting);
    BOOST_CHECK(w3.result == &event1);
  }

  event2.Set();

  Sleep(50);
}

BOOST_AUTO_TEST_CASE(TestEvent2GroupsCase)
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

  boost::thread waitThread1(boost::ref(w1));
  boost::thread waitThread2(boost::ref(w2));
  boost::thread waitThread3(boost::ref(w3));
  boost::thread waitThread4(boost::ref(w4));

  Sleep(10);

  BOOST_CHECK(!result1);
  BOOST_CHECK(!result2);

  BOOST_CHECK(w3.waiting);
  BOOST_CHECK_EQUAL(w3.result,(void*)NULL);
  BOOST_CHECK(w4.waiting);
  BOOST_CHECK_EQUAL(w4.result,(void*)NULL);

  event1.Set();

  Sleep(10);

  BOOST_CHECK(result1);
  BOOST_CHECK(!w1.waiting);
  BOOST_CHECK(!result2);
  BOOST_CHECK(w2.waiting);
  BOOST_CHECK(!w3.waiting);
  BOOST_CHECK(w3.result == &event1);
  BOOST_CHECK(!w4.waiting);
  BOOST_CHECK(w4.result == &event1);

  event2.Set();

  Sleep(50);
}

BOOST_AUTO_TEST_CASE(TestEventAutoResetBehavior)
{
  CEvent event;

  BOOST_CHECK(!event.WaitMSec(1));

  event.Set(); // event will remain signaled if there are no waits

  BOOST_CHECK(event.WaitMSec(1));
}

BOOST_AUTO_TEST_CASE(TestEventManualResetCase)
{
  CEvent event(true);
  bool result = false;
  waiter w1(event,result);
  boost::thread waitThread(w1);

  Sleep(10);

  BOOST_CHECK(!result);

  event.Set();

  Sleep(10);

  BOOST_CHECK(result);

  // with manual reset, the state should remain signaled
  BOOST_CHECK(event.WaitMSec(1));

  event.Reset();

  BOOST_CHECK(!event.WaitMSec(1));
}

BOOST_AUTO_TEST_CASE(TestEventInitValCase)
{
  CEvent event(false,true);
  BOOST_CHECK(event.WaitMSec(50));
}

BOOST_AUTO_TEST_CASE(TestEventSimpleTimeoutCase)
{
  CEvent event;
  BOOST_CHECK(!event.WaitMSec(50));
}

BOOST_AUTO_TEST_CASE(TestEventGroupChildSet)
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

  boost::thread waitThread1(boost::ref(w1));
  boost::thread waitThread2(boost::ref(w2));
  boost::thread waitThread3(boost::ref(w3));

  Sleep(10);

  BOOST_CHECK(result1);
  BOOST_CHECK(!result2);

  BOOST_CHECK(!w3.waiting);
  BOOST_CHECK(w3.result == &event1);

  Sleep(10);

  event2.Set();

  Sleep(50);
}

BOOST_AUTO_TEST_CASE(TestEventGroupChildSet2)
{
  CEvent event1(true,true);
  CEvent event2;

  CEventGroup group(&event1,&event2,NULL);

  bool result1 = false;
  bool result2 = false;

  waiter w1(event1,result1);
  waiter w2(event2,result2);
  group_wait w3(group);

  boost::thread waitThread1(boost::ref(w1));
  boost::thread waitThread2(boost::ref(w2));
  boost::thread waitThread3(boost::ref(w3));

  Sleep(10);

  BOOST_CHECK(result1);
  BOOST_CHECK(!result2);

  BOOST_CHECK(!w3.waiting);
  BOOST_CHECK(w3.result == &event1);

  Sleep(10);

  event2.Set();

  Sleep(50);
}

BOOST_AUTO_TEST_CASE(TestEventGroupWaitResetsChild)
{
  CEvent event1;
  CEvent event2;

  CEventGroup group(&event1,&event2,NULL);

  group_wait w3(group);

  boost::thread waitThread3(boost::ref(w3));

  Sleep(10);

  BOOST_CHECK(w3.waiting);
  BOOST_CHECK(w3.result == NULL);

  Sleep(10);

  event2.Set();

  Sleep(10);

  BOOST_CHECK(!w3.waiting);
  BOOST_CHECK(w3.result == &event2);
  // event2 should have been reset.
  BOOST_CHECK(event2.WaitMSec(1) == false); 
  Sleep(50);
}

BOOST_AUTO_TEST_CASE(TestEventGroupTimedWait)
{
  CEvent event1;
  CEvent event2;
  CEventGroup group(&event1,&event2,NULL);

  bool result1 = false;
  bool result2 = false;

  waiter w1(event1,result1);
  waiter w2(event2,result2);

  boost::thread waitThread1(boost::ref(w1));
  boost::thread waitThread2(boost::ref(w2));

  BOOST_CHECK(group.wait(20) == NULL); // waited ... got nothing

  Sleep(10);

  group_wait w3(group,50);
  boost::thread waitThread3(boost::ref(w3));

  Sleep(10);

  BOOST_CHECK(!result1);
  BOOST_CHECK(!result2);

  BOOST_CHECK(w3.waiting);
  BOOST_CHECK(w3.result == NULL);

  Sleep(100);

  BOOST_CHECK(!w3.waiting);
  BOOST_CHECK(w3.result == NULL);

  group_wait w4(group,50);
  boost::thread waitThread4(boost::ref(w4));

  Sleep(10);

  BOOST_CHECK(w4.waiting);
  BOOST_CHECK(w4.result == NULL);

  event1.Set();

  Sleep(10);

  BOOST_CHECK(result1);
  BOOST_CHECK(!result2);

  BOOST_CHECK(!w4.waiting);
  BOOST_CHECK(w4.result == &event1);

  event2.Set();

  Sleep(50);
}

#define TESTNUM 100000l
#define NUMTHREADS 100l

CEvent g_event;

class mass_waiter
{
public:
  CEvent& event;
  bool result;

  volatile bool waiting;

  mass_waiter() : event(g_event), waiting(false) {}
  
  void operator()()
  {
    waiting = true;
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

  poll_mass_waiter() : event(g_event), waiting(false) {}
  
  void operator()()
  {
    waiting = true;
    while ((result = event.WaitMSec(0)) == false);
    waiting = false;
  }
};


BOOST_AUTO_TEST_CASE(TestMassEvent)
{
  CEvent e02;
  CEventGroup group(&g_event,&e02,NULL);

  boost::shared_array<poll_mass_waiter> m;
  m.reset(new poll_mass_waiter[NUMTHREADS]);

  boost::shared_array<boost::thread> t;
  t.reset(new boost::thread[NUMTHREADS]);
  for(size_t i=0; i<NUMTHREADS; i++)
    t[i] = boost::thread(boost::ref(m[i]));

  Sleep(50);
  for(size_t i=0; i<NUMTHREADS; i++)
  {
    BOOST_CHECK(m[i].waiting);
  }

  g_event.Set();
  Sleep(500);

  for(size_t i=0; i<NUMTHREADS; i++)
  {
    BOOST_CHECK(!m[i].waiting);
    BOOST_CHECK(m[i].result);
  }

}
