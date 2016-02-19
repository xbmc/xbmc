/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once
#include <limits.h>

#include "AddonCallback.h"
#include "Control.h"
#include "AddonString.h"

#include "swighelper.h"

namespace XBMCAddon
{
  namespace xbmcgui
  {
    // Forward declare the interceptor as the AddonWindowInterceptor.h
    // file needs to include the Window class because of the template
    class InterceptorBase;

    //
    /// \defgroup python_xbmcgui_action Action
    /// \ingroup python_xbmcgui
    ///@{
    /// @brief __Action class.__
    ///
    /// <b><c>xbmcgui.Action():</c></b>
    ///
    /// This class serves in addition to identify carried out
    /// \ref kodi_key_action_ids of Kodi and to be able to carry out thereby own
    /// necessary steps.
    ///
    /// The data of this class are always transmitted by callback
    /// Window::onAction at a window.
    ///
    class Action : public AddonClass
    {
    public:
      Action() : id(-1), fAmount1(0.0f), fAmount2(0.0f),
                 fRepeat(0.0f), buttonCode(0), strAction("")
      { }

#ifndef SWIG
      Action(const CAction& caction) { setFromCAction(caction); }

      void setFromCAction(const CAction& caction);

      long id;
      float fAmount1;
      float fAmount2;
      float fRepeat;
      unsigned long buttonCode;
      std::string strAction;

      // Not sure if this is correct but it's here for now.
      AddonClass::Ref<Control> control; // previously pObject
#endif

      ///
      /// \ingroup python_xbmcgui_action
      /// @brief To get \ref kodi_key_action_ids
      ///
      /// This function returns the identification code used by the explained
      /// order, it is necessary to determine the type of command from
      /// \ref kodi_key_action_ids.
      ///
      /// @return                      The action's current id as a long or 0 if
      ///                              no action is mapped in the xml's.
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// def onAction(self, action):
      ///   if action.getId() == ACTION_PREVIOUS_MENU:
      ///     print('action recieved: previous')
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      long getId() { XBMC_TRACE; return id; }

      ///
      /// \ingroup python_xbmcgui_action
      /// @brief Returns the button code for this action.
      ///
      long getButtonCode() { XBMC_TRACE; return buttonCode; }

      ///
      /// \ingroup python_xbmcgui_action
      /// @brief Returns the first amount of force applied to the thumbstick.
      ///
      float getAmount1() { XBMC_TRACE; return fAmount1; }

      ///
      /// \ingroup python_xbmcgui_action
      /// @brief Returns the second amount of force applied to the thumbstick.
      ///
      float getAmount2() { XBMC_TRACE; return fAmount2; }
    };
    ///@}

    //==========================================================================
    // This is the main class for the xbmcgui.Window functionality. It is tied
    //  into the main Kodi windowing system via the Interceptor
    //==========================================================================
    //
    /// \defgroup python_xbmcgui_window Window
    /// \ingroup python_xbmcgui
    /// @{
    /// @brief __GUI window class for Add-Ons.__
    ///
    /// This class allows over their functions to create and edit windows that
    /// can be accessed from an Add-On.
    ///
    /// Likewise, all functions from here as well in the other window classes
    /// WindowDialog, WindowXML and WindowXMLDialog with inserted and
    /// available.
    ///
    ///
    ///--------------------------------------------------------------------------
    /// Class constructor for window
    /// ----------------------------
    ///
    /// <b><c>xbmcgui.Window(int windowId):</c></b>
    ///
    /// Creates a new from Add-On usable window class. This is to create
    /// window for related controls by system calls.
    ///
    /// @param[in] existingWindowId   [opt] Specify an id to use an existing
    ///                               window.
    /// @throws ValueError            if supplied window Id does not exist.
    /// @throws Exception             if more then 200 windows are created.
    ///
    /// Deleting this window will activate the old window that was active
    /// and resets (not delete) all controls that are associated with this
    /// window.
    ///
    ///
    ///--------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ..
    /// win = xbmcgui.Window()
    /// width = win.getWidth()
    /// ..
    /// ~~~~~~~~~~~~~
    ///
    ///
    //
    class Window : public AddonCallback
    {
      friend class WindowDialogMixin;
      bool isDisposed;

