/*
 *  Copyright (C) 2014-2024 Team Kodi
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
#include "peripherals/addons/AddonButtonMap.h"
#include "peripherals/devices/Peripheral.h"
#include "utils/log.h"

using namespace KODI;
using namespace JOYSTICK;
using namespace PERIPHERALS;

CAddonInputHandling::CAddonInputHandling(CPeripheral* peripheral,
                                         std::shared_ptr<CPeripheralAddon> addon,
                                         IInputHandler* handler,
                                         IDriverReceiver* receiver)
  : m_peripheral(peripheral),
    m_addon(std::move(addon)),
    m_joystickInputHandler(handler),
    m_joystickDriverReceiver(receiver)
{
}

CAddonInputHandling::CAddonInputHandling(CPeripheral* peripheral,
                                         std::shared_ptr<CPeripheralAddon> addon,
                                         KEYBOARD::IKeyboardInputHandler* handler)
  : m_peripheral(peripheral), m_addon(std::move(addon)), m_keyboardInputHandler(handler)
{
}

CAddonInputHandling::CAddonInputHandling(CPeripheral* peripheral,
                                         std::shared_ptr<CPeripheralAddon> addon,
                                         MOUSE::IMouseInputHandler* handler)
  : m_peripheral(peripheral), m_addon(std::move(addon)), m_mouseInputHandler(handler)
{
}

CAddonInputHandling::~CAddonInputHandling(void)
{
  m_joystickDriverHandler.reset();
  m_joystickInputReceiver.reset();
  m_keyboardDriverHandler.reset();
  m_mouseDriverHandler.reset();
  m_buttonMap.reset();
}

bool CAddonInputHandling::Load()
{
  std::string controllerId;
  if (m_joystickInputHandler != nullptr)
    controllerId = m_joystickInputHandler->ControllerID();
  else if (m_keyboardInputHandler != nullptr)
    controllerId = m_keyboardInputHandler->ControllerID();
  else if (m_mouseInputHandler != nullptr)
    controllerId = m_mouseInputHandler->ControllerID();

  if (!controllerId.empty())
    m_buttonMap = std::make_unique<CAddonButtonMap>(m_peripheral, m_addon, controllerId);

  if (m_buttonMap && m_buttonMap->Load())
  {
    if (m_joystickInputHandler != nullptr)
    {
      m_joystickDriverHandler =
          std::make_unique<CInputHandling>(m_joystickInputHandler, m_buttonMap.get());
      if (m_joystickDriverReceiver != nullptr)
      {
        m_joystickInputReceiver =
            std::make_unique<CDriverReceiving>(m_joystickDriverReceiver, m_buttonMap.get());

        // Interfaces are connected here because they share button map as a common resource
        m_joystickInputHandler->SetInputReceiver(m_joystickInputReceiver.get());
      }
      return true;
    }
    else if (m_keyboardInputHandler != nullptr)
    {
      m_keyboardDriverHandler = std::make_unique<KEYBOARD::CKeyboardInputHandling>(
          m_keyboardInputHandler, m_buttonMap.get());
      return true;
    }
    else if (m_mouseInputHandler != nullptr)
    {
      m_mouseDriverHandler =
          std::make_unique<MOUSE::CMouseInputHandling>(m_mouseInputHandler, m_buttonMap.get());
      return true;
    }
  }

  return false;
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
