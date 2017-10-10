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

#include "FeatureHandling.h"
#include "games/controllers/Controller.h"
#include "games/controllers/ControllerManager.h"
#include "input/joysticks/DriverPrimitive.h"
#include "input/joysticks/IButtonMap.h"
#include "input/joysticks/IInputHandler.h"
#include "threads/SystemClock.h"
#include "utils/log.h"
#include "ServiceBroker.h"

#include <vector>

using namespace KODI;
using namespace JOYSTICK;

#define ANALOG_DIGITAL_THRESHOLD  0.5f
#define DISCRETE_ANALOG_RAMPUP_TIME_MS  1500
#define DISCRETE_ANALOG_START_VALUE     0.3f

// --- CJoystickFeature --------------------------------------------------------

CJoystickFeature::CJoystickFeature(const FeatureName& name, IInputHandler* handler, IButtonMap* buttonMap) :
  m_name(name),
  m_handler(handler),
  m_buttonMap(buttonMap),
  m_bEnabled(m_handler->HasFeature(name))
{
}

bool CJoystickFeature::AcceptsInput(bool bActivation)
{
  bool bAcceptsInput = false;

  if (m_bEnabled)
  {
    if (m_handler->AcceptsInput(m_name))
      bAcceptsInput = true;
  }

  return bAcceptsInput;
}

// --- CScalarFeature ----------------------------------------------------------

CScalarFeature::CScalarFeature(const FeatureName& name, IInputHandler* handler, IButtonMap* buttonMap) :
  CJoystickFeature(name, handler, buttonMap),
  m_inputType(INPUT_TYPE::UNKNOWN),
  m_bDigitalState(false),
  m_motionStartTimeMs(0),
  m_analogState(0.0f),
  m_bActivated(false),
  m_bDiscrete(true)
{
  GAME::ControllerPtr controller = CServiceBroker::GetGameControllerManager().GetController(handler->ControllerID());
  if (controller)
    m_inputType = controller->GetInputType(name);
}

bool CScalarFeature::OnDigitalMotion(const CDriverPrimitive& source, bool bPressed)
{
  // Feature must accept input to be considered handled
  bool bHandled = AcceptsInput(bPressed);

  if (m_inputType == INPUT_TYPE::DIGITAL)
    bHandled &= OnDigitalMotion(bPressed);
  else if (m_inputType == INPUT_TYPE::ANALOG)
    bHandled &= OnAnalogMotion(bPressed ? 1.0f : 0.0f);

  return bHandled;
}

bool CScalarFeature::OnAnalogMotion(const CDriverPrimitive& source, float magnitude)
{
  // Update activated status
  if (magnitude > 0.0f)
    m_bActivated = true;

  // Update discrete status
  if (magnitude != 0.0f && magnitude != 1.0f)
    m_bDiscrete = false;

  // Feature must accept input to be considered handled
  bool bHandled = AcceptsInput(magnitude > 0.0f);

  if (m_inputType == INPUT_TYPE::DIGITAL)
    bHandled &= OnDigitalMotion(magnitude >= ANALOG_DIGITAL_THRESHOLD);
  else if (m_inputType == INPUT_TYPE::ANALOG)
    bHandled &= OnAnalogMotion(magnitude);

  return bHandled;
}

void CScalarFeature::ProcessMotions(void)
{
  if (m_inputType == INPUT_TYPE::DIGITAL && m_bDigitalState)
    ProcessDigitalMotion();
  else if (m_inputType == INPUT_TYPE::ANALOG)
    ProcessAnalogMotion();
}

bool CScalarFeature::OnDigitalMotion(bool bPressed)
{
  bool bHandled = false;

  if (m_bDigitalState != bPressed)
  {
    m_bDigitalState = bPressed;
    m_motionStartTimeMs = 0; // This is set in ProcessMotions()

    bHandled = m_bInitialPressHandled = m_handler->OnButtonPress(m_name, bPressed);

    if (m_bDigitalState)
      CLog::Log(LOGDEBUG, "FEATURE [ %s ] on %s pressed (%s)", m_name.c_str(), m_handler->ControllerID().c_str(),
        bHandled ? "handled" : "ignored");
    else
      CLog::Log(LOGDEBUG, "FEATURE [ %s ] on %s released", m_name.c_str(), m_handler->ControllerID().c_str());
  }
  else if (m_bDigitalState)
  {
    bHandled = m_bInitialPressHandled;
  }

  return bHandled;
}

