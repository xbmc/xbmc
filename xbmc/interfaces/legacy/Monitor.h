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

    ///
    /// \ingroup python_xbmc
    /// \defgroup python_monitor Monitor
    /// @{
    /// @brief <b>Kodi's monitor class.</b>
    ///
    /// Creates a new monitor to notify addon about changes.
    ///
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

      ///
      /// \ingroup python_monitor
      /// @brief onSettingsChanged method.
      ///
      /// Will be called when addon settings are changed
      ///
      virtual void    onSettingsChanged() { XBMC_TRACE; }

      ///
      /// \ingroup python_monitor
      /// @brief onScreensaverActivated method.
      ///
      /// Will be called when screensaver kicks in
      ///
      virtual void    onScreensaverActivated() { XBMC_TRACE; }

      ///
      /// \ingroup python_monitor
      /// @brief onScreensaverDeactivated method.
      ///
      /// Will be called when screensaver goes off
      ///
      virtual void    onScreensaverDeactivated() { XBMC_TRACE; }

      ///
      /// \ingroup python_monitor
      /// @brief onDPMSActivated method.
      ///
      /// Will be called when energysaving/DPMS gets active
      ///
      virtual void    onDPMSActivated() { XBMC_TRACE; }

      ///
      /// \ingroup python_monitor
      /// @brief onDPMSDeactivated method.
      ///
      /// Will be called when energysaving/DPMS is turned off
      ///
      virtual void    onDPMSDeactivated() { XBMC_TRACE; }

      ///
      /// \ingroup python_monitor
      /// @brief onScanStarted method.
      ///
      /// @param[in] library             Video / music as string
      ///
      ///
      /// @note Will be called when library clean has ended and return video or
      /// music to indicate which library is being scanned
      ///
      virtual void    onScanStarted(const String library) { XBMC_TRACE; }

      ///
      /// \ingroup python_monitor
      /// onScanFinished method.
      ///
      /// @param[in] library             Video / music as string
      ///
      ///
      /// @note Will be called when library clean has ended and return video or
      /// music to indicate which library has been scanned
      ///
      virtual void    onScanFinished(const String library) { XBMC_TRACE; }

      ///
      /// \ingroup python_monitor
      /// @warning Deprecated, use onScanStarted().
      ///
      virtual void    onDatabaseScanStarted(const String database) { XBMC_TRACE; }

      ///
      /// \ingroup python_monitor
      /// @warning Deprecated, use onScanFinished().
      ///
      virtual void    onDatabaseUpdated(const String database) { XBMC_TRACE; }

      ///
      /// \ingroup python_monitor
      /// @brief onCleanStarted method.
      ///
      /// @param[in] library             Video / music as string
      ///
      ///
      /// @note Will be called when library clean has ended and return video or
      /// music to indicate which library has been cleaned
      ///
      virtual void    onCleanStarted(const String library) { XBMC_TRACE; }

      ///
      /// \ingroup python_monitor
      /// @brief onCleanFinished method.
      ///
      /// @param[in] library             Video / music as string
      ///
      ///
      /// @note Will be called when library clean has ended and return video or
      /// music to indicate which library has been finished
      ///
      virtual void    onCleanFinished(const String library) { XBMC_TRACE; }

      ///
      /// \ingroup python_monitor
      /// @warning Deprecated, use waitForAbort() to be notified about this event.
      ///
      virtual void    onAbortRequested() { XBMC_TRACE; }

      ///
      /// \ingroup python_monitor
      /// @brief onNotification method.
      ///
      /// @param[in] sender              Sender of the notification
      /// @param[in] method              Name of the notification
      /// @param[in] data                JSON-encoded data of the notification
      ///
      /// @note Will be called when Kodi receives or sends a notification
      ///
      virtual void    onNotification(const String sender, const String method, const String data) { XBMC_TRACE; }

      ///
      /// \ingroup python_monitor
      /// @brief Wait for Abort
      ///
      /// Block until abort is requested, or until timeout occurs. If an
      /// abort requested have already been made, return immediately.
      ///
      /// @param[in] timeout             [opt] float - timeout in seconds.
      ///                                Default: no timeout.
      /// @return                        True when abort have been requested,
      ///                                False if a timeout is given and the
      ///                                operation times out.
      ///
      bool waitForAbort(double timeout = -1);

      ///
      /// \ingroup python_monitor
      /// @brief Returns True if abort has been requested.
      ///
      /// @return                        True if requested
      ///
      bool abortRequested();

      virtual ~Monitor();
    };
    /** @} */
  }
};
