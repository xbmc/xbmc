/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "dialogs/GUIDialogKeyboardGeneric.h"

namespace KODI
{
namespace WEB
{

/*!
 * \brief Kodi's keyboard for a text input on a web page: the keys along
 * the bottom of the screen, the page's own field showing what is typed
 */
class CGUIDialogWebKeyboard : public CGUIDialogKeyboardGeneric
{
public:
  CGUIDialogWebKeyboard();

  /*!
   * \brief Top edge of the screen area the keys cover, in screen pixels;
   * the screen height when the skin does not say
   */
  int GetCoveredTop();
};

} // namespace WEB
} // namespace KODI
