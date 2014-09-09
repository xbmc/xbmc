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

#pragma once

#include "AddonCallback.h"
#include "AddonString.h"

namespace XBMCAddon
{
  namespace xbmc
  {
    /**
     * Monitor class.
     * 
     * Monitor() -- Creates a new Monitor to notify addon about changes.
     */
    class Monitor : public AddonCallback
    {
      String Id;
      CEvent abortEvent;
    public:
      Monitor();

#ifndef SWIG
      inline void    OnSettingsChanged() { XBMC_TRACE; invokeCallback(new CallbackFunction<Monitor>(this,&Monitor::onSettingsChanged)); }
      inline void    OnScreensaverActivated() { XBMC_TRACE; invokeCallback(new CallbackFunction<Monitor>(this,&Monitor::onScreensaverActivated)); }
      inline void    OnScreensaverDeactivated() { XBMC_TRACE; invokeCallback(new CallbackFunction<Monitor>(this,&Monitor::onScreensaverDeactivated)); }
      inline void    OnDPMSActivated() { XBMC_TRACE; invokeCallback(new CallbackFunction<Monitor>(this,&Monitor::onDPMSActivated)); }
      inline void    OnDPMSDeactivated() { XBMC_TRACE; invokeCallback(new CallbackFunction<Monitor>(this,&Monitor::onDPMSDeactivated)); }
      inline void    OnScanStarted(const String &library)
      {
	XBMC_TRACE;
	invokeCallback(new CallbackFunction<Monitor,const String>(this,&Monitor::onScanStarted,library));
	invokeCallback(new CallbackFunction<Monitor,const String>(this,&Monitor::onDatabaseScanStarted,library));
      }
      inline void    OnScanFinished(const String &library)
      {
	XBMC_TRACE;
	invokeCallback(new CallbackFunction<Monitor,const String>(this,&Monitor::onScanFinished,library));
	invokeCallback(new CallbackFunction<Monitor,const String>(this,&Monitor::onDatabaseUpdated,library));
      }
      inline void    OnCleanStarted(const String &library) { XBMC_TRACE; invokeCallback(new CallbackFunction<Monitor,const String>(this,&Monitor::onCleanStarted,library)); }
      inline void    OnCleanFinished(const String &library) { XBMC_TRACE; invokeCallback(new CallbackFunction<Monitor,const String>(this,&Monitor::onCleanFinished,library)); }
      inline void    OnNotification(const String &sender, const String &method, const String &data) { XBMC_TRACE; invokeCallback(new CallbackFunction<Monitor,const String,const String,const String>(this,&Monitor::onNotification,sender,method,data)); }

      inline const String& GetId() { return Id; }

      void OnAbortRequested();
#endif

      /**
       * onSettingsChanged() -- onSettingsChanged method.\n
       * \n
       * Will be called when addon settings are changed\n
       */
      virtual void    onSettingsChanged() { XBMC_TRACE; }

      /**
       * onScreensaverActivated() -- onScreensaverActivated method.\n
       * \n
       * Will be called when screensaver kicks in\n
       */
      virtual void    onScreensaverActivated() { XBMC_TRACE; }

      /**
       * onScreensaverDeactivated() -- onScreensaverDeactivated method.\n
       * \n
       * Will be called when screensaver goes off\n
       */
      virtual void    onScreensaverDeactivated() { XBMC_TRACE; }

      /**
       * onDPMSActivated() -- onDPMSActivated method.\n
       * \n
       * Will be called when energysaving/DPMS gets active\n
       */
      virtual void    onDPMSActivated() { XBMC_TRACE; }

      /**
       * onDPMSDeactivated() -- onDPMSDeactivated method.\n
       * \n
       * Will be called when energysaving/DPMS is turned off\n
       */
      virtual void    onDPMSDeactivated() { XBMC_TRACE; }

      /**
       * onScanStarted(library) -- onScanStarted method.\n
       *\n
       * library : video/music as string\n
       *\n
       * Will be called when library scan has started and return video or music to indicate which library is being scanned\n
       */
      virtual void    onScanStarted(const String library) { XBMC_TRACE; }

      /**
       * onScanFinished(library) -- onScanFinished method.\n
       * \n
       * library : video/music as string\n
       * \n
       * Will be called when library scan has ended and return video or music to indicate which library has been scanned\n
       */
      virtual void    onScanFinished(const String library) { XBMC_TRACE; }

      /**
       * onDatabaseScanStarted(database) -- Deprecated, use onScanStarted().
       */
      virtual void    onDatabaseScanStarted(const String database) { XBMC_TRACE; }

      /**
       * onDatabaseUpdated(database) -- Deprecated, use onScanFinished().
       */
      virtual void    onDatabaseUpdated(const String database) { XBMC_TRACE; }

      /**
       * onCleanStarted(library) -- onCleanStarted method.\n
       * \n
       * library : video/music as string\n
       * \n
       * Will be called when library clean has started and return video or music to indicate which library is being cleaned\n
       */
      virtual void    onCleanStarted(const String library) { XBMC_TRACE; }

      /**
       * onCleanFinished(library) -- onCleanFinished method.\n
       * \n
       * library : video/music as string\n
       * \n
       * Will be called when library clean has ended and return video or music to indicate which library has been cleaned\n
       */
      virtual void    onCleanFinished(const String library) { XBMC_TRACE; }

      /**
       * onAbortRequested() -- Deprecated, use waitForAbort() to be notified about this event.\n
       */
      virtual void    onAbortRequested() { XBMC_TRACE; }

      /**
       * onNotification(sender, method, data) -- onNotification method.\n
       *\n
       * sender : sender of the notification\n
       * method : name of the notification\n
       * data   : JSON-encoded data of the notification\n
       *\n
       * Will be called when XBMC receives or sends a notification\n
       */
      virtual void    onNotification(const String sender, const String method, const String data) { XBMC_TRACE; }

      /**
       * waitForAbort([timeout]) -- Block until abort is requested, or until timeout occurs. If an
       *                            abort requested have already been made, return immediately.
       *
       * Returns True when abort have been requested, False if a timeout is given and the operation times out.
       *
       * timeout : [opt] float - timeout in seconds. Default: no timeout.\n
       */
      bool waitForAbort(double timeout = -1);

      /**
       * abortRequested() -- Returns True if abort has been requested.
       */
      bool abortRequested();

      virtual ~Monitor();
    };
  }
};
