 /*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#pragma once

#include <limits.h>

#include "WindowException.h"
#include "AddonCallback.h"
#include "Exception.h"
#include "Control.h"
#include "AddonString.h"

#include "swighelper.h"

#include "guilib/GUIWindow.h"

namespace XBMCAddon
{
  namespace xbmcgui
  {
    // Forward declare the interceptor as the AddonWindowInterceptor.h 
    // file needs to include the Window class because of the template
    class InterceptorBase;

    class Action : public AddonClass
    {
    public:
      Action() : AddonClass("Action"), id(-1), fAmount1(0.0f), fAmount2(0.0f), 
                 fRepeat(0.0f), buttonCode(0), strAction("")
      { }

#ifndef SWIG
      Action(const CAction& caction) : AddonClass("Action") { setFromCAction(caction); }

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

      long getId() { TRACE; return id; }
      long getButtonCode() { TRACE; return buttonCode; }
      float getAmount1() { TRACE; return fAmount1; }
      float getAmount2() { TRACE; return fAmount2; }
    };

    /**
     * This is the main class for the xbmcgui.Window functionality. It is tied
     *  into the main XBMC windowing system via the Interceptor
     */
    class Window : public AddonCallback
    {
      friend class WindowDialogMixin;
      bool isDisposed;
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

      Window(const char* classname) throw (WindowException);

      virtual void deallocating();

      /**
       * This helper retrieves the next available id. It is assumed 
       *  that the global lock is already being held.
       */
      static int getNextAvailalbeWindowId() throw (WindowException);

      /**
       * Child classes MUST call this in their constructors. It should
       *  be an instance of Interceptor<P extends CGUIWindow>. Control
       *  of memory managment for this class is then given to the
       *  Window
       */
      void setWindow(InterceptorBase* _window);

      /**
       * This is a helper method since poping the
       *  previous window id is a common function
       */
      void popActiveWindowId();

      /**
       * This is a helper method since getting
       *  a control by it's id is a common function
       */
      Control* GetControlById(int iControlId) throw (WindowException);

      SWIGHIDDENVIRTUAL void PulseActionEvent();
      SWIGHIDDENVIRTUAL void WaitForActionEvent();
#endif

    public:
      Window(int existingWindowId = -1) throw (WindowException);

      virtual ~Window();

#ifndef SWIG
      SWIGHIDDENVIRTUAL bool    OnMessage(CGUIMessage& message);
      SWIGHIDDENVIRTUAL bool    OnAction(const CAction &action);
      SWIGHIDDENVIRTUAL bool    OnBack(int actionId);
      SWIGHIDDENVIRTUAL void    OnDeinitWindow(int nextWindowID);

      SWIGHIDDENVIRTUAL bool    IsDialogRunning() const { TRACE; return false; };
      SWIGHIDDENVIRTUAL bool    IsDialog() const { TRACE; return false; };
      SWIGHIDDENVIRTUAL bool    IsModalDialog() const { TRACE; return false; };
      SWIGHIDDENVIRTUAL bool    IsMediaWindow() const { TRACE; return false; };
      SWIGHIDDENVIRTUAL void    dispose();

      // This is called from the InterceptorBase destructor to prevent further
      //  use of the interceptor from the window.
      inline void interceptorClear() { Synchronize lock(*this); window = NULL; }
#endif

      // callback takes a parameter
      /**
       * onAction(self, Action action) -- onAction method.
       * 
       * This method will recieve all actions that the main program will send
       * to this window.
       * By default, only the PREVIOUS_MENU and NAV_BACK actions are handled.
       * Overwrite this method to let your script handle all actions.
       * Don't forget to capture ACTION_PREVIOUS_MENU or ACTION_NAV_BACK, else the user can't close this window.
       */
      virtual void onAction(Action* action);
      // on control is not actually on Window in the api but is called
      //  into Python anyway. This must result in a problem when 
      virtual void onControl(Control* control);
      virtual void onClick(int controlId);
      virtual void onFocus(int controlId);
      virtual void onInit();

      /**
       * show(self) -- Show this window.
       * 
       * Shows this window by activating it, calling close() after it wil activate the
       * current window again.
       * Note, if your script ends this window will be closed to. To show it forever, 
       * make a loop at the end of your script ar use doModal() instead
       */
      SWIGHIDDENVIRTUAL void show();

      /**
       * setFocus(self, Control) -- Give the supplied control focus.
       * Throws: TypeError, if supplied argument is not a Control type
       *         SystemError, on Internal error
       *         RuntimeError, if control is not added to a window
       */
      SWIGHIDDENVIRTUAL void setFocus(Control* pControl) throw (WindowException);

      /**
       * setFocusId(self, int) -- Gives the control with the supplied focus.
       * Throws: 
       *         SystemError, on Internal error
       *         RuntimeError, if control is not added to a window
       */
      SWIGHIDDENVIRTUAL void setFocusId(int iControlId);

      /**
       * getFocus(self, Control) -- returns the control which is focused.
       * Throws: SystemError, on Internal error
       *         RuntimeError, if no control has focus
       */
      SWIGHIDDENVIRTUAL Control* getFocus() throw (WindowException);

      /**
       * getFocusId(self, int) -- returns the id of the control which is focused.
       * Throws: SystemError, on Internal error
       *         RuntimeError, if no control has focus
       */
      SWIGHIDDENVIRTUAL long getFocusId() throw (WindowException);

      /**
       * removeControl(self, Control) -- Removes the control from this window.
       * 
       * Throws: TypeError, if supplied argument is not a Control type
       *         RuntimeError, if control is not added to this window
       * 
       * This will not delete the control. It is only removed from the window.
       */
      SWIGHIDDENVIRTUAL void removeControl(Control* pControl) throw (WindowException);

      /**
       * removeControls(self, List) -- Removes a list of controls from this window.
       *
       * Throws: TypeError, if supplied argument is not a Control type
       *        RuntimeError, if control is not added to this window
       *
       * This will not delete the controls. They are only removed from the window.
       */
      SWIGHIDDENVIRTUAL void removeControls(std::vector<Control*> pControls) throw (WindowException);

      /**
       * getHeight(self) -- Returns the height of this screen.
       */
      SWIGHIDDENVIRTUAL long getHeight();

      /**
       * getWidth(self) -- Returns the width of this screen.
       */
      SWIGHIDDENVIRTUAL long getWidth();

      /**
       * getResolution(self) -- Returns the resolution of the scree
       *  The returned value is one of the following:
       *    0 - 1080i      (1920x1080)
       *    1 - 720p       (1280x720)
       *    2 - 480p 4:3   (720x480)
       *    3 - 480p 16:9  (720x480)
       *    4 - NTSC 4:3   (720x480)
       *    5 - NTSC 16:9  (720x480)
       *    6 - PAL 4:3    (720x576)
       *    7 - PAL 16:9   (720x576)
       *    8 - PAL60 4:3  (720x480)
       *    9 - PAL60 16:9 (720x480)\n
       */
      SWIGHIDDENVIRTUAL long getResolution();

      /**
       * setCoordinateResolution(self, int resolution) -- Sets the resolution
       * that the coordinates of all controls are defined in.  Allows XBMC
       * to scale control positions and width/heights to whatever resolution
       * XBMC is currently using.
       *  resolution is one of the following:
       *    0 - 1080i      (1920x1080)
       *    1 - 720p       (1280x720)
       *    2 - 480p 4:3   (720x480)
       *    3 - 480p 16:9  (720x480)
       *    4 - NTSC 4:3   (720x480)
       *    5 - NTSC 16:9  (720x480)
       *    6 - PAL 4:3    (720x576)
       *    7 - PAL 16:9   (720x576)
       *    8 - PAL60 4:3  (720x480)
       *    9 - PAL60 16:9 (720x480)\n
       */
      SWIGHIDDENVIRTUAL void setCoordinateResolution(long res) throw (WindowException);

      /**
       * setProperty(key, value) -- Sets a window property, similar to an infolabel.
       * 
       * key            : string - property name.
       * value          : string or unicode - value of property.
       * 
       * *Note, key is NOT case sensitive. Setting value to an empty string is equivalent to clearProperty(key)
       *        You can use the above as keywords for arguments and skip certain optional arguments.
       *        Once you use a keyword, all following arguments require the keyword.
       * 
       * example:
       *   - win = xbmcgui.Window(xbmcgui.getCurrentWindowId())
       *   - win.setProperty('Category', 'Newest')\n
       */
      SWIGHIDDENVIRTUAL void setProperty(const char* key, const String& value);

      /**
       * getProperty(key) -- Returns a window property as a string, similar to an infolabel.
       * 
       * key            : string - property name.
       * 
       * *Note, key is NOT case sensitive.
       *        You can use the above as keywords for arguments and skip certain optional arguments.
       *        Once you use a keyword, all following arguments require the keyword.
       * 
       * example:
       *   - win = xbmcgui.Window(xbmcgui.getCurrentWindowId())
       *   - category = win.getProperty('Category')
       */
      SWIGHIDDENVIRTUAL String getProperty(const char* key);

      /**
       * clearProperty(key) -- Clears the specific window property.
       * 
       * key            : string - property name.
       * 
       * *Note, key is NOT case sensitive. Equivalent to setProperty(key,'')
       *        You can use the above as keywords for arguments and skip certain optional arguments.
       *        Once you use a keyword, all following arguments require the keyword.
       * 
       * example:
       *   - win = xbmcgui.Window(xbmcgui.getCurrentWindowId())
       *   - win.clearProperty('Category')\n
       */
      SWIGHIDDENVIRTUAL void clearProperty(const char* key);

      /**
       * clearProperties() -- Clears all window properties.
       * 
       * example:
       *   - win = xbmcgui.Window(xbmcgui.getCurrentWindowId())
       *   - win.clearProperties()\n
       */
      SWIGHIDDENVIRTUAL void clearProperties();

      /**
       * close(self) -- Closes this window.
       * 
       * Closes this window by activating the old window.
       * The window is not deleted with this method.
       */
      SWIGHIDDENVIRTUAL void close();

      /**
       * doModal(self) -- Display this window until close() is called.
       */
      SWIGHIDDENVIRTUAL void doModal();

      /**
       * addControl(self, Control) -- Add a Control to this window.
       * 
       * Throws: TypeError, if supplied argument is not a Control type
       *         ReferenceError, if control is already used in another window
       *         RuntimeError, should not happen :-)
       * 
       * The next controls can be added to a window atm
       * 
       *   -ControlLabel
       *   -ControlFadeLabel
       *   -ControlTextBox
       *   -ControlButton
       *   -ControlCheckMark
       *   -ControlList
       *   -ControlGroup
       *   -ControlImage
       *   -ControlRadioButton
       *   -ControlProgress\n
       */
      SWIGHIDDENVIRTUAL void addControl(Control* pControl) throw (WindowException);

      /**
       * addControls(self, List) -- Add a list of Controls to this window.
       *
       *Throws: TypeError, if supplied argument is not of List type, or a control is not of Control type
       *        ReferenceError, if control is already used in another window
       *        RuntimeError, should not happen :-)
       */
      SWIGHIDDENVIRTUAL void addControls(std::vector<Control*> pControls) throw (WindowException);

      /**
       * getControl(self, int controlId) -- Get's the control from this window.
       * 
       * Throws: Exception, if Control doesn't exist
       * 
       * controlId doesn't have to be a python control, it can be a control id
       * from a xbmc window too (you can find id's in the xml files
       * 
       * Note, not python controls are not completely usable yet
       * You can only use the Control functions
       */
      SWIGHIDDENVIRTUAL Control* getControl(int iControlId) throw (WindowException);
    };
  }
}
