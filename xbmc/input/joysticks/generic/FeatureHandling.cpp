/*
 *  Copyright (C) 2014-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FeatureHandling.h"

#include "ServiceBroker.h"
#include "games/controllers/Controller.h"
#include "games/controllers/ControllerManager.h"
#include "input/joysticks/DriverPrimitive.h"
#include "input/joysticks/interfaces/IButtonMap.h"
#include "input/joysticks/interfaces/IInputHandler.h"
#include "utils/log.h"

#include <vector>

using namespace KODI;
using namespace JOYSTICK;

#define ANALOG_DIGITAL_THRESHOLD 0.5f
#define DISCRETE_ANALOG_RAMPUP_TIME_MS 1500
#define DISCRETE_ANALOG_START_VALUE 0.3f

// --- CJoystickFeature --------------------------------------------------------

CJoystickFeature::CJoystickFeature(const FeatureName& name,
                                   IInputHandler* handler,
                                   IButtonMap* buttonMap)
  : m_name(name),
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

void CJoystickFeature::ResetMotion()
{
  m_motionStartTimeMs = {};
}

void CJoystickFeature::StartMotion()
{
  m_motionStartTimeMs = std::chrono::steady_clock::now();
}

bool CJoystickFeature::InMotion() const
{
  return m_motionStartTimeMs.time_since_epoch().count() > 0;
}

unsigned int CJoystickFeature::MotionTimeMs() const
{
  if (!InMotion())
    return 0;

  auto now = std::chrono::steady_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_motionStartTimeMs);

  return duration.count();
}

// --- CScalarFeature ----------------------------------------------------------

CScalarFeature::CScalarFeature(const FeatureName& name,
                               IInputHandler* handler,
                               IButtonMap* buttonMap)
  : CJoystickFeature(name, handler, buttonMap)
{
  GAME::ControllerPtr controller =
      CServiceBroker::GetGameControllerManager().GetController(handler->ControllerID());
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

    // Motion is initiated in ProcessMotions()
    ResetMotion();

    bHandled = m_bInitialPressHandled = m_handler->OnButtonPress(m_name, bPressed);

    if (m_bDigitalState)
      CLog::Log(LOGDEBUG, "FEATURE [ {} ] on {} pressed ({})", m_name, m_handler->ControllerID(),
                bHandled ? "handled" : "ignored");
    else
      CLog::Log(LOGDEBUG, "FEATURE [ {} ] on {} released", m_name, m_handler->ControllerID());
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
    ResetMotion();
  else if (!InMotion())
    StartMotion();

  // Log activation/deactivation
  if (m_bDigitalState != bActivated)
  {
    m_bDigitalState = bActivated;
    CLog::Log(LOGDEBUG, "FEATURE [ {} ] on {} {}", m_name, m_handler->ControllerID(),
              bActivated ? "activated" : "deactivated");
  }

  return true;
}

void CScalarFeature::ProcessDigitalMotion()
{
  if (!InMotion())
  {
    // Button was just pressed, record start time and exit (button press event
    // was already sent this frame)
    StartMotion();
  }
  else
  {
    // Button has been pressed more than one event frame
    const unsigned int elapsed = MotionTimeMs();
    m_handler->OnButtonHold(m_name, elapsed);
  }
}

void CScalarFeature::ProcessAnalogMotion()
{
  float magnitude = m_analogState;

  // Calculate time elapsed since motion began
  unsigned int elapsed = MotionTimeMs();

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

// --- CAxisFeature ------------------------------------------------------------

CAxisFeature::CAxisFeature(const FeatureName& name, IInputHandler* handler, IButtonMap* buttonMap)
  : CJoystickFeature(name, handler, buttonMap)
{
}

bool CAxisFeature::OnDigitalMotion(const CDriverPrimitive& source, bool bPressed)
{
  return OnAnalogMotion(source, bPressed ? 1.0f : 0.0f);
}

void CAxisFeature::ProcessMotions(void)
{
  const float newState = m_axis.GetPosition();

  const bool bActivated = (newState != 0.0f);

  if (!AcceptsInput(bActivated))
    return;

  const bool bWasActivated = (m_state != 0.0f);

  if (!bActivated && bWasActivated)
    CLog::Log(LOGDEBUG, "Feature [ {} ] on {} deactivated", m_name, m_handler->ControllerID());
  else if (bActivated && !bWasActivated)
  {
    CLog::Log(LOGDEBUG, "Feature [ {} ] on {} activated {}", m_name, m_handler->ControllerID(),
              newState > 0.0f ? "positive" : "negative");
  }

  if (bActivated || bWasActivated)
  {
    m_state = newState;

    unsigned int motionTimeMs = 0;

    if (bActivated)
    {
      if (!InMotion())
        StartMotion();
      else
        motionTimeMs = MotionTimeMs();
    }
    else
      ResetMotion();

    switch (m_buttonMap->GetFeatureType(m_name))
    {
      case FEATURE_TYPE::WHEEL:
        m_handler->OnWheelMotion(m_name, newState, motionTimeMs);
        break;
      case FEATURE_TYPE::THROTTLE:
        m_handler->OnThrottleMotion(m_name, newState, motionTimeMs);
        break;
      default:
        break;
    }
  }
}

// --- CWheel ------------------------------------------------------------------

CWheel::CWheel(const FeatureName& name, IInputHandler* handler, IButtonMap* buttonMap)
  : CAxisFeature(name, handler, buttonMap)
{
}

bool CWheel::OnAnalogMotion(const CDriverPrimitive& source, float magnitude)
{
  WHEEL_DIRECTION direction = WHEEL_DIRECTION::NONE;

  std::vector<WHEEL_DIRECTION> dirs = {
      WHEEL_DIRECTION::RIGHT,
      WHEEL_DIRECTION::LEFT,
  };

  CDriverPrimitive primitive;
  for (auto dir : dirs)
  {
    if (m_buttonMap->GetWheel(m_name, dir, primitive) && primitive == source)
    {
      direction = dir;
      break;
    }
  }

  // Feature must accept input to be considered handled
  bool bHandled = AcceptsInput(magnitude > 0.0f);

  switch (direction)
  {
    case WHEEL_DIRECTION::RIGHT:
      m_axis.SetPositiveDistance(magnitude);
      break;
    case WHEEL_DIRECTION::LEFT:
      m_axis.SetNegativeDistance(magnitude);
      break;
    default:
      // Just in case, avoid sticking
      m_axis.Reset();
      break;
  }

  return bHandled;
}

// --- CThrottle ---------------------------------------------------------------

CThrottle::CThrottle(const FeatureName& name, IInputHandler* handler, IButtonMap* buttonMap)
  : CAxisFeature(name, handler, buttonMap)
{
}

bool CThrottle::OnAnalogMotion(const CDriverPrimitive& source, float magnitude)
{
  THROTTLE_DIRECTION direction = THROTTLE_DIRECTION::NONE;

  std::vector<THROTTLE_DIRECTION> dirs = {
      THROTTLE_DIRECTION::UP,
      THROTTLE_DIRECTION::DOWN,
  };

  CDriverPrimitive primitive;
  for (auto dir : dirs)
  {
    if (m_buttonMap->GetThrottle(m_name, dir, primitive) && primitive == source)
    {
      direction = dir;
      break;
    }
  }

  // Feature must accept input to be considered handled
  bool bHandled = AcceptsInput(magnitude > 0.0f);

  switch (direction)
  {
    case THROTTLE_DIRECTION::UP:
      m_axis.SetPositiveDistance(magnitude);
      break;
    case THROTTLE_DIRECTION::DOWN:
      m_axis.SetNegativeDistance(magnitude);
      break;
    default:
      // Just in case, avoid sticking
      m_axis.Reset();
      break;
  }

  return bHandled;
}

// --- CAnalogStick ------------------------------------------------------------

CAnalogStick::CAnalogStick(const FeatureName& name, IInputHandler* handler, IButtonMap* buttonMap)
  : CJoystickFeature(name, handler, buttonMap)
{
}

bool CAnalogStick::OnDigitalMotion(const CDriverPrimitive& source, bool bPressed)
{
  return OnAnalogMotion(source, bPressed ? 1.0f : 0.0f);
}

bool CAnalogStick::OnAnalogMotion(const CDriverPrimitive& source, float magnitude)
{
  ANALOG_STICK_DIRECTION direction = ANALOG_STICK_DIRECTION::NONE;

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
  const bool bWasActivated = (m_vertState != 0.0f || m_horizState != 0.0f);

  if (bActivated ^ bWasActivated)
  {
    CLog::Log(LOGDEBUG, "Feature [ {} ] on {} {}", m_name, m_handler->ControllerID(),
              bActivated ? "activated" : "deactivated");
  }

  if (bActivated || bWasActivated)
  {
    m_vertState = newVertState;
    m_horizState = newHorizState;

    unsigned int motionTimeMs = 0;

    if (bActivated)
    {
      if (!InMotion())
        StartMotion();
      else
        motionTimeMs = MotionTimeMs();
    }
    else
    {
      ResetMotion();
    }

    m_handler->OnAnalogStickMotion(m_name, newHorizState, newVertState, motionTimeMs);
  }
}

// --- CAccelerometer ----------------------------------------------------------

CAccelerometer::CAccelerometer(const FeatureName& name,
                               IInputHandler* handler,
                               IButtonMap* buttonMap)
  : CJoystickFeature(name, handler, buttonMap)
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