      void doAddControl(Control* pControl, CCriticalSection* gcontext, bool wait);
      void doRemoveControl(Control* pControl, CCriticalSection* gcontext, bool wait);

    protected:
#ifndef SWIG
      InterceptorBase* window;
      int iWindowId;

      std::vector<AddonClass::Ref<Control> > vecControls;
      int iOldWindowId;
      int iCurrentControlId;
      bool bModal;
      CEvent m_actionEvent;

      bool canPulse;

      // I REALLY hate this ... but it's the simplest fix right now.
      bool existingWindow;
      bool destroyAfterDeInit;

      /**
       * This only takes a boolean to allow subclasses to explicitly use it. A
       * default constructor can be used as a concrete class and we need to tell
       * the difference.
       * subclasses should use this constructor and not the other.
       */
      Window(bool discrim);

      virtual void deallocating();

      /**
       * This helper retrieves the next available id. It is assumed that the
       * global lock is already being held.
       */
      static int getNextAvailableWindowId();

      /**
       * Child classes MUST call this in their constructors. It should be an
       * instance of Interceptor<P extends CGUIWindow>. Control of memory
       * management for this class is then given to the Window.
       */
      void setWindow(InterceptorBase* _window);

      /**
       * This is a helper method since poping the previous window id is a common
       * function.
       */
      void popActiveWindowId();

      /**
       * This is a helper method since getting a control by it's id is a common
       * function.
       */
      Control* GetControlById(int iControlId, CCriticalSection* gc);

      SWIGHIDDENVIRTUAL void PulseActionEvent();
      SWIGHIDDENVIRTUAL bool WaitForActionEvent(unsigned int milliseconds);
#endif

    public:
      Window(int existingWindowId = -1);

      virtual ~Window();

#ifndef SWIG
      SWIGHIDDENVIRTUAL bool    OnMessage(CGUIMessage& message);
      SWIGHIDDENVIRTUAL bool    OnAction(const CAction &action);
      SWIGHIDDENVIRTUAL bool    OnBack(int actionId);
      SWIGHIDDENVIRTUAL void    OnDeinitWindow(int nextWindowID);

      SWIGHIDDENVIRTUAL bool    IsDialogRunning() const { XBMC_TRACE; return false; };
      SWIGHIDDENVIRTUAL bool    IsDialog() const { XBMC_TRACE; return false; };
      SWIGHIDDENVIRTUAL bool    IsModalDialog() const { XBMC_TRACE; return false; };
      SWIGHIDDENVIRTUAL bool    IsMediaWindow() const { XBMC_TRACE; return false; };
      SWIGHIDDENVIRTUAL void    dispose();

      /**
       * This is called from the InterceptorBase destructor to prevent further
       * use of the interceptor from the window.
       */
      inline void interceptorClear() { CSingleLock lock(*this); window = NULL; }
#endif

      //
      /// @defgroup python_xbmcgui_window_cb Callback functions from Kodi to add-on
      /// \ingroup python_xbmcgui_window
      /// @{
      /// @brief __GUI window callback functions.__
      ///
      /// Functions to handle control callbacks from Kodi.
      ///
      /// Likewise, all functions from here as well in the all window classes
      /// (\ref python_xbmcgui_window "Window",
      /// \ref python_xbmcgui_window_dialog "WindowDialog",
      /// \ref python_xbmcgui_window_xml "WindowXML" and
      /// \ref python_xbmcgui_window_dialog_xml "WindowXMLDialog") with inserted
      /// and available.
      ///
      /// ------------------------------------------------------------------------
      ///
      /// @link python_xbmcgui_window Go back to normal functions from window@endlink
      //

