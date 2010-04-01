#include "GUIWindowWebBrowser.h"
#include "GUIDialogWebBrowserOSD.h"
#include "GUIWebBrowserControl.h"
#include "GUIWindowManager.h"
#include "Key.h"

CGUIWindowWebBrowser::CGUIWindowWebBrowser(void)
  : CGUIWindow(WINDOW_WEB_BROWSER, "WebBrowser.xml")
{
}

bool CGUIWindowWebBrowser::OnAction(const CAction &action)
{
  switch (action.GetID())
  {
  case ACTION_PREVIOUS_MENU:
    g_windowManager.PreviousWindow();
    return true;
  default:
    return CGUIWindow::OnAction(action);
  }
}

bool CGUIWindowWebBrowser::OnMessage(CGUIMessage &message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_CLICKED:
  {
    unsigned int iControl = message.GetSenderId();
    CGUIWebBrowserControl *pControl = (CGUIWebBrowserControl *)GetControl(10);

    if (iControl == 601)
      pControl->Back();
    else if (iControl == 604)
      pControl->Forward();
    else
      printf("%d\n", iControl);
    return true;
  }
  default:
    return CGUIWindow::OnMessage(message);
  }
}

EVENT_RESULT CGUIWindowWebBrowser::OnMouseEvent(const CPoint &point, const CMouseEvent &event)
{
  CGUIDialogWebBrowserOSD *pOSD = (CGUIDialogWebBrowserOSD *)g_windowManager.GetWindow(WINDOW_DIALOG_WEB_BROWSER_OSD);
  if (pOSD)
  {
      pOSD->DoModal();
  }
  return EVENT_RESULT_HANDLED;
}

