/*
 *      Copyright (C) 2014-2016 Team Kodi
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
#include "input/joysticks/DriverPrimitive.h"
#include "input/joysticks/IButtonMap.h"
#include "input/joysticks/IInputHandler.h"
#include "utils/log.h"

using namespace JOYSTICK;

#define ANALOG_DIGITAL_THRESHOLD  0.5f

// --- CJoystickFeature --------------------------------------------------------

CJoystickFeature::CJoystickFeature(const FeatureName& name, IInputHandler* handler, IButtonMap* buttonMap) :
  m_name(name),
  m_handler(handler),
  m_buttonMap(buttonMap)
{
}

// --- CScalarFeature ----------------------------------------------------------

CScalarFeature::CScalarFeature(const FeatureName& name, IInputHandler* handler, IButtonMap* buttonMap) :
  CJoystickFeature(name, handler, buttonMap),
  m_inputType(handler->GetInputType(name)),
  m_bDigitalState(false),
  m_analogState(0.0f)
{
}

bool CScalarFeature::OnDigitalMotion(const CDriverPrimitive& source, bool bPressed)
{
  bool bHandled = false;

  if (m_inputType == INPUT_TYPE::DIGITAL)
  {
    if (m_bDigitalState != bPressed)
    {
      m_bDigitalState = bPressed;
      bHandled = OnDigitalMotion(bPressed);
    }
  }
  else if (m_inputType == INPUT_TYPE::ANALOG)
  {
    bHandled = OnAnalogMotion(source, bPressed ? 1.0f : 0.0f);
  }

  return bHandled;
}

bool CScalarFeature::OnAnalogMotion(const CDriverPrimitive& source, float magnitude)
{
  bool bHandled = false;

  if (m_inputType == INPUT_TYPE::DIGITAL)
  {
    bHandled = OnDigitalMotion(source, magnitude >= ANALOG_DIGITAL_THRESHOLD);
  }
  else if (m_inputType == INPUT_TYPE::ANALOG)
  {
    if (m_analogState != 0.0f || magnitude != 0.0f)
    {
      m_analogState = magnitude;
      bHandled = OnAnalogMotion(magnitude);
    }
  }

  return bHandled;
}

bool CScalarFeature::OnDigitalMotion(bool bPressed)
{
  CLog::Log(LOGDEBUG, "Feature [ %s ] on %s %s",
            m_name.c_str(), m_handler->ControllerID().c_str(), bPressed ? "pressed" : "released");

  return m_handler->OnButtonPress(m_name, bPressed);
}

bool CScalarFeature::OnAnalogMotion(float magnitude)
{
  const bool bActivated = (magnitude != 0.0f);

  if (m_bDigitalState != bActivated)
  {
    m_bDigitalState = bActivated;

    CLog::Log(LOGDEBUG, "Feature [ %s ] on %s %s",
              m_name.c_str(), m_handler->ControllerID().c_str(), bActivated ? "activated" : "deactivated");
  }

  return m_handler->OnButtonMotion(m_name, magnitude);
}

// --- CAnalogStick ------------------------------------------------------------

CAnalogStick::CAnalogStick(const FeatureName& name, IInputHandler* handler, IButtonMap* buttonMap) :
  CJoystickFeature(name, handler, buttonMap),
  m_vertState(0.0f),
  m_horizState(0.0f)
{
}

bool CAnalogStick::OnDigitalMotion(const CDriverPrimitive& source, bool bPressed)
{
  return OnAnalogMotion(source, bPressed ? 1.0f : 0.0f);
}

bool CAnalogStick::OnAnalogMotion(const CDriverPrimitive& source, float magnitude)
{
  CDriverPrimitive up;
  CDriverPrimitive down;
  CDriverPrimitive right;
  CDriverPrimitive left;

  m_buttonMap->GetAnalogStick(m_name, up, down, right,  left);

  if (source == up)
    m_vertAxis.SetPositiveDistance(magnitude);
  else if (source == down)
    m_vertAxis.SetNegativeDistance(magnitude);
  else if (source == right)
    m_horizAxis.SetPositiveDistance(magnitude);
  else if (source == left)
    m_horizAxis.SetNegativeDistance(magnitude);
  else
  {
    // Just in case, avoid sticking
    m_vertAxis.Reset();
    m_horizAxis.Reset();
  }

  return true;
}

void CAnalogStick::ProcessMotions(void)
{
  const float newVertState = m_vertAxis.GetPosition();
  const float newHorizState = m_horizAxis.GetPosition();

  if (m_vertState != 0 || m_horizState != 0 ||
      newVertState != 0 || newHorizState != 0)
  {
    m_vertState = newVertState;
    m_horizState = newHorizState;
    m_handler->OnAnalogStickMotion(m_name, newHorizState, newVertState);
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
  CDriverPrimitive positiveX;
  CDriverPrimitive positiveY;
  CDriverPrimitive positiveZ;

  m_buttonMap->GetAccelerometer(m_name, positiveX, positiveY, positiveZ);

  if (source == positiveX)
    m_xAxis.SetPositiveDistance(magnitude);
  else if (source == positiveY)
    m_yAxis.SetPositiveDistance(magnitude);
  else if (source == positiveZ)
    m_zAxis.SetPositiveDistance(magnitude);
  else
  {
    // Just in case, avoid sticking
    m_xAxis.Reset();
    m_xAxis.Reset();
    m_yAxis.Reset();
  }

  return true;
}

void CAccelerometer::ProcessMotions(void)
{
  const float newXAxis = m_xAxis.GetPosition();
  const float newYAxis = m_yAxis.GetPosition();
  const float newZAxis = m_zAxis.GetPosition();

  if (m_xAxisState != 0 || m_yAxisState != 0 || m_zAxisState != 0 ||
      newXAxis != 0 || newYAxis != 0 || newZAxis)
  {
    m_xAxisState = newXAxis;
    m_yAxisState = newYAxis;
    m_zAxisState = newZAxis;
    m_handler->OnAccelerometerMotion(m_name, newXAxis, newYAxis, newZAxis);
  }
}
