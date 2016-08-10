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

#include "AddonInputHandling.h"
#include "input/joysticks/generic/DriverReceiving.h"
#include "input/joysticks/generic/InputHandling.h"
#include "input/joysticks/IInputHandler.h"
#include "input/joysticks/IDriverReceiver.h"
#include "peripherals/addons/AddonButtonMap.h"
#include "peripherals/devices/PeripheralJoystick.h"
#include "peripherals/Peripherals.h"

using namespace JOYSTICK;
using namespace PERIPHERALS;

CAddonInputHandling::CAddonInputHandling(CPeripheral* peripheral, IInputHandler* handler, IDriverReceiver* receiver)
{
  PeripheralAddonPtr addon = g_peripherals.GetAddon(peripheral);

  if (!addon)
  {
    CLog::Log(LOGDEBUG, "Failed to locate add-on for \"%s\"", peripheral->DeviceName().c_str());
  }
  else
  {
    m_buttonMap.reset(new CAddonButtonMap(peripheral, addon, handler->ControllerID()));
    if (m_buttonMap->Load())
    {
      m_driverHandler.reset(new CInputHandling(handler, m_buttonMap.get()));

      if (receiver)
      {
        m_inputReceiver.reset(new CDriverReceiving(receiver, m_buttonMap.get()));

        // Interfaces are connected here because they share button map as a common resource
        handler->SetInputReceiver(m_inputReceiver.get());
      }
    }
    else
    {
      m_buttonMap.reset();
    }
  }
}

CAddonInputHandling::~CAddonInputHandling(void)
{
  m_driverHandler.reset();
  m_inputReceiver.reset();
  m_buttonMap.reset();
}

bool CAddonInputHandling::OnButtonMotion(unsigned int buttonIndex, bool bPressed)
{
  if (m_driverHandler)
    return m_driverHandler->OnButtonMotion(buttonIndex, bPressed);

  return false;
}

bool CAddonInputHandling::OnHatMotion(unsigned int hatIndex, HAT_STATE state)
{
  if (m_driverHandler)
    return m_driverHandler->OnHatMotion(hatIndex, state);

  return false;
}

bool CAddonInputHandling::OnAxisMotion(unsigned int axisIndex, float position)
{
  if (m_driverHandler)
    return m_driverHandler->OnAxisMotion(axisIndex, position);

  return false;
}

void CAddonInputHandling::ProcessAxisMotions(void)
{
  if (m_driverHandler)
    m_driverHandler->ProcessAxisMotions();
}

bool CAddonInputHandling::SetRumbleState(const JOYSTICK::FeatureName& feature, float magnitude)
{
  if (m_inputReceiver)
    return m_inputReceiver->SetRumbleState(feature, magnitude);

  return false;
}
