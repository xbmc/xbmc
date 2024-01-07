/*
 *  Copyright (C) 2014-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PointerDetector.h"

#include "input/InputTranslator.h"
#include "input/joysticks/DriverPrimitive.h"

using namespace KODI;
using namespace JOYSTICK;

CPointerDetector::CPointerDetector(CButtonMapping* buttonMapping)
  : CPrimitiveDetector(buttonMapping)
{
}

bool CPointerDetector::OnPosition(int x, int y)
{
  if (!m_bStarted)
  {
    m_bStarted = true;
    m_startX = x;
    m_startY = y;
    m_frameCount = 0;
  }

  if (m_frameCount++ >= MIN_FRAME_COUNT)
  {
    int differenceX = x - m_startX;
    int differenceY = y - m_startY;

    INPUT::INTERCARDINAL_DIRECTION dir = GetPointerDirection(differenceX, differenceY);

    CDriverPrimitive primitive(static_cast<RELATIVE_POINTER_DIRECTION>(dir));
    if (primitive.IsValid())
    {
      if (MapPrimitive(primitive))
        m_bStarted = false;
    }
  }

  return true;
}

KODI::INPUT::INTERCARDINAL_DIRECTION CPointerDetector::GetPointerDirection(int x, int y)
{
  using namespace INPUT;

  // Translate from left-handed coordinate system to right-handed coordinate system
  y *= -1;

  return CInputTranslator::VectorToIntercardinalDirection(static_cast<float>(x),
                                                          static_cast<float>(y));
}
