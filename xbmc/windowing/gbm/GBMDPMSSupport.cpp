/*
 *  Copyright (C) 2009-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GBMDPMSSupport.h"

#include "ServiceBroker.h"
#include "windowing/gbm/WinSystemGbm.h"

using namespace KODI::WINDOWING::GBM;

CGBMDPMSSupport::CGBMDPMSSupport()
{
  m_supportedModes.push_back(OFF);
}

bool CGBMDPMSSupport::EnablePowerSaving(PowerSavingMode mode)
{
  auto winSystem = dynamic_cast<CWinSystemGbm*>(CServiceBroker::GetWinSystem());
  if (!winSystem)
    return false;

  switch (mode)
  {
  case OFF:
    return winSystem->Hide();
  default:
    return false;
  }
}

bool CGBMDPMSSupport::DisablePowerSaving()
{
  auto winSystem = dynamic_cast<CWinSystemGbm*>(CServiceBroker::GetWinSystem());
  if (!winSystem)
    return false;

  return winSystem->Show();
}
