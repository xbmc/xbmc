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

#include "definitions.h"
#include "../AddonBase.h"

namespace kodi
{
namespace gui
{

  //============================================================================
  ///
  /// \defgroup cpp_kodi_gui_DialogContextMenu Dialog Context Menu
  /// \ingroup cpp_kodi_gui
  /// @brief \cpp_namespace{ kodi::gui::DialogContextMenu }
  /// **Context menu dialog**
  ///
  /// The function listed below permits the call of a dialogue as context menu to
  /// select of an entry as a key
  ///
  /// It has the header \ref DialogContextMenu.h "#include <kodi/gui/DialogContextMenu.h>"
  /// be included to enjoy it.
  ///
  ///
  namespace DialogContextMenu
  {
    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_DialogContextMenu
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
    /// #include <kodi/gui/DialogContextMenu.h>
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
    /// int selected = kodi::gui::DialogContextMenu::Show("Test selection", entries);
    /// if (selected < 0)
    ///   fprintf(stderr, "Item selection canceled\n");
    /// else
    ///   fprintf(stderr, "Selected item is: %i\n", selected);
    /// ~~~~~~~~~~~~~
    ///
    inline int Show(const std::string& heading, const std::vector<std::string>& entries)
    {
      using namespace ::kodi::addon;
      unsigned int size = entries.size();
      const char** cEntries = static_cast<const char**>(malloc(size*sizeof(const char**)));
      for (unsigned int i = 0; i < size; ++i)
      {
        cEntries[i] = entries[i].c_str();
      }
      int ret = CAddonBase::m_interface->toKodi->kodi_gui->dialogContextMenu->open(CAddonBase::m_interface->toKodi->kodiBase, heading.c_str(), cEntries, size);
      free(cEntries);
      return ret;
    }
    //--------------------------------------------------------------------------
  };

} /* namespace gui */
} /* namespace kodi */
