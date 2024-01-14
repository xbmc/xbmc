/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogAxisDetection.h"

#include "guilib/LocalizeStrings.h"
#include "input/joysticks/DriverPrimitive.h"
#include "input/joysticks/JoystickTranslator.h"
#include "input/joysticks/interfaces/IButtonMap.h"
#include "utils/StringUtils.h"

#include <algorithm>

using namespace KODI;
using namespace GAME;

std::string CGUIDialogAxisDetection::GetDialogText()
{
  // "Press all analog buttons now to detect them:[CR][CR]%s"
  const std::string& dialogText = g_localizeStrings.Get(35020);

  std::vector<std::string> primitives;

  for (const auto& axisEntry : m_detectedAxes)
  {
    JOYSTICK::CDriverPrimitive axis(axisEntry.second, 0, JOYSTICK::SEMIAXIS_DIRECTION::POSITIVE, 1);
    primitives.emplace_back(JOYSTICK::CJoystickTranslator::GetPrimitiveName(axis));
  }

  return StringUtils::Format(dialogText, StringUtils::Join(primitives, " | "));
}

std::string CGUIDialogAxisDetection::GetDialogHeader()
{
  return g_localizeStrings.Get(35058); // "Controller Configuration"
}

bool CGUIDialogAxisDetection::MapPrimitiveInternal(JOYSTICK::IButtonMap* buttonMap,
                                                   KEYMAP::IKeymap* keymap,
                                                   const JOYSTICK::CDriverPrimitive& primitive)
{
  if (primitive.Type() == JOYSTICK::PRIMITIVE_TYPE::SEMIAXIS)
    AddAxis(buttonMap->Location(), primitive.Index());

  return true;
}

bool CGUIDialogAxisDetection::AcceptsPrimitive(JOYSTICK::PRIMITIVE_TYPE type) const
{
  switch (type)
  {
    case JOYSTICK::PRIMITIVE_TYPE::SEMIAXIS:
      return true;
    default:
      break;
  }

  return false;
}

void CGUIDialogAxisDetection::OnLateAxis(const JOYSTICK::IButtonMap* buttonMap,
                                         unsigned int axisIndex)
{
  AddAxis(buttonMap->Location(), axisIndex);
}

void CGUIDialogAxisDetection::AddAxis(const std::string& deviceLocation, unsigned int axisIndex)
{
  auto it = std::find_if(m_detectedAxes.begin(), m_detectedAxes.end(),
                         [&deviceLocation, axisIndex](const AxisEntry& axis)
                         { return axis.first == deviceLocation && axis.second == axisIndex; });

  if (it == m_detectedAxes.end())
  {
    m_detectedAxes.emplace_back(std::make_pair(deviceLocation, axisIndex));
    m_captureEvent.Set();
  }
}
