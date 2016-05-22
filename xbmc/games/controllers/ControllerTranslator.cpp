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

#include "ControllerTranslator.h"
#include "ControllerDefinitions.h"

using namespace GAME;
using namespace JOYSTICK;

const char* CControllerTranslator::TranslateFeatureType(FEATURE_TYPE type)
{
  switch (type)
  {
    case FEATURE_TYPE::SCALAR:           return LAYOUT_XML_ELM_BUTTON;
    case FEATURE_TYPE::ANALOG_STICK:     return LAYOUT_XML_ELM_ANALOG_STICK;
    case FEATURE_TYPE::ACCELEROMETER:    return LAYOUT_XML_ELM_ACCELEROMETER;
    case FEATURE_TYPE::MOTOR:            return LAYOUT_XML_ELM_MOTOR;
    default:
      break;
  }
  return "";
}

FEATURE_TYPE CControllerTranslator::TranslateFeatureType(const std::string& strType)
{
  if (strType == LAYOUT_XML_ELM_BUTTON)           return FEATURE_TYPE::SCALAR;
  if (strType == LAYOUT_XML_ELM_ANALOG_STICK)     return FEATURE_TYPE::ANALOG_STICK;
  if (strType == LAYOUT_XML_ELM_ACCELEROMETER)    return FEATURE_TYPE::ACCELEROMETER;
  if (strType == LAYOUT_XML_ELM_MOTOR)            return FEATURE_TYPE::MOTOR;

  return FEATURE_TYPE::UNKNOWN;
}

const char* CControllerTranslator::TranslateInputType(INPUT_TYPE type)
{
  switch (type)
  {
    case INPUT_TYPE::DIGITAL: return "digital";
    case INPUT_TYPE::ANALOG:  return "analog";
    default:
      break;
  }
  return "";
}

INPUT_TYPE CControllerTranslator::TranslateInputType(const std::string& strType)
{
  if (strType == "digital") return INPUT_TYPE::DIGITAL;
  if (strType == "analog")  return INPUT_TYPE::ANALOG;

  return INPUT_TYPE::UNKNOWN;
}
