/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Port.h"

#include "InputSink.h"
#include "games/addons/GameClient.h"
#include "guilib/WindowIDs.h"
#include "input/joysticks/keymaps/KeymapHandling.h"
#include "peripherals/devices/Peripheral.h"

using namespace KODI;
using namespace GAME;

CPort::CPort(JOYSTICK::IInputHandler *gameInput) :
  m_gameInput(gameInput),
  m_inputSink(new CInputSink(gameInput))
{
}

CPort::~CPort() = default;

void CPort::RegisterInput(JOYSTICK::IInputProvider *provider)
{
  // Give input sink the lowest priority by registering it before the other
  // input handlers
  provider->RegisterInputHandler(m_inputSink.get(), false);

  // Register input handler
  provider->RegisterInputHandler(this, false);

  // Register GUI input
  m_appInput.reset(new JOYSTICK::CKeymapHandling(provider, false, this));
}

void CPort::UnregisterInput(JOYSTICK::IInputProvider *provider)
{
  // Unregister in reverse order
  if (provider == nullptr)
    m_appInput->UnregisterInputProvider();
  m_appInput.reset();

  if (provider != nullptr)
  {
    provider->UnregisterInputHandler(this);
    provider->UnregisterInputHandler(m_inputSink.get());
  }
}

std::string CPort::ControllerID() const
{
  return m_gameInput->ControllerID();
}

bool CPort::AcceptsInput(const std::string& feature) const
{
  return m_gameInput->AcceptsInput(feature);
}

bool CPort::OnButtonPress(const std::string& feature, bool bPressed)
{
  if (bPressed && !m_gameInput->AcceptsInput(feature))
    return false;

  return m_gameInput->OnButtonPress(feature, bPressed);
}

void CPort::OnButtonHold(const std::string& feature, unsigned int holdTimeMs)
{
  m_gameInput->OnButtonHold(feature, holdTimeMs);
}

bool CPort::OnButtonMotion(const std::string& feature, float magnitude, unsigned int motionTimeMs)
{
  if (magnitude > 0.0f && !m_gameInput->AcceptsInput(feature))
    return false;

  return m_gameInput->OnButtonMotion(feature, magnitude, motionTimeMs);
}

bool CPort::OnAnalogStickMotion(const std::string& feature, float x, float y, unsigned int motionTimeMs)
{
  if ((x != 0.0f || y != 0.0f) && !m_gameInput->AcceptsInput(feature))
    return false;

  return m_gameInput->OnAnalogStickMotion(feature, x, y, motionTimeMs);
}

bool CPort::OnAccelerometerMotion(const std::string& feature, float x, float y, float z)
{
  if (!m_gameInput->AcceptsInput(feature))
    return false;

  return m_gameInput->OnAccelerometerMotion(feature, x, y, z);
}

bool CPort::OnWheelMotion(const std::string& feature, float position, unsigned int motionTimeMs)
{
  if ((position != 0.0f) && !m_gameInput->AcceptsInput(feature))
    return false;

  return m_gameInput->OnWheelMotion(feature, position, motionTimeMs);
}

bool CPort::OnThrottleMotion(const std::string& feature, float position, unsigned int motionTimeMs)
{
  if ((position != 0.0f) && !m_gameInput->AcceptsInput(feature))
    return false;

  return m_gameInput->OnThrottleMotion(feature, position, motionTimeMs);
}

int CPort::GetWindowID() const
{
  return WINDOW_FULLSCREEN_GAME;
}
