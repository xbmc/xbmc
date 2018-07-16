/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
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
        invokerId = languageHook->GetInvokerId();
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

