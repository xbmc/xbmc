#include "GUIPythonWindow.h"
#include "graphiccontext.h"
#include "localizestrings.h"
#include "util.h"
#include "..\..\..\application.h"

CGUIPythonWindow::CGUIPythonWindow(DWORD dwId)
:CGUIWindow(dwId)
{
	pActionCallback = NULL;
	m_actionEvent = CreateEvent(NULL, false, false, "pythonActionEvent");
}

CGUIPythonWindow::~CGUIPythonWindow(void)
{
	CloseHandle(m_actionEvent);
}

void CGUIPythonWindow::OnAction(const CAction &action)
{
	if(pActionCallback)
	{
		PyXBMCAction* inf = new PyXBMCAction;
		inf->dwParam = action.wID;
		inf->pActionCallback = pActionCallback;

		// aquire lock?
		Py_AddPendingCall(Py_XBMC_Event_OnAction, inf);
		PulseActionEvent();
	}

	CGUIWindow::OnAction(action);
}

bool CGUIPythonWindow::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
		case GUI_MSG_WINDOW_DEINIT:
		{
			g_application.EnableOverlay();
		}
		break;

    case GUI_MSG_WINDOW_INIT:
    {
			CGUIWindow::OnMessage(message);
			g_application.DisableOverlay();
			return true;
    }
		break;

		case GUI_MSG_CLICKED:
    {
      int iControl=message.GetSenderId();
			if(pActionCallback)
			{
				PyXBMCAction* inf = new PyXBMCAction;
				inf->dwParam = iControl;
				inf->pActionCallback = pActionCallback;

				// aquire lock?
				Py_AddPendingCall(Py_XBMC_Event_OnControl, inf);
				PulseActionEvent();
			}
		}
		break;
  }

  return CGUIWindow::OnMessage(message);
}

void CGUIPythonWindow::SetActionCallback(PyObject *object)
{
	pActionCallback = object;
}

void CGUIPythonWindow::WaitForActionEvent(DWORD timeout)
{
	WaitForSingleObject(m_actionEvent, timeout);
}

void CGUIPythonWindow::PulseActionEvent()
{
	PulseEvent(m_actionEvent);
}

int Py_XBMC_Event_OnControl(void* arg)
{
	if (arg != NULL)
	{
		PyXBMCAction* action = (PyXBMCAction*)arg;

		PyObject_CallMethod(action->pActionCallback, "onControl", "i", action->dwParam);

		delete action;
	}
	return 0;
}

int Py_XBMC_Event_OnAction(void* arg)
{
	if (arg != NULL)
	{
		PyXBMCAction* action = (PyXBMCAction*)arg;

		PyObject_CallMethod(action->pActionCallback, "onAction", "l", action->dwParam);

		delete action;
	}
	return 0;
}
