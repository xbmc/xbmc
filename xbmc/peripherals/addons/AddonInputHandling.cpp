/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AddonInputHandling.h"

#include "input/joysticks/generic/DriverReceiving.h"
#include "input/joysticks/generic/InputHandling.h"
#include "input/joysticks/interfaces/IDriverReceiver.h"
#include "input/joysticks/interfaces/IInputHandler.h"
#include "input/keyboard/generic/KeyboardInputHandling.h"
#include "input/keyboard/interfaces/IKeyboardInputHandler.h"
#include "input/mouse/generic/MouseInputHandling.h"
#include "input/mouse/interfaces/IMouseInputHandler.h"
#include "peripherals/Peripherals.h"
#include "peripherals/addons/AddonButtonMap.h"
#include "utils/log.h"

using namespace KODI;
using namespace JOYSTICK;
using namespace PERIPHERALS;

CAddonInputHandling::CAddonInputHandling(CPeripherals& manager,
                                         CPeripheral* peripheral,
                                         IInputHandler* handler,
                                         IDriverReceiver* receiver)
{
  PeripheralAddonPtr addon = manager.GetAddonWithButtonMap(peripheral);

  if (!addon)
  {
    CLog::Log(LOGDEBUG, "Failed to locate add-on for \"{}\"", peripheral->DeviceName());
  }
  else
  {
    m_buttonMap.reset(new CAddonButtonMap(peripheral, addon, handler->ControllerID()));
    if (m_buttonMap->Load())
    {
      m_joystickDriverHandler = std::make_unique<CInputHandling>(handler, m_buttonMap.get());

      if (receiver)
      {
        m_joystickInputReceiver = std::make_unique<CDriverReceiving>(receiver, m_buttonMap.get());

        // Interfaces are connected here because they share button map as a common resource
        handler->SetInputReceiver(m_joystickInputReceiver.get());
      }
    }
    else
    {
      m_buttonMap.reset();
    }
  }
}

CAddonInputHandling::CAddonInputHandling(CPeripherals& manager,
                                         CPeripheral* peripheral,
                                         KEYBOARD::IKeyboardInputHandler* handler)
{
  PeripheralAddonPtr addon = manager.GetAddonWithButtonMap(peripheral);

  if (!addon)
  {
    CLog::Log(LOGDEBUG, "Failed to locate add-on for \"{}\"", peripheral->DeviceName());
  }
  else
  {
    m_buttonMap.reset(new CAddonButtonMap(peripheral, addon, handler->ControllerID()));
    if (m_buttonMap->Load())
    {
      m_keyboardDriverHandler =
          std::make_unique<KEYBOARD::CKeyboardInputHandling>(handler, m_buttonMap.get());
    }
    else
    {
      m_buttonMap.reset();
    }
  }
}

CAddonInputHandling::CAddonInputHandling(CPeripherals& manager,
                                         CPeripheral* peripheral,
                                         MOUSE::IMouseInputHandler* handler)
{
  PeripheralAddonPtr addon = manager.GetAddonWithButtonMap(peripheral);

  if (!addon)
  {
    CLog::Log(LOGDEBUG, "Failed to locate add-on for \"{}\"", peripheral->DeviceName());
  }
  else
  {
    m_buttonMap.reset(new CAddonButtonMap(peripheral, addon, handler->ControllerID()));
    if (m_buttonMap->Load())
    {
      m_mouseDriverHandler =
          std::make_unique<MOUSE::CMouseInputHandling>(handler, m_buttonMap.get());
    }
    else
    {
      m_buttonMap.reset();
    }
  }
}

CAddonInputHandling::~CAddonInputHandling(void)
{
  m_joystickDriverHandler.reset();
  m_joystickInputReceiver.reset();
  m_keyboardDriverHandler.reset();
  m_mouseDriverHandler.reset();
  m_buttonMap.reset();
}

bool CAddonInputHandling::OnButtonMotion(unsigned int buttonIndex, bool bPressed)
{
  if (m_joystickDriverHandler)
    return m_joystickDriverHandler->OnButtonMotion(buttonIndex, bPressed);

  return false;
}

bool CAddonInputHandling::OnHatMotion(unsigned int hatIndex, HAT_STATE state)
{
  if (m_joystickDriverHandler)
    return m_joystickDriverHandler->OnHatMotion(hatIndex, state);

  return false;
}

bool CAddonInputHandling::OnAxisMotion(unsigned int axisIndex,
                                       float position,
                                       int center,
                                       unsigned int range)
{
  if (m_joystickDriverHandler)
    return m_joystickDriverHandler->OnAxisMotion(axisIndex, position, center, range);

  return false;
}

void CAddonInputHandling::OnInputFrame(void)
{
  if (m_joystickDriverHandler)
    m_joystickDriverHandler->OnInputFrame();
}

bool CAddonInputHandling::OnKeyPress(const CKey& key)
{
  if (m_keyboardDriverHandler)
    return m_keyboardDriverHandler->OnKeyPress(key);

  return false;
}

void CAddonInputHandling::OnKeyRelease(const CKey& key)
{
  if (m_keyboardDriverHandler)
    m_keyboardDriverHandler->OnKeyRelease(key);
}

bool CAddonInputHandling::OnPosition(int x, int y)
{
  if (m_mouseDriverHandler)
    return m_mouseDriverHandler->OnPosition(x, y);

  return false;
}

bool CAddonInputHandling::OnButtonPress(MOUSE::BUTTON_ID button)
{
  if (m_mouseDriverHandler)
    return m_mouseDriverHandler->OnButtonPress(button);

  return false;
}

void CAddonInputHandling::OnButtonRelease(MOUSE::BUTTON_ID button)
{
  if (m_mouseDriverHandler)
    m_mouseDriverHandler->OnButtonRelease(button);
}

bool CAddonInputHandling::SetRumbleState(const JOYSTICK::FeatureName& feature, float magnitude)
{
  if (m_joystickInputReceiver)
    return m_joystickInputReceiver->SetRumbleState(feature, magnitude);

  return false;
}
