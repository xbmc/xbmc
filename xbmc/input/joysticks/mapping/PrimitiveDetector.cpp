/*
 *  Copyright (C) 2014-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PrimitiveDetector.h"

#include "ButtonMapping.h"

using namespace KODI;
using namespace JOYSTICK;

CPrimitiveDetector::CPrimitiveDetector(CButtonMapping* buttonMapping)
  : m_buttonMapping(buttonMapping)
{
}

bool CPrimitiveDetector::MapPrimitive(const CDriverPrimitive& primitive)
{
  if (primitive.IsValid())
    return m_buttonMapping->MapPrimitive(primitive);

  return false;
}
