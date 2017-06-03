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

#include <memory>

namespace kodi
{
namespace gui
{

  class CWindow;

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

    friend class CWindow;

    GUIHANDLE m_controlHandle;
    AddonToKodiFuncTable_Addon* m_interface;
    CAddonGUIControlBase* m_Window;

  private:
    CAddonGUIControlBase() = delete;
    CAddonGUIControlBase(const CAddonGUIControlBase&) = delete;
    CAddonGUIControlBase &operator=(const CAddonGUIControlBase&) = delete;
  };

  class CListItem;
  typedef std::shared_ptr<CListItem> ListItemPtr;

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
      const std::string& label = "",
      const std::string& label2 = "",
      const std::string& iconImage = "",
      const std::string& thumbnailImage = "",
      const std::string& path = "")
      : CAddonGUIControlBase(nullptr)
    {
      m_controlHandle = m_interface->kodi_gui->listItem->create(m_interface->kodiBase, label.c_str(),
                                                                label2.c_str(), iconImage.c_str(),
                                                                thumbnailImage.c_str(), path.c_str());
    }

    /*
     * Constructor used for parts given by list items from addon window
     *
     * Related to call of "ListItemPtr kodi::gui::CWindow::GetListItem(int listPos)"
     * Not needed for addon development itself
     */
    CListItem(GUIHANDLE listItemHandle)
      : CAddonGUIControlBase(nullptr)
    {
      m_controlHandle = listItemHandle;
    }

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_CListItem
    /// @brief Class destructor
    ///
    virtual ~CListItem()
    {
      m_interface->kodi_gui->listItem->destroy(m_interface->kodiBase, m_controlHandle);
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_CListItem
    /// @brief Returns the listitem label.
    ///
    /// @return                       Label of item
    ///
    std::string GetLabel()
    {
      std::string label;
      char* ret = m_interface->kodi_gui->listItem->get_label(m_interface->kodiBase, m_controlHandle);
      if (ret != nullptr)
      {
        if (std::strlen(ret))
          label = ret;
        m_interface->free_string(m_interface->kodiBase, ret);
      }
      return label;
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_CListItem
    /// @brief Sets the listitem label.
    ///
    /// @param[in] label              string or unicode - text string.
    ///
    void SetLabel(const std::string& label)
    {
      m_interface->kodi_gui->listItem->set_label(m_interface->kodiBase, m_controlHandle, label.c_str());
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_CListItem
    /// @brief Returns the second listitem label.
    ///
    /// @return                       Second label of item
    ///
    std::string GetLabel2()
    {
      std::string label;
      char* ret = m_interface->kodi_gui->listItem->get_label2(m_interface->kodiBase, m_controlHandle);
      if (ret != nullptr)
      {
        if (std::strlen(ret))
          label = ret;
        m_interface->free_string(m_interface->kodiBase, ret);
      }
      return label;
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_CListItem
    /// @brief Sets the listitem's label2.
    ///
    /// @param[in] label              string or unicode - text string.
    ///
    void SetLabel2(const std::string& label)
    {
      m_interface->kodi_gui->listItem->set_label2(m_interface->kodiBase, m_controlHandle, label.c_str());
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_CListItem
    /// @brief To get current icon image of entry
    ///
    /// @return The current icon image path (if present)
    ///
    std::string GetIconImage()
    {
      std::string image;
      char* ret = m_interface->kodi_gui->listItem->get_icon_image(m_interface->kodiBase, m_controlHandle);
      if (ret != nullptr)
      {
        if (std::strlen(ret))
          image = ret;
        m_interface->free_string(m_interface->kodiBase, ret);
      }
      return image;
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_CListItem
    /// @brief To set icon image of entry
    ///
    /// @param image                    The image to use for.
    ///
    /// @note Alternative can be \ref SetArt used
    ///
    ///
    void SetIconImage(const std::string& image)
    {
      m_interface->kodi_gui->listItem->set_icon_image(m_interface->kodiBase, m_controlHandle, image.c_str());
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_CListItem
    /// @brief Sets the listitem's art
    ///
    /// @param[in] type                 Type of Art to set
    ///  - Some default art values (any string possible):
    ///  | value (type)  | Type                                              |
    ///  |:-------------:|:--------------------------------------------------|
    ///  | thumb         | string - image filename
    ///  | poster        | string - image filename
    ///  | banner        | string - image filename
    ///  | fanart        | string - image filename
    ///  | clearart      | string - image filename
    ///  | clearlogo     | string - image filename
    ///  | landscape     | string - image filename
    ///  | icon          | string - image filename
    /// @return                         The url to use for Art
    ///
    std::string GetArt(const std::string& type)
    {
      std::string strReturn;
      char* ret = m_interface->kodi_gui->listItem->get_art(m_interface->kodiBase, m_controlHandle, type.c_str());
      if (ret != nullptr)
      {
        if (std::strlen(ret))
          strReturn = ret;
        m_interface->free_string(m_interface->kodiBase, ret);
      }
      return strReturn;
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_CListItem
    /// @brief Sets the listitem's art
    ///
    /// @param[in] type                 Type of Art to set
    /// @param[in] url                  The url to use for Art
    ///  - Some default art values (any string possible):
    ///  | value (type)  | Type                                              |
    ///  |:-------------:|:--------------------------------------------------|
    ///  | thumb         | string - image filename
    ///  | poster        | string - image filename
    ///  | banner        | string - image filename
    ///  | fanart        | string - image filename
    ///  | clearart      | string - image filename
    ///  | clearlogo     | string - image filename
    ///  | landscape     | string - image filename
    ///  | icon          | string - image filename
    ///
    void SetArt(const std::string& type, const std::string& url)
    {
      m_interface->kodi_gui->listItem->set_art(m_interface->kodiBase, m_controlHandle, type.c_str(), url.c_str());
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_CListItem
    /// @brief Returns the path / filename of this listitem.
    ///
    /// @return                       Path string
    ///
    std::string GetPath()
    {
      std::string strReturn;
      char* ret = m_interface->kodi_gui->listItem->get_path(m_interface->kodiBase, m_controlHandle);
      if (ret != nullptr)
      {
        if (std::strlen(ret))
          strReturn = ret;
        m_interface->free_string(m_interface->kodiBase, ret);
      }
      return strReturn;
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_CListItem
    /// @brief Sets the listitem's path.
    ///
    /// @param[in] path               string or unicode - path, activated when
    ///                               item is clicked.
    ///
    /// @note You can use the above as keywords for arguments.
    ///
    void SetPath(const std::string& path)
    {
      m_interface->kodi_gui->listItem->set_path(m_interface->kodiBase, m_controlHandle, path.c_str());
    }
    //--------------------------------------------------------------------------

  };

} /* namespace gui */
} /* namespace kodi */