      // callback takes a parameter
      ///
      /// \ingroup python_xbmcgui_window_cb
      /// @brief __onAction method.__
      ///
      /// This method will receive all actions that the main program will send
      /// to this window.
      ///
      /// @param[in] self                Own base class pointer
      /// @param[in] action              The action id to perform, see
      ///                                \ref python_xbmcgui_action to get use
      ///                                of them
      ///
      /// @note
      /// - By default, only the `PREVIOUS_MENU` and `NAV_BACK actions` are
      ///   handled.
      /// - Overwrite this method to let your script handle all actions.
      /// - Don't forget to capture `ACTION_PREVIOUS_MENU` or `ACTION_NAV_BACK`,
      ///   else the user can't close this window.
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// # Define own function where becomes called from Kodi
      /// def onAction(self, action):
      ///   if action.getId() == ACTION_PREVIOUS_MENU:
      ///     print('action recieved: previous')
      ///     self.close()
      ///   if action.getId() == ACTION_SHOW_INFO:
      ///     print('action recieved: show info')
      ///   if action.getId() == ACTION_STOP:
      ///     print('action recieved: stop')
      ///   if action.getId() == ACTION_PAUSE:
      ///     print('action recieved: pause')
      /// ..
      /// ~~~~~~~~~~~~~
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      virtual void onAction(Action* action);
#else
      void onAction(void* self, Action* action); // Python function style as doxygen part (not used to build!)
#endif /* DOXYGEN_SHOULD_SKIP_THIS */

      // on control is not actually on Window in the api but is called into Python anyway.
      ///
      /// \ingroup python_xbmcgui_window_cb
      /// @brief __onControl method.__
      ///
      /// This method will receive all click events on owned and selected
      /// controls when the control itself doesn't handle the message.
      ///
      /// @param[in] self                Own base class pointer
      /// @param[in] control             The Control class
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// # Define own funtion where becomes called from Kodi
      /// def onControl(self, control):
      ///   print("Window.onControl(control=[%s])"%control)
      /// ..
      /// ~~~~~~~~~~~~~
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      virtual void onControl(Control* control);
#else
      void onControl(void* self, Control* control); // Python function style as doxygen part (not used to build!)
#endif /* DOXYGEN_SHOULD_SKIP_THIS */

      ///
      /// \ingroup python_xbmcgui_window_cb
      /// @brief __onClick method.__
      ///
      /// This method will receive all click events that the main program will
      /// send to this window.
      ///
      /// @param[in] self                Own base class pointer
      /// @param[in] controlId           The one time clicked GUI control
      ///                                identifier
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// # Define own funtion where becomes called from Kodi
      ///  def onClick(self,controlId):
      ///    if controlId == 10:
      ///      print("The control with Id 10 is clicked")
      /// ..
      /// ~~~~~~~~~~~~~
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      virtual void onClick(int controlId);
#else
      void onClick(void* self, int controlId); // Python function style as doxygen part (not used to build!)
#endif /* DOXYGEN_SHOULD_SKIP_THIS */

      ///
      /// \ingroup python_xbmcgui_window_cb
      /// @brief __onDoubleClick method.__
      ///
      /// This method will receive all double click events that the main program
      /// will send to this window.
      ///
      /// @param[in] self                Own base class pointer
      /// @param[in] controlId           The double clicked GUI control
      ///                                identifier
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// # Define own funtion where becomes called from Kodi
      /// def onDoubleClick(self,controlId):
      ///   if controlId == 10:
      ///     print("The control with Id 10 is double clicked")
      /// ..
      /// ~~~~~~~~~~~~~
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      virtual void onDoubleClick(int controlId);
#else
      void onDoubleClick(void* self, int controlId); // Python function style as doxygen part (not used to build!)
#endif /* DOXYGEN_SHOULD_SKIP_THIS */

