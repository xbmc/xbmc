#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE TestEvent
#include <boost/test/unit_test.hpp>

#include "threads/Event.h"

#include <boost/thread/thread.hpp>
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
public:
  CEvent* result;
  bool waiting;

  group_wait(CEventGroup& o) : event(o), result(NULL), waiting(false) {}

  void operator()()
  {
    waiting = true;
    result = event.wait();
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

  Sleep(100);

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

  Sleep(100);

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
  timed_waiter w1(event,result1,100);
  boost::thread waitThread1(w1);

  Sleep(10);

  BOOST_CHECK(result1 == 0);

  Sleep(150);

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

  Sleep(100);

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

  Sleep(100);
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

    Sleep(100);

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

  Sleep(100);
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

  Sleep(100);

  BOOST_CHECK(!result1);
  BOOST_CHECK(!result2);

  BOOST_CHECK(w3.waiting);
  BOOST_CHECK(w3.result == NULL);
  BOOST_CHECK(w4.waiting);
  BOOST_CHECK(w4.result == NULL);

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

  Sleep(100);
}
