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
#include "Application.h"
#include "input/InputManager.h"
#include "ServiceBroker.h"

#include <cmath>

#define AXIS_THRESHOLD       0.75f // Axis must exceed this value to be mapped

using namespace KODI;
using namespace JOYSTICK;

bool CJoystickMonitor::OnButtonMotion(unsigned int buttonIndex, bool bPressed)
{
  if (bPressed)
  {
    CServiceBroker::GetInputManager().SetMouseActive(false);
    return ResetTimers();
  }

  return false;
}

bool CJoystickMonitor::OnHatMotion(unsigned int hatIndex, HAT_STATE state)
{
  if (state != HAT_STATE::UNPRESSED)
  {
    CServiceBroker::GetInputManager().SetMouseActive(false);
    return ResetTimers();
  }

  return false;
}

bool CJoystickMonitor::OnAxisMotion(unsigned int axisIndex, float position, int center, unsigned int range)
{
  const bool bIsActive = (std::abs(position) >= AXIS_THRESHOLD);

  if (bIsActive)
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