      ///
      /// \ingroup python_xbmcgui_window_cb
      /// @brief __onFocus method.__
      ///
      /// This method will receive all focus events that the main program will
      /// send to this window.
      ///
      /// @param[in] self                Own base class pointer
      /// @param[in] controlId           The focused GUI control identifier
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// # Define own funtion where becomes called from Kodi
      ///  def onDoubleClick(self,controlId):
      ///    if controlId == 10:
      ///      print("The control with Id 10 is focused")
      /// ..
      /// ~~~~~~~~~~~~~
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      virtual void onFocus(int controlId);
#else
      void onFocus(void* self, int controlId); // Python function style as doxygen part (not used to build!)
#endif /* DOXYGEN_SHOULD_SKIP_THIS */

      ///
      /// \ingroup python_xbmcgui_window_cb
      /// @brief __onInit method.__
      ///
      /// This method will be called to initialize the window
      ///
      /// @param[in] self                Own base class pointer
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// # Define own funtion where becomes called from Kodi
      /// def onInit(self):
      ///   print("Window.onInit method called from Kodi")
      /// ..
      /// ~~~~~~~~~~~~~
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      virtual void onInit();
#else
      void onInit(void* self); // Python function style as doxygen part (not used to build!)
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      //@}

      ///
      /// \ingroup python_xbmcgui_window
      /// @brief Show this window.
      ///
      /// Shows this window by activating it, calling close() after it wil
      /// activate the current window again.
      ///
      /// @note If your script ends this window will be closed to. To show it
      /// forever, make a loop at the end of your script or use doModal()
      /// instead.
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      SWIGHIDDENVIRTUAL
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      void show();

      ///
      /// \ingroup python_xbmcgui_window
      /// @brief Give the supplied control focus.
      ///
      /// @param[in] pControl            Control class to focus
      /// @throws TypeError              If supplied argument is not a Control
      ///                                type
      /// @throws SystemError            On Internal error
      /// @throws RuntimeError           If control is not added to a window
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      SWIGHIDDENVIRTUAL
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      void setFocus(Control* pControl);

      ///
      /// \ingroup python_xbmcgui_window
      /// Gives the control with the supplied focus.
      ///
      /// @param[in] iControlId          On skin defined id of control
      /// @throws SystemError            On Internal error
      /// @throws RuntimeError           If control is not added to a window
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      SWIGHIDDENVIRTUAL
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      void setFocusId(int iControlId);

      ///
      /// \ingroup python_xbmcgui_window
      /// Returns the control which is focused.
      ///
      /// @return                        Focused control class
      /// @throws SystemError            On Internal error
      /// @throws RuntimeError           If no control has focus
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      SWIGHIDDENVIRTUAL
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      Control* getFocus();

      ///
      /// \ingroup python_xbmcgui_window
      /// @brief Returns the id of the control which is focused.
      ///
      /// @return                        Focused control id
      /// @throws SystemError            On Internal error
      /// @throws RuntimeError           If no control has focus
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      SWIGHIDDENVIRTUAL
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      long getFocusId();

      ///
      /// \ingroup python_xbmcgui_window
      /// @brief Removes the control from this window.
      ///
      /// @param[in] pControl            Control class to remove
      /// @throws TypeError              If supplied argument is not a Control
      ///                                type
      /// @throws RuntimeError           If control is not added to this window
      ///
      /// This will not delete the control. It is only removed from the window.
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      SWIGHIDDENVIRTUAL
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      void removeControl(Control* pControl);

      ///
      /// \ingroup python_xbmcgui_window
      /// @brief Removes a list of controls from this window.
      ///
      /// @param[in] pControls          List with controls to remove
      /// @throws TypeError             If supplied argument is not a Control
      ///                               type
      /// @throws RuntimeError          If control is not added to this window
      ///
      /// This will not delete the controls. They are only removed from the
      /// window.
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      SWIGHIDDENVIRTUAL
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      void removeControls(std::vector<Control*> pControls);

      ///
      /// \ingroup python_xbmcgui_window
      /// @brief Returns the height of this screen.
      ///
      /// @return                       Screen height
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      SWIGHIDDENVIRTUAL
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      long getHeight();

