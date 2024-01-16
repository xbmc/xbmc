/*
 *  Copyright (C) 2012-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PeripheralImon.h"

#include "input/InputManager.h"
#include "settings/Settings.h"
#include "utils/log.h"

using namespace PERIPHERALS;

std::atomic<long> CPeripheralImon::m_lCountOfImonsConflictWithDInput(0L);

CPeripheralImon::CPeripheralImon(CPeripherals& manager,
                                 const PeripheralScanResult& scanResult,
                                 CPeripheralBus* bus)
  : CPeripheralHID(manager, scanResult, bus)
{
  m_features.push_back(FEATURE_IMON);
  m_bImonConflictsWithDInput = false;
}

void CPeripheralImon::OnDeviceRemoved()
{
  if (m_bImonConflictsWithDInput)
  {
    if (--m_lCountOfImonsConflictWithDInput == 0)
      ActionOnImonConflict(false);
  }
}

bool CPeripheralImon::InitialiseFeature(const PeripheralFeature feature)
{
  if (feature == FEATURE_IMON)
  {
#if defined(TARGET_WINDOWS)
    if (HasSetting("disable_winjoystick") && GetSettingBool("disable_winjoystick"))
      m_bImonConflictsWithDInput = true;
    else
#endif // TARGET_WINDOWS
      m_bImonConflictsWithDInput = false;

    if (m_bImonConflictsWithDInput)
    {
      ++m_lCountOfImonsConflictWithDInput;
      ActionOnImonConflict(true);
    }
    return CPeripheral::InitialiseFeature(feature);
  }

  return CPeripheralHID::InitialiseFeature(feature);
}

void CPeripheralImon::AddSetting(const std::string& strKey,
                                 const std::shared_ptr<const CSetting>& setting,
                                 int order)
{
#if !defined(TARGET_WINDOWS)
  if (strKey.compare("disable_winjoystick") != 0)
#endif // !TARGET_WINDOWS
    CPeripheralHID::AddSetting(strKey, setting, order);
}

void CPeripheralImon::OnSettingChanged(const std::string& strChangedSetting)
{
  if (strChangedSetting.compare("disable_winjoystick") == 0)
  {
    if (m_bImonConflictsWithDInput && !GetSettingBool("disable_winjoystick"))
    {
      m_bImonConflictsWithDInput = false;
      if (--m_lCountOfImonsConflictWithDInput == 0)
        ActionOnImonConflict(false);
    }
    else if (!m_bImonConflictsWithDInput && GetSettingBool("disable_winjoystick"))
    {
      m_bImonConflictsWithDInput = true;
      ++m_lCountOfImonsConflictWithDInput;
      ActionOnImonConflict(true);
    }
  }
}

void CPeripheralImon::ActionOnImonConflict(bool deviceInserted /*= true*/)
{
}
