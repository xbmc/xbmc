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

#include "TestHelpers.h"
#include "threads/Atomics.h"

#include <memory>
#include <iostream>

#define TESTNUM 100000l
#define NUMTHREADS 10l

class DoIncrement : public IRunnable
{
  long* number;
public:
  inline DoIncrement(long* num) : number(num) {}

  virtual void Run()
  {
    for (long i = 0; i<TESTNUM; i++)
      AtomicIncrement(number);
  }
};

class DoDecrement : public IRunnable
{
  long* number;
public:
  inline DoDecrement(long* num) : number(num) {}

  virtual void Run()
  {
    for (long i = 0; i<TESTNUM; i++)
      AtomicDecrement(number);
  }
};

class DoAdd : public IRunnable
{
  long* number;
  long toAdd;

public:
  inline DoAdd(long* num, long toAd) : number(num), toAdd(toAd) {}

  virtual void Run()
  {
    for (long i = 0; i<TESTNUM; i++)
      AtomicAdd(number,toAdd);
  }
};

class DoSubtract : public IRunnable
{
  long* number;
  long toAdd;
public:
  inline DoSubtract(long* num, long toAd) : number(num), toAdd(toAd) {}

  virtual void Run()
  {
    for (long i = 0; i<TESTNUM; i++)
      AtomicSubtract(number,toAdd);
  }
};

TEST(TestMassAtomic, Increment)
{
  long lNumber = 0;
  DoIncrement di(&lNumber);

  std::vector<std::shared_ptr<thread>> t(NUMTHREADS);
  for (size_t i=0;i<NUMTHREADS;++i)
    t[i].reset(new thread(di));

  for(size_t i=0; i<NUMTHREADS; i++)
    t[i]->join();

  EXPECT_EQ((NUMTHREADS * TESTNUM), lNumber);
 }

TEST(TestMassAtomic, Decrement)
{
  long lNumber = (NUMTHREADS * TESTNUM);
  DoDecrement dd(&lNumber);

  std::vector<std::shared_ptr<thread>> t(NUMTHREADS);
  for (size_t i=0;i<NUMTHREADS;++i)
    t[i].reset(new thread(dd));

  for(size_t i=0; i<NUMTHREADS; i++)
    t[i]->join();

  EXPECT_EQ(0, lNumber);
 }

TEST(TestMassAtomic, Add)
{
  long lNumber = 0;
  long toAdd = 10;
  DoAdd da(&lNumber,toAdd);

  std::vector<std::shared_ptr<thread>> t(NUMTHREADS);
  for(size_t i=0; i<NUMTHREADS; i++)
    t[i].reset(new thread(da));

  for(size_t i=0; i<NUMTHREADS; i++)
    t[i]->join();

  EXPECT_EQ((NUMTHREADS * TESTNUM) * toAdd, lNumber);
 }

TEST(TestMassAtomic, Subtract)
{
  long toSubtract = 10;
  long lNumber = (NUMTHREADS * TESTNUM) * toSubtract;
  DoSubtract ds(&lNumber,toSubtract);

  std::vector<std::shared_ptr<thread>> t(NUMTHREADS);
  for(size_t i=0; i<NUMTHREADS; i++)
    t[i].reset(new thread(ds));

  for(size_t i=0; i<NUMTHREADS; i++)
    t[i]->join();

  EXPECT_EQ(0, lNumber);
 }

#define STARTVAL 767856l

TEST(TestAtomic, Increment)
{
  long check = STARTVAL;
  EXPECT_EQ(STARTVAL + 1l, AtomicIncrement(&check));
  EXPECT_EQ(STARTVAL + 1l,check);
}

TEST(TestAtomic, Decrement)
{
  long check = STARTVAL;
  EXPECT_EQ(STARTVAL - 1l, AtomicDecrement(&check));
  EXPECT_EQ(STARTVAL - 1l,check);
}

TEST(TestAtomic, Add)
{
  long check = STARTVAL;
  EXPECT_EQ(STARTVAL + 123l, AtomicAdd(&check,123l));
  EXPECT_EQ(STARTVAL + 123l,check);
}

TEST(TestAtomic, Subtract)
{
  long check = STARTVAL;
  EXPECT_EQ(STARTVAL - 123l, AtomicSubtract(&check,123l));
  EXPECT_EQ(STARTVAL - 123l, check);
}

