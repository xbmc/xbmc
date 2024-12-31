/*
 *  Copyright (C) 2014-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "HatDetector.h"

#include "input/joysticks/DriverPrimitive.h"

using namespace KODI;
using namespace JOYSTICK;

CHatDetector::CHatDetector(CButtonMapping* buttonMapping, unsigned int hatIndex)
  : CPrimitiveDetector(buttonMapping), m_hatIndex(hatIndex)
{
}

bool CHatDetector::OnMotion(HAT_STATE state)
{
  return MapPrimitive(CDriverPrimitive(m_hatIndex, static_cast<HAT_DIRECTION>(state)));
}
