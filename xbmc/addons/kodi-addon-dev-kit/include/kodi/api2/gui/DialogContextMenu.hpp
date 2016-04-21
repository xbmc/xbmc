#pragma once
/*
 *      Copyright (C) 2015-2016 Team KODI
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
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <vector>

API_NAMESPACE

namespace KodiAPI
{
namespace GUI
{

  //============================================================================
  ///
  /// \defgroup CPP_KodiAPI_GUI_DialogContextMenu Dialog Context Menu
  /// \ingroup CPP_KodiAPI_GUI
  /// @{
  /// @brief <b>Selection dialog</b>
  ///
  /// The function listed below permits the call of a dialogue as context menu to
  /// select of an entry as a key
  ///
  /// These are pure static functions them no other initialization need.
  ///
  /// It has the header \ref DialogSelect.hpp "#include <kodi/api2/gui/DialogContext.hpp>"
  /// be included to enjoy it.
  ///
  ///
  namespace DialogContextMenu
  {
    //==========================================================================
    ///
    /// \ingroup CPP_KodiAPI_GUI_DialogContextMenu
    /// @brief Show a context menu dialog about given parts.
    ///
    /// @param[in] heading   Dialog heading name
    /// @param[in] entries   String list about entries
    /// @return          The selected entry, if return <tt>-1</tt> was nothing selected or canceled
    ///
    ///
    ///-------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.cpp}
    /// #include <kodi/api2/gui/DialogContextMenu.hpp>
    ///
    /// const std::vector<std::string> entries
    /// {
    ///   "Test 1",
    ///   "Test 2",
    ///   "Test 3",
    ///   "Test 4",
    ///   "Test 5"
    /// };
    ///
    /// int selected = KodiAPI::GUI::DialogSelect::Show("Test selection", entries);
    /// if (selected < 0)
    ///   fprintf(stderr, "Item selection canceled\n");
    /// else
    ///   fprintf(stderr, "Selected item is: %i\n", selected);
    /// ~~~~~~~~~~~~~
    ///
    int Show(
      const std::string&              heading,
      const std::vector<std::string>& entries);
    //--------------------------------------------------------------------------
  };
  /// @}

} /* namespace GUI */
} /* namespace KodiAPI */

END_NAMESPACE()
