/*
 *      Copyright (C) 2014-2016 Team Kodi
 *      http://kodi.tv
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
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "AddonJoystickInputHandling.h"
#include "input/joysticks/generic/GenericJoystickInputHandling.h"
#include "input/joysticks/IJoystickInputHandler.h"
#include "peripherals/addons/AddonJoystickButtonMap.h"
#include "peripherals/Peripherals.h"

using namespace JOYSTICK;
using namespace PERIPHERALS;

#ifndef SAFE_DELETE
  #define SATE_DELETE(x)  do { delete (x); (x) = NULL; } while (0)
#endif

CAddonJoystickInputHandling::CAddonJoystickInputHandling(CPeripheral* peripheral, IJoystickInputHandler* handler)
  : m_driverHandler(NULL)
{
  PeripheralAddonPtr addon = g_peripherals.GetAddon(peripheral);

  if (!addon)
  {
    CLog::Log(LOGDEBUG, "Failed to locate add-on for \"%s\"", peripheral->DeviceName().c_str());
  }
  else
  {
    m_buttonMap = new CAddonJoystickButtonMap(peripheral, addon, handler->ControllerID());
    if (m_buttonMap->Load())
      m_driverHandler = new CGenericJoystickInputHandling(handler, m_buttonMap);
    else
      SAFE_DELETE(m_buttonMap);
  }
}

CAddonJoystickInputHandling::~CAddonJoystickInputHandling(void)
{
  delete m_driverHandler;
  delete m_buttonMap;
}

bool CAddonJoystickInputHandling::OnButtonMotion(unsigned int buttonIndex, bool bPressed)
{
  if (m_driverHandler)
    return m_driverHandler->OnButtonMotion(buttonIndex, bPressed);

  return false;
}

bool CAddonJoystickInputHandling::OnHatMotion(unsigned int hatIndex, HAT_STATE state)
{
  if (m_driverHandler)
    return m_driverHandler->OnHatMotion(hatIndex, state);

  return false;
}

bool CAddonJoystickInputHandling::OnAxisMotion(unsigned int axisIndex, float position)
{
  if (m_driverHandler)
    return m_driverHandler->OnAxisMotion(axisIndex, position);

  return false;
}

void CAddonJoystickInputHandling::ProcessAxisMotions(void)
{
  if (m_driverHandler)
    m_driverHandler->ProcessAxisMotions();
}
