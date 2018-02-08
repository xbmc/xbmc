/*
 *      Copyright (C) 2014-2017 Team Kodi
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
#include "input/joysticks/interfaces/IButtonMapper.h"
#include "peripherals/addons/AddonButtonMap.h"
#include "peripherals/Peripherals.h"
#include "utils/log.h"

using namespace KODI;
using namespace JOYSTICK;
using namespace PERIPHERALS;

CAddonButtonMapping::CAddonButtonMapping(CPeripherals& manager, CPeripheral* peripheral, IButtonMapper* mapper)
{
  PeripheralAddonPtr addon = manager.GetAddonWithButtonMap(peripheral);

  if (!addon)
  {
    CLog::Log(LOGDEBUG, "Failed to locate add-on for \"%s\"", peripheral->DeviceName().c_str());
  }
  else
  {
    const std::string controllerId = mapper->ControllerID();
    m_buttonMap.reset(new CAddonButtonMap(peripheral, addon, controllerId));
    if (m_buttonMap->Load())
    {
      IKeymap *keymap = peripheral->GetKeymap(controllerId);
      m_buttonMapping.reset(new CButtonMapping(mapper, m_buttonMap.get(), keymap));

      // Allow the mapper to save our button map
      mapper->SetButtonMapCallback(peripheral->DeviceName(), this);
    }
    else
      m_buttonMap.reset();
  }
}

CAddonButtonMapping::~CAddonButtonMapping(void)
{
  m_buttonMapping.reset();
  m_buttonMap.reset();
}

bool CAddonButtonMapping::OnButtonMotion(unsigned int buttonIndex, bool bPressed)
{
  if (m_buttonMapping)
    return m_buttonMapping->OnButtonMotion(buttonIndex, bPressed);

  return false;
}

bool CAddonButtonMapping::OnHatMotion(unsigned int hatIndex, HAT_STATE state)
{
  if (m_buttonMapping)
    return m_buttonMapping->OnHatMotion(hatIndex, state);

  return false;
}

bool CAddonButtonMapping::OnAxisMotion(unsigned int axisIndex, float position, int center, unsigned int range)
{
  if (m_buttonMapping)
    return m_buttonMapping->OnAxisMotion(axisIndex, position, center, range);

  return false;
}

void CAddonButtonMapping::ProcessAxisMotions(void)
{
  if (m_buttonMapping)
    m_buttonMapping->ProcessAxisMotions();
}

bool CAddonButtonMapping::OnKeyPress(const CKey& key)
{
  if (m_buttonMapping)
    return m_buttonMapping->OnKeyPress(key);

  return false;
}

void CAddonButtonMapping::OnKeyRelease(const CKey& key)
{
  if (m_buttonMapping)
    m_buttonMapping->OnKeyRelease(key);
}

bool CAddonButtonMapping::OnPosition(int x, int y)
{
  if (m_buttonMapping)
    return m_buttonMapping->OnPosition(x, y);

  return false;
}

bool CAddonButtonMapping::OnButtonPress(MOUSE::BUTTON_ID button)
{
  if (m_buttonMapping)
    return m_buttonMapping->OnButtonPress(button);

  return false;
}

void CAddonButtonMapping::OnButtonRelease(MOUSE::BUTTON_ID button)
{
  if (m_buttonMapping)
    m_buttonMapping->OnButtonRelease(button);
}

void CAddonButtonMapping::SaveButtonMap()
{
  if (m_buttonMapping)
    m_buttonMapping->SaveButtonMap();
}

void CAddonButtonMapping::ResetIgnoredPrimitives()
{
  if (m_buttonMapping)
    m_buttonMapping->ResetIgnoredPrimitives();
}

void CAddonButtonMapping::RevertButtonMap()
{
  if (m_buttonMapping)
    m_buttonMapping->RevertButtonMap();
}
