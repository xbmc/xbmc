/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "games/controllers/ControllerTypes.h"
#include "input/joysticks/JoystickTypes.h"
#include "input/keyboard/KeyboardTypes.h"

#include <string>

namespace tinyxml2
{
class XMLElement;
}

namespace KODI
{
namespace GAME
{

/*!
 * \ingroup games
 */
class CPhysicalFeature
{
public:
  CPhysicalFeature() = default;
  CPhysicalFeature(int labelId);
  CPhysicalFeature(const CPhysicalFeature& other) { *this = other; }

  void Reset(void);

  CPhysicalFeature& operator=(const CPhysicalFeature& rhs);

  JOYSTICK::FEATURE_TYPE Type(void) const { return m_type; }
  JOYSTICK::FEATURE_CATEGORY Category(void) const { return m_category; }
  const std::string& Name(void) const { return m_strName; }

  // GUI properties
  std::string Label(void) const;
  int LabelID(void) const { return m_labelId; }
  std::string CategoryLabel(void) const;

  // Input properties
  JOYSTICK::INPUT_TYPE InputType(void) const { return m_inputType; }
  KEYBOARD::XBMCKey Keycode() const { return m_keycode; }

  bool Deserialize(const tinyxml2::XMLElement* pElement,
                   const CController* controller,
                   JOYSTICK::FEATURE_CATEGORY category,
                   int categoryLabelId);

private:
  const CController* m_controller = nullptr; // Used for translating addon-specific labels
  JOYSTICK::FEATURE_TYPE m_type = JOYSTICK::FEATURE_TYPE::UNKNOWN;
  JOYSTICK::FEATURE_CATEGORY m_category = JOYSTICK::FEATURE_CATEGORY::UNKNOWN;
  int m_categoryLabelId = -1;
  std::string m_strName;
  int m_labelId = -1;
  JOYSTICK::INPUT_TYPE m_inputType = JOYSTICK::INPUT_TYPE::UNKNOWN;
  KEYBOARD::XBMCKey m_keycode = KEYBOARD::XBMCKey::XBMCK_UNKNOWN;
};

} // namespace GAME
} // namespace KODI
