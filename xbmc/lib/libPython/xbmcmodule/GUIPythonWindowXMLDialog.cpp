#include "stdafx.h"
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


void CGUIPythonWindowXMLDialog::Close()
{
  CGUIMessage msg(GUI_MSG_WINDOW_DEINIT,0,0);
  OnMessage(msg);

  m_gWindowManager.RemoveDialog(GetID());
  m_bRunning = false;
}

