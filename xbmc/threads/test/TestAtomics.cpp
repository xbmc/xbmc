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

#include "threads/Atomics.h"

#include <boost/test/unit_test.hpp>
#include <boost/shared_array.hpp>
#include <boost/thread/thread.hpp>

#define TESTNUM 100000l
#define NUMTHREADS 10l

void doIncrement(long* number)
{
  for (long i = 0; i<TESTNUM; i++)
    AtomicIncrement(number);
}

void doDecrement(long* number)
{
  for (long i = 0; i<TESTNUM; i++)
    AtomicDecrement(number);
}

void doAdd(long* number, long toAdd)
{
  for (long i = 0; i<TESTNUM; i++)
    AtomicAdd(number,toAdd);
}

void doSubtract(long* number, long toAdd)
{
  for (long i = 0; i<TESTNUM; i++)
    AtomicSubtract(number,toAdd);
}

BOOST_AUTO_TEST_CASE(TestMassAtomicIncrement)
{
  long lNumber = 0;
  boost::shared_array<boost::thread> t;
  t.reset(new boost::thread[NUMTHREADS]);
  for(size_t i=0; i<NUMTHREADS; i++)
    t[i] = boost::thread(boost::bind(&doIncrement,&lNumber));

  for(size_t i=0; i<NUMTHREADS; i++)
    t[i].join();

  BOOST_CHECK_EQUAL((NUMTHREADS * TESTNUM), lNumber);
 }

BOOST_AUTO_TEST_CASE(TestMassAtomicDecrement)
{
  long lNumber = (NUMTHREADS * TESTNUM);
  boost::shared_array<boost::thread> t;
  t.reset(new boost::thread[NUMTHREADS]);
  for(size_t i=0; i<NUMTHREADS; i++)
    t[i] = boost::thread(boost::bind(&doDecrement,&lNumber));

  for(size_t i=0; i<NUMTHREADS; i++)
    t[i].join();

  BOOST_CHECK_EQUAL(0, lNumber);
 }

BOOST_AUTO_TEST_CASE(TestMassAtomicAdd)
{
  long lNumber = 0;
  long toAdd = 10;
  boost::shared_array<boost::thread> t;
  t.reset(new boost::thread[NUMTHREADS]);
  for(size_t i=0; i<NUMTHREADS; i++)
    t[i] = boost::thread(boost::bind(&doAdd,&lNumber,toAdd));

  for(size_t i=0; i<NUMTHREADS; i++)
    t[i].join();

  BOOST_CHECK_EQUAL((NUMTHREADS * TESTNUM) * toAdd, lNumber);
 }

BOOST_AUTO_TEST_CASE(TestMassAtomicSubtract)
{
  long toSubtract = 10;
  long lNumber = (NUMTHREADS * TESTNUM) * toSubtract;
  boost::shared_array<boost::thread> t;
  t.reset(new boost::thread[NUMTHREADS]);
  for(size_t i=0; i<NUMTHREADS; i++)
    t[i] = boost::thread(boost::bind(&doSubtract,&lNumber,toSubtract));

  for(size_t i=0; i<NUMTHREADS; i++)
    t[i].join();

  BOOST_CHECK_EQUAL(0, lNumber);
 }

#define STARTVAL 767856l

BOOST_AUTO_TEST_CASE(TestAtomicIncrement)
{
  long check = STARTVAL;
  BOOST_CHECK_EQUAL(STARTVAL + 1l, AtomicIncrement(&check));
  BOOST_CHECK_EQUAL(STARTVAL + 1l,check);
}

BOOST_AUTO_TEST_CASE(TestAtomicDecrement)
{
  long check = STARTVAL;
  BOOST_CHECK_EQUAL(STARTVAL - 1l, AtomicDecrement(&check));
  BOOST_CHECK_EQUAL(STARTVAL - 1l,check);
}

BOOST_AUTO_TEST_CASE(TestAtomicAdd)
{
  long check = STARTVAL;
  BOOST_CHECK_EQUAL(STARTVAL + 123l, AtomicAdd(&check,123l));
  BOOST_CHECK_EQUAL(STARTVAL + 123l,check);
}

BOOST_AUTO_TEST_CASE(TestAtomicSubtract)
{
  long check = STARTVAL;
  BOOST_CHECK_EQUAL(STARTVAL - 123l, AtomicSubtract(&check,123l));
  BOOST_CHECK_EQUAL(STARTVAL - 123l, check);
}

