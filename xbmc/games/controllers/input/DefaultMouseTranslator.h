/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "input/mouse/MouseTypes.h"

namespace KODI
{
namespace GAME
{

/*!
 * \ingroup games
 */
class CDefaultMouseTranslator
{
public:
  /*!
   * \brief Translate a mouse button ID to a mouse button name
   *
   * \param button The button ID
   *
   * \return The mouse button's name, or empty if unknown
   */
  static const char* TranslateMouseButton(MOUSE::BUTTON_ID button);

  /*!
   * \brief Translate a mouse pointer direction
   *
   * \param direction The relative pointer direction
   *
   * \return The relative pointer direction, or empty if unknown
   */
  static const char* TranslateMousePointer(MOUSE::POINTER_DIRECTION direction);
};

} // namespace GAME
} // namespace KODI
