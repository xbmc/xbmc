/*
 *  Copyright (C) 2009-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Win32DPMSSupport.h"

#include "ServiceBroker.h"
#include "rendering/dx/RenderContext.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"
#include "windowing/windows/WinSystemWin32.h"

CWin32DPMSSupport::CWin32DPMSSupport()
{
  m_supportedModes.push_back(OFF);
  m_supportedModes.push_back(STANDBY);
}

bool CWin32DPMSSupport::EnablePowerSaving(PowerSavingMode mode)
{
  auto winSystem = dynamic_cast<CWinSystemWin32*>(CServiceBroker::GetWinSystem());
  if (!winSystem)
    return false;

  if (!winSystem->GetGfxContext().IsFullScreenRoot())
  {
    CLog::Log(LOGDEBUG, "DPMS: not in fullscreen, power saving disabled");
    return false;
  }

  switch (mode)
  {
  case OFF:
    // Turn off display
    return SendMessage(DX::Windowing()->GetHwnd(), WM_SYSCOMMAND, SC_MONITORPOWER, static_cast<LPARAM>(2)) == 0;
  case STANDBY:
    // Set display to low power
    return SendMessage(DX::Windowing()->GetHwnd(), WM_SYSCOMMAND, SC_MONITORPOWER, static_cast<LPARAM>(1)) == 0;
  default:
    return true;
  }
}

bool CWin32DPMSSupport::DisablePowerSaving()
{
  // Turn display on
  return SendMessage(DX::Windowing()->GetHwnd(), WM_SYSCOMMAND, SC_MONITORPOWER, static_cast<LPARAM>(-1)) == 0;
}
