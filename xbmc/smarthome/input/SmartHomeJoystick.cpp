/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SmartHomeJoystick.h"

#include "ISmartHomeJoystickHandler.h"
#include "games/controllers/Controller.h"
#include "input/joysticks/interfaces/IInputProvider.h"
#include "peripherals/devices/Peripheral.h"

using namespace KODI;
using namespace SMART_HOME;

CSmartHomeJoystick::CSmartHomeJoystick(PERIPHERALS::PeripheralPtr peripheral,
                                       GAME::ControllerPtr controller,
                                       ISmartHomeJoystickHandler& joystickHandler)
  : m_peripheral(std::move(peripheral)),
    m_controller(std::move(controller)),
    m_joystickHandler(joystickHandler),
    m_controllerState(*m_controller)
{
  // Upcast peripheral to input interface
  JOYSTICK::IInputProvider* inputProvider = m_peripheral.get();

  // Register input handler to capture all input
  inputProvider->RegisterInputHandler(this, false);
}

CSmartHomeJoystick::~CSmartHomeJoystick()
{
  // Upcast peripheral to input interface
  JOYSTICK::IInputProvider* inputProvider = m_peripheral.get();

  // Unregister input handler
  inputProvider->UnregisterInputHandler(this);
}

std::string CSmartHomeJoystick::ControllerID(void) const
{
  return m_controller->ID();
}

bool CSmartHomeJoystick::HasFeature(const std::string& feature) const
{
  return true; // Capture input for all features
}

bool CSmartHomeJoystick::AcceptsInput(const std::string& feature) const
{
  return true; // Accept input for all features
}

bool CSmartHomeJoystick::OnButtonPress(const std::string& feature, bool bPressed)
{
  m_controllerState.SetDigitalButton(feature, bPressed);
  return true;
}

bool CSmartHomeJoystick::OnButtonMotion(const std::string& feature,
                                        float magnitude,
                                        unsigned int motionTimeMs)
{
  m_controllerState.SetAnalogButton(feature, magnitude);
  return true;
}

bool CSmartHomeJoystick::OnAnalogStickMotion(const std::string& feature,
                                             float x,
                                             float y,
                                             unsigned int motionTimeMs)
{
  m_controllerState.SetAnalogStick(feature, {x, y});
  return true;
}

bool CSmartHomeJoystick::OnAccelerometerMotion(const std::string& feature,
                                               float x,
                                               float y,
                                               float z)
{
  m_controllerState.SetAccelerometer(feature, {x, y, z});
  return true;
}

bool CSmartHomeJoystick::OnWheelMotion(const std::string& feature,
                                       float position,
                                       unsigned int motionTimeMs)
{
  m_controllerState.SetWheel(feature, position);
  return true;
}

bool CSmartHomeJoystick::OnThrottleMotion(const std::string& feature,
                                          float position,
                                          unsigned int motionTimeMs)
{
  m_controllerState.SetThrottle(feature, position);
  return true;
}

void CSmartHomeJoystick::OnInputFrame()
{
  return m_joystickHandler.OnInputFrame(*m_peripheral, m_controllerState);
}
