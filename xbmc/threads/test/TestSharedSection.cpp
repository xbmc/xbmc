#include <boost/test/unit_test.hpp>

#include "threads/SharedSection.h"
#include "threads/SingleLock.h"

#include <boost/thread/thread.hpp>
#include <stdio.h>

//=============================================================================
// Helper classes
//=============================================================================

static void Sleep(unsigned int millis) { boost::thread::sleep(boost::get_system_time() + boost::posix_time::milliseconds(millis)); }

template<class L>
class locker
{
  CSharedSection& sec;
  unsigned int wait;
public:
  volatile bool haslock;
  volatile bool obtainedlock;

  locker(CSharedSection& o, unsigned int waitTime = 0) : sec(o), wait(waitTime), haslock(false), obtainedlock(false) {}
  
  void operator()()
  {
    L lock(sec);
    haslock = true;
    obtainedlock = true;
    if (wait)
      Sleep(wait);
    haslock = false;
  }
};

BOOST_AUTO_TEST_CASE(TestCritSectionCase)
{
  CCriticalSection sec;

  CSingleLock l1(sec);
  CSingleLock l2(sec);
}

BOOST_AUTO_TEST_CASE(TestSharedSectionCase)
{
  CSharedSection sec;

  CSharedLock l1(sec);
  CSharedLock l2(sec);
}

BOOST_AUTO_TEST_CASE(TestGetSharedLockWhileTryingExclusiveLock)
{
  CSharedSection sec;

  CSharedLock l1(sec); // get a shared lock

  locker<CExclusiveLock> l2(sec);
  boost::thread waitThread1(boost::ref(l2)); // try to get an exclusive lock
  Sleep(10);
  BOOST_CHECK(!l2.haslock);  // this thread is waiting ...
  BOOST_CHECK(!l2.obtainedlock);  // this thread is waiting ...

  // now try and get a SharedLock
  locker<CSharedLock> l3(sec,50);
  boost::thread waitThread3(boost::ref(l3)); // try to get a shared lock
  Sleep(10);
  BOOST_CHECK(l3.haslock);

  Sleep(50);
  // l3 should have released.
  BOOST_CHECK(!l3.haslock);

  // but the exclusive lock should still not have happened
  BOOST_CHECK(!l2.haslock);  // this thread is waiting ...
  BOOST_CHECK(!l2.obtainedlock);  // this thread is waiting ...

  // let it go
  l1.Leave(); // the last shared lock leaves.

  Sleep(10); // give the exclusive lock some time

  BOOST_CHECK(l2.obtainedlock);  // the exclusive lock was captured
  BOOST_CHECK(!l2.haslock);  // ... but it doesn't have it anymore

  Sleep(50);
}

BOOST_AUTO_TEST_CASE(TestSharedSection2Case)
{
  CSharedSection sec;

  locker<CSharedLock> l1(sec,20);

  {
    CSharedLock lock(sec);
    boost::thread waitThread1(boost::ref(l1));

    Sleep(10);
    BOOST_CHECK(l1.haslock);

    waitThread1.join();
  }

  locker<CSharedLock> l2(sec,20);
  {
    CExclusiveLock lock(sec);
    boost::thread waitThread1(boost::ref(l2));

    Sleep(5);
    BOOST_CHECK(!l2.haslock);

    lock.Leave();
    Sleep(5);
    BOOST_CHECK(l2.haslock);
    
    waitThread1.join();
  }
}

