/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
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
    /// @brief **Kodi's monitor class.**
    ///
    /// \python_class{ xbmc.Monitor() }
    ///
    /// Creates a new monitor to notify addon about changes.
    ///
    class Monitor : public AddonCallback
    {
      String Id;
      long invokerId;
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
      }
      inline void    OnScanFinished(const String &library)
      {
	XBMC_TRACE;
	invokeCallback(new CallbackFunction<Monitor,const String>(this,&Monitor::onScanFinished,library));
      }
      inline void    OnCleanStarted(const String &library) { XBMC_TRACE; invokeCallback(new CallbackFunction<Monitor,const String>(this,&Monitor::onCleanStarted,library)); }
      inline void    OnCleanFinished(const String &library) { XBMC_TRACE; invokeCallback(new CallbackFunction<Monitor,const String>(this,&Monitor::onCleanFinished,library)); }
      inline void    OnNotification(const String &sender, const String &method, const String &data) { XBMC_TRACE; invokeCallback(new CallbackFunction<Monitor,const String,const String,const String>(this,&Monitor::onNotification,sender,method,data)); }

      inline const String& GetId() { return Id; }
      inline long GetInvokerId() { return invokerId; }

      /**
       * Called from XBPython to notify registered monitors that a script is aborting/ending.
       */
      void AbortNotify();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_monitor
      /// @brief \python_func{ onSettingsChanged() }
      ///-----------------------------------------------------------------------
      /// onSettingsChanged method.
      ///
      /// Will be called when addon settings are changed
      ///
      onSettingsChanged();
#else
      virtual void onSettingsChanged() { XBMC_TRACE; }
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_monitor
      /// @brief \python_func{ onScreensaverActivated() }
      ///-----------------------------------------------------------------------
      /// onScreensaverActivated method.
      ///
      /// Will be called when screensaver kicks in
      ///
      onScreensaverActivated();
#else
      virtual void onScreensaverActivated() { XBMC_TRACE; }
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_monitor
      /// @brief \python_func{ onScreensaverDeactivated() }
      ///-----------------------------------------------------------------------
      /// onScreensaverDeactivated method.
      ///
      /// Will be called when screensaver goes off
      ///
      onScreensaverDeactivated();
#else
      virtual void onScreensaverDeactivated() { XBMC_TRACE; }
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_monitor
      /// @brief \python_func{ onDPMSActivated() }
      ///-----------------------------------------------------------------------
      /// onDPMSActivated method.
      ///
      /// Will be called when energysaving/DPMS gets active
      ///
      onDPMSActivated();
#else
      virtual void onDPMSActivated() { XBMC_TRACE; }
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_monitor
      /// @brief \python_func{ onDPMSDeactivated() }
      ///-----------------------------------------------------------------------
      /// onDPMSDeactivated method.
      ///
      /// Will be called when energysaving/DPMS is turned off
      ///
      onDPMSDeactivated();
#else
      virtual void onDPMSDeactivated() { XBMC_TRACE; }
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_monitor
      /// @brief \python_func{ onScanStarted(library) }
      ///-----------------------------------------------------------------------
      /// onScanStarted method.
      ///
      /// @param library             Video / music as string
      ///
      ///
      /// @note Will be called when library clean has ended and return video or
      /// music to indicate which library is being scanned
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v14 New function added.
      ///
      onScanStarted(...);
#else
      virtual void onScanStarted(const String library) { XBMC_TRACE; }
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_monitor
      /// @brief \python_func{ onScanFinished(library)  }
      ///-----------------------------------------------------------------------
      /// onScanFinished method.
      ///
      /// @param library             Video / music as string
      ///
      ///
      /// @note Will be called when library clean has ended and return video or
      /// music to indicate which library has been scanned
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v14 New function added.
      ///
      onScanFinished(...);
#else
      virtual void onScanFinished(const String library) { XBMC_TRACE; }
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_monitor
      /// @brief \python_func{ onCleanStarted(library) }
      ///-----------------------------------------------------------------------
      /// @brief onCleanStarted method.
      ///
      /// @param library             Video / music as string
      ///
      ///
      /// @note Will be called when library clean has ended and return video or
      /// music to indicate which library has been cleaned
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v14 New function added.
      ///
      onCleanStarted(...);
#else
      virtual void onCleanStarted(const String library) { XBMC_TRACE; }
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_monitor
      /// @brief \python_func{ onCleanFinished(library) }
      ///-----------------------------------------------------------------------
      /// onCleanFinished method.
      ///
      /// @param library             Video / music as string
      ///
      ///
      /// @note Will be called when library clean has ended and return video or
      /// music to indicate which library has been finished
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v14 New function added.
      ///
      onCleanFinished(...);
#else
      virtual void onCleanFinished(const String library) { XBMC_TRACE; }
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_monitor
      /// @brief \python_func{ onNotification(sender, method, data) }
      ///-----------------------------------------------------------------------
      /// onNotification method.
      ///
      /// @param sender              Sender of the notification
      /// @param method              Name of the notification
      /// @param data                JSON-encoded data of the notification
      ///
      /// @note Will be called when Kodi receives or sends a notification
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v13 New function added.
      ///
      onNotification(...);
#else
      virtual void onNotification(const String sender, const String method, const String data) { XBMC_TRACE; }
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      /// \ingroup python_monitor
      /// @brief \python_func{ waitForAbort([timeout]) }
      ///-----------------------------------------------------------------------
      /// Wait for Abort
      /// \anchor xbmc_Monitor_waitForAbort
      ///
      /// Block until abort is requested, or until timeout occurs. If an
      /// abort requested have already been made, return immediately.
      ///
      /// @param timeout                 [opt] float - timeout in seconds.
      ///                                Default: no timeout.
      ///
      /// @return                        True when abort have been requested,
      ///                                False if a timeout is given and the
      ///                                operation times out.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v14 New function added.
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// monitor = xbmc.Monitor()
      /// # do something
      /// monitor.waitForAbort(10) # sleeps for 10 secs or returns early if kodi aborts
      /// if monitor.abortRequested():
      ///     # abort was requested to Kodi (e.g. shutdown), do your cleanup logic
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      waitForAbort(...);
#else
      bool waitForAbort(double timeout = -1);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      /// \ingroup python_monitor
      /// @brief \python_func{ abortRequested() }
      ///-----------------------------------------------------------------------
      /// Returns True if abort has been requested.
      ///
      /// @return                        True if requested
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v14 New function added.
      ///
      abortRequested();
#else
      bool abortRequested();
#endif
      ~Monitor() override;
    };
    /** @} */
  }
};
