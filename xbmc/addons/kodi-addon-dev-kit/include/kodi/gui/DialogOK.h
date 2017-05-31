#pragma once
/*
 *      Copyright (C) 2005-2017 Team KODI
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

#include "../AddonBase.h"
#include "definitions.h"

namespace kodi
{
namespace gui
{

  //============================================================================
  ///
  /// \defgroup cpp_kodi_gui_DialogOK Dialog OK
  /// \ingroup cpp_kodi_gui
  /// @{
  /// @brief \cpp_namespace{ kodi::gui::DialogOK }
  /// **OK dialog**
  ///
  /// The functions listed below permit the call of a dialogue of information, a
  /// confirmation of the user by press from OK required.
  ///
  /// It has the header \ref DialogOK.h "#include <kodi/gui/DialogOK.h>"
  /// be included to enjoy it.
  ///
  namespace DialogOK
  {
    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_DialogOK
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
    /// #include <kodi/gui/DialogOK.h>
    /// ...
    /// kodi::gui::DialogOK::ShowAndGetInput("Test dialog", "Hello World!\nI'm a call from add-on\n :) :D");
    /// ~~~~~~~~~~~~~
    ///
    inline void ShowAndGetInput(const std::string& heading, const std::string& text)
    {
      using namespace ::kodi::addon;
      CAddonBase::m_interface->toKodi->kodi_gui->dialogOK->show_and_get_input_single_text(CAddonBase::m_interface->toKodi->kodiBase,
                                                                                          heading.c_str(), text.c_str());
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_DialogOK
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
    /// #include <kodi/gui/DialogOK.h>
    /// ...
    /// kodi::gui::DialogOK::ShowAndGetInput("Test dialog", "Hello World!", "I'm a call from add-on", " :) :D");
    /// ~~~~~~~~~~~~~
    ///
    inline void ShowAndGetInput(const std::string& heading, const std::string& line0, const std::string& line1, const std::string& line2)
    {
      using namespace ::kodi::addon;
      CAddonBase::m_interface->toKodi->kodi_gui->dialogOK->show_and_get_input_line_text(CAddonBase::m_interface->toKodi->kodiBase,
                                                                                        heading.c_str(), line0.c_str(), line1.c_str(),
                                                                                        line2.c_str());
    }
    //--------------------------------------------------------------------------
  }
  /// @}

} /* namespace gui */
} /* namespace kodi */
