/*
 *      Copyright (C) 2012 Team XBMC
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
#include "guilib/LocalizeStrings.h"
#include "threads/Atomics.h"
#include "settings/GUISettings.h"
#if defined (TARGET_WINDOWS)
#include "system.h" // For HAS_SDL_JOYSTICK
#if defined (HAS_SDL_JOYSTICK)
#include "input/windows/WINJoystick.h"
#endif // HAS_SDL_JOYSTICK
#endif // TARGET_WINDOWS


using namespace PERIPHERALS;
using namespace std;

volatile long CPeripheralImon::m_CountOfImonsConflictWithDInput = 0;


CPeripheralImon::CPeripheralImon(const PeripheralType type, const PeripheralBusType busType, const CStdString &strLocation, const CStdString &strDeviceName, int iVendorId, int iProductId) :
  CPeripheralHID(type, busType, strLocation, strDeviceName.IsEmpty() ? g_localizeStrings.Get(35001) : strDeviceName, iVendorId, iProductId)
{
  m_features.push_back(FEATURE_IMON);
#if defined(TARGET_WINDOWS)
  if (iProductId >= 0x34 && iProductId <= 0x46)
    m_ImonConflictsWithDInput = true;
  else 
#endif // TARGET_WINDOWS
    m_ImonConflictsWithDInput = false;
}

void CPeripheralImon::OnDeviceRemoved()
{
  if (m_ImonConflictsWithDInput)
  {
    if (AtomicDecrement(&m_CountOfImonsConflictWithDInput) == 0)
      ActionOnImonConflict(false);    
  }
}

bool CPeripheralImon::InitialiseFeature(const PeripheralFeature feature)
{
  if (feature == FEATURE_IMON)
  {
    if (m_ImonConflictsWithDInput)
    {
      AtomicIncrement(&m_CountOfImonsConflictWithDInput);
      ActionOnImonConflict(true);
    }
    return CPeripheral::InitialiseFeature(feature);
  }

  return CPeripheralHID::InitialiseFeature(feature);
}

void CPeripheralImon::ActionOnImonConflict(bool deviceInserted /*= true*/)
{
  if (deviceInserted || m_CountOfImonsConflictWithDInput == 0)
  {
#if defined(TARGET_WINDOWS) && defined (HAS_SDL_JOYSTICK)
    bool enableJoystickNow = (!deviceInserted || !g_guiSettings.GetBool("input.disablejoystickwithimon")) 
        && g_guiSettings.GetBool("input.enablejoystick");
    CLog::Log(LOGNOTICE, "Problematic iMON hardware %s. Joystick usage: %s", (deviceInserted ? "detected" : "was removed"),
        (enableJoystickNow) ? "enabled." : "disabled." );
    g_Joystick.SetEnabled(enableJoystickNow);
    CSetting* setting = g_guiSettings.GetSetting("input.disablejoystickwithimon");
    if(setting)
      setting->SetVisible(deviceInserted && !setting->IsAdvanced());
#endif
  }
}

