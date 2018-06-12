/*
 *      Copyright (C) 2015-2017 Team Kodi
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

#pragma once

#include "ControllerTypes.h"
#include "input/joysticks/JoystickTypes.h"
#include "input/XBMC_keysym.h"

#include <string>

class TiXmlElement;

namespace KODI
{
namespace GAME
{

class CControllerFeature
{
public:
  CControllerFeature() = default;
  CControllerFeature(int labelId);
  CControllerFeature(const CControllerFeature& other) { *this = other; }

  void Reset(void);

  CControllerFeature& operator=(const CControllerFeature& rhs);

  JOYSTICK::FEATURE_TYPE Type(void) const { return m_type; }
  JOYSTICK::FEATURE_CATEGORY Category(void) const { return m_category; }
  const std::string &Name(void) const { return m_strName; }

  // GUI properties
  std::string Label(void) const;
  int LabelID(void) const { return m_labelId; }
  std::string CategoryLabel(void) const;

  // Input properties
  JOYSTICK::INPUT_TYPE InputType(void) const { return m_inputType; }
  XBMCKey Keycode() const { return m_keycode; }

  bool Deserialize(const TiXmlElement* pElement,
                   const CController* controller,
                   JOYSTICK::FEATURE_CATEGORY category,
                   int categoryLabelId);

private:
  const CController *m_controller = nullptr; // Used for translating addon-specific labels
  JOYSTICK::FEATURE_TYPE m_type = JOYSTICK::FEATURE_TYPE::UNKNOWN;
  JOYSTICK::FEATURE_CATEGORY m_category = JOYSTICK::FEATURE_CATEGORY::UNKNOWN;
  int m_categoryLabelId = -1;
  std::string m_strName;
  int m_labelId = -1;
  JOYSTICK::INPUT_TYPE m_inputType = JOYSTICK::INPUT_TYPE::UNKNOWN;
  XBMCKey m_keycode = XBMCK_UNKNOWN;
};

}
}
