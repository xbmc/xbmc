/*
*  Copyright (C) 2023-2024 Team Kodi
*  This file is part of Kodi - https://kodi.tv
*
*  SPDX-License-Identifier: GPL-2.0-or-later
*  See LICENSES/README.md for more information.
*/

#include "AgentJoystick.h"

#include "games/controllers/Controller.h"
#include "games/controllers/ControllerIDs.h"
#include "games/controllers/input/ControllerActivity.h"
#include "input/joysticks/interfaces/IInputProvider.h"
#include "peripherals/devices/Peripheral.h"

using namespace KODI;
using namespace GAME;

CAgentJoystick::CAgentJoystick(PERIPHERALS::PeripheralPtr peripheral)
  : m_peripheral(std::move(peripheral)),
    m_controllerActivity(std::make_unique<CControllerActivity>())
{
}

CAgentJoystick::~CAgentJoystick() = default;

void CAgentJoystick::Initialize()
{
  // Record appearance to detect changes
  m_controllerAppearance = m_peripheral->ControllerProfile();

  // Upcast peripheral to input interface
  JOYSTICK::IInputProvider* inputProvider = m_peripheral.get();

  // Register input handler to silently observe all input
  inputProvider->RegisterInputHandler(this, true);
}

void CAgentJoystick::Deinitialize()
{
  // Upcast peripheral to input interface
  JOYSTICK::IInputProvider* inputProvider = m_peripheral.get();

  // Unregister input handler
  inputProvider->UnregisterInputHandler(this);

  // Reset appearance
  m_controllerAppearance.reset();
}

void CAgentJoystick::ClearButtonState()
{
  return m_controllerActivity->ClearButtonState();
}

float CAgentJoystick::GetActivation() const
{
  return m_controllerActivity->GetActivation();
}

std::string CAgentJoystick::ControllerID(void) const
{
  if (m_controllerAppearance)
    return m_controllerAppearance->ID();

  return DEFAULT_CONTROLLER_ID;
}

bool CAgentJoystick::HasFeature(const std::string& feature) const
{
  return true; // Capture input for all features
}

bool CAgentJoystick::AcceptsInput(const std::string& feature) const
{
  return true; // Accept input for all features
}

bool CAgentJoystick::OnButtonPress(const std::string& feature, bool bPressed)
{
  m_controllerActivity->OnButtonPress(bPressed);
  return true;
}

void CAgentJoystick::OnButtonHold(const std::string& feature, unsigned int holdTimeMs)
{
  m_controllerActivity->OnButtonPress(true);
}

bool CAgentJoystick::OnButtonMotion(const std::string& feature,
                                    float magnitude,
                                    unsigned int motionTimeMs)
{
  m_controllerActivity->OnButtonMotion(magnitude);
  return true;
}

bool CAgentJoystick::OnAnalogStickMotion(const std::string& feature,
                                         float x,
                                         float y,
                                         unsigned int motionTimeMs)
{
  m_controllerActivity->OnAnalogStickMotion(x, y);
  return true;
}

bool CAgentJoystick::OnWheelMotion(const std::string& feature,
                                   float position,
                                   unsigned int motionTimeMs)
{
  m_controllerActivity->OnWheelMotion(position);
  return true;
}

bool CAgentJoystick::OnThrottleMotion(const std::string& feature,
                                      float position,
                                      unsigned int motionTimeMs)
{
  m_controllerActivity->OnThrottleMotion(position);
  return true;
}

void CAgentJoystick::OnInputFrame()
{
  m_controllerActivity->OnInputFrame();
}
