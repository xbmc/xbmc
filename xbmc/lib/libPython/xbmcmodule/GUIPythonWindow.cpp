#include "../../../stdafx.h"
#include "GUIPythonWindow.h"
#include "pyutil.h"
#include "..\..\..\application.h"
#include "window.h"
#include "control.h"

using namespace PYXBMC;

CGUIPythonWindow::CGUIPythonWindow(DWORD dwId)
:CGUIWindow(dwId)
{
	pCallbackWindow = NULL;
	m_actionEvent = CreateEvent(NULL, true, false, "pythonActionEvent");
}

CGUIPythonWindow::~CGUIPythonWindow(void)
{
	CloseHandle(m_actionEvent);
}

void CGUIPythonWindow::OnAction(const CAction &action)
{
	if(pCallbackWindow)
	{
		PyXBMCAction* inf = new PyXBMCAction;
		inf->dwParam = action.wID;
		inf->pCallbackWindow = pCallbackWindow;

		// aquire lock?
		Py_AddPendingCall(Py_XBMC_Event_OnAction, inf);
		PulseActionEvent();
	}

	CGUIWindow::OnAction(action);
}

bool CGUIPythonWindow::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
		case GUI_MSG_WINDOW_DEINIT:
		{
      g_graphicsContext.SetOverlay(true);
		}
		break;

    case GUI_MSG_WINDOW_INIT:
    {
			CGUIWindow::OnMessage(message);
      g_graphicsContext.SetOverlay(false);
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
							ControlButton_CheckExact(inf->pObject)||
                            ControlCheckMark_CheckExact(inf->pObject))
					{
						// create a new call and set it in the python queue
						inf->dwParam = iControl;
						inf->pCallbackWindow = pCallbackWindow;

						// aquire lock?
						Py_AddPendingCall(Py_XBMC_Event_OnControl, inf);
						PulseActionEvent();
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

		PyObject_CallMethod(action->pCallbackWindow, "onAction", "l", action->dwParam);

		delete action;
	}
	return 0;
}
