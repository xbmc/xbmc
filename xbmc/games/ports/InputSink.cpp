/*
 *      Copyright (C) 2017-present Team Kodi
 *      This file is part of Kodi - https://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "InputSink.h"
#include "games/controllers/ControllerIDs.h"

using namespace KODI;
using namespace GAME;

CInputSink::CInputSink(JOYSTICK::IInputHandler* gameInput) :
  m_gameInput(gameInput)
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

bool CInputSink::OnButtonMotion(const std::string& feature, float magnitude, unsigned int motionTimeMs)
{
  return true;
}

bool CInputSink::OnAnalogStickMotion(const std::string& feature, float x, float y, unsigned int motionTimeMs)
{
  return true;
}

bool CInputSink::OnAccelerometerMotion(const std::string& feature, float x, float y, float z)
{
  return true;
}

bool CInputSink::OnWheelMotion(const std::string& feature, float position, unsigned int motionTimeMs)
{
  return true;
}

bool CInputSink::OnThrottleMotion(const std::string& feature, float position, unsigned int motionTimeMs)
{
  return true;
}
