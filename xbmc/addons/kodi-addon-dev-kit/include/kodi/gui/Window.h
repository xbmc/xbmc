/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "../AddonBase.h"
#include "ListItem.h"

#ifdef BUILD_KODI_ADDON
#include "../ActionIDs.h"
#else
#include "input/actions/ActionIDs.h"
#endif

namespace kodi
{
namespace gui
{

  class CListItem;

  //============================================================================
  ///
  /// \defgroup cpp_kodi_gui_CWindow Window
  /// \ingroup cpp_kodi_gui
  /// @brief \cpp_class{ kodi::gui::CWindow }
  /// **Main window control class**
  ///
  /// The with  \ref Window.h "#include <kodi/gui/Window.h>"
  /// included file brings support to create a window or dialog on Kodi.
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

  //============================================================================
  ///
  /// \defgroup cpp_kodi_gui_CWindow_Defs Definitions, structures and enumerators
  /// \ingroup cpp_kodi_gui_CWindow
  /// @brief <b>Library definition values</b>
  ///

  class CWindow : public CAddonGUIControlBase
  {
  public:
    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_CWindow
    /// @brief Class constructor with needed values for window / dialog.
    ///
    /// Creates a new Window class.
    ///
    /// @param[in] xmlFilename          XML file for the skin
    /// @param[in] defaultSkin          default skin to use if needed not available
    /// @param[in] asDialog             Use window as dialog if set
    /// @param[in] isMedia              [opt] bool - if False, create a regular window.
    ///                                 if True, create a mediawindow.
    ///                                 (default=false)
    ///                                 @note only usable for windows not for dialogs.
    ///
    ///
    CWindow(const std::string& xmlFilename, const std::string& defaultSkin, bool asDialog, bool isMedia = false)
      : CAddonGUIControlBase(nullptr)
    {
      m_controlHandle = m_interface->kodi_gui->window->create(m_interface->kodiBase, xmlFilename.c_str(),
                                                              defaultSkin.c_str(), asDialog, isMedia);
      if (!m_controlHandle)
        kodi::Log(ADDON_LOG_FATAL, "kodi::gui::CWindow can't create window class from Kodi !!!");
      m_interface->kodi_gui->window->set_callbacks(m_interface->kodiBase, m_controlHandle, this,
                                                   CBOnInit, CBOnFocus, CBOnClick, CBOnAction,
                                                   CBGetContextButtons, CBOnContextButton);
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup CWindow
    /// @brief Class destructor
    ///
    ///
    ///
    ~CWindow() override
    {
      if (m_controlHandle)
        m_interface->kodi_gui->window->destroy(m_interface->kodiBase, m_controlHandle);
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_CWindow
    /// @brief Show this window.
    ///
    /// Shows this window by activating it, calling close() after it wil activate the
    /// current window again.
    ///
    /// @note If your Add-On ends this window will be closed to. To show it forever,
    /// make a loop at the end of your Add-On or use doModal() instead.
    ///
    /// @warning If used must be the class be global present until Kodi becomes
    /// closed. The creation can be done after before "Show" becomes called, but
    /// not delete class after them.
    ///
    /// @return                         Return true if call and show is successed,
    ///                                 if false was something failed to get needed
    ///                                 skin parts.
    ///
    bool Show()
    {
      return m_interface->kodi_gui->window->show(m_interface->kodiBase, m_controlHandle);
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_CWindow
    /// @brief Closes this window.
    ///
    /// Closes this window by activating the old window.
    /// @note The window is not deleted with this method.
    ///
    void Close()
    {
      m_interface->kodi_gui->window->close(m_interface->kodiBase, m_controlHandle);
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_CWindow
    /// @brief Display this window until close() is called.
    ///
    void DoModal()
    {
      m_interface->kodi_gui->window->do_modal(m_interface->kodiBase, m_controlHandle);
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_CWindow
    /// @brief Gives the control with the supplied focus.
    ///
    /// @param[in] iControlId           On skin defined id of control
    /// @return                         Return true if call and focus is successed,
    ///                                 if false was something failed to get needed
    ///                                 skin parts.
    ///
    ///
    bool SetFocusId(int iControlId)
    {
      return m_interface->kodi_gui->window->set_focus_id(m_interface->kodiBase, m_controlHandle, iControlId);
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_CWindow
    /// @brief Returns the id of the control which is focused.
    ///
    /// @return                         Focused control id
    ///
    ///
    int GetFocusId()
    {
      return m_interface->kodi_gui->window->get_focus_id(m_interface->kodiBase, m_controlHandle);
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_CWindow
    /// @brief To set the used label on given control id
    ///
    /// @param[in] controlId            Control id where label need to set
    /// @param[in] label                Label to use
    ///
    ///
    void SetControlLabel(int controlId, const std::string& label)
    {
      m_interface->kodi_gui->window->set_control_label(m_interface->kodiBase, m_controlHandle, controlId, label.c_str());
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_CWindow
    /// @brief Sets a window property, similar to an infolabel.
    ///
    /// @param[in] key                  string - property name.
    /// @param[in] value                string or unicode - value of property.
    ///
    /// @note  Key is NOT case sensitive. Setting value to an empty string is
    ///        equivalent to clearProperty(key).\n
    ///        You can use the above as keywords for arguments and skip certain
    ///        optional arguments.\n
    ///        Once you use a keyword, all following arguments require the keyword.
    ///
    void SetProperty(const std::string& key, const std::string& value)
    {
      m_interface->kodi_gui->window->set_property(m_interface->kodiBase, m_controlHandle, key.c_str(), value.c_str());
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_CWindow
    /// @brief Returns a window property as a string, similar to an infolabel.
    ///
    /// @param[in] key                  string - property name.
    /// @return                         The property as strin (if present)
    ///
    /// @note  Key is NOT case sensitive. Setting value to an empty string is
    ///        equivalent to clearProperty(key).\n
    ///        You can use the above as keywords for arguments and skip certain
    ///        optional arguments.\n
    ///        Once you use a keyword, all following arguments require the keyword.
    ///
    ///
    std::string GetProperty(const std::string& key) const
    {
      std::string label;
      char* ret = m_interface->kodi_gui->window->get_property(m_interface->kodiBase, m_controlHandle, key.c_str());
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
    /// \ingroup cpp_kodi_gui_CWindow
    /// @brief Sets a window property with integer value
    ///
    /// @param[in] key                  string - property name.
    /// @param[in] value                integer value to set
    ///
    ///
    void SetPropertyInt(const std::string& key, int value)
    {
      m_interface->kodi_gui->window->set_property_int(m_interface->kodiBase, m_controlHandle, key.c_str(), value);
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_CWindow
    /// @brief Returns a window property with integer value
    ///
    /// @param[in] key                  string - property name.
    /// @return                         integer value of property
    ///
    int GetPropertyInt(const std::string& key) const
    {
      return m_interface->kodi_gui->window->get_property_int(m_interface->kodiBase, m_controlHandle, key.c_str());
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_CWindow
    /// @brief Sets a window property with boolean value
    ///
    /// @param[in] key                  string - property name.
    /// @param[in] value                boolean value to set
    ///
    ///
    void SetPropertyBool(const std::string& key, bool value)
    {
      m_interface->kodi_gui->window->set_property_bool(m_interface->kodiBase, m_controlHandle, key.c_str(), value);
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_CWindow
    /// @brief Returns a window property with boolean value
    ///
    /// @param[in] key                  string - property name.
    /// @return                         boolean value of property
    ///
    bool GetPropertyBool(const std::string& key) const
    {
      return m_interface->kodi_gui->window->get_property_bool(m_interface->kodiBase, m_controlHandle, key.c_str());
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_CWindow
    /// @brief Sets a window property with double value
    ///
    /// @param[in] key                  string - property name.
    /// @param[in] value                double value to set
    ///
    ///
    void SetPropertyDouble(const std::string& key, double value)
    {
      m_interface->kodi_gui->window->set_property_double(m_interface->kodiBase, m_controlHandle, key.c_str(), value);
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_CWindow
    /// @brief Returns a window property with double value
    ///
    /// @param[in] key                  string - property name.
    /// @return                         double value of property
    ///
    ///
    double GetPropertyDouble(const std::string& key) const
    {
      return m_interface->kodi_gui->window->get_property_double(m_interface->kodiBase, m_controlHandle, key.c_str());
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_CWindow
    /// @brief Remove all present properties from window
    ///
    ///
    ///
    void ClearProperties()
    {
      m_interface->kodi_gui->window->clear_properties(m_interface->kodiBase, m_controlHandle);
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_CWindow
    /// @brief Clears the specific window property.
    ///
    /// @param[in] key                string - property name.
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
      m_interface->kodi_gui->window->clear_property(m_interface->kodiBase, m_controlHandle, key.c_str());
    }
    //--------------------------------------------------------------------------

    //@{
    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_CWindow
    /// @brief Function delete all entries in integrated list.
    ///
    ///
    ///
    void ClearList()
    {
      m_interface->kodi_gui->window->clear_item_list(m_interface->kodiBase, m_controlHandle);
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_CWindow
    /// @brief To add a list item in the on window integrated list.
    ///
    /// @param[in] item                 List item to add
    /// @param[in] itemPosition         [opt] The position for item, default is on end
    ///
    ///
    void AddListItem(ListItemPtr item, int itemPosition = -1)
    {
      m_interface->kodi_gui->window->add_list_item(m_interface->kodiBase, m_controlHandle, item->m_controlHandle, itemPosition);
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_CWindow
    /// @brief To add a list item based upon string in the on window integrated list.
    ///
    /// @param[in] item                 List item to add
    /// @param[in] itemPosition         [opt] The position for item, default is on end
    ///
    ///
    void AddListItem(const std::string item, int itemPosition = -1)
    {
      m_interface->kodi_gui->window->add_list_item(m_interface->kodiBase, m_controlHandle, std::make_shared<kodi::gui::CListItem>(item)->m_controlHandle, itemPosition);
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_CWindow
    /// @brief Remove list item on position.
    ///
    /// @param[in] itemPosition         List position to remove
    ///
    ///
    void RemoveListItem(int itemPosition)
    {
      m_interface->kodi_gui->window->remove_list_item_from_position(m_interface->kodiBase, m_controlHandle, itemPosition);
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_CWindow
    /// @brief Remove item with given control class from list.
    ///
    /// @param[in] item                 List item control class to remove
    ///
    ///
    void RemoveListItem(ListItemPtr item)
    {
      m_interface->kodi_gui->window->remove_list_item(m_interface->kodiBase, m_controlHandle, item->m_controlHandle);
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_CWindow
    /// @brief To get list item control class on wanted position.
    ///
    /// @param[in] listPos              Position from where control is needed
    /// @return                         The list item control class or null if not found
    ///
    /// @warning Function returns a new generated **CListItem** class!
    ///
    ListItemPtr GetListItem(int listPos)
    {
      GUIHANDLE handle = m_interface->kodi_gui->window->get_list_item(m_interface->kodiBase, m_controlHandle, listPos);
      if (!handle)
        return ListItemPtr();

      return std::make_shared<kodi::gui::CListItem>(handle);
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_CWindow
    /// @brief To set position of selected part in list.
    ///
    /// @param[in] listPos              Position to use
    ///
    ///
    void SetCurrentListPosition(int listPos)
    {
      m_interface->kodi_gui->window->set_current_list_position(m_interface->kodiBase, m_controlHandle, listPos);
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_CWindow
    /// @brief To get current selected position in list
    ///
    /// @return                         Current list position
    ///
    ///
    int GetCurrentListPosition()
    {
      return m_interface->kodi_gui->window->get_current_list_position(m_interface->kodiBase, m_controlHandle);
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_CWindow
    /// @brief To get the amount of entries in the list.
    ///
    /// @return                         Size of in window integrated control class
    ///
    ///
    int GetListSize()
    {
      return m_interface->kodi_gui->window->get_list_size(m_interface->kodiBase, m_controlHandle);
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_CWindow
    /// @brief Sets a container property, similar to an infolabel.
    ///
    /// @param[in] key        string - property name.
    /// @param[in] value      string or unicode - value of property.
    ///
    /// @note Key is NOT case sensitive.\n
    /// You can use the above as keywords for arguments and skip certain
    /// optional arguments.\n
    /// Once you use a keyword, all following arguments require the keyword.
    ///
    ///
    void SetContainerProperty(const std::string& key, const std::string& value)
    {
      m_interface->kodi_gui->window->set_container_property(m_interface->kodiBase, m_controlHandle, key.c_str(), value.c_str());
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_CWindow
    /// @brief Sets the content type of the container.
    ///
    /// @param[in] value      string or unicode - content value.
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
    ///
    void SetContainerContent(const std::string& value)
    {
      m_interface->kodi_gui->window->set_container_content(m_interface->kodiBase, m_controlHandle, value.c_str());
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_CWindow
    /// @brief Get the id of the currently visible container.
    ///
    /// @return               currently visible container id
    ///
    ///
    int GetCurrentContainerId()
    {
      return m_interface->kodi_gui->window->get_current_container_id(m_interface->kodiBase, m_controlHandle);
    }
    //--------------------------------------------------------------------------
    //@}

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_CWindow
    /// @brief To inform Kodi that it need to render region new.
    ///
    ///
    void MarkDirtyRegion()
    {
      return m_interface->kodi_gui->window->mark_dirty_region(m_interface->kodiBase, m_controlHandle);
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    //
    /// @defgroup cpp_kodi_gui_CWindow_callbacks Callback functions from Kodi to add-on
    /// \ingroup cpp_kodi_gui_CWindow
    //@{
    /// @brief <b>GUI window callback functions.</b>
    ///
    /// Functions to handle control callbacks from Kodi
    ///
    /// ------------------------------------------------------------------------
    ///
    /// @link cpp_kodi_gui_CWindow Go back to normal functions from CWindow@endlink
    //

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_CWindow_callbacks
    /// @brief OnInit method.
    ///
    /// @return                         Return true if initialize was done successful
    ///
    ///
    virtual bool OnInit() { return false; }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_CWindow_callbacks
    /// @brief OnFocus method.
    ///
    /// @param[in] controlId            GUI control identifier
    /// @return                         Return true if focus condition was handled there or false to handle them by Kodi itself
    ///
    ///
    virtual bool OnFocus(int controlId) { return false; }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_CWindow_callbacks
    /// @brief OnClick method.
    ///
    /// @param[in] controlId            GUI control identifier
    /// @return                         Return true if click was handled there
    ///                                 or false to handle them by Kodi itself
    ///
    ///
    virtual bool OnClick(int controlId) { return false; }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_CWindow_callbacks
    /// @brief OnAction method.
    ///
    /// @param[in] actionId             The action id to perform, see
    ///                                 \ref kodi_key_action_ids to get list of
    ///                                 them
    /// @return                         Return true if action was handled there
    ///                                 or false to handle them by Kodi itself
    ///
    ///
    /// This method will receive all actions that the main program will send
    /// to this window.
    ///
    /// @note
    /// - By default, only the \c PREVIOUS_MENU and \c NAV_BACK actions are handled.
    /// - Overwrite this method to let your code handle all actions.
    /// - Don't forget to capture \c ACTION_PREVIOUS_MENU or \c ACTION_NAV_BACK, else the user can't close this window.
    ///
    ///
    ///--------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.cpp}
    /// ..
    /// /* Window used with parent / child way */
    /// bool cYOUR_CLASS::OnAction(int actionId)
    /// {
    ///   switch (action)
    ///   {
    ///     case ACTION_PREVIOUS_MENU:
    ///     case ACTION_NAV_BACK:
    ///       printf("action recieved: previous");
    ///       Close();
    ///       return true;
    ///     case ACTION_SHOW_INFO:
    ///       printf("action recieved: show info");
    ///       break;
    ///     case ACTION_STOP:
    ///       printf("action recieved: stop");
    ///       break;
    ///     case ACTION_PAUSE:
    ///       printf("action recieved: pause");
    ///       break;
    ///     default:
    ///       break;
    ///   }
    ///   return false;
    /// }
    /// ..
    /// ~~~~~~~~~~~~~
    ///
    virtual bool OnAction(int actionId)
    {
      switch (actionId)
      {
        case ACTION_PREVIOUS_MENU:
        case ACTION_NAV_BACK:
          Close();
          return true;
         default:
          break;
      }
      return false;
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_CWindow_callbacks
    /// @brief Get context menu buttons for list entry
    ///
    /// @param[in] itemNumber   selected list item entry
    /// @param[in] buttons      list where context menus becomes added with his
    ///                         identifier and name.
    ///
    virtual void GetContextButtons(int itemNumber, std::vector< std::pair<unsigned int, std::string> > &buttons)
    {
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_CWindow_callbacks
    /// @brief Called after selection in context menu
    ///
    /// @param[in] itemNumber   selected list item entry
    /// @param[in] button       the pressed button id
    /// @return                 true if handled, otherwise false
    ///
    virtual bool OnContextButton(int itemNumber, unsigned int button)
    {
      return false;
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// \ingroup cpp_kodi_gui_CWindow_callbacks
    /// @brief **Set independent callbacks**
    ///
    /// If the class is used independent (with "new CWindow") and
    /// not as parent (with "cCLASS_own : CWindow") from own must be the
    /// callback from Kodi to add-on overdriven with own functions!
    ///
    /// @param[in] cbhdl                The pointer to own handle data
    ///                                 structure / class
    /// @param[in] CBOnInit             Own defined window init function
    /// @param[in] CBOnFocus            Own defined focus function
    /// @param[in] CBOnClick            Own defined click function
    /// @param[in] CBOnAction           Own defined action function
    /// @param[in] CBGetContextButtons  [opt] To get context menu entries for
    ///                                 lists function
    /// @param[in] CBOnContextButton    [opt] Used context menu entry function
    ///
    ///
    ///--------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.cpp}
    /// ...
    ///
    /// bool OnInit(GUIHANDLE cbhdl)
    /// {
    ///   ...
    ///   return true;
    /// }
    ///
    /// bool OnFocus(GUIHANDLE cbhdl, int controlId)
    /// {
    ///   ...
    ///   return true;
    /// }
    ///
    /// bool OnClick(GUIHANDLE cbhdl, int controlId)
    /// {
    ///   ...
    ///   return true;
    /// }
    ///
    /// bool OnAction(GUIHANDLE cbhdl, int actionId)
    /// {
    ///   ...
    ///   return true;
    /// }
    ///
    /// ...
    /// /* Somewhere where you create the window */
    /// CWindow myWindow = new CWindow;
    /// myWindow->SetIndependentCallbacks(myWindow, OnInit, OnFocus, OnClick, OnAction);
    /// ...
    /// ~~~~~~~~~~~~~
    ///
    void SetIndependentCallbacks(
      GUIHANDLE                 cbhdl,
      bool (*CBOnInit)            (GUIHANDLE cbhdl),
      bool (*CBOnFocus)           (GUIHANDLE cbhdl, int controlId),
      bool (*CBOnClick)           (GUIHANDLE cbhdl, int controlId),
      bool (*CBOnAction)          (GUIHANDLE cbhdl, int actionId),
      void (*CBGetContextButtons) (GUIHANDLE cbhdl, int itemNumber, gui_context_menu_pair* buttons, unsigned int* size) = nullptr,
      bool (*CBOnContextButton)   (GUIHANDLE cbhdl, int itemNumber, unsigned int button) = nullptr)
    {
      if (!cbhdl ||
          !CBOnInit || !CBOnFocus || !CBOnClick || !CBOnAction)
      {
        kodi::Log(ADDON_LOG_FATAL, "kodi::gui::CWindow::%s called with nullptr !!!", __FUNCTION__);
        return;
      }

      m_interface->kodi_gui->window->set_callbacks(m_interface->kodiBase, m_controlHandle, cbhdl,
                                                   CBOnInit, CBOnFocus, CBOnClick, CBOnAction,
                                                   CBGetContextButtons, CBOnContextButton);
    }
    //--------------------------------------------------------------------------
    //@}

  private:
    static bool CBOnInit(GUIHANDLE cbhdl)
    {
      return static_cast<CWindow*>(cbhdl)->OnInit();
    }

    static bool CBOnFocus(GUIHANDLE cbhdl, int controlId)
    {
      return static_cast<CWindow*>(cbhdl)->OnFocus(controlId);
    }

    static bool CBOnClick(GUIHANDLE cbhdl, int controlId)
    {
      return static_cast<CWindow*>(cbhdl)->OnClick(controlId);
    }

    static bool CBOnAction(GUIHANDLE cbhdl, int actionId)
    {
      return static_cast<CWindow*>(cbhdl)->OnAction(actionId);
    }

    static void CBGetContextButtons(GUIHANDLE cbhdl, int itemNumber, gui_context_menu_pair* buttons, unsigned int* size)
    {
      std::vector< std::pair<unsigned int, std::string> > buttonList;
      static_cast<CWindow*>(cbhdl)->GetContextButtons(itemNumber, buttonList);
      if (!buttonList.empty())
      {
        unsigned int presentSize = static_cast<unsigned int>(buttonList.size());
        if (presentSize > *size)
          kodi::Log(ADDON_LOG_WARNING, "GetContextButtons: More as allowed '%i' entries present!", *size);
        else
          *size = presentSize;
        for (unsigned int i = 0; i < *size; ++i)
        {
          buttons[i].id = buttonList[i].first;
          strncpy(buttons[i].name, buttonList[i].second.c_str(), ADDON_MAX_CONTEXT_ENTRY_NAME_LENGTH);
        }
      }
    }

    static bool CBOnContextButton(GUIHANDLE cbhdl, int itemNumber, unsigned int button)
    {
      return static_cast<CWindow*>(cbhdl)->OnContextButton(itemNumber, button);
    }
  };

} /* namespace gui */
} /* namespace kodi */
