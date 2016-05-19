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
  /// \defgroup CPP_KodiAPI_GUI_DialogSelect Dialog Select
  /// \ingroup CPP_KodiAPI_GUI
  /// @{
  /// @brief <b>Selection dialog</b>
  ///
  /// The function listed below permits the call of a dialogue to select of an
  /// entry as a key
  ///
  /// These are pure static functions them no other initialization need.
  ///
  /// It has the header \ref DialogSelect.hpp "#include <kodi/api2/gui/DialogSelect.hpp>"
  /// be included to enjoy it.
  ///
  namespace DialogSelect
  {
    //==========================================================================
    ///
    /// \ingroup CPP_KodiAPI_GUI_DialogSelect
    /// @brief Show a selection dialog about given parts.
    ///
    /// @param[in] heading   Dialog heading name
    /// @param[in] entries   String list about entries
    /// @param[in] selected  [opt] Predefined selection (default is <tt>-1</tt> for the first)
    /// @param[in] autoclose [opt] To close dialog automatic after a time
    /// @return          The selected entry, if return <tt>-1</tt> was nothing selected or canceled
    ///
    ///
    ///-------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.cpp}
    /// #include <kodi/api2/gui/DialogSelect.hpp>
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
    /// int selected = KodiAPI::GUI::DialogSelect::Show("Test selection", entries, -1);
    /// if (selected < 0)
    ///   fprintf(stderr, "Item selection canceled\n");
    /// else
    ///   fprintf(stderr, "Selected item is: %i\n", selected);
    /// ~~~~~~~~~~~~~
    ///
    int Show(
      const std::string&              heading,
      const std::vector<std::string>& entries,
      int                             selected = -1,
      bool                            autoclose = false);
    //--------------------------------------------------------------------------
  };
  /// @}

} /* namespace GUI */
} /* namespace KodiAPI */

END_NAMESPACE()
