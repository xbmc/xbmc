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

#include "DefaultJoystick.h"
#include "KeymapHandler.h"
#include "JoystickEasterEgg.h"
#include "JoystickIDs.h"
#include "JoystickTranslator.h"
#include "guilib/GUIWindowManager.h"
#include "input/InputManager.h"
#include "input/Key.h"
#include "Application.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

#define ANALOG_DIGITAL_THRESHOLD   0.5f

using namespace KODI;
using namespace JOYSTICK;

CDefaultJoystick::CDefaultJoystick(void) :
  m_handler(new CKeymapHandler(&CInputManager::GetInstance()))
{
}

CDefaultJoystick::~CDefaultJoystick(void)
{
  delete m_handler;
}

std::string CDefaultJoystick::ControllerID(void) const
{
  return GetControllerID();
}

bool CDefaultJoystick::HasFeature(const FeatureName& feature) const
{
  if (GetKeyID(feature) != 0)
    return true;

  // Try analog stick directions
  if (GetKeyID(feature, ANALOG_STICK_DIRECTION::UP)    != 0 ||
      GetKeyID(feature, ANALOG_STICK_DIRECTION::DOWN)  != 0 ||
      GetKeyID(feature, ANALOG_STICK_DIRECTION::RIGHT) != 0 ||
      GetKeyID(feature, ANALOG_STICK_DIRECTION::LEFT)  != 0)
    return true;

  return false;
}

bool CDefaultJoystick::AcceptsInput()
{
  return g_application.IsAppFocused();
}

unsigned int CDefaultJoystick::GetDelayMs(const FeatureName& feature) const
{
  return m_handler->GetHoldTimeMs(GetKeyID(feature), GetWindowID(), GetFallthrough());
}

bool CDefaultJoystick::OnButtonPress(const FeatureName& feature, bool bPressed)
{
  if (bPressed && m_easterEgg && m_easterEgg->OnButtonPress(feature))
    return true;

  const unsigned int keyId = GetKeyID(feature);
  const int windowId = GetWindowID();
  const bool bFallthrough = GetFallthrough();

  m_handler->OnDigitalKey(keyId, windowId, bFallthrough, bPressed);
  return true;
}

void CDefaultJoystick::OnButtonHold(const FeatureName& feature, unsigned int holdTimeMs)
{
  const unsigned int keyId = GetKeyID(feature);
  const int windowId = GetWindowID();
  const bool bFallthrough = GetFallthrough();

  m_handler->OnDigitalKey(keyId, windowId, bFallthrough, true, holdTimeMs);
}

bool CDefaultJoystick::OnButtonMotion(const FeatureName& feature, float magnitude, unsigned int motionTimeMs)
{
  const unsigned int keyId = GetKeyID(feature);
  const int windowId = GetWindowID();
  const bool bFallthrough = GetFallthrough();

  m_handler->OnAnalogKey(keyId, windowId, bFallthrough, magnitude, motionTimeMs);
  return true;
}

bool CDefaultJoystick::OnAnalogStickMotion(const FeatureName& feature, float x, float y, unsigned int motionTimeMs)
{
  bool bHandled = false;

  // Calculate the direction of the stick's position
  const ANALOG_STICK_DIRECTION analogStickDir = CJoystickTranslator::VectorToAnalogStickDirection(x, y);

  // Calculate the magnitude projected onto that direction
  const float magnitude = std::max(std::abs(x), std::abs(y));

  // Deactivate directions in which the stick is not pointing first
  for (ANALOG_STICK_DIRECTION dir : GetDirections())
  {
    if (dir != analogStickDir)
      DeactivateDirection(feature, dir);
  }

  // Now activate direction the analog stick is pointing
  if (magnitude != 0.0f)
    bHandled = ActivateDirection(feature, magnitude, analogStickDir, motionTimeMs);

  return bHandled;
}

bool CDefaultJoystick::OnAccelerometerMotion(const FeatureName& feature, float x, float y, float z)
{
  return false; //! @todo implement
}

unsigned int CDefaultJoystick::GetActionID(const FeatureName& feature)
{
  const unsigned int keyId = GetKeyID(feature);
  const int windowId = GetWindowID();
  const bool bFallthrough = GetFallthrough();

  if (keyId > 0)
    return m_handler->GetActionID(keyId, windowId, bFallthrough);

  return ACTION_NONE;
}

int CDefaultJoystick::GetWindowID() const
{
  return g_windowManager.GetActiveWindowID();
}

bool CDefaultJoystick::ActivateDirection(const FeatureName& feature, float magnitude, ANALOG_STICK_DIRECTION dir, unsigned int motionTimeMs)
{
  bool bHandled = false;

  // Calculate the button key ID and input type for the analog stick's direction
  const unsigned int  keyId     = GetKeyID(feature, dir);
  const int           windowId  = GetWindowID();
  const bool          bFallthrough = GetFallthrough();

  m_handler->OnAnalogKey(keyId, windowId, bFallthrough, magnitude, motionTimeMs);
  bHandled = true;

  if (bHandled)
    m_currentDirections[feature] = dir;

  return bHandled;
}

void CDefaultJoystick::DeactivateDirection(const FeatureName& feature, ANALOG_STICK_DIRECTION dir)
{
  if (m_currentDirections[feature] == dir)
  {
    // Calculate the button key ID and input type for this direction
    const unsigned int  keyId     = GetKeyID(feature, dir);
    const int           windowId  = GetWindowID();
    const bool          bFallthrough = GetFallthrough();

    m_handler->OnAnalogKey(keyId, windowId, bFallthrough, 0.0f, 0);

    m_currentDirections[feature] = ANALOG_STICK_DIRECTION::UNKNOWN;
  }
}

const std::vector<ANALOG_STICK_DIRECTION>& CDefaultJoystick::GetDirections(void)
{
  static std::vector<ANALOG_STICK_DIRECTION> directions;
  if (directions.empty())
  {
    directions.push_back(ANALOG_STICK_DIRECTION::UP);
    directions.push_back(ANALOG_STICK_DIRECTION::DOWN);
    directions.push_back(ANALOG_STICK_DIRECTION::RIGHT);
    directions.push_back(ANALOG_STICK_DIRECTION::LEFT);
  }
  return directions;
}
