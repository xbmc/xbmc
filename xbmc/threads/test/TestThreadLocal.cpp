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
class Runnable : public IRunnable
{
public:
  bool waiting;
  bool threadLocalHadValue;
  ThreadLocal<Thinggy>& threadLocal;

  inline Runnable(ThreadLocal<Thinggy>& tl) : waiting(false), threadLocal(tl) {}
  inline void Run()
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


TEST(TestThreadLocal, Simple)
{
  GlobalThreadLocal runnable;
  thread t(runnable);

  gate.Wait();
  EXPECT_TRUE(runnable.waiting);
  EXPECT_TRUE(staticThinggy != NULL);
  EXPECT_TRUE(staticThreadLocal.get() == NULL);
  waiter.Set();
  gate.Wait();
  EXPECT_TRUE(runnable.threadLocalHadValue);
  EXPECT_TRUE(!destructorCalled);
  delete staticThinggy;
  EXPECT_TRUE(destructorCalled);
  cleanup();
}

TEST(TestThreadLocal, Stack)
{
  StackThreadLocal runnable;
  thread t(runnable);

  gate.Wait();
  EXPECT_TRUE(runnable.waiting);
  EXPECT_TRUE(staticThinggy != NULL);
  EXPECT_TRUE(runnable.threadLocal.get() == NULL);
  waiter.Set();
  gate.Wait();
  EXPECT_TRUE(runnable.threadLocalHadValue);
  EXPECT_TRUE(!destructorCalled);
  delete staticThinggy;
  EXPECT_TRUE(destructorCalled);
  cleanup();
}

TEST(TestThreadLocal, Heap)
{
  HeapThreadLocal runnable;
  thread t(runnable);

  gate.Wait();
  EXPECT_TRUE(runnable.waiting);
  EXPECT_TRUE(staticThinggy != NULL);
  EXPECT_TRUE(runnable.threadLocal.get() == NULL);
  waiter.Set();
  gate.Wait();
  EXPECT_TRUE(runnable.threadLocalHadValue);
  EXPECT_TRUE(!destructorCalled);
  delete staticThinggy;
  EXPECT_TRUE(destructorCalled);
  cleanup();
}

TEST(TestThreadLocal, HeapDestroyed)
{
  {
    HeapThreadLocal runnable;
    thread t(runnable);

    gate.Wait();
    EXPECT_TRUE(runnable.waiting);
    EXPECT_TRUE(staticThinggy != NULL);
    EXPECT_TRUE(runnable.threadLocal.get() == NULL);
    waiter.Set();
    gate.Wait();
    EXPECT_TRUE(runnable.threadLocalHadValue);
    EXPECT_TRUE(!destructorCalled);
  } // runnable goes out of scope

  // even though the threadlocal is gone ...
  EXPECT_TRUE(!destructorCalled);
  delete staticThinggy;
  EXPECT_TRUE(destructorCalled);
  cleanup();
}

