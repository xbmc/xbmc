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

namespace GAME
{

class CControllerTranslator
{
public:
  static const char* TranslateFeatureType(JOYSTICK::FEATURE_TYPE type);
  static JOYSTICK::FEATURE_TYPE TranslateFeatureType(const std::string& strType);

  static const char* TranslateInputType(JOYSTICK::INPUT_TYPE type);
  static JOYSTICK::INPUT_TYPE TranslateInputType(const std::string& strType);
};

}