      ///
      /// \ingroup python_xbmcgui_window
      /// @brief Returns the width of this screen.
      ///
      /// @return                       Screen width
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      SWIGHIDDENVIRTUAL
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      long getWidth();

      ///
      /// \ingroup python_xbmcgui_window
      /// @briefReturns The resolution of the screen
      ///
      /// @return                       Used Resolution
      ///  The returned value is one of the following:
      ///  | value | Resolution                |
      ///  |:-----:|:--------------------------|
      ///  |   0   | 1080i      (1920x1080)
      ///  |   1   | 720p       (1280x720)
      ///  |   2   | 480p 4:3   (720x480)
      ///  |   3   | 480p 16:9  (720x480)
      ///  |   4   | NTSC 4:3   (720x480)
      ///  |   5   | NTSC 16:9  (720x480)
      ///  |   6   | PAL 4:3    (720x576)
      ///  |   7   | PAL 16:9   (720x576)
      ///  |   8   | PAL60 4:3  (720x480)
      ///  |   9   | PAL60 16:9 (720x480)
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      SWIGHIDDENVIRTUAL
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      long getResolution();

      ///
      /// \ingroup python_xbmcgui_window
      /// @brief Sets the resolution
      ///
      /// That the coordinates of all controls are defined in.  Allows Kodi
      /// to scale control positions and width/heights to whatever resolution
      /// Kodi is currently using.
      ///
      /// @param[in] res                Coordinate resolution to set
      ///  Resolution is one of the following:
      ///  | value | Resolution                |
      ///  |:-----:|:--------------------------|
      ///  |   0   | 1080i      (1920x1080)
      ///  |   1   | 720p       (1280x720)
      ///  |   2   | 480p 4:3   (720x480)
      ///  |   3   | 480p 16:9  (720x480)
      ///  |   4   | NTSC 4:3   (720x480)
      ///  |   5   | NTSC 16:9  (720x480)
      ///  |   6   | PAL 4:3    (720x576)
      ///  |   7   | PAL 16:9   (720x576)
      ///  |   8   | PAL60 4:3  (720x480)
      ///  |   9   | PAL60 16:9 (720x480)
      ///
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// win = xbmcgui.Window(xbmcgui.getCurrentWindowId())
      /// win.setCoordinateResolution(0)
      /// ..
      /// ~~~~~~~~~~~~~
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      SWIGHIDDENVIRTUAL
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      void setCoordinateResolution(long res);

      ///
      /// \ingroup python_xbmcgui_window
      /// @brief Sets a window property, similar to an infolabel.
      ///
      /// @param[in] key                 string - property name.
      /// @param[in] value               string or unicode - value of property.
      ///
      /// @note  Key is NOT case sensitive. Setting value to an empty string is
      ///        equivalent to clearProperty(key).\n
      ///        You can use the above as keywords for arguments and skip
      ///        certain optional arguments.\n
      ///        Once you use a keyword, all following arguments require the
      ///        keyword.
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// win = xbmcgui.Window(xbmcgui.getCurrentWindowId())
      /// win.setProperty('Category', 'Newest')
      /// ..
      /// ~~~~~~~~~~~~~
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      SWIGHIDDENVIRTUAL
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      void setProperty(const char* key, const String& value);

      ///
      /// \ingroup python_xbmcgui_window
      /// @brief Returns a window property as a string, similar to an infolabel.
      ///
      /// @param[in] key                string - property name.
      ///
      /// @note  Key is NOT case sensitive.\n
      ///        You can use the above as keywords for arguments and skip
      ///        certain optional arguments.
      ///        Once you use a keyword, all following arguments require the
      ///        keyword.
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// win = xbmcgui.Window(xbmcgui.getCurrentWindowId())
      /// category = win.getProperty('Category')
      /// ..
      /// ~~~~~~~~~~~~~
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      SWIGHIDDENVIRTUAL
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      String getProperty(const char* key);

