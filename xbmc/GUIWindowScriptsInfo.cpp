#include "stdafx.h"
#include "GUIWindowScriptsInfo.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define CONTROL_TEXTAREA 5

CGUIWindowScriptsInfo::CGUIWindowScriptsInfo(void)
    : CGUIDialog(0)
{}

CGUIWindowScriptsInfo::~CGUIWindowScriptsInfo(void)
{}

bool CGUIWindowScriptsInfo::OnAction(const CAction &action)
{
  if (action.wID == ACTION_PREVIOUS_MENU)
  {
    Close();
    return true;
  }
  if (action.wID == ACTION_SHOW_INFO)
  {
    // erase debug screen
    strInfo = "";
    CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), CONTROL_TEXTAREA);
    msg.SetLabel(strInfo);
    OnMessage(msg);
    return true;
  }
  return CGUIWindow::OnAction(action);
}

bool CGUIWindowScriptsInfo::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_DEINIT:
  {}
    break;

  case GUI_MSG_WINDOW_INIT:
    {
      CGUIWindow::OnMessage(message);
      return true;
    }
    break;

  case GUI_MSG_USER:
    {
      strInfo += message.GetLabel();
      CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), CONTROL_TEXTAREA);
      msg.SetLabel(strInfo);
      msg.SetParam1(5); // 5 pages max
      OnMessage(msg);
    }
    break;
  }
  return CGUIWindow::OnMessage(message);
}
