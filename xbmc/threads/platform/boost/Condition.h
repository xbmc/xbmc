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

#include "threads/Helpers.h"
#include <boost/thread/condition_variable.hpp>

namespace XbmcThreads
{

  /**
   * This is a thin wrapper around boost::condition_variable. It is subject
   *  to "spurious returns" as it is built on boost which is built on posix
   *  on many of our platforms.
   */
  class ConditionVariable : public NonCopyable
  {
  private:
    boost::condition_variable_any impl;

  public:
    template<typename L> inline void wait(L& lock) { impl.wait(lock); }

    template<typename L> inline bool wait(L& lock, int milliseconds)
    {
      return (impl.timed_wait(lock, boost::posix_time::milliseconds(milliseconds)));
    }

    inline void notifyAll() { impl.notify_all(); }
    inline void notify() { impl.notify_one(); }
  };

}
