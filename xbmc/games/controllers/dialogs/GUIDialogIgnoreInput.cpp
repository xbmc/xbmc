/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogIgnoreInput.h"

#include "guilib/LocalizeStrings.h"
#include "input/joysticks/JoystickTranslator.h"
#include "input/joysticks/interfaces/IButtonMap.h"
#include "input/joysticks/interfaces/IButtonMapCallback.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <iterator>

using namespace KODI;
using namespace GAME;

bool CGUIDialogIgnoreInput::AcceptsPrimitive(JOYSTICK::PRIMITIVE_TYPE type) const
{
  switch (type)
  {
    case JOYSTICK::PRIMITIVE_TYPE::BUTTON:
    case JOYSTICK::PRIMITIVE_TYPE::SEMIAXIS:
      return true;
    default:
      break;
  }

  return false;
}

std::string CGUIDialogIgnoreInput::GetDialogText()
{
  // "Some controllers have buttons and axes that interfere with mapping. Press
  // these now to disable them:[CR]%s"
  const std::string& dialogText = g_localizeStrings.Get(35014);

  std::vector<std::string> primitives;

  std::transform(m_capturedPrimitives.begin(), m_capturedPrimitives.end(),
                 std::back_inserter(primitives),
                 [](const JOYSTICK::CDriverPrimitive& primitive)
                 { return JOYSTICK::CJoystickTranslator::GetPrimitiveName(primitive); });

  return StringUtils::Format(dialogText, StringUtils::Join(primitives, " | "));
}

std::string CGUIDialogIgnoreInput::GetDialogHeader()
{

  return g_localizeStrings.Get(35019); // "Ignore input"
}

bool CGUIDialogIgnoreInput::MapPrimitiveInternal(JOYSTICK::IButtonMap* buttonMap,
                                                 KEYMAP::IKeymap* keymap,
                                                 const JOYSTICK::CDriverPrimitive& primitive)
{
  // Check if we have already started capturing primitives for a device
  const bool bHasDevice = !m_location.empty();

  // If a primitive comes from a different device, ignore it
  if (bHasDevice && m_location != buttonMap->Location())
  {
    CLog::Log(LOGDEBUG, "{}: ignoring input from device {}", buttonMap->ControllerID(),
              buttonMap->Location());
    return false;
  }

  if (!bHasDevice)
  {
    CLog::Log(LOGDEBUG, "{}: capturing input for device {}", buttonMap->ControllerID(),
              buttonMap->Location());
    m_location = buttonMap->Location();
  }

  if (AddPrimitive(primitive))
  {
    buttonMap->SetIgnoredPrimitives(m_capturedPrimitives);
    m_captureEvent.Set();
  }

  return true;
}

void CGUIDialogIgnoreInput::OnClose(bool bAccepted)
{
  for (auto& callback : ButtonMapCallbacks())
  {
    if (bAccepted)
    {
      // See documentation of IButtonMapCallback::ResetIgnoredPrimitives()
      // for why this call is needed
      if (m_location.empty())
        callback.second->ResetIgnoredPrimitives();

      if (m_location.empty() || m_location == callback.first)
        callback.second->SaveButtonMap();
    }
    else
      callback.second->RevertButtonMap();
  }
}

bool CGUIDialogIgnoreInput::AddPrimitive(const JOYSTICK::CDriverPrimitive& primitive)
{
  bool bValid = false;

  if (primitive.Type() == JOYSTICK::PRIMITIVE_TYPE::BUTTON ||
      primitive.Type() == JOYSTICK::PRIMITIVE_TYPE::SEMIAXIS)
  {
    auto PrimitiveMatch = [&primitive](const JOYSTICK::CDriverPrimitive& other)
    { return primitive.Type() == other.Type() && primitive.Index() == other.Index(); };

    bValid = std::find_if(m_capturedPrimitives.begin(), m_capturedPrimitives.end(),
                          PrimitiveMatch) == m_capturedPrimitives.end();
  }

  if (bValid)
  {
    m_capturedPrimitives.emplace_back(primitive);
    return true;
  }

  return false;
}
