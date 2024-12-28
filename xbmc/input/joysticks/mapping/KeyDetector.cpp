/*
 *  Copyright (C) 2014-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "KeyDetector.h"

#include "input/joysticks/DriverPrimitive.h"

using namespace KODI;
using namespace JOYSTICK;

CKeyDetector::CKeyDetector(CButtonMapping* buttonMapping, XBMCKey keycode)
  : CPrimitiveDetector(buttonMapping), m_keycode(keycode)
{
}

bool CKeyDetector::OnMotion(bool bPressed)
{
  if (bPressed)
    return MapPrimitive(CDriverPrimitive(m_keycode));

  return false;
}
