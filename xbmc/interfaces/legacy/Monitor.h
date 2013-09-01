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
     * Monitor class.\n
     * \n
     * Monitor() -- Creates a new Monitor to notify addon about changes.\n
     */
    class Monitor : public AddonCallback
    {
      String Id;
    public:
      Monitor();

#ifndef SWIG
      inline void    OnSettingsChanged() { TRACE; invokeCallback(new CallbackFunction<Monitor>(this,&Monitor::onSettingsChanged)); }
      inline void    OnScreensaverActivated() { TRACE; invokeCallback(new CallbackFunction<Monitor>(this,&Monitor::onScreensaverActivated)); }
      inline void    OnScreensaverDeactivated() { TRACE; invokeCallback(new CallbackFunction<Monitor>(this,&Monitor::onScreensaverDeactivated)); }
      inline void    OnDatabaseUpdated(const String &database) { TRACE; invokeCallback(new CallbackFunction<Monitor,const String>(this,&Monitor::onDatabaseUpdated,database)); }
      inline void    OnDatabaseScanStarted(const String &database) { TRACE; invokeCallback(new CallbackFunction<Monitor,const String>(this,&Monitor::onDatabaseScanStarted,database)); }
      inline void    OnAbortRequested() { TRACE; invokeCallback(new CallbackFunction<Monitor>(this,&Monitor::onAbortRequested)); }
      inline void    OnNotification(const String &sender, const String &method, const String &data) { TRACE; invokeCallback(new CallbackFunction<Monitor,const String,const String,const String>(this,&Monitor::onNotification,sender,method,data)); }
#endif

      /**
       * onSettingsChanged() -- onSettingsChanged method.\n
       * \n
       * Will be called when addon settings are changed\n
       */
      virtual void    onSettingsChanged() { TRACE; }

      /**
       * onScreensaverActivated() -- onScreensaverActivated method.\n
       * \n
       * Will be called when screensaver kicks in\n
       */
      virtual void    onScreensaverActivated() { TRACE; }

      /**
       * onScreensaverDeactivated() -- onScreensaverDeactivated method.\n
       * \n
       * Will be called when screensaver goes off\n
       */
      virtual void    onScreensaverDeactivated() { TRACE; }

      /**
       * onDatabaseUpdated(database) -- onDatabaseUpdated method.\n
       * \n
       * database : video/music as string\n
       * \n
       * Will be called when database gets updated and return video or music to indicate which DB has been changed\n
       */
      virtual void    onDatabaseUpdated(const String database) { TRACE; }

      /**
       * onDatabaseScanStarted(database) -- onDatabaseScanStarted method.\n
       *\n
       * database : video/music as string\n
       *\n
       * Will be called when database update starts and return video or music to indicate which DB is being updated\n
       */
      virtual void    onDatabaseScanStarted(const String database) { TRACE; }
      
      /**
       * onAbortRequested() -- onAbortRequested method.\n
       * \n
       * Will be called when XBMC requests Abort\n
       */
      virtual void    onAbortRequested() { TRACE; }

      /**
       * onNotification(sender, method, data) -- onNotification method.\n
       *\n
       * sender : sender of the notification\n
       * method : name of the notification\n
       * data   : JSON-encoded data of the notification\n
       *\n
       * Will be called when XBMC receives or sends a notification\n
       */
      virtual void    onNotification(const String sender, const String method, const String data) { TRACE; }

      virtual ~Monitor();

      inline const String& GetId() { return Id; }
    };
  }
};
