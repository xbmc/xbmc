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
#include "ListItem.h"

#ifdef BUILD_KODI_ADDON
#include "../ActionIDs.h"
#else
#include "input/ActionIDs.h"
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
    virtual ~CWindow()
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
        unsigned int presentSize = buttonList.size();
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
