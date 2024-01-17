/*
 *  Copyright (C) 2014-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "InputHandling.h"

#include "input/joysticks/DriverPrimitive.h"
#include "input/joysticks/JoystickUtils.h"
#include "input/joysticks/dialogs/GUIDialogNewJoystick.h"
#include "input/joysticks/interfaces/IButtonMap.h"
#include "input/joysticks/interfaces/IInputHandler.h"
#include "utils/log.h"

#include <array>
#include <cmath>
#include <tuple>

using namespace KODI;
using namespace JOYSTICK;

CGUIDialogNewJoystick* const CInputHandling::m_dialog = new CGUIDialogNewJoystick;

CInputHandling::CInputHandling(IInputHandler* handler, IButtonMap* buttonMap)
  : m_handler(handler), m_buttonMap(buttonMap)
{
}

CInputHandling::~CInputHandling(void) = default;

bool CInputHandling::OnButtonMotion(unsigned int buttonIndex, bool bPressed)
{
  return OnDigitalMotion(CDriverPrimitive(PRIMITIVE_TYPE::BUTTON, buttonIndex), bPressed);
}

bool CInputHandling::OnHatMotion(unsigned int hatIndex, HAT_STATE state)
{
  bool bHandled = false;

  bHandled |=
      OnDigitalMotion(CDriverPrimitive(hatIndex, HAT_DIRECTION::UP), state & HAT_DIRECTION::UP);
  bHandled |= OnDigitalMotion(CDriverPrimitive(hatIndex, HAT_DIRECTION::RIGHT),
                              state & HAT_DIRECTION::RIGHT);
  bHandled |=
      OnDigitalMotion(CDriverPrimitive(hatIndex, HAT_DIRECTION::DOWN), state & HAT_DIRECTION::DOWN);
  bHandled |=
      OnDigitalMotion(CDriverPrimitive(hatIndex, HAT_DIRECTION::LEFT), state & HAT_DIRECTION::LEFT);

  return bHandled;
}

bool CInputHandling::OnAxisMotion(unsigned int axisIndex,
                                  float position,
                                  int center,
                                  unsigned int range)
{
  bool bHandled = false;

  if (center != 0)
  {
    float translatedPostion = std::min((position - center) / range, 1.0f);

    // Calculate the direction the trigger travels from the center point
    SEMIAXIS_DIRECTION dir;
    if (center > 0)
      dir = SEMIAXIS_DIRECTION::NEGATIVE;
    else
      dir = SEMIAXIS_DIRECTION::POSITIVE;

    CDriverPrimitive offsetSemiaxis(axisIndex, center, dir, range);

    bHandled = OnAnalogMotion(offsetSemiaxis, translatedPostion);
  }
  else
  {
    CDriverPrimitive positiveSemiaxis(axisIndex, 0, SEMIAXIS_DIRECTION::POSITIVE, 1);
    CDriverPrimitive negativeSemiaxis(axisIndex, 0, SEMIAXIS_DIRECTION::NEGATIVE, 1);

    bHandled |= OnAnalogMotion(positiveSemiaxis, position > 0.0f ? position : 0.0f);
    bHandled |= OnAnalogMotion(negativeSemiaxis, position < 0.0f ? -position : 0.0f);
  }

  return bHandled;
}

void CInputHandling::OnInputFrame(void)
{
  // Handle driver input
  for (auto& it : m_features)
    it.second->ProcessMotions();

  // Handle higher-level controller input
  m_handler->OnInputFrame();
}

bool CInputHandling::OnDigitalMotion(const CDriverPrimitive& source, bool bPressed)
{
  bool bHandled = false;

  FeatureName featureName;
  if (m_buttonMap->GetFeature(source, featureName))
  {
    auto it = m_features.find(featureName);
    if (it == m_features.end())
    {
      FeaturePtr feature(CreateFeature(featureName));
      if (feature)
        std::tie(it, std::ignore) = m_features.insert({featureName, std::move(feature)});
    }

    if (it != m_features.end())
      bHandled = it->second->OnDigitalMotion(source, bPressed);
  }
  else if (bPressed)
  {
    // If button didn't resolve to a feature, check if the button map is empty
    // and ask the user if they would like to start mapping the controller
    if (m_buttonMap->IsEmpty())
    {
      CLog::Log(LOGDEBUG, "Empty button map detected for {}", m_buttonMap->ControllerID());
      m_dialog->ShowAsync();
    }
  }

  return bHandled;
}

bool CInputHandling::OnAnalogMotion(const CDriverPrimitive& source, float magnitude)
{
  bool bHandled = false;

  FeatureName featureName;
  if (m_buttonMap->GetFeature(source, featureName))
  {
    auto it = m_features.find(featureName);
    if (it == m_features.end())
    {
      FeaturePtr feature(CreateFeature(featureName));
      if (feature)
        std::tie(it, std::ignore) = m_features.insert({featureName, std::move(feature)});
    }

    if (it != m_features.end())
      bHandled = it->second->OnAnalogMotion(source, magnitude);
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
