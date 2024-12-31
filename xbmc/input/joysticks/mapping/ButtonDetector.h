/*
 *  Copyright (C) 2014-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "PrimitiveDetector.h"

namespace KODI
{
namespace KEYMAP
{
class IKeymap;
} // namespace KEYMAP

namespace JOYSTICK
{
class CButtonMapping;

/*!
 * \ingroup joystick
 *
 * \brief Detects when a button should be mapped
 */
class CButtonDetector : public CPrimitiveDetector
{
public:
  CButtonDetector(CButtonMapping* buttonMapping, unsigned int buttonIndex);

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
  const unsigned int m_buttonIndex;
};
} // namespace JOYSTICK
} // namespace KODI
