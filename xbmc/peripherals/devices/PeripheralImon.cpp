/*
 *      Copyright (C) 2012-2013 Team XBMC
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

#include "PeripheralImon.h"
#include "utils/log.h"
#include "settings/Settings.h"
#include "threads/Atomics.h"
#include "input/InputManager.h"

using namespace PERIPHERALS;
using namespace std;

volatile long CPeripheralImon::m_lCountOfImonsConflictWithDInput = 0;


CPeripheralImon::CPeripheralImon(const PeripheralScanResult& scanResult) :
  CPeripheralHID(scanResult)
{
  m_features.push_back(FEATURE_IMON);
  m_bImonConflictsWithDInput = false;
}

void CPeripheralImon::OnDeviceRemoved()
{
  if (m_bImonConflictsWithDInput)
  {
    if (AtomicDecrement(&m_lCountOfImonsConflictWithDInput) == 0)
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
      AtomicIncrement(&m_lCountOfImonsConflictWithDInput);
      ActionOnImonConflict(true);
    }
    return CPeripheral::InitialiseFeature(feature);
  }

  return CPeripheralHID::InitialiseFeature(feature);
}

void CPeripheralImon::AddSetting(const std::string &strKey, const CSetting *setting, int order)
{
#if !defined(TARGET_WINDOWS)
  if (strKey.compare("disable_winjoystick")!=0)
#endif // !TARGET_WINDOWS
    CPeripheralHID::AddSetting(strKey, setting, order);
}

void CPeripheralImon::OnSettingChanged(const std::string &strChangedSetting)
{
  if (strChangedSetting.compare("disable_winjoystick") == 0)
  {
    if (m_bImonConflictsWithDInput && !GetSettingBool("disable_winjoystick"))
    {
      m_bImonConflictsWithDInput = false;
      if (AtomicDecrement(&m_lCountOfImonsConflictWithDInput) == 0)
        ActionOnImonConflict(false);
    }
    else if(!m_bImonConflictsWithDInput && GetSettingBool("disable_winjoystick"))
    {
      m_bImonConflictsWithDInput = true;
      AtomicIncrement(&m_lCountOfImonsConflictWithDInput);
      ActionOnImonConflict(true);
    }
  }
}

void CPeripheralImon::ActionOnImonConflict(bool deviceInserted /*= true*/)
{
  if (deviceInserted || m_lCountOfImonsConflictWithDInput == 0)
  {
#if defined(TARGET_WINDOWS) && defined (HAS_SDL_JOYSTICK)
    bool enableJoystickNow = !deviceInserted && CSettings::Get().GetBool("input.enablejoystick");
    CLog::Log(LOGNOTICE, "Problematic iMON hardware %s. Joystick usage: %s", (deviceInserted ? "detected" : "was removed"),
        (enableJoystickNow) ? "enabled." : "disabled." );
    CInputManager::Get().SetEnabledJoystick(enableJoystickNow);
#endif
  }
}

