/*
 *  Copyright (C) 2014-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

namespace KODI
{
namespace JOYSTICK
{
class CButtonMapping;
class CDriverPrimitive;

/*!
 * \ingroup joystick
 *
 * \brief Detects and dispatches mapping events
 *
 * A mapping event usually occurs when a driver primitive is pressed or
 * exceeds a certain threshold.
 *
 * Detection can be quite complicated due to driver bugs, so each type of
 * driver primitive is given its own detector class inheriting from this one.
 */
class CPrimitiveDetector
{
protected:
  CPrimitiveDetector(CButtonMapping* buttonMapping);

  /*!
   * \brief Dispatch a mapping event
   *
   * \return True if the primitive was mapped, false otherwise
   */
  bool MapPrimitive(const CDriverPrimitive& primitive);

private:
  CButtonMapping* const m_buttonMapping;
};
} // namespace JOYSTICK
} // namespace KODI
