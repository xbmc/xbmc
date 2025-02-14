/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "../AddonBase.h"
#include "../c-api/gui/window.h"
#include "ListItem.h"
#include "input/ActionIDs.h"

#ifdef __cplusplus

namespace kodi
{
namespace gui
{

//==============================================================================
/// @defgroup cpp_kodi_gui_windows_window_Defs Definitions, structures and enumerators
/// @ingroup cpp_kodi_gui_windows_window
/// @brief **Library definition values**\n
/// Additional values, structures and things that are used in the Window class.
///
/// ------------------------------------------------------------------------
///
/// @link cpp_kodi_gui_windows_window Go back to normal functions from CWindow@endlink
///

//==============================================================================
/// @ingroup cpp_kodi_gui_windows_window_Defs
/// @brief **Handler for addon-sided processing class**\n
/// If the callback functions used by the window are not used directly in the
/// @ref cpp_kodi_gui_windows_window "CWindow" class and are outside of it.
///
/// This value here corresponds to a <b>`void*`</b> and returns the address
/// requested by the add-on for callbacks.
///
using ClientHandle = KODI_GUI_CLIENT_HANDLE;
//------------------------------------------------------------------------------

class CListItem;

//==============================================================================
/// @addtogroup cpp_kodi_gui_windows_window
/// @brief @cpp_class{ kodi::gui::CWindow }
/// **Main window control class**\n
/// The addon uses its own skin xml file and displays it in Kodi using this class.
///
/// The with  @ref Window.h "#include <kodi/gui/Window.h>"
/// included file brings support to create a window or dialog on Kodi.
///
/// The add-on has to integrate its own @ref cpp_kodi_gui_windows_window_callbacks "callback functions"
/// in order to process the necessary user access to its window.
///
///
/// --------------------------------------------------------------------------
///
/// **Window header example:**
/// ~~~~~~~~~~~~~{.xml}
/// <?xml version="1.0" encoding="UTF-8"?>
/// <window>
///   <onload>RunScript(script.foobar)</onload>
///   <onunload>SetProperty(foo,bar)</onunload>
///   <defaultcontrol always="false">2</defaultcontrol>
///   <menucontrol>9000</menucontrol>
///   <backgroundcolor>0xff00ff00</backgroundcolor>
///   <views>50,51,509,510</views>
///   <visible>Window.IsActive(Home)</visible>
///   <animation effect="fade" time="100">WindowOpen</animation>
///   <animation effect="slide" end="0,576" time="100">WindowClose</animation>
///   <zorder>1</zorder>
///   <coordinates>
///     <left>40</left>
///     <top>50</top>
///     <origin x="100" y="50">Window.IsActive(Home)</origin>
///   </coordinates>
///   <previouswindow>MyVideos</previouswindow>
///   <controls>
///     <control>
///     </control>
///     ....
///   </controls>
/// </window>
/// ~~~~~~~~~~~~~
///
/// --------------------------------------------------------------------------
///
/// On functions defined input variable <b><tt>controlId</tt> (GUI control identifier)</b>
/// is the on window.xml defined value behind type added with <tt><b>id="..."</b></tt> and
/// used to identify for changes there and on callbacks.
///
/// ~~~~~~~~~~~~~{.xml}
///  <control type="label" id="31">
///    <description>Title Label</description>
///    ...
///  </control>
///  <control type="progress" id="32">
///    <description>progress control</description>
///    ...
///  </control>
/// ~~~~~~~~~~~~~
///
///
class ATTR_DLL_LOCAL CWindow : public CAddonGUIControlBase
{
public:
  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_window
  /// @brief Class constructor with needed values for window / dialog.
  ///
  /// Creates a new Window class.
  ///
  /// @param[in] xmlFilename XML file for the skin
  /// @param[in] defaultSkin Default skin to use if needed not available
  /// @param[in] asDialog Use window as dialog if set
  /// @param[in] isMedia [opt] bool - if False, create a regular window.
  ///                    if True, create a mediawindow. (default=false)
  ///
  /// @note <b>`isMedia`</b> value as true only usable for windows not for dialogs.
  ///
  CWindow(const std::string& xmlFilename,
          const std::string& defaultSkin,
          bool asDialog,
          bool isMedia = false)
    : CAddonGUIControlBase(nullptr)
  {
    m_controlHandle = m_interface->kodi_gui->window->create(
        m_interface->kodiBase, xmlFilename.c_str(), defaultSkin.c_str(), asDialog, isMedia);
    if (!m_controlHandle)
      kodi::Log(ADDON_LOG_FATAL, "kodi::gui::CWindow can't create window class from Kodi !!!");
    m_interface->kodi_gui->window->set_callbacks(m_interface->kodiBase, m_controlHandle, this,
                                                 CBOnInit, CBOnFocus, CBOnClick, CBOnAction,
                                                 CBGetContextButtons, CBOnContextButton);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup CWindow
  /// @brief Class destructor.
  ///
  ~CWindow() override
  {
    if (m_controlHandle)
      m_interface->kodi_gui->window->destroy(m_interface->kodiBase, m_controlHandle);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_window
  /// @brief Show this window.
  ///
  /// Shows this window by activating it, calling close() after it will activate
  /// the current window again.
  ///
  /// @note If your Add-On ends this window will be closed to. To show it forever,
  /// make a loop at the end of your Add-On or use @ref DoModal() instead.
  ///
  /// @warning If used must be the class be global present until Kodi becomes
  /// closed. The creation can be done before "Show" becomes called, but
  /// not delete class after them.
  ///
  /// @return True on success, false otherwise.
  ///
  bool Show()
  {
    return m_interface->kodi_gui->window->show(m_interface->kodiBase, m_controlHandle);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_window
  /// @brief Closes this window.
  ///
  /// Closes this window by activating the old window.
  /// @note The window is not deleted with this method.
  ///
  void Close() { m_interface->kodi_gui->window->close(m_interface->kodiBase, m_controlHandle); }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_window
  /// @brief Display this window until close() is called.
  ///
  void DoModal()
  {
    m_interface->kodi_gui->window->do_modal(m_interface->kodiBase, m_controlHandle);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_window
  /// @brief Gives the control with the supplied focus.
  ///
  /// @param[in] controlId On skin defined id of control
  /// @return True on success, false otherwise.
  ///
  bool SetFocusId(int controlId)
  {
    return m_interface->kodi_gui->window->set_focus_id(m_interface->kodiBase, m_controlHandle,
                                                       controlId);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_window
  /// @brief Returns the id of the control which is focused.
  ///
  /// @return Focused control id
  ///
  int GetFocusId()
  {
    return m_interface->kodi_gui->window->get_focus_id(m_interface->kodiBase, m_controlHandle);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_window
  /// @brief To set the used label on given control id.
  ///
  /// @param[in] controlId Control id where label need to set
  /// @param[in] label Label to use
  ///
  void SetControlLabel(int controlId, const std::string& label)
  {
    m_interface->kodi_gui->window->set_control_label(m_interface->kodiBase, m_controlHandle,
                                                     controlId, label.c_str());
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_window
  /// @brief To set the visibility on given control id.
  ///
  /// @param[in] controlId Control id where visibility is changed
  /// @param[in] visible Boolean value with `true` for visible, `false` for hidden
  ///
  void SetControlVisible(int controlId, bool visible)
  {
    m_interface->kodi_gui->window->set_control_visible(m_interface->kodiBase, m_controlHandle,
                                                       controlId, visible);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_window
  /// @brief To set the selection on given control id.
  ///
  /// @param[in] controlId Control id where selection is changed
  /// @param[in] selected Boolean value with `true` for selected, `false` for not
  ///
  void SetControlSelected(int controlId, bool selected)
  {
    m_interface->kodi_gui->window->set_control_selected(m_interface->kodiBase, m_controlHandle,
                                                        controlId, selected);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_window
  /// @brief Sets a window property, similar to an infolabel.
  ///
  /// @param[in] key string - property name.
  /// @param[in] value string or unicode - value of property.
  ///
  /// @note  Key is NOT case sensitive. Setting value to an empty string is
  ///        equivalent to clearProperty(key).\n
  ///        You can use the above as keywords for arguments and skip certain
  ///        optional arguments.\n
  ///        Once you use a keyword, all following arguments require the keyword.
  ///
  void SetProperty(const std::string& key, const std::string& value)
  {
    m_interface->kodi_gui->window->set_property(m_interface->kodiBase, m_controlHandle, key.c_str(),
                                                value.c_str());
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_window
  /// @brief Returns a window property as a string, similar to an infolabel.
  ///
  /// @param[in] key string - property name.
  /// @return The property as string (if present)
  ///
  /// @note  Key is NOT case sensitive. Setting value to an empty string is
  ///        equivalent to clearProperty(key).\n
  ///        You can use the above as keywords for arguments and skip certain
  ///        optional arguments.\n
  ///        Once you use a keyword, all following arguments require the keyword.
  ///
  std::string GetProperty(const std::string& key) const
  {
    std::string label;
    char* ret = m_interface->kodi_gui->window->get_property(m_interface->kodiBase, m_controlHandle,
                                                            key.c_str());
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
  /// @ingroup cpp_kodi_gui_windows_window
  /// @brief Sets a window property with integer value
  ///
  /// @param[in] key string - property name.
  /// @param[in] value integer value to set
  ///
  void SetPropertyInt(const std::string& key, int value)
  {
    m_interface->kodi_gui->window->set_property_int(m_interface->kodiBase, m_controlHandle,
                                                    key.c_str(), value);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_window
  /// @brief Returns a window property with integer value
  ///
  /// @param[in] key string - property name.
  /// @return integer value of property
  ///
  int GetPropertyInt(const std::string& key) const
  {
    return m_interface->kodi_gui->window->get_property_int(m_interface->kodiBase, m_controlHandle,
                                                           key.c_str());
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_window
  /// @brief Sets a window property with boolean value
  ///
  /// @param[in] key string - property name.
  /// @param[in] value boolean value to set
  ///
  void SetPropertyBool(const std::string& key, bool value)
  {
    m_interface->kodi_gui->window->set_property_bool(m_interface->kodiBase, m_controlHandle,
                                                     key.c_str(), value);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_window
  /// @brief Returns a window property with boolean value
  ///
  /// @param[in] key string - property name.
  /// @return boolean value of property
  ///
  bool GetPropertyBool(const std::string& key) const
  {
    return m_interface->kodi_gui->window->get_property_bool(m_interface->kodiBase, m_controlHandle,
                                                            key.c_str());
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_window
  /// @brief Sets a window property with double value
  ///
  /// @param[in] key string - property name.
  /// @param[in] value double value to set
  ///
  void SetPropertyDouble(const std::string& key, double value)
  {
    m_interface->kodi_gui->window->set_property_double(m_interface->kodiBase, m_controlHandle,
                                                       key.c_str(), value);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_window
  /// @brief Returns a window property with double value
  ///
  /// @param[in] key string - property name.
  /// @return double value of property
  ///
  double GetPropertyDouble(const std::string& key) const
  {
    return m_interface->kodi_gui->window->get_property_double(m_interface->kodiBase,
                                                              m_controlHandle, key.c_str());
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_window
  /// @brief Remove all present properties from window
  ///
  void ClearProperties()
  {
    m_interface->kodi_gui->window->clear_properties(m_interface->kodiBase, m_controlHandle);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_window
  /// @brief Clears the specific window property.
  ///
  /// @param[in] key string - property name.
  ///
  /// @note Key is NOT case sensitive. Equivalent to SetProperty(key, "")
  ///       You can use the above as keywords for arguments and skip certain
  ///       optional arguments.
  ///       Once you use a keyword, all following arguments require the
  ///       keyword.
  ///
  ///
  ///-----------------------------------------------------------------------
  ///
  /// **Example:**
  /// ~~~~~~~~~~~~~{.cpp}
  /// ..
  /// ClearProperty('Category')
  /// ..
  /// ~~~~~~~~~~~~~
  ///
  void ClearProperty(const std::string& key)
  {
    m_interface->kodi_gui->window->clear_property(m_interface->kodiBase, m_controlHandle,
                                                  key.c_str());
  }
  //----------------------------------------------------------------------------

  /// @{
  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_window
  /// @brief Function delete all entries in integrated list.
  ///
  void ClearList()
  {
    m_interface->kodi_gui->window->clear_item_list(m_interface->kodiBase, m_controlHandle);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_window
  /// @brief To add a list item in the on window integrated list.
  ///
  /// @param[in] item List item to add
  /// @param[in] itemPosition [opt] The position for item, default is on end
  ///
  void AddListItem(const std::shared_ptr<CListItem>& item, int itemPosition = -1)
  {
    m_interface->kodi_gui->window->add_list_item(m_interface->kodiBase, m_controlHandle,
                                                 item->m_controlHandle, itemPosition);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_window
  /// @brief To add a list item based upon string in the on window integrated list.
  ///
  /// @param[in] item List item to add
  /// @param[in] itemPosition [opt] The position for item, default is on end
  ///
  void AddListItem(const std::string& item, int itemPosition = -1)
  {
    m_interface->kodi_gui->window->add_list_item(
        m_interface->kodiBase, m_controlHandle,
        std::make_shared<kodi::gui::CListItem>(item)->m_controlHandle, itemPosition);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_window
  /// @brief Remove list item on position.
  ///
  /// @param[in] itemPosition List position to remove
  ///
  void RemoveListItem(int itemPosition)
  {
    m_interface->kodi_gui->window->remove_list_item_from_position(m_interface->kodiBase,
                                                                  m_controlHandle, itemPosition);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_window
  /// @brief Remove item with given control class from list.
  ///
  /// @param[in] item List item control class to remove
  ///
  void RemoveListItem(const std::shared_ptr<CListItem>& item)
  {
    m_interface->kodi_gui->window->remove_list_item(m_interface->kodiBase, m_controlHandle,
                                                    item->m_controlHandle);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_window
  /// @brief To get list item control class on wanted position.
  ///
  /// @param[in] listPos Position from where control is needed
  /// @return The list item control class or null if not found
  ///
  /// @warning Function returns a new generated **CListItem** class!
  ///
  std::shared_ptr<CListItem> GetListItem(int listPos)
  {
    KODI_GUI_LISTITEM_HANDLE handle = m_interface->kodi_gui->window->get_list_item(
        m_interface->kodiBase, m_controlHandle, listPos);
    if (!handle)
      return std::shared_ptr<CListItem>();

    return std::make_shared<kodi::gui::CListItem>(handle);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_window
  /// @brief To set position of selected part in list.
  ///
  /// @param[in] listPos Position to use
  ///
  void SetCurrentListPosition(int listPos)
  {
    m_interface->kodi_gui->window->set_current_list_position(m_interface->kodiBase, m_controlHandle,
                                                             listPos);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_window
  /// @brief To get current selected position in list
  ///
  /// @return Current list position
  ///
  int GetCurrentListPosition()
  {
    return m_interface->kodi_gui->window->get_current_list_position(m_interface->kodiBase,
                                                                    m_controlHandle);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_window
  /// @brief To get the amount of entries in the list.
  ///
  /// @return Size of in window integrated control class
  ///
  int GetListSize()
  {
    return m_interface->kodi_gui->window->get_list_size(m_interface->kodiBase, m_controlHandle);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_window
  /// @brief Sets a container property, similar to an infolabel.
  ///
  /// @param[in] key string - property name.
  /// @param[in] value string or unicode - value of property.
  ///
  /// @note Key is NOT case sensitive.\n
  /// You can use the above as keywords for arguments and skip certain
  /// optional arguments.\n
  /// Once you use a keyword, all following arguments require the keyword.
  ///
  void SetContainerProperty(const std::string& key, const std::string& value)
  {
    m_interface->kodi_gui->window->set_container_property(m_interface->kodiBase, m_controlHandle,
                                                          key.c_str(), value.c_str());
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_window
  /// @brief Sets the content type of the container.
  ///
  /// @param[in] value string or unicode - content value.
  ///
  /// __Available content types__
  /// | Name        | Media                                    |
  /// |:-----------:|:-----------------------------------------|
  /// | actors      | Videos
  /// | addons      | Addons, Music, Pictures, Programs, Videos
  /// | albums      | Music, Videos
  /// | artists     | Music, Videos
  /// | countries   | Music, Videos
  /// | directors   | Videos
  /// | files       | Music, Videos
  /// | games       | Games
  /// | genres      | Music, Videos
  /// | images      | Pictures
  /// | mixed       | Music, Videos
  /// | movies      | Videos
  /// | Musicvideos | Music, Videos
  /// | playlists   | Music, Videos
  /// | seasons     | Videos
  /// | sets        | Videos
  /// | songs       | Music
  /// | studios     | Music, Videos
  /// | tags        | Music, Videos
  /// | tvshows     | Videos
  /// | videos      | Videos
  /// | years       | Music, Videos
  ///
  void SetContainerContent(const std::string& value)
  {
    m_interface->kodi_gui->window->set_container_content(m_interface->kodiBase, m_controlHandle,
                                                         value.c_str());
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_window
  /// @brief Get the id of the currently visible container.
  ///
  /// @return currently visible container id
  ///
  int GetCurrentContainerId()
  {
    return m_interface->kodi_gui->window->get_current_container_id(m_interface->kodiBase,
                                                                   m_controlHandle);
  }
  //----------------------------------------------------------------------------
  /// @}

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_window
  /// @brief To inform Kodi that it need to render region new.
  ///
  void MarkDirtyRegion()
  {
    return m_interface->kodi_gui->window->mark_dirty_region(m_interface->kodiBase, m_controlHandle);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @defgroup cpp_kodi_gui_windows_window_callbacks Callback functions from Kodi to add-on
  /// @ingroup cpp_kodi_gui_windows_window
  /// @{
  /// @brief <b>GUI window callback functions.</b>\n
  /// Functions to handle control callbacks from Kodi
  ///
  /// ------------------------------------------------------------------------
  ///
  /// @link cpp_kodi_gui_windows_window Go back to normal functions from CWindow@endlink
  //

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_window_callbacks
  /// @brief OnInit method.
  ///
  /// @return Return true if initialize was done successful
  ///
  ///
  virtual bool OnInit() { return false; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_window_callbacks
  /// @brief OnFocus method.
  ///
  /// @param[in] controlId GUI control identifier
  /// @return Return true if focus condition was handled there or false to handle
  ///         them by Kodi itself
  ///
  ///
  virtual bool OnFocus(int controlId) { return false; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_window_callbacks
  /// @brief OnClick method.
  ///
  /// @param[in] controlId GUI control identifier
  /// @return Return true if click was handled there or false to handle them by
  ///         Kodi itself
  ///
  ///
  virtual bool OnClick(int controlId) { return false; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_window_callbacks
  /// @brief OnAction method.
  ///
  /// @param[in] actionId             The action id to perform, see
  ///                                 @ref kodi_key_action_ids to get list of
  ///                                 them
  /// @return Return true if action was handled there
  ///                                 or false to handle them by Kodi itself
  ///
  ///
  /// This method will receive all actions that the main program will send
  /// to this window.
  ///
  /// @note
  /// - By default, only the @c ADDON_ACTION_PREVIOUS_MENU and @c ADDON_ACTION_NAV_BACK actions are handled.
  /// - Overwrite this method to let your code handle all actions.
  /// - Don't forget to capture @ref ADDON_ACTION_PREVIOUS_MENU or @ref ADDON_ACTION_NAV_BACK, else the user can't close this window.
  ///
  ///
  ///----------------------------------------------------------------------------
  ///
  /// **Example:**
  /// ~~~~~~~~~~~~~{.cpp}
  /// ..
  /// // Window used with parent / child way
  /// bool cYOUR_CLASS::OnAction(ADDON_ACTION actionId)
  /// {
  ///   switch (action)
  ///   {
  ///     case ADDON_ACTION_PREVIOUS_MENU:
  ///     case ADDON_ACTION_NAV_BACK:
  ///       printf("action received: previous");
  ///       Close();
  ///       return true;
  ///     case ADDON_ACTION_SHOW_INFO:
  ///       printf("action received: show info");
  ///       break;
  ///     case ADDON_ACTION_STOP:
  ///       printf("action received: stop");
  ///       break;
  ///     case ADDON_ACTION_PAUSE:
  ///       printf("action received: pause");
  ///       break;
  ///     default:
  ///       break;
  ///   }
  ///   return false;
  /// }
  /// ..
  /// ~~~~~~~~~~~~~
  ///
  virtual bool OnAction(ADDON_ACTION actionId)
  {
    switch (actionId)
    {
      case ADDON_ACTION_PREVIOUS_MENU:
      case ADDON_ACTION_NAV_BACK:
        Close();
        return true;
      default:
        break;
    }
    return false;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_window_callbacks
  /// @brief Get context menu buttons for list entry.
  ///
  /// @param[in] itemNumber Selected list item entry
  /// @param[in] buttons List where context menus becomes added with his
  ///                    identifier and name
  ///
  virtual void GetContextButtons(int itemNumber,
                                 std::vector<std::pair<unsigned int, std::string>>& buttons)
  {
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_window_callbacks
  /// @brief Called after selection in context menu.
  ///
  /// @param[in] itemNumber Selected list item entry
  /// @param[in] button The pressed button id
  /// @return true if handled, otherwise false
  ///
  virtual bool OnContextButton(int itemNumber, unsigned int button) { return false; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_gui_windows_window_callbacks
  /// @brief **Set independent callbacks**
  ///
  /// If the class is used independent (with "new CWindow") and
  /// not as parent (with \"cCLASS_own : public @ref cpp_kodi_gui_windows_window "kodi::gui::CWindow"\") from own must be the
  /// callback from Kodi to add-on overdriven with own functions!
  ///
  /// @param[in] cbhdl The pointer to own handle data structure / class
  /// @param[in] CBOnInit Own defined window init function
  /// @param[in] CBOnFocus Own defined focus function
  /// @param[in] CBOnClick Own defined click function
  /// @param[in] CBOnAction Own defined action function
  /// @param[in] CBGetContextButtons [opt] To get context menu entries for
  ///                                lists function
  /// @param[in] CBOnContextButton [opt] Used context menu entry function
  ///
  ///
  ///----------------------------------------------------------------------------
  ///
  /// **Example:**
  /// ~~~~~~~~~~~~~{.cpp}
  /// ...
  ///
  /// bool OnInit(kodi::gui::ClientHandle cbhdl)
  /// {
  ///   ...
  ///   return true;
  /// }
  ///
  /// bool OnFocus(kodi::gui::ClientHandle cbhdl, int controlId)
  /// {
  ///   ...
  ///   return true;
  /// }
  ///
  /// bool OnClick(kodi::gui::ClientHandle cbhdl, int controlId)
  /// {
  ///   ...
  ///   return true;
  /// }
  ///
  /// bool OnAction(kodi::gui::ClientHandle cbhdl, ADDON_ACTION actionId)
  /// {
  ///   ...
  ///   return true;
  /// }
  ///
  /// ...
  /// // Somewhere where you create the window
  /// CWindow myWindow = new CWindow;
  /// myWindow->SetIndependentCallbacks(myWindow, OnInit, OnFocus, OnClick, OnAction);
  /// ...
  /// ~~~~~~~~~~~~~
  ///
  void SetIndependentCallbacks(kodi::gui::ClientHandle cbhdl,
                               bool (*CBOnInit)(kodi::gui::ClientHandle cbhdl),
                               bool (*CBOnFocus)(kodi::gui::ClientHandle cbhdl, int controlId),
                               bool (*CBOnClick)(kodi::gui::ClientHandle cbhdl, int controlId),
                               bool (*CBOnAction)(kodi::gui::ClientHandle cbhdl,
                                                  ADDON_ACTION actionId),
                               void (*CBGetContextButtons)(kodi::gui::ClientHandle cbhdl,
                                                           int itemNumber,
                                                           gui_context_menu_pair* buttons,
                                                           unsigned int* size) = nullptr,
                               bool (*CBOnContextButton)(kodi::gui::ClientHandle cbhdl,
                                                         int itemNumber,
                                                         unsigned int button) = nullptr)
  {
    if (!cbhdl || !CBOnInit || !CBOnFocus || !CBOnClick || !CBOnAction)
    {
      kodi::Log(ADDON_LOG_FATAL, "kodi::gui::CWindow::%s called with nullptr !!!", __FUNCTION__);
      return;
    }

    m_interface->kodi_gui->window->set_callbacks(m_interface->kodiBase, m_controlHandle, cbhdl,
                                                 CBOnInit, CBOnFocus, CBOnClick, CBOnAction,
                                                 CBGetContextButtons, CBOnContextButton);
  }
  //----------------------------------------------------------------------------
  /// @}

private:
  static bool CBOnInit(KODI_GUI_CLIENT_HANDLE cbhdl)
  {
    return static_cast<CWindow*>(cbhdl)->OnInit();
  }

  static bool CBOnFocus(KODI_GUI_CLIENT_HANDLE cbhdl, int controlId)
  {
    return static_cast<CWindow*>(cbhdl)->OnFocus(controlId);
  }

  static bool CBOnClick(KODI_GUI_CLIENT_HANDLE cbhdl, int controlId)
  {
    return static_cast<CWindow*>(cbhdl)->OnClick(controlId);
  }

  static bool CBOnAction(KODI_GUI_CLIENT_HANDLE cbhdl, ADDON_ACTION actionId)
  {
    return static_cast<CWindow*>(cbhdl)->OnAction(actionId);
  }

  static void CBGetContextButtons(KODI_GUI_CLIENT_HANDLE cbhdl,
                                  int itemNumber,
                                  gui_context_menu_pair* buttons,
                                  unsigned int* size)
  {
    std::vector<std::pair<unsigned int, std::string>> buttonList;
    static_cast<CWindow*>(cbhdl)->GetContextButtons(itemNumber, buttonList);
    if (!buttonList.empty())
    {
      unsigned int presentSize = static_cast<unsigned int>(buttonList.size());
      if (presentSize > *size)
        kodi::Log(ADDON_LOG_WARNING, "GetContextButtons: More as allowed '%i' entries present!",
                  *size);
      else
        *size = presentSize;
      for (unsigned int i = 0; i < *size; ++i)
      {
        buttons[i].id = buttonList[i].first;
        strncpy(buttons[i].name, buttonList[i].second.c_str(), ADDON_MAX_CONTEXT_ENTRY_NAME_LENGTH);
      }
    }
  }

  static bool CBOnContextButton(KODI_GUI_CLIENT_HANDLE cbhdl, int itemNumber, unsigned int button)
  {
    return static_cast<CWindow*>(cbhdl)->OnContextButton(itemNumber, button);
  }
};

} /* namespace gui */
} /* namespace kodi */

#endif /* __cplusplus */
