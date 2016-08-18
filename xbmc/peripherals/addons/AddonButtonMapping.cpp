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

#include "AddonButtonMapping.h"
#include "input/joysticks/generic/ButtonMapping.h"
#include "input/joysticks/IButtonMapper.h"
#include "peripherals/addons/AddonButtonMap.h"
#include "peripherals/Peripherals.h"

using namespace JOYSTICK;
using namespace PERIPHERALS;

CAddonButtonMapping::CAddonButtonMapping(CPeripheral* peripheral, IButtonMapper* mapper)
{
  PeripheralAddonPtr addon = g_peripherals.GetAddonWithButtonMap(peripheral);

  if (!addon)
  {
    CLog::Log(LOGDEBUG, "Failed to locate add-on for \"%s\"", peripheral->DeviceName().c_str());
  }
  else
  {
    m_buttonMap.reset(new CAddonButtonMap(peripheral, addon, mapper->ControllerID()));
    if (m_buttonMap->Load())
      m_driverHandler.reset(new CButtonMapping(mapper, m_buttonMap.get()));
    else
      m_buttonMap.reset();
  }
}

CAddonButtonMapping::~CAddonButtonMapping(void)
{
  m_driverHandler.reset();
  m_buttonMap.reset();
}

bool CAddonButtonMapping::OnButtonMotion(unsigned int buttonIndex, bool bPressed)
{
  if (m_driverHandler)
    return m_driverHandler->OnButtonMotion(buttonIndex, bPressed);

  return false;
}

bool CAddonButtonMapping::OnHatMotion(unsigned int hatIndex, HAT_STATE state)
{
  if (m_driverHandler)
    return m_driverHandler->OnHatMotion(hatIndex, state);

  return false;
}

bool CAddonButtonMapping::OnAxisMotion(unsigned int axisIndex, float position)
{
  if (m_driverHandler)
    return m_driverHandler->OnAxisMotion(axisIndex, position);

  return false;
}

void CAddonButtonMapping::ProcessAxisMotions(void)
{
  if (m_driverHandler)
    m_driverHandler->ProcessAxisMotions();
}
