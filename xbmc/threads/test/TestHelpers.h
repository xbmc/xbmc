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

#pragma once

#include <boost/thread/thread.hpp>

#define BOOST_MILLIS(x) (boost::get_system_time() + boost::posix_time::milliseconds(x))

inline static void Sleep(unsigned int millis) { boost::thread::sleep(BOOST_MILLIS(millis)); }

template<class E> inline static bool waitForWaiters(E& event, int numWaiters, int milliseconds)
{
  for( int i = 0; i < milliseconds; i++)
  {
    if (event.getNumWaits() == numWaiters)
      return true;
    Sleep(1);
  }
  return false;
}
  
inline static bool waitForThread(volatile long& mutex, int numWaiters, int milliseconds)
{
  CCriticalSection sec;
  for( int i = 0; i < milliseconds; i++)
  {
    if (mutex == (long)numWaiters)
      return true;

    {
      CSingleLock tmplock(sec); // kick any memory syncs
    }
    Sleep(1);
  }
  return false;
}

class AtomicGuard
{
  volatile long* val;
public:
  inline AtomicGuard(volatile long* val_) : val(val_) { if (val) AtomicIncrement(val); }
  inline ~AtomicGuard() { if (val) AtomicDecrement(val); }
};

