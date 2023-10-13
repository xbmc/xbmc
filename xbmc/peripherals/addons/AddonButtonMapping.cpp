/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AddonButtonMapping.h"

#include "input/joysticks/generic/ButtonMapping.h"
#include "input/joysticks/interfaces/IButtonMapper.h"
#include "peripherals/Peripherals.h"
#include "peripherals/addons/AddonButtonMap.h"
#include "utils/log.h"

#include <memory>

using namespace KODI;
using namespace JOYSTICK;
using namespace PERIPHERALS;

CAddonButtonMapping::CAddonButtonMapping(CPeripherals& manager,
                                         CPeripheral* peripheral,
                                         IButtonMapper* mapper)
{
  PeripheralAddonPtr addon = manager.GetAddonWithButtonMap(peripheral);

  if (!addon)
  {
    CLog::Log(LOGDEBUG, "Failed to locate add-on for \"{}\"", peripheral->DeviceName());
  }
  else
  {
    const std::string controllerId = mapper->ControllerID();
    m_buttonMap = std::make_unique<CAddonButtonMap>(peripheral, addon, controllerId);
    if (m_buttonMap->Load())
    {
      IKeymap* keymap = peripheral->GetKeymap(controllerId);
      m_buttonMapping = std::make_unique<CButtonMapping>(mapper, m_buttonMap.get(), keymap);

      // Allow the mapper to save our button map
      mapper->SetButtonMapCallback(peripheral->Location(), this);
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

bool CAddonButtonMapping::OnAxisMotion(unsigned int axisIndex,
                                       float position,
                                       int center,
                                       unsigned int range)
{
  if (m_buttonMapping)
    return m_buttonMapping->OnAxisMotion(axisIndex, position, center, range);

  return false;
}

void CAddonButtonMapping::OnInputFrame(void)
{
  if (m_buttonMapping)
    m_buttonMapping->OnInputFrame();
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
