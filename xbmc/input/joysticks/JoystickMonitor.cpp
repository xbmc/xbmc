/*
 *      Copyright (C) 2015-2017 Team Kodi
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

#include "JoystickMonitor.h"
#include "JoystickIDs.h"
#include "Application.h"
#include "input/InputManager.h"
#include "ServiceBroker.h"

#include <cmath>

using namespace KODI;
using namespace JOYSTICK;

#define AXIS_DEADZONE  0.05f

std::string CJoystickMonitor::ControllerID() const
{
  return DEFAULT_CONTROLLER_ID;
}

bool CJoystickMonitor::AcceptsInput(const FeatureName& feature) const
{
  // Only accept input when screen saver is active
  return g_application.IsInScreenSaver();
}

bool CJoystickMonitor::OnButtonPress(const FeatureName& feature, bool bPressed)
{
  if (bPressed)
  {
    CServiceBroker::GetInputManager().SetMouseActive(false);
    return ResetTimers();
  }

  return false;
}

bool CJoystickMonitor::OnButtonMotion(const FeatureName& feature, float magnitude, unsigned int motionTimeMs)
{
  if (std::fabs(magnitude) > AXIS_DEADZONE)
  {
    CServiceBroker::GetInputManager().SetMouseActive(false);
    return ResetTimers();
  }

  return false;
}

bool CJoystickMonitor::OnAnalogStickMotion(const FeatureName& feature, float x, float y, unsigned int motionTimeMs)
{
  // Analog stick deadzone already processed
  if (x != 0.0f || y != 0.0f)
  {
    CServiceBroker::GetInputManager().SetMouseActive(false);
    return ResetTimers();
  }

  return false;
}

bool CJoystickMonitor::ResetTimers(void)
{
  g_application.ResetSystemIdleTimer();
  g_application.ResetScreenSaver();
  return g_application.WakeUpScreenSaverAndDPMS();
}
