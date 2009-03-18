/*
 *      Copyright (C) 2005-2008 Team XBMC
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

#include "stdafx.h"
#include "GUIPythonWindow.h"
#include "pyutil.h"
#include "window.h"
#include "control.h"
#include "action.h"
#include "GUIWindowManager.h"
#include "../XBPython.h"

using namespace PYXBMC;

PyXBMCAction::~PyXBMCAction() {
     if (pObject) {
       Py_DECREF(pObject);
     }

     pObject = NULL;
}

CGUIPythonWindow::CGUIPythonWindow(DWORD dwId)
: CGUIWindow(dwId, "")
{
#ifdef _LINUX
  PyInitPendingCalls();
#endif
  pCallbackWindow = NULL;
  m_actionEvent = CreateEvent(NULL, true, false, NULL);
  m_loadOnDemand = false;
}

CGUIPythonWindow::~CGUIPythonWindow(void)
{
  CloseHandle(m_actionEvent);
}

bool CGUIPythonWindow::OnAction(const CAction &action)
{
  // do the base class window first, and the call to python after this
  bool ret = CGUIWindow::OnAction(action);

  // workaround - for scripts which try to access the active control (focused) when there is none.
  // for example - the case when the mouse enters the screen.
  CGUIControl *pControl = GetFocusedControl();
  if (action.wID == ACTION_MOUSE && !pControl)
     return ret;

  if(pCallbackWindow)
  {
    PyXBMCAction* inf = new PyXBMCAction;
    inf->pObject = Action_FromAction(action);
    inf->pCallbackWindow = pCallbackWindow;

    // aquire lock?
    Py_AddPendingCall(Py_XBMC_Event_OnAction, inf);
    PulseActionEvent();
  }
  return ret;
}

bool CGUIPythonWindow::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_WINDOW_DEINIT:
    {
      m_gWindowManager.ShowOverlay(OVERLAY_STATE_SHOWN);
    }
    break;

    case GUI_MSG_WINDOW_INIT:
    {
      CGUIWindow::OnMessage(message);
      m_gWindowManager.ShowOverlay(OVERLAY_STATE_HIDDEN);
      return true;
    }
    break;

    case GUI_MSG_CLICKED:
    {
      int iControl=message.GetSenderId();
      if(pCallbackWindow)
      {
        PyXBMCAction* inf = new PyXBMCAction;
        inf->pObject = NULL;
        // find python control object with same iControl
        std::vector<Control*>::iterator it = ((Window*)pCallbackWindow)->vecControls.begin();
        while (it != ((Window*)pCallbackWindow)->vecControls.end())
        {
          Control* pControl = *it;
          if (pControl->iControlId == iControl)
          {
            inf->pObject = (PyObject*)pControl;
            Py_INCREF(inf->pObject);
            break;
          }
          ++it;
        }
        // did we find our control?
        if (inf->pObject)
        {
          // currently we only accept messages from a button or controllist with a select action
          if ((ControlList_CheckExact(inf->pObject) && (message.GetParam1() == ACTION_SELECT_ITEM || message.GetParam1() == ACTION_MOUSE_LEFT_CLICK)) ||
            ControlButton_CheckExact(inf->pObject) || ControlRadioButton_CheckExact(inf->pObject) ||
            ControlCheckMark_CheckExact(inf->pObject))
          {
            // create a new call and set it in the python queue
            inf->pCallbackWindow = pCallbackWindow;

            // aquire lock?
            Py_AddPendingCall(Py_XBMC_Event_OnControl, inf);
            PulseActionEvent();

            // return true here as we are handling the event
            return true;
          }
        }
      }
    }
    break;
  }

  return CGUIWindow::OnMessage(message);
}

void CGUIPythonWindow::SetCallbackWindow(PyObject *object)
{
  pCallbackWindow = object;
}

void CGUIPythonWindow::WaitForActionEvent(DWORD timeout)
{
  g_pythonParser.WaitForEvent(m_actionEvent, timeout);
  ResetEvent(m_actionEvent);
}

void CGUIPythonWindow::PulseActionEvent()
{
  SetEvent(m_actionEvent);
}


#ifdef _LINUX

/*
vector<PyXBMCAction*> g_actionQueue;
CRITICAL_SECTION g_critSection;

void Py_InitCriticalSection()
{
  static bool first_call = true;
  if (first_call) 
  {
    InitializeCriticalSection(&g_critSection);
    first_call = false;
  }
}

void Py_AddPendingActionCall(PyXBMCAction* inf)
{
  EnterCriticalSection(&g_critSection);
  g_actionQueue.push_back(inf);
  LeaveCriticalSection(&g_critSection);
}

void Py_MakePendingActionCalls()
{
  vector<PyXBMCAction*>::iterator iter;
  iter = g_actionQueue.begin();
  while (iter!=g_actionQueue.end())
  {    
    PyXBMCAction* arg = (*iter);
    EnterCriticalSection(&g_critSection);
    g_actionQueue.erase(iter);
    LeaveCriticalSection(&g_critSection);

    if (arg->type==0) 
    {
      Py_XBMC_Event_OnAction(arg);
    } else if (arg->type==1) {
      Py_XBMC_Event_OnControl(arg);
    }   

    EnterCriticalSection(&g_critSection);
    iter=g_actionQueue.begin();
    LeaveCriticalSection(&g_critSection);
  }
}

*/

#endif

/*
 * called from python library!
 */
int Py_XBMC_Event_OnControl(void* arg)
{
  if (arg != NULL)
  {
    PyXBMCAction* action = (PyXBMCAction*)arg;
    PyObject *ret = PyObject_CallMethod(action->pCallbackWindow, (char*)"onControl", (char*)"(O)", action->pObject);
    if (ret) {
       Py_DECREF(ret);
    }
    delete action;
  }
  return 0;
}

/*
 * called from python library!
 */
int Py_XBMC_Event_OnAction(void* arg)
{
  if (arg != NULL)
  {
    PyXBMCAction* action = (PyXBMCAction*)arg;
    Action *pAction= (Action *)action->pObject;

    PyObject *ret = PyObject_CallMethod(action->pCallbackWindow, (char*)"onAction", (char*)"(O)", pAction);
    if (ret) {
      Py_DECREF(ret);
    }
    else {
      CLog::Log(LOGERROR,"Exception in python script's onAction");
    	PyErr_Print();
    }
    delete action;
  }
  return 0;
}
