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

API_NAMESPACE

namespace KodiAPI
{
namespace GUI
{

  //============================================================================
  ///
  /// \defgroup CPP_KodiAPI_GUI_DialogOK Dialog OK
  /// \ingroup CPP_KodiAPI_GUI
  /// @{
  /// @brief <b>OK dialog</b>
  ///
  /// The functions listed below permit the call of a dialogue of information, a
  /// confirmation of the user by press from OK required.
  ///
  /// These are pure static functions them no other initialization need.
  ///
  /// It has the header \ref DialogOK.hpp "#include <kodi/api2/gui/DialogOK.hpp>"
  /// be included to enjoy it.
  ///
  namespace DialogOK
  {
    //==========================================================================
    ///
    /// \ingroup CPP_KodiAPI_GUI_DialogOK
    /// @brief Use dialog to inform user with text and confirmation with OK with continued string.
    ///
    /// @param[in] heading Dialog heading.
    /// @param[in] text Multi-line text.
    ///
    ///
    ///-------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.cpp}
    /// #include <kodi/api2/gui/DialogOK.hpp>
    /// ...
    /// KodiAPI::GUI::DialogOK::ShowAndGetInput("Test dialog", "Hello World!\nI'm a call from add-on\n :) :D");
    /// ~~~~~~~~~~~~~
    ///
    void ShowAndGetInput(
      const std::string&      heading,
      const std::string&      text);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CPP_KodiAPI_GUI_DialogOK
    /// @brief Use dialog to inform user with text and confirmation with OK with strings separated to the lines.
    ///
    /// @param[in] heading Dialog heading.
    /// @param[in] line0 Line #1 text.
    /// @param[in] line1 Line #2 text.
    /// @param[in] line2 Line #3 text.
    ///
    ///
    ///-------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.cpp}
    /// #include <kodi/api2/gui/DialogOK.hpp>
    /// ...
    /// KodiAPI::GUI::DialogOK::ShowAndGetInput("Test dialog", "Hello World!", "I'm a call from add-on", " :) :D");
    /// ~~~~~~~~~~~~~
    ///
    void ShowAndGetInput(
      const std::string&      heading,
      const std::string&      line0,
      const std::string&      line1,
      const std::string&      line2);
    //--------------------------------------------------------------------------
  };
  /// @}

} /* namespace GUI */
} /* namespace KodiAPI */

END_NAMESPACE()
