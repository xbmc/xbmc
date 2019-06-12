/*
 *  Copyright (C) 2009-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "X11DPMSSupport.h"

#include "ServiceBroker.h"
#include "utils/log.h"
#include "windowing/X11/WinSystemX11.h"

#include <X11/Xlib.h>
#include <X11/extensions/dpms.h>

namespace
{
// Mapping of PowerSavingMode to X11's mode constants.
const CARD16 X_DPMS_MODES[] =
{
  DPMSModeStandby,
  DPMSModeSuspend,
  DPMSModeOff
};
}

CX11DPMSSupport::CX11DPMSSupport()
{
  CWinSystemX11* winSystem = dynamic_cast<CWinSystemX11*>(CServiceBroker::GetWinSystem());
  if (!winSystem)
    return;

  Display* dpy = winSystem->GetDisplay();
  if (!dpy)
    return;

  int event_base, error_base; // we ignore these
  if (!DPMSQueryExtension(dpy, &event_base, &error_base))
  {
    CLog::Log(LOGINFO, "DPMS: X11 extension not present, power-saving will not be available");
    return;
  }

  if (!DPMSCapable(dpy))
  {
    CLog::Log(LOGINFO, "DPMS: display does not support power-saving");
    return;
  }

  m_supportedModes.push_back(SUSPEND); // best compromise
  m_supportedModes.push_back(OFF); // next best
  m_supportedModes.push_back(STANDBY); // rather lame, < 80% power according to DPMS spec
}

bool CX11DPMSSupport::EnablePowerSaving(PowerSavingMode mode)
{
  CWinSystemX11* winSystem = dynamic_cast<CWinSystemX11*>(CServiceBroker::GetWinSystem());
  if (!winSystem)
    return false;

  Display* dpy = winSystem->GetDisplay();
  if (!dpy)
    return false;

  // This is not needed on my ATI Radeon, but the docs say that DPMSForceLevel
  // after a DPMSDisable (from SDL) should not normally work.
  DPMSEnable(dpy);
  DPMSForceLevel(dpy, X_DPMS_MODES[mode]);
  // There shouldn't be any errors if we called DPMSEnable; if they do happen,
  // they're asynchronous and messy to detect.
  XFlush(dpy);
  return true;
}

bool CX11DPMSSupport::DisablePowerSaving()
{
  CWinSystemX11* winSystem = dynamic_cast<CWinSystemX11*>(CServiceBroker::GetWinSystem());
  if (!winSystem)
    return false;

  Display* dpy = winSystem->GetDisplay();
  if (!dpy)
    return false;

  DPMSForceLevel(dpy, DPMSModeOn);
  DPMSDisable(dpy);
  XFlush(dpy);

  winSystem->RecreateWindow();

  return true;
}
