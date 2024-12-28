/*
 *  Copyright (C) 2014-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "PrimitiveDetector.h"
#include "input/keyboard/KeyboardTypes.h"

namespace KODI
{
namespace JOYSTICK
{
class CButtonMapping;

/*!
 * \ingroup joystick
 *
 * \brief Detects when a keyboard key should be mapped
 */
class CKeyDetector : public CPrimitiveDetector
{
public:
  CKeyDetector(CButtonMapping* buttonMapping, XBMCKey keycode);

  /*!
   * \brief Key state has been updated
   *
   * \param bPressed The new state
   *
   * \return True if this press was handled, false if it should fall through
   *         to the next driver handler
   */
  bool OnMotion(bool bPressed);

private:
  // Construction parameters
  const XBMCKey m_keycode;
};
} // namespace JOYSTICK
} // namespace KODI
