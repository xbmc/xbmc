/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/*!
* \brief Represents a Menu type (e.g. dvd menus, bluray menus, etc)
*/
enum class MenuType
{
  /*! No supported menu */
  NONE,
  /*! Supports native menus (e.g. those provided natively by blurays or dvds) */
  NATIVE,
  /*! Application specific menu such as the simplified menu for blurays */
  SIMPLIFIED
};
