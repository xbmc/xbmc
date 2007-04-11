#include "../../../stdafx.h"
#include "GUIPythonWindowXMLDialog.h"

CGUIPythonWindowXMLDialog::CGUIPythonWindowXMLDialog(DWORD dwId, CStdString strXML, CStdString strFallBackPath)
: CGUIPythonWindowXML(dwId,strXML,strFallBackPath)
{
  m_bRunning = false;
  m_loadOnDemand = false;
}

CGUIPythonWindowXMLDialog::~CGUIPythonWindowXMLDialog(void)
{
}

void CGUIPythonWindowXMLDialog::Activate(DWORD dwParentId)
{
  m_gWindowManager.RouteToWindow(this);

  // active this dialog...
  CGUIMessage msg(GUI_MSG_WINDOW_INIT,0,0);
  OnMessage(msg);
  m_bRunning = true;
}

bool CGUIPythonWindowXMLDialog::OnMessage(CGUIMessage& message)
{
  switch(message.GetMessage())
  {
    case GUI_MSG_WINDOW_INIT:
    {
      CGUIPythonWindowXML::OnMessage(message);
      return true;
    }
    break;
    case GUI_MSG_SETFOCUS:
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
    break;
    case GUI_MSG_CLICKED:
    {
      int iControl=message.GetSenderId();
      if(pCallbackWindow && iControl && iControl != this->GetID()) // pCallbackWindow &&  != this->GetID())
      {
        CGUIControl* controlClicked = (CGUIControl*)this->GetControl(iControl);
        
        // The old python way used to check list AND SELECITEM method or  if its a button, checkmark.
        // Its done this way for now to allow other controls without a python version like togglebutton to still raise a onAction event
        if (controlClicked->GetControlType() == CGUIControl::GUICONTAINER_LIST &&  message.GetParam1() == ACTION_SELECT_ITEM  || controlClicked->GetControlType() != CGUIControl::GUICONTAINER_LIST)
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
      }
    }
    break;
  }

  // we do not message CGUIPythonWindow here..
  return CGUIWindow::OnMessage(message);
}

void CGUIPythonWindowXMLDialog::Close()
{
  CGUIMessage msg(GUI_MSG_WINDOW_DEINIT,0,0);
  OnMessage(msg);

  m_gWindowManager.RemoveDialog(GetID());
  m_bRunning = false;
}