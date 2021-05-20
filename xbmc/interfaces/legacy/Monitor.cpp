/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Monitor.h"

#include "threads/SystemClock.h"

#include <algorithm>
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
        invokerId = languageHook->GetInvokerId();
        languageHook->RegisterMonitorCallback(this);
      }
    }

    void Monitor::AbortNotify()
    {
      XBMC_TRACE;
      abortEvent.Set();
    }

    bool Monitor::waitForAbort(double timeout)
    {
      XBMC_TRACE;
      int timeoutMS = ceil(timeout * 1000);
      XbmcThreads::EndTime endTime(timeoutMS);

      if (timeoutMS <= 0)
        endTime.SetInfinite();

      while (!endTime.IsTimePast())
      {
        {
          DelayedCallGuard dg(languageHook);
          unsigned int t = std::min(endTime.MillisLeft(), 100u);
          if (abortEvent.Wait(std::chrono::milliseconds(t)))
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

