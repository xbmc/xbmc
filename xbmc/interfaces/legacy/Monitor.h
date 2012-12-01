/*
 *      Copyright (C) 2005-2012 Team XBMC
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

#include "AddonCallback.h"
#include "AddonString.h"

namespace XBMCAddon
{
  namespace xbmc
  {
    /**
     * <pre>
     * Monitor class.
     * 
     * Monitor() -- Creates a new Monitor to notify addon about changes.
     * </pre>
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
      inline void    OnAbortRequested() { TRACE; invokeCallback(new CallbackFunction<Monitor>(this,&Monitor::onAbortRequested)); }
#endif

      /**
       * <pre>
       * onSettingsChanged() -- onSettingsChanged method.
       * 
       * Will be called when addon settings are changed
       * </pre>
       */
      virtual void    onSettingsChanged() { TRACE; }

      /**
       * <pre>
       * onScreensaverActivated() -- onScreensaverActivated method.
       * 
       * Will be called when screensaver kicks in
       * </pre>
       */
      virtual void    onScreensaverActivated() { TRACE; }

      /**
       * <pre>
       * onScreensaverDeactivated() -- onScreensaverDeactivated method.
       * 
       * Will be called when screensaver goes off
       * </pre>
       */
      virtual void    onScreensaverDeactivated() { TRACE; }

      /**
       * <pre>
       * onDatabaseUpdated(database) -- onDatabaseUpdated method.
       * 
       * database - video/music as stri
       * 
       * Will be called when database gets updated and return video or music to indicate which DB has been changed
       * </pre>
       */
      virtual void    onDatabaseUpdated(const String database) { TRACE; }

      /**
       * <pre>
       * onAbortRequested() -- onAbortRequested method.
       * 
       * Will be called when XBMC requests Abort
       * </pre>
       */
      virtual void    onAbortRequested() { TRACE; }

      virtual ~Monitor();

      inline const String& GetId() { return Id; }
    };
  }
};
