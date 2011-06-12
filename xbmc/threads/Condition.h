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

/**
 * This is a thin wrapper around boost::condition_variable (I
 * would prefer to use it directly but ...) and will be replacing
 * existing WaitForSingleObject, SDL_cond, etc.
 */
#include <boost/thread/condition_variable.hpp>

#include "threads/SingleLock.h"

namespace XbmcThreads
{
  // Did I meantion it was a 'thin' wrapper?
  class ConditionVariable
  {
  private:
    boost::condition_variable_any impl;

  public:

    enum TimedWaitResponse { OK = 0, TIMEDOUT, INTERRUPTED=-1, ERROR=-2 };

    inline void wait(CSingleLock& lock) { impl.wait(lock); }
    inline void wait(CCriticalSection& mutex) { impl.wait(mutex); }

    TimedWaitResponse wait(CSingleLock& lock, int milliseconds);
    TimedWaitResponse wait(CCriticalSection& mutex, int milliseconds);

    inline void notifyAll() { impl.notify_all(); }
    inline void notify() { impl.notify_one(); }
  };
}

