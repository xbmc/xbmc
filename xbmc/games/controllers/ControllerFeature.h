/*
 *      Copyright (C) 2015-2016 Team Kodi
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

#include <string>

class TiXmlElement;

namespace GAME
{

class CControllerFeature
{
public:
  CControllerFeature(void) { Reset(); }
  CControllerFeature(const CControllerFeature& other) { *this = other; }

  void Reset(void);

  CControllerFeature& operator=(const CControllerFeature& rhs);

  JOYSTICK::FEATURE_TYPE Type(void) const       { return m_type; }
  const std::string&     Group(void) const      { return m_group; }
  const std::string&     Name(void) const       { return m_strName; }
  const std::string&     Label(void) const      { return m_strLabel; }
  unsigned int           LabelID(void) const    { return m_labelId; }
  JOYSTICK::INPUT_TYPE   InputType(void) const  { return m_inputType; }

  bool Deserialize(const TiXmlElement* pElement, const CController* controller, const std::string& strGroup);

private:
  JOYSTICK::FEATURE_TYPE m_type;
  std::string            m_group;
  std::string            m_strName;
  std::string            m_strLabel;
  unsigned int           m_labelId;
  JOYSTICK::INPUT_TYPE   m_inputType;
};

}
