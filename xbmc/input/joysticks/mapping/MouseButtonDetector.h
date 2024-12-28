/*
 *  Copyright (C) 2014-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "PrimitiveDetector.h"
#include "input/mouse/MouseTypes.h"

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
class CMouseButtonDetector : public CPrimitiveDetector
{
public:
  CMouseButtonDetector(CButtonMapping* buttonMapping, MOUSE::BUTTON_ID buttonIndex);

  /*!
   * \brief Button state has been updated
   *
   * \param bPressed The new state
   *
   * \return True if this press was handled, false if it should fall through
   *         to the next driver handler
   */
  bool OnMotion(bool bPressed);

private:
  // Construction parameters
  const MOUSE::BUTTON_ID m_buttonIndex;
};
} // namespace JOYSTICK
} // namespace KODI