      ///
      /// \ingroup python_xbmcgui_window
      /// @brief Clears the specific window property.
      ///
      /// @param[in] key                string - property name.
      ///
      /// @note Key is NOT case sensitive. Equivalent to setProperty(key,'')
      ///       You can use the above as keywords for arguments and skip certain
      ///       optional arguments.
      ///       Once you use a keyword, all following arguments require the
      ///       keyword.
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// win = xbmcgui.Window(xbmcgui.getCurrentWindowId())
      /// win.clearProperty('Category')
      /// ..
      /// ~~~~~~~~~~~~~
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      SWIGHIDDENVIRTUAL
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      void clearProperty(const char* key);

      ///
      /// \ingroup python_xbmcgui_window
      /// @brief Clears all window properties.
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// win = xbmcgui.Window(xbmcgui.getCurrentWindowId())
      /// win.clearProperties()
      /// ..
      /// ~~~~~~~~~~~~~
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      SWIGHIDDENVIRTUAL
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      void clearProperties();

      ///
      /// \ingroup python_xbmcgui_window
      /// @brief Closes this window.
      ///
      /// Closes this window by activating the old window.
      ///
      /// @note The window is not deleted with this method.
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      SWIGHIDDENVIRTUAL
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      void close();

      ///
      /// \ingroup python_xbmcgui_window
      /// @brief Display this window until close() is called.
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      SWIGHIDDENVIRTUAL
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      void doModal();

      ///
      /// \ingroup python_xbmcgui_window
      /// @brief Add a Control to this window.
      ///
      /// @param[in] pControl           Control to add
      /// @throws TypeError             If supplied argument is not a Control
      ///                               type
      /// @throws ReferenceError        If control is already used in another
      ///                               window
      /// @throws RuntimeError          Should not happen :-)
      ///
      /// The next controls can be added to a window atm
      ///   | Control class       | Description                                                |
      ///   |---------------------|------------------------------------------------------------|
      ///   | \ref python_xbmcgui_control_label       "ControlLabel"        | Label control to show text
      ///   | \ref python_xbmcgui_control_fadelabel   "ControlFadeLabel"    | The fadelabel has multiple labels which it cycles through.
      ///   | \ref python_xbmcgui_control_textbox     "ControlTextBox"      | To show bigger text field
      ///   | \ref python_xbmcgui_control_button      "ControlButton"       | Brings a button to do some actions
      ///   | \ref python_xbmcgui_control_list        "ControlList"         | Add a list for something like files
      ///   | \ref python_xbmcgui_control_group       "ControlGroup"        | Is for a group which brings the others together
      ///   | \ref python_xbmcgui_control_image       "ControlImage"        | Controls a image on skin
      ///   | \ref python_xbmcgui_control_radiobutton "ControlRadioButton"  | For a radio button which handle boolean values
      ///   | \ref python_xbmcgui_control_progress    "ControlProgress"     | Progress bar for a performed work or something else
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      SWIGHIDDENVIRTUAL
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      void addControl(Control* pControl);

      ///
      /// \ingroup python_xbmcgui_window
      /// @brief Add a list of Controls to this window.
      ///
      /// @param[in] pControls          List with controls to add
      /// @throws TypeError             If supplied argument is not of List
      ///                               type, or a control is not of Control
      ///                               type
      /// @throws ReferenceError        If control is already used in another
      ///                               window
      /// @throws RuntimeError          Should not happen :-)
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      SWIGHIDDENVIRTUAL
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      void addControls(std::vector<Control*> pControls);

      ///
      /// \ingroup python_xbmcgui_window
      /// @brief Gets the control from this window.
      ///
      /// @param[in] iControlId         Control id to get
      /// @throws Exception             If Control doesn't exist
      ///
      /// @remark controlId doesn't have to be a python control, it can be a
      /// control id from a Kodi window too (you can find id's in the xml files.
      ///
      /// @note Not python controls are not completely usable yet
      /// You can only use the Control functions
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      SWIGHIDDENVIRTUAL
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      Control* getControl(int iControlId);
    };
    ///@}
  }
}
