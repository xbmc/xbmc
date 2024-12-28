/*
 *  Copyright (C) 2014-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "PrimitiveDetector.h"
#include "input/joysticks/JoystickTypes.h"

namespace KODI
{
namespace JOYSTICK
{
class CButtonMapping;

/*!
 * \ingroup joystick
 *
 * \brief Detects when a mouse button should be mapped
 */
class CPointerDetector : public CPrimitiveDetector
{
public:
  CPointerDetector(CButtonMapping* buttonMapping);

  /*!
   * \brief Pointer position has been updated
   *
   * \param x The new x coordinate
   * \param y The new y coordinate
   *
   * \return Always true - pointer motion events are always absorbed while
   *         button mapping
   */
  bool OnMotion(int x, int y);

private:
  // Utility function
  static INPUT::INTERCARDINAL_DIRECTION GetPointerDirection(int x, int y);

  static const unsigned int MIN_FRAME_COUNT = 10;

  // State variables
  bool m_bStarted = false;
  int m_startX = 0;
  int m_startY = 0;
  unsigned int m_frameCount = 0;
};
} // namespace JOYSTICK
} // namespace KODI
