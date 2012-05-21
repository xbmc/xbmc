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

#include "threads/ThreadLocal.h"

#include "threads/Event.h"
#include "TestHelpers.h"

using namespace XbmcThreads;

bool destructorCalled = false;

class Thinggy
{
public:
  inline ~Thinggy() { destructorCalled = true; }
};

Thinggy* staticThinggy = NULL;
CEvent gate;
ThreadLocal<Thinggy> staticThreadLocal;

void cleanup()
{
  if (destructorCalled)
    staticThinggy = NULL;
  destructorCalled = false;
}

CEvent waiter;
class Runnable
{
public:
  bool waiting;
  bool threadLocalHadValue;
  ThreadLocal<Thinggy>& threadLocal;

  inline Runnable(ThreadLocal<Thinggy>& tl) : waiting(false), threadLocal(tl) {}
  inline void operator()()
  {
    staticThinggy = new Thinggy;
    staticThreadLocal.set(staticThinggy);
    waiting = true;
    gate.Set();
    waiter.Wait();
    waiting = false;

    threadLocalHadValue = staticThreadLocal.get() != NULL;
    gate.Set();
  }
};

class GlobalThreadLocal : public Runnable
{
public:
  GlobalThreadLocal() : Runnable(staticThreadLocal) {}
};

class StackThreadLocal : public Runnable
{
public:
  ThreadLocal<Thinggy> threadLocal;
  inline StackThreadLocal() : Runnable(threadLocal) {}
};

class HeapThreadLocal : public Runnable
{
public:
  ThreadLocal<Thinggy>* hthreadLocal;
  inline HeapThreadLocal() : Runnable(*(new ThreadLocal<Thinggy>)) { hthreadLocal = &threadLocal; }
  inline ~HeapThreadLocal() { delete hthreadLocal; }
};


TEST(TestSimpleThreadLocal)
{
  GlobalThreadLocal runnable;
  thread t(ref(runnable));

  gate.Wait();
  CHECK(runnable.waiting);
  CHECK(staticThinggy != NULL);
  CHECK(staticThreadLocal.get() == NULL);
  waiter.Set();
  gate.Wait();
  CHECK(runnable.threadLocalHadValue);
  CHECK(!destructorCalled);
  delete staticThinggy;
  CHECK(destructorCalled);
  cleanup();
}

TEST(TestStackThreadLocal)
{
  StackThreadLocal runnable;
  thread t(ref(runnable));

  gate.Wait();
  CHECK(runnable.waiting);
  CHECK(staticThinggy != NULL);
  CHECK(runnable.threadLocal.get() == NULL);
  waiter.Set();
  gate.Wait();
  CHECK(runnable.threadLocalHadValue);
  CHECK(!destructorCalled);
  delete staticThinggy;
  CHECK(destructorCalled);
  cleanup();
}

TEST(TestHeapThreadLocal)
{
  HeapThreadLocal runnable;
  thread t(ref(runnable));

  gate.Wait();
  CHECK(runnable.waiting);
  CHECK(staticThinggy != NULL);
  CHECK(runnable.threadLocal.get() == NULL);
  waiter.Set();
  gate.Wait();
  CHECK(runnable.threadLocalHadValue);
  CHECK(!destructorCalled);
  delete staticThinggy;
  CHECK(destructorCalled);
  cleanup();
}

TEST(TestHeapThreadLocalDestroyed)
{
  {
    HeapThreadLocal runnable;
    thread t(ref(runnable));

    gate.Wait();
    CHECK(runnable.waiting);
    CHECK(staticThinggy != NULL);
    CHECK(runnable.threadLocal.get() == NULL);
    waiter.Set();
    gate.Wait();
    CHECK(runnable.threadLocalHadValue);
    CHECK(!destructorCalled);
  } // runnable goes out of scope

  // even though the threadlocal is gone ...
  CHECK(!destructorCalled);
  delete staticThinggy;
  CHECK(destructorCalled);
  cleanup();
}

