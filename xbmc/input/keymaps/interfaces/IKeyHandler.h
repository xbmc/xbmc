/*
 *  Copyright (C) 2015-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

namespace KODI
{
namespace KEYMAP
{
/*!
 * \ingroup keymap
 *
 * \brief Interface for handling keymap keys
 *
 * Keys can be mapped to analog actions (e.g. "AnalogSeekForward") or digital
 * actions (e.g. "Up").
 */
class IKeyHandler
{
public:
  virtual ~IKeyHandler() = default;

  /*!
   * \brief Return true if the key is "pressed" (has a magnitude greater
   *        than 0.5)
   *
   * \return True if the key is "pressed", false otherwise
   */
  virtual bool IsPressed() const = 0;

  /*!
   * \brief A key mapped to a digital feature has been pressed or released
   *
   * \param bPressed   true if the key's button/axis is activated, false if deactivated
   * \param holdTimeMs The held time in ms for pressed buttons, or 0 for released
   *
   * \return True if the key is mapped to an action, false otherwise
   */
  virtual bool OnDigitalMotion(bool bPressed, unsigned int holdTimeMs) = 0;

  /*!
   * \brief Callback for keys mapped to analog features
   *
   * \param magnitude     The amount of the analog action
   * \param motionTimeMs  The time since the magnitude was 0
   *
   * \return True if the key is mapped to an action, false otherwise
   */
  virtual bool OnAnalogMotion(float magnitude, unsigned int motionTimeMs) = 0;
};
} // namespace KEYMAP
} // namespace KODI
