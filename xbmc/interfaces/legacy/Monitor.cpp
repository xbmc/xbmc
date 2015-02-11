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

#include <algorithm>
#include "Monitor.h"
#include <math.h>

namespace XBMCAddon
{
  namespace xbmc
  {
    Monitor::Monitor(): abortEvent(true)
    {
      XBMC_TRACE;
      if (languageHook)
      {
        Id = languageHook->GetAddonId();
        languageHook->RegisterMonitorCallback(this);
      }
    }

    void Monitor::OnAbortRequested()
    {
      XBMC_TRACE;
      abortEvent.Set();
      invokeCallback(new CallbackFunction<Monitor>(this,&Monitor::onAbortRequested));
    }

    bool Monitor::waitForAbort(double timeout)
    {
      XBMC_TRACE;
      int timeoutMS = ceil(timeout * 1000);
      XbmcThreads::EndTime endTime(timeoutMS > 0 ? timeoutMS : XbmcThreads::EndTime::InfiniteValue);
      while (!endTime.IsTimePast())
      {
        {
          DelayedCallGuard dg(languageHook);
          unsigned int t = std::min(endTime.MillisLeft(), 100u);
          if (abortEvent.WaitMSec(t))
            return true;
        }
        if (languageHook)
          languageHook->MakePendingCalls();
      }
      return false;
    }

    bool Monitor::abortRequested()
    {
      XBMC_TRACE;
      return abortEvent.Signaled();
    }

    Monitor::~Monitor()
    {
      XBMC_TRACE;
      deallocating();
      DelayedCallGuard dg(languageHook);
      // we're shutting down so unregister me.
      if (languageHook)
      {
        DelayedCallGuard dc;
        languageHook->UnregisterMonitorCallback(this);
      }
    }
  }
}