bool CScalarFeature::OnAnalogMotion(float magnitude)
{
  const bool bActivated = (magnitude != 0.0f);

  // Update analog state
  m_analogState = magnitude;

  // Update motion time
  if (!bActivated)
    m_motionStartTimeMs = 0;
  else if (m_motionStartTimeMs == 0)
    m_motionStartTimeMs = XbmcThreads::SystemClockMillis();

  // Log activation/deactivation
  if (m_bDigitalState != bActivated)
  {
    m_bDigitalState = bActivated;
    CLog::Log(LOGDEBUG, "FEATURE [ %s ] on %s %s", m_name.c_str(), m_handler->ControllerID().c_str(),
              bActivated ? "activated" : "deactivated");
  }

  return true;
}

void CScalarFeature::ProcessDigitalMotion()
{
  if (m_motionStartTimeMs == 0)
  {
    // Button was just pressed, record start time and exit (button press event
    // was already sent this frame)
    m_motionStartTimeMs = XbmcThreads::SystemClockMillis();
  }
  else
  {
    // Button has been pressed more than one event frame
    const unsigned int elapsed = XbmcThreads::SystemClockMillis() - m_motionStartTimeMs;
    m_handler->OnButtonHold(m_name, elapsed);
  }
}

void CScalarFeature::ProcessAnalogMotion()
{
  float magnitude = m_analogState;

  // Calculate time elapsed since motion began
  unsigned int elapsed = 0;
  if (m_motionStartTimeMs > 0)
    elapsed = XbmcThreads::SystemClockMillis() - m_motionStartTimeMs;

  // If analog value is discrete, ramp up magnitude
  if (m_bActivated && m_bDiscrete)
  {
    if (elapsed < DISCRETE_ANALOG_RAMPUP_TIME_MS)
    {
      magnitude *= static_cast<float>(elapsed) / DISCRETE_ANALOG_RAMPUP_TIME_MS;
      if (magnitude < DISCRETE_ANALOG_START_VALUE)
        magnitude = DISCRETE_ANALOG_START_VALUE;
    }
  }

  m_handler->OnButtonMotion(m_name, magnitude, elapsed);
}

// --- CAnalogStick ------------------------------------------------------------

CAnalogStick::CAnalogStick(const FeatureName& name, IInputHandler* handler, IButtonMap* buttonMap) :
  CJoystickFeature(name, handler, buttonMap),
  m_vertState(0.0f),
  m_horizState(0.0f),
  m_motionStartTimeMs(0)
{
}

bool CAnalogStick::OnDigitalMotion(const CDriverPrimitive& source, bool bPressed)
{
  return OnAnalogMotion(source, bPressed ? 1.0f : 0.0f);
}

bool CAnalogStick::OnAnalogMotion(const CDriverPrimitive& source, float magnitude)
{
  ANALOG_STICK_DIRECTION direction = ANALOG_STICK_DIRECTION::UNKNOWN;

  std::vector<ANALOG_STICK_DIRECTION> dirs = {
    ANALOG_STICK_DIRECTION::UP,
    ANALOG_STICK_DIRECTION::DOWN,
    ANALOG_STICK_DIRECTION::RIGHT,
    ANALOG_STICK_DIRECTION::LEFT,
  };

  CDriverPrimitive primitive;
  for (auto dir : dirs)
  {
    if (m_buttonMap->GetAnalogStick(m_name, dir, primitive) && primitive == source)
    {
      direction = dir;
      break;
    }
  }

  // Feature must accept input to be considered handled
  bool bHandled = AcceptsInput(magnitude > 0.0f);

  switch (direction)
  {
  case ANALOG_STICK_DIRECTION::UP:
    m_vertAxis.SetPositiveDistance(magnitude);
    break;
  case ANALOG_STICK_DIRECTION::DOWN:
    m_vertAxis.SetNegativeDistance(magnitude);
    break;
  case ANALOG_STICK_DIRECTION::RIGHT:
    m_horizAxis.SetPositiveDistance(magnitude);
    break;
  case ANALOG_STICK_DIRECTION::LEFT:
    m_horizAxis.SetNegativeDistance(magnitude);
    break;
  default:
    // Just in case, avoid sticking
    m_vertAxis.Reset();
    m_horizAxis.Reset();
    break;
  }

  return bHandled;
}

