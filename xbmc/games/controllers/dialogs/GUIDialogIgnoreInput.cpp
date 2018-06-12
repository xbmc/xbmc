/*
 *      Copyright (C) 2017 Team Kodi
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

#include "GUIDialogIgnoreInput.h"
#include "guilib/LocalizeStrings.h"
#include "input/joysticks/interfaces/IButtonMap.h"
#include "input/joysticks/interfaces/IButtonMapCallback.h"
#include "input/joysticks/JoystickTranslator.h"
#include "utils/log.h"
#include "utils/StringUtils.h"

#include <algorithm>
#include <iterator>

using namespace KODI;
using namespace GAME;

std::string CGUIDialogIgnoreInput::GetDialogText()
{
  // "Some controllers have buttons and axes that interfere with mapping. Press
  // these now to disable them:[CR]%s"
  std::string dialogText = g_localizeStrings.Get(35014);

  std::vector<std::string> primitives;

  std::transform(m_capturedPrimitives.begin(), m_capturedPrimitives.end(), std::back_inserter(primitives),
    [](const JOYSTICK::CDriverPrimitive& primitive)
    {
      return JOYSTICK::CJoystickTranslator::GetPrimitiveName(primitive);
    });

  return StringUtils::Format(dialogText.c_str(), StringUtils::Join(primitives, " | ").c_str());
}

std::string CGUIDialogIgnoreInput::GetDialogHeader()
{

  return g_localizeStrings.Get(35019); // "Ignore input"
}

bool CGUIDialogIgnoreInput::MapPrimitiveInternal(JOYSTICK::IButtonMap* buttonMap,
                                                 IKeymap* keymap,
                                                 const JOYSTICK::CDriverPrimitive& primitive)
{
  // Check if we have already started capturing primitives for a device
  const bool bHasDevice = !m_deviceName.empty();

  // If a primitive comes from a different device, ignore it
  if (bHasDevice && m_deviceName != buttonMap->DeviceName())
  {
    CLog::Log(LOGDEBUG, "%s: ignoring input from device %s", buttonMap->ControllerID().c_str(), buttonMap->DeviceName().c_str());
    return false;
  }

  if (!bHasDevice)
  {
    CLog::Log(LOGDEBUG, "%s: capturing input for device %s", buttonMap->ControllerID().c_str(), buttonMap->DeviceName().c_str());
    m_deviceName = buttonMap->DeviceName();
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
      if (m_deviceName.empty())
        callback.second->ResetIgnoredPrimitives();

      if (m_deviceName.empty() || m_deviceName == callback.first)
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
      {
        return primitive.Type() == other.Type() &&
               primitive.Index() == other.Index();
      };

    bValid = std::find_if(m_capturedPrimitives.begin(), m_capturedPrimitives.end(), PrimitiveMatch) == m_capturedPrimitives.end();
  }

  if (bValid)
  {
    m_capturedPrimitives.emplace_back(primitive);
    return true;
  }

  return false;
}
