
#include "GUIWindowSettings.h"

CGUIWindowSettings::CGUIWindowSettings(void)
:CGUIWindow(0)
{
}

CGUIWindowSettings::~CGUIWindowSettings(void)
{
}


void CGUIWindowSettings::OnKey(const CKey& key)
{
  if (key.IsButton())
  {
    if ( key.GetButtonCode() == KEY_BUTTON_BACK  || key.GetButtonCode() == KEY_REMOTE_BACK)
    {
      m_gWindowManager.ActivateWindow(0); // back 2 home
      return;
    }
  }
  CGUIWindow::OnKey(key);
}

bool CGUIWindowSettings::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
    case GUI_MSG_WINDOW_INIT:
    {
     
    }
		break;

    case GUI_MSG_SETFOCUS:
    {
      
    }
    break;

    case GUI_MSG_CLICKED:
    {
      int iControl=message.GetSenderId();
    }
    break;
  }

  return CGUIWindow::OnMessage(message);
}
