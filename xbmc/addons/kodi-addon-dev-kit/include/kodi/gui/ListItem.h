#pragma once
/*
 *      Copyright (C) 2005-2017 Team Kodi
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

  class CAddonGUIControlBase
  {
  public:
    GUIHANDLE GetControlHandle() const { return m_controlHandle; }

  protected:
    CAddonGUIControlBase(CAddonGUIControlBase* window)
    : m_controlHandle(nullptr),
      m_interface(::kodi::addon::CAddonBase::m_interface->toKodi),
      m_Window(window) {}
    virtual ~CAddonGUIControlBase() = default;

    GUIHANDLE m_controlHandle;
    AddonToKodiFuncTable_Addon* m_interface;
    CAddonGUIControlBase* m_Window;

  private:
    CAddonGUIControlBase() = delete;
    CAddonGUIControlBase(const CAddonGUIControlBase&) = delete;
    CAddonGUIControlBase &operator=(const CAddonGUIControlBase&) = delete;
  };

  //============================================================================
  ///
  /// \defgroup cpp_kodi_gui_CListItem List Item
  /// \ingroup cpp_kodi_gui
  /// @brief \cpp_class{ kodi::gui::CListItem }
  /// **Selectable window list item**
  ///
  /// The list item control is used for creating item lists in Kodi
  ///
  /// The with \ref ListItem.h "#include <kodi/gui/ListItem.h>" given
  /// class is used to create a item entry for a list on window and to support it's
  /// control.
  ///

  //============================================================================
  ///
  /// \defgroup cpp_kodi_gui_CListItem_Defs Definitions, structures and enumerators
  /// \ingroup cpp_kodi_gui_CListItem
  /// @brief **Library definition values**
  ///

  class CListItem : public CAddonGUIControlBase
  {
  public:
    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_CListItem
    /// @brief Class constructor with parameters
    ///
    /// @param[in] label                Item label
    /// @param[in] label2               Second Item label (if needed)
    /// @param[in] iconImage            Item icon image (if needed)
    /// @param[in] thumbnailImage       Thumbnail Image of item (if needed)
    /// @param[in] path                 Path to where item is defined
    ///
    CListItem(
      const std::string& label,
      const std::string& label2,
      const std::string& iconImage,
      const std::string& thumbnailImage,
      const std::string& path)
      : CAddonGUIControlBase(nullptr)
    {
      m_controlHandle = m_interface->kodi_gui->listItem->create(m_interface->kodiBase, label.c_str(),
                                                                     label2.c_str(), iconImage.c_str(),
                                                                     thumbnailImage.c_str(), path.c_str());
    }

    CListItem(GUIHANDLE listItemHandle)
      : CAddonGUIControlBase(nullptr)
    {
      m_controlHandle = listItemHandle;
    }

    ///
    /// \ingroup cpp_kodi_gui_CListItem
    /// @brief Class destructor
    ///
    virtual ~CListItem()
    {
      m_interface->kodi_gui->listItem->destroy(m_interface->kodiBase, m_controlHandle);
    }
    //--------------------------------------------------------------------------
  };

} /* namespace gui */
} /* namespace kodi */
