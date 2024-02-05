/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DefaultButtonMap.h"

#include "DefaultKeyboardTranslator.h"
#include "DefaultMouseTranslator.h"
#include "games/controllers/Controller.h"
#include "games/controllers/ControllerIDs.h"
#include "input/joysticks/DriverPrimitive.h"
#include "peripherals/devices/Peripheral.h"
#include "utils/log.h"

using namespace KODI;
using namespace GAME;

CDefaultButtonMap::CDefaultButtonMap(PERIPHERALS::CPeripheral* device, std::string strControllerId)
  : m_device(device), m_strControllerId(std::move(strControllerId))
{
}

CDefaultButtonMap::~CDefaultButtonMap() = default;

std::string CDefaultButtonMap::Location() const
{
  return m_device->Location();
}

bool CDefaultButtonMap::Load()
{
  // Verify we're using the appropriate controller
  switch (m_device->Type())
  {
    case PERIPHERALS::PERIPHERAL_KEYBOARD:
    {
      if (m_strControllerId == DEFAULT_KEYBOARD_ID)
        return true;
      break;
    }
    case PERIPHERALS::PERIPHERAL_MOUSE:
    {
      if (m_strControllerId == DEFAULT_MOUSE_ID)
        return true;
      break;
    }
    default:
      break;
  }

  CLog::Log(LOGDEBUG, "Failed to load default button map for \"{}\" with profile {}",
            m_device->Location(), m_strControllerId);
  return false;
}

std::string CDefaultButtonMap::GetAppearance() const
{
  ControllerPtr controller = m_device->ControllerProfile();
  if (controller)
    return controller->ID();

  return m_strControllerId;
}

bool CDefaultButtonMap::GetFeature(const JOYSTICK::CDriverPrimitive& primitive,
                                   JOYSTICK::FeatureName& feature)
{
  switch (primitive.Type())
  {
    case JOYSTICK::PRIMITIVE_TYPE::KEY:
    {
      std::string featureName = CDefaultKeyboardTranslator::TranslateKeycode(primitive.Keycode());
      if (!featureName.empty())
      {
        feature = std::move(featureName);
        return true;
      }
      break;
    }
    case JOYSTICK::PRIMITIVE_TYPE::MOUSE_BUTTON:
    {
      std::string featureName =
          CDefaultMouseTranslator::TranslateMouseButton(primitive.MouseButton());
      if (!featureName.empty())
      {
        feature = std::move(featureName);
        return true;
      }
      break;
    }
    case JOYSTICK::PRIMITIVE_TYPE::RELATIVE_POINTER:
    {
      std::string featureName =
          CDefaultMouseTranslator::TranslateMousePointer(primitive.PointerDirection());
      if (!featureName.empty())
      {
        feature = std::move(featureName);
        return true;
      }
      break;
    }
    default:
      break;
  }

  return false;
}
