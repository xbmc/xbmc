/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#ifdef TARGET_WINDOWS
#include "powermanagement/WinIdleTimer.h"
#endif
#include "utils/Stopwatch.h"
#include "windowing/OSScreenSaver.h"

#include <string>

namespace ADDON
{
class IAddon;
using AddonPtr = std::shared_ptr<IAddon>;
} // namespace ADDON

class CApplicationPlayer;
class CSetting;

/*!
 * \brief Class handling application support for screensavers, dpms and shutdown timers.
 */

class CApplicationPowerHandling
{
public:
  explicit CApplicationPowerHandling(CApplicationPlayer& appPlayer);

  bool IsInScreenSaver() const { return m_screensaverActive; }
  bool IsScreenSaverInhibited() const;
  void ResetScreenSaver();
  void SetScreenSaverLockFailed() { m_iScreenSaveLock = -1; }
  void SetScreenSaverUnlocked() { m_iScreenSaveLock = 1; }
  void StopScreenSaverTimer();
  std::string ScreensaverIdInUse() const { return m_screensaverIdInUse; }

  void SetRenderGUI(bool renderGUI);

  int GlobalIdleTime();
  void ResetSystemIdleTimer();
  bool IsIdleShutdownInhibited() const;

  void ResetShutdownTimers();
  void StopShutdownTimer();

  bool IsDPMSActive() const { return m_dpmsIsActive; }
  bool ToggleDPMS(bool manual);

  // Wakes up from the screensaver and / or DPMS. Returns true if woken up.
  bool WakeUpScreenSaverAndDPMS(bool bPowerOffKeyPressed = false);

  bool OnSettingChanged(const CSetting& setting);
  bool OnSettingAction(const CSetting& setting);

protected:
  void ActivateScreenSaver(bool forceType = false);
  void CheckOSScreenSaverInhibitionSetting();
  // Checks whether the screensaver and / or DPMS should become active.
  void CheckScreenSaverAndDPMS();
  void InhibitScreenSaver(bool inhibit);
  void ResetScreenSaverTimer();
  bool WakeUpScreenSaver(bool bPowerOffKeyPressed = false);

  void InhibitIdleShutdown(bool inhibit);

  /*! \brief Helper method to determine how to handle TMSG_SHUTDOWN
  */
  void HandleShutdownMessage();
  void CheckShutdown();

  float NavigationIdleTime();

  bool m_renderGUI{false};

  bool m_bInhibitScreenSaver = false;
  bool m_bResetScreenSaver = false;
  ADDON::AddonPtr
      m_pythonScreenSaver; // @warning: Fallback for Python interface, for binaries not needed!
  bool m_screensaverActive = false;
  // -1 = failed, 0 = locked, 1 = unlocked, 2 = check in progress
  int m_iScreenSaveLock = 0;
  std::string m_screensaverIdInUse;

  bool m_dpmsIsActive = false;
  bool m_dpmsIsManual = false;

  bool m_bInhibitIdleShutdown = false;
  CStopWatch m_navigationTimer;
  CStopWatch m_shutdownTimer;

#ifdef TARGET_WINDOWS
  CWinIdleTimer m_idleTimer;
  CWinIdleTimer m_screenSaverTimer;
#else
  CStopWatch m_idleTimer;
  CStopWatch m_screenSaverTimer;
#endif

  // OS screen saver inhibitor that is always active if user selected a Kodi screen saver
  KODI::WINDOWING::COSScreenSaverInhibitor m_globalScreensaverInhibitor;
  // Inhibitor that is active e.g. during video playback
  KODI::WINDOWING::COSScreenSaverInhibitor m_screensaverInhibitor;

  CApplicationPlayer& m_appPlayer; //!< Reference to application player
};
