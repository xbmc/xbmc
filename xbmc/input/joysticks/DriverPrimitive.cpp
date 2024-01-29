/*
 *  Copyright (C) 2014-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DriverPrimitive.h"

#include "input/keyboard/KeyboardTranslator.h"
#include "utils/StringUtils.h"

#include <utility>

using namespace KODI;
using namespace JOYSTICK;

CDriverPrimitive::CDriverPrimitive(void) = default;

CDriverPrimitive::CDriverPrimitive(PRIMITIVE_TYPE type, unsigned int index)
  : m_type(type), m_driverIndex(index)
{
}

CDriverPrimitive::CDriverPrimitive(unsigned int hatIndex, HAT_DIRECTION direction)
  : m_type(PRIMITIVE_TYPE::HAT), m_driverIndex(hatIndex), m_hatDirection(direction)
{
}

CDriverPrimitive::CDriverPrimitive(unsigned int axisIndex,
                                   int center,
                                   SEMIAXIS_DIRECTION direction,
                                   unsigned int range)
  : m_type(PRIMITIVE_TYPE::SEMIAXIS),
    m_driverIndex(axisIndex),
    m_center(center),
    m_semiAxisDirection(direction),
    m_range(range)
{
}

CDriverPrimitive::CDriverPrimitive(XBMCKey keycode)
  : m_type(PRIMITIVE_TYPE::KEY), m_keycode(keycode)
{
}

CDriverPrimitive::CDriverPrimitive(MOUSE::BUTTON_ID index)
  : m_type(PRIMITIVE_TYPE::MOUSE_BUTTON), m_driverIndex(static_cast<unsigned int>(index))
{
}

CDriverPrimitive::CDriverPrimitive(RELATIVE_POINTER_DIRECTION direction)
  : m_type(PRIMITIVE_TYPE::RELATIVE_POINTER), m_pointerDirection(direction)
{
}

bool CDriverPrimitive::operator==(const CDriverPrimitive& rhs) const
{
  if (m_type == rhs.m_type)
  {
    switch (m_type)
    {
      case PRIMITIVE_TYPE::BUTTON:
      case PRIMITIVE_TYPE::MOTOR:
      case PRIMITIVE_TYPE::MOUSE_BUTTON:
        return m_driverIndex == rhs.m_driverIndex;
      case PRIMITIVE_TYPE::HAT:
        return m_driverIndex == rhs.m_driverIndex && m_hatDirection == rhs.m_hatDirection;
      case PRIMITIVE_TYPE::SEMIAXIS:
        return m_driverIndex == rhs.m_driverIndex && m_center == rhs.m_center &&
               m_semiAxisDirection == rhs.m_semiAxisDirection && m_range == rhs.m_range;
      case PRIMITIVE_TYPE::KEY:
        return m_keycode == rhs.m_keycode;
      case PRIMITIVE_TYPE::RELATIVE_POINTER:
        return m_pointerDirection == rhs.m_pointerDirection;
      default:
        return true;
    }
  }
  return false;
}

bool CDriverPrimitive::operator<(const CDriverPrimitive& rhs) const
{
  if (m_type < rhs.m_type)
    return true;
  if (m_type > rhs.m_type)
    return false;

  if (m_type == PRIMITIVE_TYPE::BUTTON || m_type == PRIMITIVE_TYPE::HAT ||
      m_type == PRIMITIVE_TYPE::SEMIAXIS || m_type == PRIMITIVE_TYPE::MOTOR ||
      m_type == PRIMITIVE_TYPE::MOUSE_BUTTON)
  {
    if (m_driverIndex < rhs.m_driverIndex)
      return true;
    if (m_driverIndex > rhs.m_driverIndex)
      return false;
  }

  if (m_type == PRIMITIVE_TYPE::HAT)
  {
    if (m_hatDirection < rhs.m_hatDirection)
      return true;
    if (m_hatDirection > rhs.m_hatDirection)
      return false;
  }

  if (m_type == PRIMITIVE_TYPE::SEMIAXIS)
  {
    if (m_center < rhs.m_center)
      return true;
    if (m_center > rhs.m_center)
      return false;

    if (m_semiAxisDirection < rhs.m_semiAxisDirection)
      return true;
    if (m_semiAxisDirection > rhs.m_semiAxisDirection)
      return false;

    if (m_range < rhs.m_range)
      return true;
    if (m_range > rhs.m_range)
      return false;
  }

  if (m_type == PRIMITIVE_TYPE::KEY)
  {
    if (m_keycode < rhs.m_keycode)
      return true;
    if (m_keycode > rhs.m_keycode)
      return false;
  }

  if (m_type == PRIMITIVE_TYPE::RELATIVE_POINTER)
  {
    if (m_pointerDirection < rhs.m_pointerDirection)
      return true;
    if (m_pointerDirection > rhs.m_pointerDirection)
      return false;
  }

  return false;
}

bool CDriverPrimitive::IsValid(void) const
{
  if (m_type == PRIMITIVE_TYPE::BUTTON || m_type == PRIMITIVE_TYPE::MOTOR ||
      m_type == PRIMITIVE_TYPE::MOUSE_BUTTON)
    return true;

  if (m_type == PRIMITIVE_TYPE::HAT)
  {
    return m_hatDirection == HAT_DIRECTION::UP || m_hatDirection == HAT_DIRECTION::DOWN ||
           m_hatDirection == HAT_DIRECTION::RIGHT || m_hatDirection == HAT_DIRECTION::LEFT;
  }

  if (m_type == PRIMITIVE_TYPE::SEMIAXIS)
  {
    unsigned int maxRange = 1;

    switch (m_center)
    {
      case -1:
      {
        if (m_semiAxisDirection != SEMIAXIS_DIRECTION::POSITIVE)
          return false;
        maxRange = 2;
        break;
      }
      case 0:
      {
        if (m_semiAxisDirection != SEMIAXIS_DIRECTION::POSITIVE &&
            m_semiAxisDirection != SEMIAXIS_DIRECTION::NEGATIVE)
          return false;
        break;
      }
      case 1:
      {
        if (m_semiAxisDirection != SEMIAXIS_DIRECTION::POSITIVE)
          return false;
        maxRange = 2;
        break;
      }
      default:
        break;
    }

    return 1 <= m_range && m_range <= maxRange;
  }

  if (m_type == PRIMITIVE_TYPE::KEY)
    return m_keycode != XBMCK_UNKNOWN;

  if (m_type == PRIMITIVE_TYPE::RELATIVE_POINTER)
  {
    return m_pointerDirection == RELATIVE_POINTER_DIRECTION::UP ||
           m_pointerDirection == RELATIVE_POINTER_DIRECTION::DOWN ||
           m_pointerDirection == RELATIVE_POINTER_DIRECTION::RIGHT ||
           m_pointerDirection == RELATIVE_POINTER_DIRECTION::LEFT;
  }

  return false;
}

std::string CDriverPrimitive::ToString() const
{
  switch (m_type)
  {
    case PRIMITIVE_TYPE::BUTTON:
      return StringUtils::Format("button {}", m_driverIndex);
    case PRIMITIVE_TYPE::MOTOR:
      return StringUtils::Format("motor {}", m_driverIndex);
    case PRIMITIVE_TYPE::MOUSE_BUTTON:
      return StringUtils::Format("mouse button {}", m_driverIndex);
    case PRIMITIVE_TYPE::HAT:
    {
      switch (m_hatDirection)
      {
        case HAT_DIRECTION::UP:
          return StringUtils::Format("hat {} up", m_driverIndex);
        case HAT_DIRECTION::DOWN:
          return StringUtils::Format("hat {} down", m_driverIndex);
        case HAT_DIRECTION::RIGHT:
          return StringUtils::Format("hat {} right", m_driverIndex);
        case HAT_DIRECTION::LEFT:
          return StringUtils::Format("hat {} left", m_driverIndex);
        default:
          break;
      }
      break;
    }
    case PRIMITIVE_TYPE::SEMIAXIS:
    {
      switch (m_semiAxisDirection)
      {
        case SEMIAXIS_DIRECTION::POSITIVE:
          return StringUtils::Format("semiaxis +{}", m_driverIndex);
        case SEMIAXIS_DIRECTION::NEGATIVE:
          return StringUtils::Format("semiaxis -{}", m_driverIndex);
        default:
          break;
      }
      break;
    }
    case PRIMITIVE_TYPE::KEY:
      return StringUtils::Format("key {}",
                                 KEYBOARD::CKeyboardTranslator::TranslateKeycode(m_keycode));
    case PRIMITIVE_TYPE::RELATIVE_POINTER:
    {
      switch (m_pointerDirection)
      {
        case RELATIVE_POINTER_DIRECTION::UP:
          return StringUtils::Format("pointer {} up", m_driverIndex);
        case RELATIVE_POINTER_DIRECTION::DOWN:
          return StringUtils::Format("pointer {} down", m_driverIndex);
        case RELATIVE_POINTER_DIRECTION::RIGHT:
          return StringUtils::Format("pointer {} right", m_driverIndex);
        case RELATIVE_POINTER_DIRECTION::LEFT:
          return StringUtils::Format("pointer {} left", m_driverIndex);
        default:
          break;
      }
      break;
    }
    default:
      break;
  }

  return "";
}
