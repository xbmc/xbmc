#include "stdafx.h"
#include "GUIPythonWindowDialog.h"
#include "../../../../guilib/GUIWindowManager.h"

CGUIPythonWindowDialog::CGUIPythonWindowDialog(DWORD dwId)
:CGUIPythonWindow(dwId)
{
  m_bRunning = false;
  m_loadOnDemand = false;
}

CGUIPythonWindowDialog::~CGUIPythonWindowDialog(void)
{
}

void CGUIPythonWindowDialog::Activate(DWORD dwParentId)
{
  m_gWindowManager.RouteToWindow(this);

  // active this dialog...
  CGUIMessage msg(GUI_MSG_WINDOW_INIT,0,0);
  OnMessage(msg);
  m_bRunning = true;
}

bool CGUIPythonWindowDialog::OnMessage(CGUIMessage& message)
{
  switch(message.GetMessage())
  {
    case GUI_MSG_WINDOW_INIT:
    {
      CGUIWindow::OnMessage(message);
      return true;
    }
    break;

    case GUI_MSG_CLICKED:
    {
      return CGUIPythonWindow::OnMessage(message);
    }
    break;
  }

  // we do not message CGUIPythonWindow here..
  return CGUIWindow::OnMessage(message);
}

void CGUIPythonWindowDialog::Close()
{
  CGUIMessage msg(GUI_MSG_WINDOW_DEINIT,0,0);
  OnMessage(msg);

  m_gWindowManager.RemoveDialog(GetID());
  m_bRunning = false;
}
