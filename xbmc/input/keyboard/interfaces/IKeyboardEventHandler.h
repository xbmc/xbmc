/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

class CKey;
struct XBMC_keysym;

namespace KODI
{
namespace KEYBOARD
{

/*!
 * \ingroup keyboard
 *
 * \brief Interface for handling keyboard events before Kodi acts on them
 *
 * The event still carries the driver's scan code, which is what a handler
 * that forwards keys to another process needs.
 */
class IKeyboardEventHandler
{
public:
  virtual ~IKeyboardEventHandler() = default;

  /*!
   * \brief A key was pressed or released
   *
   * \param keysym The event as delivered by the windowing system
   * \param key The key as Kodi's keymaps see it
   * \param pressed True on press, false on release
   *
   * \return True if the event was handled and Kodi must not act on it
   */
  virtual bool OnKeyboardEvent(const XBMC_keysym& keysym, const CKey& key, bool pressed) = 0;
};

} // namespace KEYBOARD
} // namespace KODI
