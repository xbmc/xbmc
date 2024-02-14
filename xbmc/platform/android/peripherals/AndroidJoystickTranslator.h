/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

namespace PERIPHERALS
{
class CAndroidJoystickTranslator
{
public:
  /*!
     * \brief Translate an axis ID to an Android enum suitable for logging
     *
     * \param axisId The axis ID given in <android/input.h>
     *
     * \return The translated enum label, or "unknown" if unknown
     */
  static const char* TranslateAxis(int axisId);

  /*!
     * \brief Translate a key code to an Android enum suitable for logging
     *
     * \param keyCode The key code given in <android/keycodes.h>
     *
     * \return The translated enum label, or "unknown" if unknown
     */
  static const char* TranslateKeyCode(int keyCode);
};
} // namespace PERIPHERALS
