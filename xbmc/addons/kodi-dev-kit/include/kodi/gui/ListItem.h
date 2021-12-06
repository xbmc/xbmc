/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "../AddonBase.h"
#include "../c-api/gui/list_item.h"

#ifdef __cplusplus

#include <memory>

namespace kodi
{
namespace gui
{

class CWindow;

class ATTR_DLL_LOCAL CAddonGUIControlBase
{
public:
  KODI_GUI_LISTITEM_HANDLE GetControlHandle() const { return m_controlHandle; }

protected:
  explicit CAddonGUIControlBase(CAddonGUIControlBase* window)
    : m_controlHandle(nullptr),
      m_interface(::kodi::addon::CPrivateBase::m_interface->toKodi),
      m_Window(window)
  {
  }

  virtual ~CAddonGUIControlBase() = default;

  friend class CWindow;

  KODI_GUI_LISTITEM_HANDLE m_controlHandle;
  AddonToKodiFuncTable_Addon* m_interface;
  CAddonGUIControlBase* m_Window;

private:
  CAddonGUIControlBase() = delete;
  CAddonGUIControlBase(const CAddonGUIControlBase&) = delete;
  CAddonGUIControlBase& operator=(const CAddonGUIControlBase&) = delete;
};

class CListItem;

//==============================================================================
/// @addtogroup cpp_kodi_gui_windows_listitem
/// @brief @cpp_class{ kodi::gui::CListItem }
/// **Selectable window list item**\n
/// The list item control is used for creating item lists in Kodi.
///
/// The with @ref ListItem.h "#include <kodi/gui/ListItem.h>" given
/// class is used to create a item entry for a list on window and to support it's
/// control.
///
class ATTR_DLL_LOCAL CListItem : public CAddonGUIControlBase
{
public:
  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_listitem
  /// @brief Class constructor with parameters.
  ///
  /// @param[in] label [opt] Item label
  /// @param[in] label2 [opt] Second Item label (if needed)
  /// @param[in] path [opt] Path to where item is defined
  ///
  CListItem(const std::string& label = "",
            const std::string& label2 = "",
            const std::string& path = "")
    : CAddonGUIControlBase(nullptr)
  {
    m_controlHandle = m_interface->kodi_gui->listItem->create(m_interface->kodiBase, label.c_str(),
                                                              label2.c_str(), path.c_str());
  }

  /*
   * Constructor used for parts given by list items from addon window
   *
   * Related to call of "std::shared_ptr<CListItem> kodi::gui::CWindow::GetListItem(int listPos)"
   * Not needed for addon development itself
   */
  explicit CListItem(KODI_GUI_LISTITEM_HANDLE listItemHandle) : CAddonGUIControlBase(nullptr)
  {
    m_controlHandle = listItemHandle;
  }

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_listitem
  /// @brief Class destructor
  ///
  ~CListItem() override
  {
    m_interface->kodi_gui->listItem->destroy(m_interface->kodiBase, m_controlHandle);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_listitem
  /// @brief Returns the listitem label.
  ///
  /// @return Label of item
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
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_listitem
  /// @brief Sets the listitem label.
  ///
  /// @param[in] label              string or unicode - text string.
  ///
  void SetLabel(const std::string& label)
  {
    m_interface->kodi_gui->listItem->set_label(m_interface->kodiBase, m_controlHandle,
                                               label.c_str());
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_listitem
  /// @brief Returns the second listitem label.
  ///
  /// @return Second label of item
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
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_listitem
  /// @brief Sets the listitem's label2.
  ///
  /// @param[in] label              string or unicode - text string.
  ///
  void SetLabel2(const std::string& label)
  {
    m_interface->kodi_gui->listItem->set_label2(m_interface->kodiBase, m_controlHandle,
                                                label.c_str());
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_listitem
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
  /// @return   The url to use for Art
  ///
  std::string GetArt(const std::string& type)
  {
    std::string strReturn;
    char* ret = m_interface->kodi_gui->listItem->get_art(m_interface->kodiBase, m_controlHandle,
                                                         type.c_str());
    if (ret != nullptr)
    {
      if (std::strlen(ret))
        strReturn = ret;
      m_interface->free_string(m_interface->kodiBase, ret);
    }
    return strReturn;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_listitem
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
    m_interface->kodi_gui->listItem->set_art(m_interface->kodiBase, m_controlHandle, type.c_str(),
                                             url.c_str());
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_listitem
  /// @brief Returns the path / filename of this listitem.
  ///
  /// @return Path string
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
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_listitem
  /// @brief Sets the listitem's path.
  ///
  /// @param[in] path string or unicode - path, activated when item is clicked.
  ///
  /// @note You can use the above as keywords for arguments.
  ///
  void SetPath(const std::string& path)
  {
    m_interface->kodi_gui->listItem->set_path(m_interface->kodiBase, m_controlHandle, path.c_str());
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_listitem
  /// @brief Sets a listitem property, similar to an infolabel.
  ///
  /// @param[in] key string - property name.
  /// @param[in] value string or unicode - value of property.
  ///
  /// @note Key is NOT case sensitive.
  ///       You can use the above as keywords for arguments and skip certain@n
  ///       optional arguments.\n
  ///       Once you use a keyword, all following arguments require the
  ///       keyword.
  ///
  /// Some of these are treated internally by Kodi, such as the
  /// <b>'StartOffset'</b> property, which is the offset in seconds at which to
  /// start playback of an item.  Others may be used in the skin to add
  /// extra information, such as <b>'WatchedCount'</b> for tvshow items
  ///
  void SetProperty(const std::string& key, const std::string& value)
  {
    m_interface->kodi_gui->listItem->set_property(m_interface->kodiBase, m_controlHandle,
                                                  key.c_str(), value.c_str());
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_listitem
  /// @brief Returns a listitem property as a string, similar to an infolabel.
  ///
  /// @param[in] key string - property name.
  /// @return string - List item property
  ///
  /// @note Key is NOT case sensitive.\n
  ///       You can use the above as keywords for arguments and skip certain
  ///       optional arguments.\n
  ///       Once you use a keyword, all following arguments require the
  ///       keyword.
  ///
  std::string GetProperty(const std::string& key)
  {
    std::string label;
    char* ret = m_interface->kodi_gui->listItem->get_property(m_interface->kodiBase,
                                                              m_controlHandle, key.c_str());
    if (ret != nullptr)
    {
      if (std::strlen(ret))
        label = ret;
      m_interface->free_string(m_interface->kodiBase, ret);
    }
    return label;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_listitem
  /// @brief To control selection of item in list (also multiple selection,
  /// in list on several items possible).
  ///
  /// @param[in] selected if true becomes set as selected
  ///
  void Select(bool selected)
  {
    m_interface->kodi_gui->listItem->select(m_interface->kodiBase, m_controlHandle, selected);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_listitem
  /// @brief Returns the listitem's selected status.
  ///
  /// @return true if selected, otherwise false
  ///
  bool IsSelected()
  {
    return m_interface->kodi_gui->listItem->is_selected(m_interface->kodiBase, m_controlHandle);
  }
  //----------------------------------------------------------------------------
};

} /* namespace gui */
} /* namespace kodi */

#endif /* __cplusplus */
