/*
 *      Copyright (C) 2017 Team Kodi
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include <set>
#include <string>
#include <vector>

class CWindowTranslator
{
public:
  /*!
   * \brief Get a list of all known window names
   */
  static void GetWindows(std::vector<std::string> &windowList);

  /*!
   * \brief Translate between a window name and its ID
   * \param window The name of the window
   * \return ID of the window, or WINDOW_INVALID if not found
   */
  static int TranslateWindow(const std::string &window);

  /*!
   * \brief Translate between a window id and it's name
   * \param window id of the window
   * \return name of the window, or an empty string if not found
   */
  static std::string TranslateWindow(int windowId);

  /*!
   * \brief Get the window ID that should be used as fallback for keymap input
   * \return The fallback window, or -1 for no fallback window
   */
  static int GetFallbackWindow(int windowId);

  /*!
   * \brief Get the special window ID if conditions met
   * \return The special window ID or the given window ID
   */
  static int GetVirtualWindow(int windowId);

private:
  struct WindowMapItem
  {
    const char *windowName;
    int windowId;
  };

  struct WindowNameCompare
  {
    bool operator()(const WindowMapItem &lhs, const WindowMapItem &rhs) const;
  };

  struct WindowIDCompare
  {
    bool operator()(const WindowMapItem &lhs, const WindowMapItem &rhs) const;
  };

  using WindowMapByName = std::set<WindowMapItem, WindowNameCompare>;
  using WindowMapByID = std::set<WindowMapItem, WindowIDCompare>;

  static WindowMapByID CreateWindowMappingByID();

  static const WindowMapByName WindowMappingByName;
};
