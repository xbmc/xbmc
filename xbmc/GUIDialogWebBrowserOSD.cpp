#include "GUIDialogWebBrowserOSD.h"
#include "GUIWindowManager.h"
#include "Key.h"

CGUIDialogWebBrowserOSD::CGUIDialogWebBrowserOSD(void)
  : CGUIDialog(WINDOW_DIALOG_WEB_BROWSER_OSD, "WebBrowserOSD.xml")
{
}

bool CGUIDialogWebBrowserOSD::OnMessage(CGUIMessage &message)
{
  CGUIWindow *pWindow = (CGUIWindow *)g_windowManager.GetWindow(WINDOW_WEB_BROWSER);

  if (message.GetMessage() == GUI_MSG_CLICKED)
  {
    return pWindow->OnMessage(message);
  }
  else
  {
    return CGUIDialog::OnMessage(message);
  }
}

