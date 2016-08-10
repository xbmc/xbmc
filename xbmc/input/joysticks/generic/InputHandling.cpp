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

#include "InputHandling.h"
#include "input/joysticks/DriverPrimitive.h"
#include "input/joysticks/IButtonMap.h"
#include "input/joysticks/JoystickUtils.h"

using namespace JOYSTICK;

CInputHandling::CInputHandling(IInputHandler* handler, IButtonMap* buttonMap)
 : m_handler(handler),
   m_buttonMap(buttonMap)
{
}

CInputHandling::~CInputHandling(void)
{
}

bool CInputHandling::OnButtonMotion(unsigned int buttonIndex, bool bPressed)
{
  return OnDigitalMotion(CDriverPrimitive(PRIMITIVE_TYPE::BUTTON, buttonIndex), bPressed);
}

bool CInputHandling::OnHatMotion(unsigned int hatIndex, HAT_STATE state)
{
  bool bHandled = false;

  bHandled |= OnDigitalMotion(CDriverPrimitive(hatIndex, HAT_DIRECTION::UP),    state & HAT_DIRECTION::UP);
  bHandled |= OnDigitalMotion(CDriverPrimitive(hatIndex, HAT_DIRECTION::RIGHT), state & HAT_DIRECTION::RIGHT);
  bHandled |= OnDigitalMotion(CDriverPrimitive(hatIndex, HAT_DIRECTION::DOWN),  state & HAT_DIRECTION::DOWN);
  bHandled |= OnDigitalMotion(CDriverPrimitive(hatIndex, HAT_DIRECTION::LEFT),  state & HAT_DIRECTION::LEFT);

  return bHandled;
}

bool CInputHandling::OnAxisMotion(unsigned int axisIndex, float position)
{
  bool bHandled = false;

  CDriverPrimitive positiveSemiaxis(axisIndex, SEMIAXIS_DIRECTION::POSITIVE);
  CDriverPrimitive negativeSemiaxis(axisIndex, SEMIAXIS_DIRECTION::NEGATIVE);

  bHandled |= OnAnalogMotion(positiveSemiaxis, position > 0.0f ? position : 0.0f);
  bHandled |= OnAnalogMotion(negativeSemiaxis, position < 0.0f ? -position : 0.0f);

  return bHandled;
}

void CInputHandling::ProcessAxisMotions(void)
{
  for (std::map<FeatureName, FeaturePtr>::iterator it = m_features.begin(); it != m_features.end(); ++it)
    it->second->ProcessMotions();
}

bool CInputHandling::OnDigitalMotion(const CDriverPrimitive& source, bool bPressed)
{
  bool bHandled = false;

  FeatureName featureName;
  if (m_buttonMap->GetFeature(source, featureName))
  {
    FeaturePtr& feature = m_features[featureName];

    if (!feature)
      feature = FeaturePtr(CreateFeature(featureName));

    if (feature)
      bHandled = feature->OnDigitalMotion(source, bPressed);
  }

  return bHandled;
}

bool CInputHandling::OnAnalogMotion(const CDriverPrimitive& source, float magnitude)
{
  bool bHandled = false;

  FeatureName featureName;
  if (m_buttonMap->GetFeature(source, featureName))
  {
    FeaturePtr& feature = m_features[featureName];

    if (!feature)
      feature = FeaturePtr(CreateFeature(featureName));

    if (feature)
      bHandled = feature->OnAnalogMotion(source, magnitude);
  }

  return bHandled;
}

CJoystickFeature* CInputHandling::CreateFeature(const FeatureName& featureName)
{
  CJoystickFeature* feature = nullptr;

  switch (m_buttonMap->GetFeatureType(featureName))
  {
    case FEATURE_TYPE::SCALAR:
    {
      feature = new CScalarFeature(featureName, m_handler, m_buttonMap);
      break;
    }
    case FEATURE_TYPE::ANALOG_STICK:
    {
      feature = new CAnalogStick(featureName, m_handler, m_buttonMap);
      break;
    }
    case FEATURE_TYPE::ACCELEROMETER:
    {
      feature = new CAccelerometer(featureName, m_handler, m_buttonMap);
      break;
    }
    default:
      break;
  }
  
  return feature;
}
