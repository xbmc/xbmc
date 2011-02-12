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

#include "ReferenceCounting.h"

//#define LOG_LIFECYCLE_EVENTS

#ifdef LOG_LIFECYCLE_EVENTS
#include "utils/log.h"
#endif

namespace xbmcutil
{
  Referenced::~Referenced() 
  {
// Cannot use the log from the destructor of Referenced because the
//  log uses Referenced and in the destructor it's uninitialized.
//#ifdef LOG_LIFECYCLE_EVENTS
//    CLog::Log(LOGDEBUG,"REFCNT deleting 0x%lx", (long)(((void*)this)));
//#endif
  }

  void Referenced::Release() const
  {
    long ct = InterlockedDecrement((long*)&refs);
#ifdef LOG_LIFECYCLE_EVENTS
//    CLog::Log(LOGDEBUG,"REFCNT decrementing to %ld on %s 0x%lx", ct,classname.c_str(), (long)(((void*)this)));
    CLog::Log(LOGDEBUG,"REFCNT decrementing to %ld on 0x%lx", ct, (long)(((void*)this)));
#endif
    if(ct == 0)
      delete this;
  }

  void Referenced::Acquire() const
  {
#ifdef LOG_LIFECYCLE_EVENTS
//    CLog::Log(LOGDEBUG,"REFCNT incrementing to %ld on %s 0x%lx",
//              InterlockedIncrement((long*)&refs),classname.c_str(), (long)(((void*)this)));
    CLog::Log(LOGDEBUG,"REFCNT incrementing to %ld on 0x%lx",
              InterlockedIncrement((long*)&refs), (long)(((void*)this)));
#else
    InterlockedIncrement((long*)&refs);
#endif
  }
}
