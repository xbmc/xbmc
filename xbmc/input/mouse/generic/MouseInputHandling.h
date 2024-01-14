/*
 *  Copyright (C) 2016-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "input/mouse/MouseTypes.h"
#include "input/mouse/interfaces/IMouseDriverHandler.h"

namespace KODI
{
namespace JOYSTICK
{
class IButtonMap;
}

namespace MOUSE
{
class IMouseInputHandler;

/*!
 * \ingroup mouse
 *
 * \brief Class to translate input from driver info to higher-level features
 */
class CMouseInputHandling : public IMouseDriverHandler
{
public:
  CMouseInputHandling(IMouseInputHandler* handler, JOYSTICK::IButtonMap* buttonMap);

  ~CMouseInputHandling(void) override = default;

  // implementation of IMouseDriverHandler
  bool OnPosition(int x, int y) override;
  bool OnButtonPress(BUTTON_ID button) override;
  void OnButtonRelease(BUTTON_ID button) override;

private:
  // Utility functions
  static POINTER_DIRECTION GetPointerDirection(int x, int y);
  static POINTER_DIRECTION GetOrthogonalDirectionCCW(POINTER_DIRECTION direction);

  static void GetRotation(POINTER_DIRECTION source,
                          POINTER_DIRECTION target,
                          int (&rotation)[2][2]);
  static void GetRotation(int deg, int (&rotation)[2][2]);

  static void GetReflectionCCW(POINTER_DIRECTION source,
                               POINTER_DIRECTION target,
                               int (&reflection)[2][2]);
  static void GetReflection(int deg, int (&reflection)[2][2]);

  // Construction parameters
  IMouseInputHandler* const m_handler;
  JOYSTICK::IButtonMap* const m_buttonMap;

  // Mouse parameters
  bool m_bHasPosition = false;
  int m_x = 0;
  int m_y = 0;
};
} // namespace MOUSE
} // namespace KODI
