#include "stdafx.h"
#include "window.h"
#include "dialog.h"
#include "winxml.h"
#include "pyutil.h"
#include "action.h"
#include "GUIPythonWindow.h"
#include "GUIButtonControl.h"
#include "GUICheckMarkControl.h"

#define ACTIVE_WINDOW	m_gWindowManager.GetActiveWindow()

#pragma code_seg("PY_TEXT")
#pragma data_seg("PY_DATA")
#pragma bss_seg("PY_BSS")
#pragma const_seg("PY_RDATA")

#ifdef __cplusplus
extern "C" {
#endif

namespace PYXBMC
{
  extern PyObject* ControlSpin_New(void);

  // used by Dialog to to create a new dialogWindow
  bool Window_CreateNewWindow(Window* pWindow, bool bAsDialog)
  {
    if (pWindow->iWindowId != -1)
    {
      // user specified window id, use this one if it exists
      // It is not possible to capture key presses or button presses
      PyGUILock();
      pWindow->pWindow = m_gWindowManager.GetWindow(pWindow->iWindowId);
      PyGUIUnlock();
      if (!pWindow->pWindow)
      {
        PyErr_SetString(PyExc_ValueError, "Window id does not exist");
        return false;
      }
      pWindow->iOldWindowId = 0;
      pWindow->bModal = false;
      pWindow->iCurrentControlId = 3000;
      pWindow->bIsPythonWindow = false;
    }
    else
    {
      // window id's 13000 - 13100 are reserved for python
      // get first window id that is not in use
      int id = WINDOW_PYTHON_START;
      // if window 13099 is in use it means python can't create more windows
      PyGUILock();
      if (m_gWindowManager.GetWindow(WINDOW_PYTHON_END))
      {
        PyGUIUnlock();
        PyErr_SetString(PyExc_Exception, "maximum number of windows reached");
        return false;
      }
      while(id < WINDOW_PYTHON_END && m_gWindowManager.GetWindow(id) != NULL) id++;
      PyGUIUnlock();

      pWindow->iWindowId = id;
      pWindow->iOldWindowId = 0;
      pWindow->bModal = false;
      pWindow->bIsPythonWindow = true;

      if (!bAsDialog && !pWindow->bUsingXML) {
        pWindow->pWindow = new CGUIPythonWindow(id);
      } else if (pWindow->bUsingXML && !bAsDialog) {
          pWindow->pWindow = new CGUIPythonWindowXML(id,pWindow->sXMLFileName,pWindow->sFallBackPath);
      } else if (pWindow->bUsingXML && bAsDialog) {
          pWindow->pWindow = new CGUIPythonWindowXMLDialog(id,pWindow->sXMLFileName,pWindow->sFallBackPath);
      } else {
        pWindow->pWindow = new CGUIPythonWindowDialog(id);
      }
    if (pWindow->bIsPythonWindow)
        ((CGUIPythonWindow*)pWindow->pWindow)->SetCallbackWindow((PyObject*)pWindow);

      PyGUILock();
      m_gWindowManager.Add(pWindow->pWindow);
      PyGUIUnlock();
    }
    return true;
  }

  /* Searches for a control in Window->vecControls
   * If we can't find any but the window has the controlId (in case of a not python window)
   * we create a new control with basic functionality
   */
  Control* Window_GetControlById(Window* self, int iControlId)
  {
    Control* pControl = NULL;
    CGUIWindow* pWindow = NULL;

    // lock xbmc GUI before accessing data from it
    PyGUILock();

    pWindow = (CGUIWindow*)m_gWindowManager.GetWindow(self->iWindowId);
    if (!pWindow)
    {
      PyGUIUnlock();
      return NULL;
    }

    // check if control exists
    CGUIControl* pGUIControl = (CGUIControl*)pWindow->GetControl(iControlId);
    PyGUIUnlock();
    if (!pGUIControl)
    {
      // control does not exist.
      CStdString error;
      error.Format("Non-Existent Control %d",iControlId);
      PyErr_SetString(PyExc_TypeError, error.c_str());
      return NULL;
    }

    // find in window vector first!!!
    // this saves us from creating a complete new control
    vector<Control*>::iterator it = self->vecControls.begin();
    while (it != self->vecControls.end())
    {
      Control* control = *it;
      if (control->iControlId == iControlId)
      {
        Py_INCREF(control);
        return control;
      } else ++it;
    }

    // allocate a new control with a new reference
    CLabelInfo li;
    switch(pGUIControl->GetControlType())
    {
    case CGUIControl::GUICONTROL_BUTTON:
      pControl = (Control*)ControlButton_Type.tp_alloc(&ControlButton_Type, 0);
      new(&((ControlButton*)pControl)->strFont) string();    
      new(&((ControlButton*)pControl)->strText) string();    
      new(&((ControlButton*)pControl)->strText2) string();    
      new(&((ControlButton*)pControl)->strTextureFocus) string();    
      new(&((ControlButton*)pControl)->strTextureNoFocus) string(); 

      li = ((CGUIButtonControl *)pGUIControl)->GetLabelInfo();

      ((ControlButton*)pControl)->dwDisabledColor = li.disabledColor;
      ((ControlButton*)pControl)->dwFocusedColor  = li.focusedColor;
      ((ControlButton*)pControl)->dwTextColor  = li.textColor;
      ((ControlButton*)pControl)->dwShadowColor   = li.shadowColor;
      ((ControlButton*)pControl)->strFont = li.font->GetFontName();
      ((ControlButton*)pControl)->dwAlign = li.align;
      break;
    case CGUIControl::GUICONTROL_CHECKMARK:
      pControl = (Control*)ControlCheckMark_Type.tp_alloc(&ControlCheckMark_Type, 0);
      new(&((ControlCheckMark*)pControl)->strFont) string();    
      new(&((ControlCheckMark*)pControl)->strText) string();    
      new(&((ControlCheckMark*)pControl)->strTextureFocus) string();    
      new(&((ControlCheckMark*)pControl)->strTextureNoFocus) string();    

      li = ((CGUICheckMarkControl *)pGUIControl)->GetLabelInfo();

      ((ControlCheckMark*)pControl)->dwDisabledColor = li.disabledColor;
      //((ControlCheckMark*)pControl)->dwShadowColor = li.shadowColor;
      ((ControlCheckMark*)pControl)->dwTextColor  = li.textColor;
      ((ControlCheckMark*)pControl)->strFont = li.font->GetFontName();
      ((ControlCheckMark*)pControl)->dwAlign = li.align;
      break;
    case CGUIControl::GUICONTROL_LABEL:
      pControl = (Control*)ControlLabel_Type.tp_alloc(&ControlLabel_Type, 0);
      new(&((ControlLabel*)pControl)->strText) string();
      new(&((ControlLabel*)pControl)->strFont) string();
      break;
    case CGUIControl::GUICONTROL_SPIN:
      pControl = (Control*)ControlSpin_Type.tp_alloc(&ControlSpin_Type, 0);
      new(&((ControlSpin*)pControl)->strTextureUp) string();    
      new(&((ControlSpin*)pControl)->strTextureDown) string();    
      new(&((ControlSpin*)pControl)->strTextureUpFocus) string();    
      new(&((ControlSpin*)pControl)->strTextureDownFocus) string();      
      break;
    case CGUIControl::GUICONTROL_FADELABEL:
      pControl = (Control*)ControlFadeLabel_Type.tp_alloc(&ControlFadeLabel_Type, 0);
      new(&((ControlFadeLabel*)pControl)->strFont) string();
      new(&((ControlFadeLabel*)pControl)->vecLabels) std::vector<string>();    
      break;
    case CGUIControl::GUICONTROL_TEXTBOX:
      pControl = (Control*)ControlTextBox_Type.tp_alloc(&ControlTextBox_Type, 0);
      ((ControlTextBox*)pControl)->pControlSpin = (ControlSpin*)ControlSpin_New();
      if (((ControlTextBox*)pControl)->pControlSpin) 
         new(&((ControlTextBox*)pControl)->strFont) string();        
      break;
    case CGUIControl::GUICONTROL_IMAGE:
      pControl = (Control*)ControlImage_Type.tp_alloc(&ControlImage_Type, 0);
      new(&((ControlImage*)pControl)->strFileName) string();    
      break;
    case CGUIControl::GUICONTROL_LIST:
      pControl = (Control*)ControlList_Type.tp_alloc(&ControlList_Type, 0);
      new(&((ControlList*)pControl)->strFont) string();    
      new(&((ControlList*)pControl)->strTextureButton) string();    
      new(&((ControlList*)pControl)->strTextureButtonFocus) string();
      new(&((ControlList*)pControl)->vecItems) std::vector<PYXBMC::ListItem*>();
      // create a python spin control
      ((ControlList*)pControl)->pControlSpin = (ControlSpin*)ControlSpin_New();
      break;
    case CGUIControl::GUICONTROL_PROGRESS:
      pControl = (Control*)ControlProgress_Type.tp_alloc(&ControlProgress_Type, 0);
      new(&((ControlProgress*)pControl)->strTextureLeft) string();    
      new(&((ControlProgress*)pControl)->strTextureMid) string();    
      new(&((ControlProgress*)pControl)->strTextureRight) string();    
      new(&((ControlProgress*)pControl)->strTextureBg) string();     
      new(&((ControlProgress*)pControl)->strTextureOverlay) string();     
      break;
    case CGUIControl::GUICONTAINER_LIST:
      pControl = (Control*)ControlList_Type.tp_alloc(&ControlList_Type, 0);
      new(&((ControlList*)pControl)->strFont) string();    
      new(&((ControlList*)pControl)->strTextureButton) string();    
      new(&((ControlList*)pControl)->strTextureButtonFocus) string();
      new(&((ControlList*)pControl)->vecItems) std::vector<PYXBMC::ListItem*>();
      // create a python spin control
      ((ControlList*)pControl)->pControlSpin = (ControlSpin*)ControlSpin_New();
      break;
    case CGUIControl::GUICONTROL_GROUP:
      pControl = (Control*)ControlGroup_Type.tp_alloc(&ControlGroup_Type, 0);
      break;
    }

    if (!pControl)
    {
      // throw an exeption
      PyErr_SetString(PyExc_Exception, "Unknown control type for python");
      return NULL;
    }

    PyGUILock();
    Py_INCREF(pControl);
    // we have a valid control here, fill in all the 'Control' data
    pControl->pGUIControl = pGUIControl;
    pControl->iControlId = pGUIControl->GetID();
    pControl->iParentId = self->iWindowId;
    pControl->dwHeight = (int)pGUIControl->GetHeight();
    pControl->dwWidth = (int)pGUIControl->GetWidth();
    pControl->dwPosX = (int)pGUIControl->GetXPosition();
    pControl->dwPosY = (int)pGUIControl->GetYPosition();
    pControl->iControlUp = pGUIControl->GetControlIdUp();
    pControl->iControlDown = pGUIControl->GetControlIdDown();
    pControl->iControlLeft = pGUIControl->GetControlIdLeft();
    pControl->iControlRight = pGUIControl->GetControlIdRight();

    // It got this far so means the control isn't actually in the vector of controls 
    // so lets add it to save doing all that next time
    self->vecControls.push_back(pControl);

    PyGUIUnlock();

    // return the control with increased reference (+1)
    return pControl;
  }

  PyObject* Window_New(PyTypeObject *type, PyObject *args, PyObject *kwds)
  {
    Window *self;

    self = (Window*)type->tp_alloc(type, 0);
    if (!self) return NULL;
    new(&self->sXMLFileName) string();
    new(&self->sFallBackPath) string();
    new(&self->vecControls) std::vector<Control*>();
    self->iWindowId = -1;

    if (!PyArg_ParseTuple(args, "|i", &self->iWindowId)) return NULL;

    // create new GUIWindow
    if (!Window_CreateNewWindow(self, false))
    {
      // error is already set by Window_CreateNewWindow, just release the memory
      self->vecControls.clear();
      self->vecControls.~vector();
      self->sFallBackPath.~string();          
      self->sXMLFileName.~string();        
      self->ob_type->tp_free((PyObject*)self);
      return NULL;
    }

    return (PyObject*)self;
  }

  void Window_Dealloc(Window* self)
  {
    PyGUILock();
    if (self->bIsPythonWindow)
    {
      // first change to an existing window
      if (ACTIVE_WINDOW == self->iWindowId)
      {
        if(m_gWindowManager.GetWindow(self->iOldWindowId))
        {
          m_gWindowManager.ActivateWindow(self->iOldWindowId);
        }
        // old window does not exist anymore, switch to home
        else m_gWindowManager.ActivateWindow(WINDOW_HOME);
      }
    }

    // free the window's resources and unload it (free all guicontrols)
    self->pWindow->FreeResources(true);

    // and free our list of controls
    std::vector<Control*>::iterator it = self->vecControls.begin();
    while (it != self->vecControls.end())
    {
      Control* pControl = *it;
      // initialize control to zero
      pControl->pGUIControl = NULL;
      pControl->iControlId = 0;
      pControl->iParentId = 0;
      Py_DECREF(pControl);
      ++it;
    }

    PyGUIUnlock();
    if (self->bIsPythonWindow)
    {
      m_gWindowManager.Remove(self->pWindow->GetID());
      delete self->pWindow;
    }
    
    self->vecControls.clear();
    self->vecControls.~vector();
    self->sFallBackPath.~string();          
    self->sXMLFileName.~string();            
    self->ob_type->tp_free((PyObject*)self);
  }

  PyDoc_STRVAR(show__doc__,
    "show(self) -- Show this window.\n"
    "\n"
    "Shows this window by activating it, calling close() after it wil activate the\n"
    "current window again.\n"
    "Note, if your script ends this window will be closed to. To show it forever, \n"
    "make a loop at the end of your script ar use doModal() instead");

  PyObject* Window_Show(Window *self, PyObject *args)
  {
    if (self->iOldWindowId != self->iWindowId &&
        self->iWindowId != ACTIVE_WINDOW)
      self->iOldWindowId = ACTIVE_WINDOW;

    PyGUILock();
    // if it's a idalog, we have to activate it a bit different
    if (WindowDialog_Check(self))	((CGUIPythonWindowDialog*)self->pWindow)->Activate(ACTIVE_WINDOW);
    else if (WindowXMLDialog_Check(self))	((CGUIPythonWindowXMLDialog*)self->pWindow)->Activate(ACTIVE_WINDOW);
    // activate the window
    else m_gWindowManager.ActivateWindow(self->iWindowId);
    PyGUIUnlock();

    Py_INCREF(Py_None);
    return Py_None;
  }

  PyDoc_STRVAR(close__doc__,
    "close(self) -- Closes this window.\n"
    "\n"
    "Closes this window by activating the old window.\n"
    "The window is not deleted with this method.");

  PyObject* Window_Close(Window *self, PyObject *args)
  {
    self->bModal = false;
    if (self->bIsPythonWindow)
      ((CGUIPythonWindow*)self->pWindow)->PulseActionEvent();

    PyGUILock();

    // if it's a dialog, we have to close it a bit different
    if (WindowDialog_Check(self))	((CGUIPythonWindowDialog*)self->pWindow)->Close();
    else if (WindowXMLDialog_Check(self)) ((CGUIPythonWindowXMLDialog*)self->pWindow)->Close();
    // close the window by activating the parent one
    else m_gWindowManager.ActivateWindow(self->iOldWindowId);
    self->iOldWindowId = 0;

    PyGUIUnlock();

    Py_INCREF(Py_None);
    return Py_None;
  }

  PyDoc_STRVAR(onAction__doc__,
    "onAction(self, Action action) -- onAction method.\n"
    "\n"
    "This method will recieve all actions that the main program will send\n"
    "to this window.\n"
    "By default, only the PREVIOUS_MENU action is handled.\n"
    "Overwrite this method to let your script handle all actions.\n"
    "Don't forget to capture ACTION_PREVIOUS_MENU, else the user can't close this window.");

  PyObject* Window_OnAction(Window *self, PyObject *args)
  {
    Action* action;
    if (!PyArg_ParseTuple(args, "O", &action)) return NULL;

    if(action->id == ACTION_PREVIOUS_MENU)
    {
      Window_Close(self, args);
    }
    Py_INCREF(Py_None);
    return Py_None;
  }

  PyDoc_STRVAR(doModal__doc__,
    "doModal(self) -- Display this window until close() is called.");

  PyObject* Window_DoModal(Window *self, PyObject *args)
  {
    if (self->bIsPythonWindow)
    {
      self->bModal = true;

      if(self->iWindowId != ACTIVE_WINDOW) Window_Show(self, NULL);

      while(self->bModal)
      {
        Py_BEGIN_ALLOW_THREADS
	  ((CGUIPythonWindow*)self->pWindow)->WaitForActionEvent(INFINITE);
        Py_END_ALLOW_THREADS

        // only call Py_MakePendingCalls from a python thread
	Py_MakePendingCalls();
      }
    }
    Py_INCREF(Py_None);
    return Py_None;
  }

  PyDoc_STRVAR(addControl__doc__,
    "addControl(self, Control) -- Add a Control to this window.\n"
    "\n"
    "Throws: TypeError, if supplied argument is not a Control type\n"
    "        ReferenceError, if control is already used in another window\n"
    "        RuntimeError, should not happen :-)\n"
    "\n"
    "The next controls can be added to a window atm\n"
    "\n"
    "  -ControlLabel\n"
    "  -ControlFadeLabel\n"
    "  -ControlTextBox\n"
    "  -ControlButton\n"
    "  -ControlCheckMark\n"
    "  -ControlList\n"
    "  -ControlGroup\n"
    "  -ControlImage\n");

  PyObject* Window_AddControl(Window *self, PyObject *args)
  {
    CGUIWindow* pWindow = NULL;
    Control* pControl;

    if (!PyArg_ParseTuple(args, "O", &pControl)) return NULL;
    // type checking, object should be of type Control
    if(!Control_Check(pControl))
    {
      PyErr_SetString(PyExc_TypeError, "Object should be of type Control");
      return NULL;
    }

    if(pControl->iControlId != 0)
    {
      PyErr_SetString(PyExc_ReferenceError, "Control is already used");
      return NULL;
    }

    // lock xbmc GUI before accessing data from it
    PyGUILock();

    pWindow = (CGUIWindow*)m_gWindowManager.GetWindow(self->iWindowId);
    if (!pWindow)
    {
      PyGUIUnlock();
      return NULL;
    }

    pControl->iParentId = self->iWindowId;
    // assign control id, if id is already in use, try next id
    do pControl->iControlId = ++self->iCurrentControlId;
    while (pWindow->GetControl(pControl->iControlId));

    PyGUIUnlock();

    // Control Label
    if (ControlLabel_Check(pControl))
      ControlLabel_Create((ControlLabel*)pControl);

    // Control Fade Label
    else if (ControlFadeLabel_Check(pControl))
      ControlFadeLabel_Create((ControlFadeLabel*)pControl);

    // Control TextBox
    else if (ControlTextBox_Check(pControl))
      ControlTextBox_Create((ControlTextBox*)pControl);

    // Control Button
    else if (ControlButton_Check(pControl))
      ControlButton_Create((ControlButton*)pControl);

    // Control CheckMark
    else if (ControlCheckMark_Check(pControl))
      ControlCheckMark_Create((ControlCheckMark*)pControl);

    // Image
    else if (ControlImage_Check(pControl))
      ControlImage_Create((ControlImage*)pControl);

    // Control List
    else if (ControlList_Check(pControl))
      ControlList_Create((ControlList*)pControl);

    // Control Progress
    else if (ControlProgress_Check(pControl))
      ControlProgress_Create((ControlProgress*)pControl);

    // Control Group
    else if (ControlGroup_Check(pControl))
      ControlGroup_Create((ControlGroup*)pControl);

    //unknown control type to add, should not happen
    else
    {
      PyErr_SetString(PyExc_RuntimeError, "Object is a Control, but can't be added to a window");
      return NULL;
    }

    Py_INCREF(pControl);

    // set default navigation for control
    pControl->iControlUp = pControl->iControlId;
    pControl->iControlDown = pControl->iControlId;
    pControl->iControlLeft = pControl->iControlId;
    pControl->iControlRight = pControl->iControlId;

    pControl->pGUIControl->SetNavigation(pControl->iControlUp,
      pControl->iControlDown,	pControl->iControlLeft, pControl->iControlRight);

    PyGUILock();

    // add control to list and allocate recources for the control
    self->vecControls.push_back(pControl);
    pControl->pGUIControl->AllocResources();
    pWindow->Add(pControl->pGUIControl);

    PyGUIUnlock();

    Py_INCREF(Py_None);
    return Py_None;
  }

  PyDoc_STRVAR(getControl__doc__,
    "getControl(self, int controlId) -- Get's the control from this window.\n"
    "\n"
    "Throws: Exception, if Control doesn't exist\n"
    "\n"
    "controlId doesn't have to be a python control, it can be a control id\n"
    "from a xbmc window too (you can find id's in the xml files\n"
    "\n"
    "Note, not python controls are not completely usable yet\n"
    "You can only use the Control functions\n"
    "");

  PyObject* Window_GetControl(Window *self, PyObject *args)
  {
    int iControlId;
    if (!PyArg_ParseTuple(args, "i", &iControlId)) return NULL;

    return (PyObject*)Window_GetControlById(self, iControlId);
  }

  PyDoc_STRVAR(setFocus__doc__,
    "setFocus(self, Control) -- Give the supplied control focus.\n"
    "Throws: TypeError, if supplied argument is not a Control type\n"
    "        SystemError, on Internal error\n"
    "        RuntimeError, if control is not added to a window\n"
    "\n");

  PyObject* Window_SetFocus(Window *self, PyObject *args)
  {
    CGUIWindow* pWindow = (CGUIWindow*)m_gWindowManager.GetWindow(self->iWindowId);
    if (PyWindowIsNull(pWindow)) return NULL;

    Control* pControl;
    if (!PyArg_ParseTuple(args, "O", &pControl)) return NULL;
    // type checking, object should be of type Control
    if(!Control_Check(pControl))
    {
      PyErr_SetString(PyExc_TypeError, "Object should be of type Control");
      return NULL;
    }

    if(!pWindow->GetControl(pControl->iControlId))
    {
      PyErr_SetString(PyExc_RuntimeError, "Control does not exist in window");
      return NULL;
    }

    PyGUILock();
    CGUIMessage msg = CGUIMessage(GUI_MSG_SETFOCUS,pControl->iParentId, pControl->iControlId);
    pWindow->OnMessage(msg);
    PyGUIUnlock();

    Py_INCREF(Py_None);
    return Py_None;
  }
  
  PyDoc_STRVAR(setFocusId__doc__,
    "setFocusId(self, int) -- Gives the control with the supplied focus.\n"
    "Throws: \n"
    "        SystemError, on Internal error\n"
    "        RuntimeError, if control is not added to a window\n"
    "\n");

  PyObject* Window_SetFocusId(Window *self, PyObject *args)
  {
    CGUIWindow* pWindow = (CGUIWindow*)m_gWindowManager.GetWindow(self->iWindowId);
    if (PyWindowIsNull(pWindow)) return NULL;

    int iControlId;
    if (!PyArg_ParseTuple(args, "i", &iControlId)) return NULL;

    if(!pWindow->GetControl(iControlId))
    {
      PyErr_SetString(PyExc_RuntimeError, "Control does not exist in window");
      return NULL;
    }

    PyGUILock();
    CGUIMessage msg = CGUIMessage(GUI_MSG_SETFOCUS,self->iWindowId,iControlId);
    pWindow->OnMessage(msg);
    PyGUIUnlock();

    Py_INCREF(Py_None);
    return Py_None;
  }

  PyDoc_STRVAR(getFocus__doc__,
    "getFocus(self, Control) -- returns the control which is focused.\n"
    "Throws: SystemError, on Internal error\n"
    "        RuntimeError, if no control has focus\n"
    "\n");

  PyObject* Window_GetFocus(Window *self, PyObject *args)
  {
    int iControlId = -1;
    CGUIWindow* pWindow = (CGUIWindow*)m_gWindowManager.GetWindow(self->iWindowId);
    if (PyWindowIsNull(pWindow)) return NULL;


    PyGUILock();
    iControlId = pWindow->GetFocusedControlID();
    PyGUIUnlock();

    if(iControlId == -1)
    {
      PyErr_SetString(PyExc_RuntimeError, "No control in this window has focus");
      return NULL;
    }

    return (PyObject*)Window_GetControlById(self, iControlId);
  }

  PyDoc_STRVAR(getFocusId__doc__,
    "getFocusId(self, int) -- returns the id of the control which is focused.\n"
    "Throws: SystemError, on Internal error\n"
    "        RuntimeError, if no control has focus\n"
    "\n");

  PyObject* Window_GetFocusId(Window *self, PyObject *args)
  {
    int iControlId = -1;
    CGUIWindow* pWindow = (CGUIWindow*)m_gWindowManager.GetWindow(self->iWindowId);
    if (PyWindowIsNull(pWindow)) return NULL;

    PyGUILock();
    iControlId = pWindow->GetFocusedControlID();
    PyGUIUnlock();

    if(iControlId == -1)
    {
      PyErr_SetString(PyExc_RuntimeError, "No control in this window has focus");
      return NULL;
    }

    return PyLong_FromLong((long)iControlId);
  }
  
  PyDoc_STRVAR(removeControl__doc__,
    "removeControl(self, Control) -- Removes the control from this window.\n"
    "\n"
    "Throws: TypeError, if supplied argument is not a Control type\n"
    "        RuntimeError, if control is not added to this window\n"
    "\n"
    "This will not delete the control. It is only removed from the window.");

  PyObject* Window_RemoveControl(Window *self, PyObject *args)
  {
    CGUIWindow* pWindow = (CGUIWindow*)m_gWindowManager.GetWindow(self->iWindowId);
    if (PyWindowIsNull(pWindow)) return NULL;

    Control* pControl;
    if (!PyArg_ParseTuple(args, "O", &pControl)) return NULL;
    // type checking, object should be of type Control
    if(!Control_Check(pControl))
    {
      PyErr_SetString(PyExc_TypeError, "Object should be of type Control");
      return NULL;
    }

    if(!pWindow->GetControl(pControl->iControlId))
    {
      PyErr_SetString(PyExc_RuntimeError, "Control does not exist in window");
      return NULL;
    }

    // delete control from vecControls in window object
    vector<Control*>::iterator it = self->vecControls.begin();
    while (it != self->vecControls.end())
    {
      Control* control = *it;
      if (control->iControlId == pControl->iControlId)
      {
        it = self->vecControls.erase(it);
      } else ++it;
    }

    PyGUILock();

    pWindow->Remove(pControl->iControlId);
    pControl->pGUIControl->FreeResources();
    delete pControl->pGUIControl;

    PyGUIUnlock();

    // initialize control to zero
    pControl->pGUIControl = NULL;
    pControl->iControlId = 0;
    pControl->iParentId = 0;
    Py_DECREF(pControl);

    Py_INCREF(Py_None);
    return Py_None;
  }

  PyDoc_STRVAR(getHeight__doc__,
    "getHeight(self) -- Returns the height of this screen.");

  PyObject* Window_GetHeight(Window *self, PyObject *args)
  {
    return PyLong_FromLong(g_graphicsContext.GetHeight());
  }

  PyDoc_STRVAR(getWidth__doc__,
    "getWidth(self) -- Returns the width of this screen.");

  PyObject* Window_GetWidth(Window *self, PyObject *args)
  {
    return PyLong_FromLong(g_graphicsContext.GetWidth());
  }

  PyDoc_STRVAR(getResolution__doc__,
    "getResolution(self) -- Returns the resolution of the screen."
    " The returned value is one of the following:\n"
    "   0 - 1080i      (1920x1080)\n"
    "   1 - 720p       (1280x720)\n"
    "   2 - 480p 4:3   (720x480)\n"
    "   3 - 480p 16:9  (720x480)\n"
    "   4 - NTSC 4:3   (720x480)\n"
    "   5 - NTSC 16:9  (720x480)\n"
    "   6 - PAL 4:3    (720x576)\n"
    "   7 - PAL 16:9   (720x576)\n"
    "   8 - PAL60 4:3  (720x480)\n"
    "   9 - PAL60 16:9 (720x480)\n");

  PyObject* Window_GetResolution(Window *self, PyObject *args)
  {
    return PyLong_FromLong((long)g_graphicsContext.GetVideoResolution());
  }

    PyDoc_STRVAR(setCoordinateResolution__doc__,
    "setCoordinateResolution(self, int resolution) -- Sets the resolution\n"
    "that the coordinates of all controls are defined in.  Allows XBMC\n"
    "to scale control positions and width/heights to whatever resolution\n"
    "XBMC is currently using.\n"
    " resolution is one of the following:\n"
    "   0 - 1080i      (1920x1080)\n"
    "   1 - 720p       (1280x720)\n"
    "   2 - 480p 4:3   (720x480)\n"
    "   3 - 480p 16:9  (720x480)\n"
    "   4 - NTSC 4:3   (720x480)\n"
    "   5 - NTSC 16:9  (720x480)\n"
    "   6 - PAL 4:3    (720x576)\n"
    "   7 - PAL 16:9   (720x576)\n"
    "   8 - PAL60 4:3  (720x480)\n"
    "   9 - PAL60 16:9 (720x480)\n");

  PyObject* Window_SetCoordinateResolution(Window *self, PyObject *args)
  {
    long res;
    if (!PyArg_ParseTuple(args, "l", &res)) return NULL;

    if (res < HDTV_1080i || res > AUTORES)
    {
      PyErr_SetString(PyExc_RuntimeError, "Invalid resolution.");
      return NULL;
    }

    CGUIWindow* pWindow = (CGUIWindow*)m_gWindowManager.GetWindow(self->iWindowId);
    if (PyWindowIsNull(pWindow)) return NULL;

    pWindow->SetCoordsRes((RESOLUTION)res);

    Py_INCREF(Py_None);
    return Py_None;
  }

  PyMethodDef Window_methods[] = {
    //{"load", (PyCFunction)Window_Load, METH_VARARGS, ""},
    {"onAction", (PyCFunction)Window_OnAction, METH_VARARGS, onAction__doc__},
    {"doModal", (PyCFunction)Window_DoModal, METH_VARARGS, doModal__doc__},
    {"show", (PyCFunction)Window_Show, METH_VARARGS, show__doc__},
    {"close", (PyCFunction)Window_Close, METH_VARARGS, close__doc__},
    {"addControl", (PyCFunction)Window_AddControl, METH_VARARGS, addControl__doc__},
    {"getControl", (PyCFunction)Window_GetControl, METH_VARARGS, getControl__doc__},
    {"removeControl", (PyCFunction)Window_RemoveControl, METH_VARARGS, removeControl__doc__},
    {"setFocus", (PyCFunction)Window_SetFocus, METH_VARARGS, setFocus__doc__},
    {"setFocusId", (PyCFunction)Window_SetFocusId, METH_VARARGS, setFocusId__doc__},
    {"getFocus", (PyCFunction)Window_GetFocus, METH_VARARGS, getFocus__doc__},
    {"getFocusId", (PyCFunction)Window_GetFocusId, METH_VARARGS, getFocusId__doc__},
    {"getHeight", (PyCFunction)Window_GetHeight, METH_VARARGS, getHeight__doc__},
    {"getWidth", (PyCFunction)Window_GetWidth, METH_VARARGS, getWidth__doc__},
    {"getResolution", (PyCFunction)Window_GetResolution, METH_VARARGS, getResolution__doc__},
    {"setCoordinateResolution", (PyCFunction)Window_SetCoordinateResolution, METH_VARARGS, setCoordinateResolution__doc__},
    {NULL, NULL, 0, NULL}
  };

  PyDoc_STRVAR(window_documentation,
    "Window class.\n"
    "\n"
    "Window(self[, int windowId) -- Create a new Window to draw on.\n"
    "                               Specify an id to use an existing window.\n"
    "\n"
    "Throws: ValueError, if supplied window Id does not exist.\n"
    "        Exception, if more then 200 windows are created.\n"
    "\n"
    "Deleting this window will activate the old window that was active\n"
    "and resets (not delete) all controls that are associated with this window.");

// Restore code and data sections to normal.
#pragma code_seg()
#pragma data_seg()
#pragma bss_seg()
#pragma const_seg()

  PyTypeObject Window_Type;

  void initWindow_Type()
  {
    PyInitializeTypeObject(&Window_Type);

    Window_Type.tp_name = "xbmcgui.Window";
    Window_Type.tp_basicsize = sizeof(Window);
    Window_Type.tp_dealloc = (destructor)Window_Dealloc;
    Window_Type.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
    Window_Type.tp_doc = window_documentation;
    Window_Type.tp_methods = Window_methods;
    Window_Type.tp_new = Window_New;
  }
}

#ifdef __cplusplus
}
#endif
