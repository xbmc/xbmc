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
 * \brief Detects when a D-pad direction should be mapped
 */
class CHatDetector : public CPrimitiveDetector
{
public:
  CHatDetector(CButtonMapping* buttonMapping, unsigned int hatIndex);

  /*!
   * \brief Hat state has been updated
   *
   * \param state The new state
   *
   * \return True if state is a cardinal direction, false otherwise
   */
  bool OnMotion(HAT_STATE state);

private:
  // Construction parameters
  const unsigned int m_hatIndex;
};
} // namespace JOYSTICK
} // namespace KODI
