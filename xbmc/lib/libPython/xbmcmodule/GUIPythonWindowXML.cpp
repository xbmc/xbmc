#include "../../../stdafx.h"
#include "GUIPythonWindowXML.h"
#include "guiwindow.h"
#include "pyutil.h"
#include "..\..\..\application.h"
#include "window.h"
#include "control.h"
#include "action.h"

using namespace PYXBMC;

CGUIPythonWindowXML::CGUIPythonWindowXML(DWORD dwId, CStdString strXML)
: CGUIWindow(dwId,strXML)
{
	pCallbackWindow = NULL;
	m_actionEvent = CreateEvent(NULL, true, false, NULL);
  m_loadOnDemand = false;
}

CGUIPythonWindowXML::~CGUIPythonWindowXML(void)
{
  	CloseHandle(m_actionEvent);
}

bool CGUIPythonWindowXML::OnAction(const CAction &action)
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

bool CGUIPythonWindowXML::OnMessage(CGUIMessage& message)
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
      PyXBMCAction* inf = new PyXBMCAction;
		  inf->pObject = NULL;
      // create a new call and set it in the python queue
		  inf->pCallbackWindow = pCallbackWindow;
      Py_AddPendingCall(Py_XBMC_Event_OnInit, inf);
			return true;
    }
		break;
  case GUI_MSG_SETFOCUS:
    {
      if (CGUIWindow::OnMessage(message))
      {
        // check if our focused control is one of our category buttons
        int iControl=message.GetControlId();
			  if(pCallbackWindow)
			  {
				  PyXBMCAction* inf = new PyXBMCAction;
				  inf->pObject = NULL;
          // create a new call and set it in the python queue
				  inf->pCallbackWindow = pCallbackWindow;
          inf->controlId = iControl;
				  // aquire lock?
				  Py_AddPendingCall(Py_XBMC_Event_OnFocus, inf);
				  PulseActionEvent();
        }
      }
    }
		case GUI_MSG_CLICKED:
    {
      int iControl=message.GetSenderId();
			if(pCallbackWindow && iControl != this->GetID()) 
			{
				PyXBMCAction* inf = new PyXBMCAction;
				inf->pObject = NULL;
        // create a new call and set it in the python queue
				inf->pCallbackWindow = pCallbackWindow;
        inf->controlId = iControl;
				// aquire lock?
				Py_AddPendingCall(Py_XBMC_Event_OnClick, inf);
				PulseActionEvent();
      }
        // Currently we assume that your not using addContorl etc so the vector list of controls has nothing so nothing to check for anyway
        /*
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
						Py_AddPendingCall(Py_XBMC_Event_OnClick, inf);
						PulseActionEvent();
					}
				}*/
		}
		break;
  }

  return CGUIWindow::OnMessage(message);
}

void CGUIPythonWindowXML::SetCallbackWindow(PyObject *object)
{
	pCallbackWindow = object;
}

void CGUIPythonWindowXML::WaitForActionEvent(DWORD timeout)
{
	WaitForSingleObject(m_actionEvent, timeout);
	ResetEvent(m_actionEvent);
}

void CGUIPythonWindowXML::PulseActionEvent()
{
	SetEvent(m_actionEvent);
}

void CGUIPythonWindowXML::Activate(DWORD dwParentId)
{
  // Currently not used
	m_dwParentWindowID = dwParentId;
	m_pParentWindow = m_gWindowManager.GetWindow(m_dwParentWindowID);
	if (!m_pParentWindow)
	{
		m_dwParentWindowID=0;
		return;
	}

	m_gWindowManager.RouteToWindow(this);

  // active this dialog...
  CGUIMessage msg(GUI_MSG_WINDOW_INIT,0,0);
  OnMessage(msg);
  m_bRunning = true;
}

int Py_XBMC_Event_OnClick(void* arg)
{
	if (arg != NULL)
	{
		PyXBMCAction* action = (PyXBMCAction*)arg;
		PyObject_CallMethod(action->pCallbackWindow, "onClick", "i", action->controlId);
		delete action;
	}
	return 0;
}

int Py_XBMC_Event_OnFocus(void* arg)
{
	if (arg != NULL)
	{
		PyXBMCAction* action = (PyXBMCAction*)arg;
		PyObject_CallMethod(action->pCallbackWindow, "onFocus", "i", action->controlId);
		delete action;
	}
	return 0;
}

int Py_XBMC_Event_OnInit(void* arg)
{
	if (arg != NULL)
	{
		PyXBMCAction* action = (PyXBMCAction*)arg;
		PyObject_CallMethod(action->pCallbackWindow, "onInit",""); //, "O", &self);
		delete action;
	}
	return 0;
}
