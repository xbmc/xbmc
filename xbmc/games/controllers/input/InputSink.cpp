/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "InputSink.h"

#include "games/controllers/ControllerIDs.h"

using namespace KODI;
using namespace GAME;

CInputSink::CInputSink(JOYSTICK::IInputHandler* gameInput) : m_gameInput(gameInput)
{
}

std::string CInputSink::ControllerID(void) const
{
  return DEFAULT_CONTROLLER_ID;
}

bool CInputSink::AcceptsInput(const std::string& feature) const
{
  return m_gameInput->AcceptsInput(feature);
}

bool CInputSink::OnButtonPress(const std::string& feature, bool bPressed)
{
  return true;
}

bool CInputSink::OnButtonMotion(const std::string& feature,
                                float magnitude,
                                unsigned int motionTimeMs)
{
  return true;
}

bool CInputSink::OnAnalogStickMotion(const std::string& feature,
                                     float x,
                                     float y,
                                     unsigned int motionTimeMs)
{
  return true;
}

bool CInputSink::OnAccelerometerMotion(const std::string& feature, float x, float y, float z)
{
  return true;
}

bool CInputSink::OnWheelMotion(const std::string& feature,
                               float position,
                               unsigned int motionTimeMs)
{
  return true;
}

bool CInputSink::OnThrottleMotion(const std::string& feature,
                                  float position,
                                  unsigned int motionTimeMs)
{
  return true;
}
