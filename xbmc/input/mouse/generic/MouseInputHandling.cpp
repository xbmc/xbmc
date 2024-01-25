/*
 *  Copyright (C) 2016-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "MouseInputHandling.h"

#include "input/InputTranslator.h"
#include "input/joysticks/interfaces/IButtonMap.h"
#include "input/mouse/interfaces/IMouseInputHandler.h"

using namespace KODI;
using namespace MOUSE;

CMouseInputHandling::CMouseInputHandling(IMouseInputHandler* handler,
                                         JOYSTICK::IButtonMap* buttonMap)
  : m_handler(handler), m_buttonMap(buttonMap)
{
}

bool CMouseInputHandling::OnPosition(int x, int y)
{
  using namespace JOYSTICK;

  if (!m_bHasPosition)
  {
    m_bHasPosition = true;
    m_x = x;
    m_y = y;
    return true;
  }

  int dx = x - m_x;
  int dy = y - m_y;

  bool bHandled = false;

  // Get direction of motion
  POINTER_DIRECTION dir = GetPointerDirection(dx, dy);

  CDriverPrimitive source(dir);
  if (source.IsValid())
  {
    // Get pointer in direction of motion
    PointerName pointerName;
    if (m_buttonMap->GetFeature(source, pointerName))
    {
      // Get orthogonal direction of motion
      POINTER_DIRECTION dirCCW = GetOrthogonalDirectionCCW(dir);

      // Get mapped directions of motion for rotation and reflection
      CDriverPrimitive target;
      CDriverPrimitive targetCCW;

      if (m_buttonMap->GetRelativePointer(pointerName, dir, target))
        m_buttonMap->GetRelativePointer(pointerName, dirCCW, targetCCW);

      if (target.IsValid())
      {
        // Invert y to right-handed cartesian system
        dy *= -1;

        // Perform rotation
        int rotation[2][2] = {{1, 0}, {0, 1}};

        GetRotation(dir, target.PointerDirection(), rotation);

        dx = rotation[0][0] * dx + rotation[0][1] * dy;
        dy = rotation[1][0] * dx + rotation[1][1] * dy;

        if (targetCCW.IsValid())
        {
          // Perform reflection
          int reflection[2][2] = {{1, 0}, {0, 1}};

          GetReflectionCCW(target.PointerDirection(), targetCCW.PointerDirection(), reflection);

          dx = reflection[0][0] * dx + reflection[0][1] * dy;
          dy = reflection[1][0] * dx + reflection[1][1] * dy;
        }

        // Invert y back to left-handed coordinate system
        dy *= -1;
      }

      bHandled = m_handler->OnMotion(pointerName, dx, dy);
    }
  }
  else
  {
    // Don't fall through - might disrupt the game
    bHandled = true;
  }

  m_x = x;
  m_y = y;

  m_handler->OnInputFrame();

  return bHandled;
}

bool CMouseInputHandling::OnButtonPress(BUTTON_ID button)
{
  bool bHandled = false;

  JOYSTICK::CDriverPrimitive source(button);

  ButtonName buttonName;
  if (m_buttonMap->GetFeature(source, buttonName))
    bHandled = m_handler->OnButtonPress(buttonName);

  m_handler->OnInputFrame();

  return bHandled;
}

void CMouseInputHandling::OnButtonRelease(BUTTON_ID button)
{
  JOYSTICK::CDriverPrimitive source(button);

  ButtonName buttonName;
  if (m_buttonMap->GetFeature(source, buttonName))
    m_handler->OnButtonRelease(buttonName);

  m_handler->OnInputFrame();
}

POINTER_DIRECTION CMouseInputHandling::GetPointerDirection(int x, int y)
{
  using namespace INPUT;

  return CInputTranslator::VectorToCardinalDirection(static_cast<float>(x), static_cast<float>(-y));
}

POINTER_DIRECTION CMouseInputHandling::GetOrthogonalDirectionCCW(POINTER_DIRECTION direction)
{
  switch (direction)
  {
    case POINTER_DIRECTION::RIGHT:
      return POINTER_DIRECTION::UP;
    case POINTER_DIRECTION::UP:
      return POINTER_DIRECTION::LEFT;
    case POINTER_DIRECTION::LEFT:
      return POINTER_DIRECTION::DOWN;
    case POINTER_DIRECTION::DOWN:
      return POINTER_DIRECTION::RIGHT;
    default:
      break;
  }

  return POINTER_DIRECTION::NONE;
}

void CMouseInputHandling::GetRotation(POINTER_DIRECTION source,
                                      POINTER_DIRECTION target,
                                      int (&rotation)[2][2])
{
  switch (source)
  {
    case POINTER_DIRECTION::RIGHT:
    {
      switch (target)
      {
        case POINTER_DIRECTION::UP:
          GetRotation(90, rotation);
          break;
        case POINTER_DIRECTION::LEFT:
          GetRotation(180, rotation);
          break;
        case POINTER_DIRECTION::DOWN:
          GetRotation(270, rotation);
          break;
        default:
          break;
      }
      break;
    }
    case POINTER_DIRECTION::UP:
    {
      switch (target)
      {
        case POINTER_DIRECTION::LEFT:
          GetRotation(90, rotation);
          break;
        case POINTER_DIRECTION::DOWN:
          GetRotation(180, rotation);
          break;
        case POINTER_DIRECTION::RIGHT:
          GetRotation(270, rotation);
          break;
        default:
          break;
      }
      break;
    }
    case POINTER_DIRECTION::LEFT:
    {
      switch (target)
      {
        case POINTER_DIRECTION::DOWN:
          GetRotation(90, rotation);
          break;
        case POINTER_DIRECTION::RIGHT:
          GetRotation(180, rotation);
          break;
        case POINTER_DIRECTION::UP:
          GetRotation(270, rotation);
          break;
        default:
          break;
      }
      break;
    }
    case POINTER_DIRECTION::DOWN:
    {
      switch (target)
      {
        case POINTER_DIRECTION::RIGHT:
          GetRotation(90, rotation);
          break;
        case POINTER_DIRECTION::UP:
          GetRotation(180, rotation);
          break;
        case POINTER_DIRECTION::LEFT:
          GetRotation(270, rotation);
          break;
        default:
          break;
      }
      break;
    }
    default:
      break;
  }
}

void CMouseInputHandling::GetRotation(int deg, int (&rotation)[2][2])
{
  switch (deg)
  {
    case 90:
    {
      rotation[0][0] = 0;
      rotation[0][1] = -1;
      rotation[1][0] = 1;
      rotation[1][1] = 0;
      break;
    }
    case 180:
    {
      rotation[0][0] = -1;
      rotation[0][1] = 0;
      rotation[1][0] = 0;
      rotation[1][1] = -1;
      break;
    }
    case 270:
    {
      rotation[0][0] = 0;
      rotation[0][1] = 1;
      rotation[1][0] = -1;
      rotation[1][1] = 0;
      break;
    }
    default:
      break;
  }
}

void CMouseInputHandling::GetReflectionCCW(POINTER_DIRECTION source,
                                           POINTER_DIRECTION target,
                                           int (&rotation)[2][2])
{
  switch (source)
  {
    case POINTER_DIRECTION::RIGHT:
    {
      switch (target)
      {
        case POINTER_DIRECTION::DOWN:
          GetReflection(0, rotation);
          break;
        default:
          break;
      }
      break;
    }
    case POINTER_DIRECTION::UP:
    {
      switch (target)
      {
        case POINTER_DIRECTION::RIGHT:
          GetReflection(90, rotation);
          break;
        default:
          break;
      }
      break;
    }
    case POINTER_DIRECTION::LEFT:
    {
      switch (target)
      {
        case POINTER_DIRECTION::UP:
          GetReflection(180, rotation);
          break;
        default:
          break;
      }
      break;
    }
    case POINTER_DIRECTION::DOWN:
    {
      switch (target)
      {
        case POINTER_DIRECTION::LEFT:
          GetReflection(270, rotation);
          break;
        default:
          break;
      }
      break;
    }
    default:
      break;
  }
}

void CMouseInputHandling::GetReflection(int deg, int (&reflection)[2][2])
{
  switch (deg)
  {
    case 0:
    case 180:
    {
      reflection[0][0] = 1;
      reflection[0][1] = 0;
      reflection[1][0] = 0;
      reflection[1][1] = -1;
      break;
    }
    case 90:
    case 270:
    {
      reflection[0][0] = -1;
      reflection[0][1] = 0;
      reflection[1][0] = 0;
      reflection[1][1] = 1;
      break;
    }
    default:
      break;
  }
}
