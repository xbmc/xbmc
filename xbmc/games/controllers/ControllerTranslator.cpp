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

#include "ControllerTranslator.h"
#include "ControllerDefinitions.h"

using namespace KODI;
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
    case FEATURE_TYPE::RELPOINTER:       return LAYOUT_XML_ELM_RELPOINTER;
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
  if (strType == LAYOUT_XML_ELM_RELPOINTER)       return FEATURE_TYPE::RELPOINTER;
  if (strType == LAYOUT_XML_ELM_ABSPOINTER)       return FEATURE_TYPE::ABSPOINTER;

  return FEATURE_TYPE::UNKNOWN;
}

const char* CControllerTranslator::TranslateFeatureCategory(FEATURE_CATEGORY category)
{
  switch (category)
  {
    case FEATURE_CATEGORY::FACE:          return FEATURE_CATEGORY_FACE;
    case FEATURE_CATEGORY::SHOULDER:      return FEATURE_CATEGORY_SHOULDER;
    case FEATURE_CATEGORY::TRIGGER:       return FEATURE_CATEGORY_TRIGGER;
    case FEATURE_CATEGORY::ANALOG_STICK:  return FEATURE_CATEGORY_ANALOG_STICK;
    case FEATURE_CATEGORY::ACCELEROMETER: return FEATURE_CATEGORY_ACCELEROMETER;
    case FEATURE_CATEGORY::HAPTICS:       return FEATURE_CATEGORY_HAPTICS;
    case FEATURE_CATEGORY::MOUSE_BUTTON:  return FEATURE_CATEGORY_MOUSE_BUTTON;
    case FEATURE_CATEGORY::POINTER:       return FEATURE_CATEGORY_POINTER;
    case FEATURE_CATEGORY::LIGHTGUN:      return FEATURE_CATEGORY_LIGHTGUN;
    case FEATURE_CATEGORY::OFFSCREEN:     return FEATURE_CATEGORY_OFFSCREEN;
    case FEATURE_CATEGORY::KEY:           return FEATURE_CATEGORY_KEY;
    case FEATURE_CATEGORY::KEYPAD:        return FEATURE_CATEGORY_KEYPAD;
    case FEATURE_CATEGORY::HARDWARE:      return FEATURE_CATEGORY_HARDWARE;
    default:
      break;
  }
  return "";
}

FEATURE_CATEGORY CControllerTranslator::TranslateFeatureCategory(const std::string& strCategory)
{
  if (strCategory == FEATURE_CATEGORY_FACE)           return FEATURE_CATEGORY::FACE;
  if (strCategory == FEATURE_CATEGORY_SHOULDER)       return FEATURE_CATEGORY::SHOULDER;
  if (strCategory == FEATURE_CATEGORY_TRIGGER)        return FEATURE_CATEGORY::TRIGGER;
  if (strCategory == FEATURE_CATEGORY_ANALOG_STICK)   return FEATURE_CATEGORY::ANALOG_STICK;
  if (strCategory == FEATURE_CATEGORY_ACCELEROMETER)  return FEATURE_CATEGORY::ACCELEROMETER;
  if (strCategory == FEATURE_CATEGORY_HAPTICS)        return FEATURE_CATEGORY::HAPTICS;
  if (strCategory == FEATURE_CATEGORY_MOUSE_BUTTON)   return FEATURE_CATEGORY::MOUSE_BUTTON;
  if (strCategory == FEATURE_CATEGORY_POINTER)        return FEATURE_CATEGORY::POINTER;
  if (strCategory == FEATURE_CATEGORY_LIGHTGUN)       return FEATURE_CATEGORY::LIGHTGUN;
  if (strCategory == FEATURE_CATEGORY_OFFSCREEN)      return FEATURE_CATEGORY::OFFSCREEN;
  if (strCategory == FEATURE_CATEGORY_KEY)            return FEATURE_CATEGORY::KEY;
  if (strCategory == FEATURE_CATEGORY_KEYPAD)         return FEATURE_CATEGORY::KEYPAD;
  if (strCategory == FEATURE_CATEGORY_HARDWARE)       return FEATURE_CATEGORY::HARDWARE;

  return FEATURE_CATEGORY::UNKNOWN;
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