void CAnalogStick::ProcessMotions(void)
{
  const float newVertState = m_vertAxis.GetPosition();
  const float newHorizState = m_horizAxis.GetPosition();

  const bool bActivated = (newVertState != 0.0f || newHorizState != 0.0f);

  if (!AcceptsInput(bActivated))
    return;

  const bool bWasActivated = (m_vertState != 0.0f || m_horizState != 0.0f);

  if (bActivated ^ bWasActivated)
  {
    CLog::Log(LOGDEBUG, "Feature [ %s ] on %s %s", m_name.c_str(), m_handler->ControllerID().c_str(),
              bActivated ? "activated" : "deactivated");
  }

  if (bActivated || bWasActivated)
  {
    m_vertState = newVertState;
    m_horizState = newHorizState;

    unsigned int motionTimeMs = 0;

    if (bActivated)
    {
      if (m_motionStartTimeMs == 0)
        m_motionStartTimeMs = XbmcThreads::SystemClockMillis();
      else
        motionTimeMs = XbmcThreads::SystemClockMillis() - m_motionStartTimeMs;
    }
    else
    {
      m_motionStartTimeMs = 0;
    }

    m_handler->OnAnalogStickMotion(m_name, newHorizState, newVertState, motionTimeMs);
  }
}

// --- CAccelerometer ----------------------------------------------------------

CAccelerometer::CAccelerometer(const FeatureName& name, IInputHandler* handler, IButtonMap* buttonMap) :
  CJoystickFeature(name, handler, buttonMap),
  m_xAxisState(0.0f),
  m_yAxisState(0.0f),
  m_zAxisState(0.0f)
{
}

bool CAccelerometer::OnDigitalMotion(const CDriverPrimitive& source, bool bPressed)
{
  return OnAnalogMotion(source, bPressed ? 1.0f : 0.0f);
}

bool CAccelerometer::OnAnalogMotion(const CDriverPrimitive& source, float magnitude)
{
  // Feature must accept input to be considered handled
  bool bHandled = AcceptsInput(true);

  CDriverPrimitive positiveX;
  CDriverPrimitive positiveY;
  CDriverPrimitive positiveZ;

  m_buttonMap->GetAccelerometer(m_name, positiveX, positiveY, positiveZ);

  if (source == positiveX)
  {
    m_xAxis.SetPositiveDistance(magnitude);
  }
  else if (source == positiveY)
  {
    m_yAxis.SetPositiveDistance(magnitude);
  }
  else if (source == positiveZ)
  {
    m_zAxis.SetPositiveDistance(magnitude);
  }
  else
  {
    // Just in case, avoid sticking
    m_xAxis.Reset();
    m_xAxis.Reset();
    m_yAxis.Reset();
  }

  return bHandled;
}

void CAccelerometer::ProcessMotions(void)
{
  const float newXAxis = m_xAxis.GetPosition();
  const float newYAxis = m_yAxis.GetPosition();
  const float newZAxis = m_zAxis.GetPosition();

  const bool bActivated = (newXAxis != 0.0f || newYAxis != 0.0f || newZAxis != 0.0f);

  if (!AcceptsInput(bActivated))
    return;

  const bool bWasActivated = (m_xAxisState != 0.0f || m_yAxisState != 0.0f || m_zAxisState != 0.0f);

  if (bActivated || bWasActivated)
  {
    m_xAxisState = newXAxis;
    m_yAxisState = newYAxis;
    m_zAxisState = newZAxis;
    m_handler->OnAccelerometerMotion(m_name, newXAxis, newYAxis, newZAxis);
  }
}
