
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
 return CGUIWindow::OnMessage(message);
}
