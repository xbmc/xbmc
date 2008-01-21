#include "stdafx.h"
#include "GUIPythonWindow.h"
#include "pyutil.h"
#include "../../../Application.h"
#include "window.h"
#include "control.h"
#include "action.h"

using namespace PYXBMC;

CGUIPythonWindow::CGUIPythonWindow(DWORD dwId)
: CGUIWindow(dwId, "")
{
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
            break;
          }
          ++it;
        }
        // did we find our control?
        if (inf->pObject)
        {
          // currently we only accept messages from a button or controllist with a select action
          if ((ControlList_CheckExact(inf->pObject) && (message.GetParam1() == ACTION_SELECT_ITEM || message.GetParam1() == ACTION_MOUSE_LEFT_CLICK))||
            ControlButton_CheckExact(inf->pObject) ||
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
  WaitForSingleObject(m_actionEvent, timeout);
  ResetEvent(m_actionEvent);
}

void CGUIPythonWindow::PulseActionEvent()
{
  SetEvent(m_actionEvent);
}

/*
 * called from python library!
 */
int Py_XBMC_Event_OnControl(void* arg)
{
  if (arg != NULL)
  {
    PyXBMCAction* action = (PyXBMCAction*)arg;

    PyObject_CallMethod(action->pCallbackWindow, "onControl", "O", action->pObject);

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

    PyObject_CallMethod(action->pCallbackWindow, "onAction", "O", action->pObject);

    delete action;
  }
  return 0;
}
