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

#include "pyutil.h"
#include "GUIPythonWindow.h"
#include "window.h"
#include "control.h"
#include "action.h"
#include "guilib/GUIWindowManager.h"
#include "../XBPython.h"
#include "utils/log.h"
#include "threads/SingleLock.h"

using namespace PYXBMC;

PyXBMCAction::PyXBMCAction(void*& callback)
  : param(0), pCallbackWindow(NULL), pObject(NULL), controlId(0), type(0)
{
  // this is ugly, but we can't grab python lock
  // while holding gfx, that will potentially deadlock
  CSingleExit ex(g_graphicsContext);
  PyEval_AcquireLock();
  // callback is a reference to a pointer in the
  // window, that is controlled by the python lock.
  // callback can become null while we are trying
  // to grab python lock above, so anything using
  // this should allow that situation
  void* tmp = callback; // copy the referenced value
  pCallbackWindow = (PyObject*)tmp; // assign internally
  Py_XINCREF((PyObject*)callback);

  PyEval_ReleaseLock();
}

PyXBMCAction::~PyXBMCAction() {
  Py_XDECREF((PyObject*)pObject);
  Py_XDECREF((PyObject*)pCallbackWindow);
}

CGUIPythonWindow::CGUIPythonWindow(int id)
  : CGUIWindow(id, ""), m_actionEvent(true)
{
  pCallbackWindow = NULL;
  m_threadState = NULL;
  m_loadOnDemand = false;
  m_destroyAfterDeinit = false;
}

CGUIPythonWindow::~CGUIPythonWindow(void)
{
}

bool CGUIPythonWindow::OnAction(const CAction &action)
{
  // call the base class first, then call python
  bool ret = CGUIWindow::OnAction(action);

  // workaround - for scripts which try to access the active control (focused) when there is none.
  // for example - the case when the mouse enters the screen.
  CGUIControl *pControl = GetFocusedControl();
  if (action.IsMouse() && !pControl)
     return ret;

  if(pCallbackWindow)
  {
    PyXBMCAction* inf = new PyXBMCAction(pCallbackWindow);
    inf->pObject = Action_FromAction(action);

    // aquire lock?
    PyXBMC_AddPendingCall((PyThreadState*)m_threadState, Py_XBMC_Event_OnAction, inf);
    PulseActionEvent();
  }
  return ret;
}

bool CGUIPythonWindow::OnBack(int actionID)
{
  // if we have a callback window then python handles the closing
  if (!pCallbackWindow)
    return CGUIWindow::OnBack(actionID);
  return true;
}

bool CGUIPythonWindow::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_WINDOW_DEINIT:
    {
      g_windowManager.ShowOverlay(OVERLAY_STATE_SHOWN);
    }
    break;

    case GUI_MSG_WINDOW_INIT:
    {
      CGUIWindow::OnMessage(message);
      g_windowManager.ShowOverlay(OVERLAY_STATE_HIDDEN);
      return true;
    }
    break;

    case GUI_MSG_CLICKED:
    {
      int iControl=message.GetSenderId();
      if(pCallbackWindow)
      {
        PyXBMCAction* inf = new PyXBMCAction(pCallbackWindow);
        // find python control object with same iControl
        std::vector<Control*>::iterator it = ((PYXBMC::Window*)pCallbackWindow)->vecControls.begin();
        while (it != ((PYXBMC::Window*)pCallbackWindow)->vecControls.end())
        {
          Control* pControl = *it;
          if (pControl->iControlId == iControl)
          {
            inf->pObject = pControl;
            Py_INCREF((PyObject*)inf->pObject);
            break;
          }
          ++it;
        }
        // did we find our control?
        if (inf->pObject)
        {
          // currently we only accept messages from a button or controllist with a select action
          if ((ControlList_CheckExact((PyObject*)inf->pObject) && (message.GetParam1() == ACTION_SELECT_ITEM || message.GetParam1() == ACTION_MOUSE_LEFT_CLICK)) ||
            ControlButton_CheckExact((PyObject*)inf->pObject) || ControlRadioButton_CheckExact((PyObject*)inf->pObject) ||
            ControlCheckMark_CheckExact((PyObject*)inf->pObject))
          {
            // aquire lock?
            PyXBMC_AddPendingCall((PyThreadState*)m_threadState, Py_XBMC_Event_OnControl, inf);
            PulseActionEvent();

            // return true here as we are handling the event
            return true;
          }
        }

        // if we get here, we didn't add the action
        delete inf;
      }
    }
    break;
  }

  return CGUIWindow::OnMessage(message);
}

void CGUIPythonWindow::OnDeinitWindow(int nextWindowID /*= 0*/)
{
  CGUIWindow::OnDeinitWindow(nextWindowID);
  if (m_destroyAfterDeinit)
    g_windowManager.Delete(GetID());
}

void CGUIPythonWindow::SetDestroyAfterDeinit(bool destroy /*= true*/)
{
  m_destroyAfterDeinit = destroy;
}

void CGUIPythonWindow::SetCallbackWindow(void *state, void *object)
{
  pCallbackWindow = object;
  m_threadState   = state;
}

void CGUIPythonWindow::WaitForActionEvent(unsigned int timeout)
{
  g_pythonParser.WaitForEvent(m_actionEvent, timeout);
  m_actionEvent.Reset();
}

void CGUIPythonWindow::PulseActionEvent()
{
  m_actionEvent.Set();
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
  if(!arg)
    return 0;

  PyXBMCAction* action = (PyXBMCAction*)arg;
  if (action->pCallbackWindow)
  {
    PyXBMCAction* action = (PyXBMCAction*)arg;
    PyObject *ret = PyObject_CallMethod((PyObject*)action->pCallbackWindow, (char*)"onControl", (char*)"(O)", (PyObject*)action->pObject);
    if (ret) {
       Py_DECREF(ret);
    }
  }
  delete action;
  return 0;
}

/*
 * called from python library!
 */
int Py_XBMC_Event_OnAction(void* arg)
{
  if(!arg)
    return 0;

  PyXBMCAction* action = (PyXBMCAction*)arg;
  if (action->pCallbackWindow)
  {
    Action *pAction= (Action *)action->pObject;

    PyObject *ret = PyObject_CallMethod((PyObject*)action->pCallbackWindow, (char*)"onAction", (char*)"(O)", (PyObject*)pAction);
    if (ret) {
      Py_DECREF(ret);
    }
    else {
      CLog::Log(LOGERROR,"Exception in python script's onAction");
      PyErr_Print();
    }
  }
  delete action;
  return 0;
}
