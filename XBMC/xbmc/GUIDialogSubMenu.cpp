#include "stdafx.h"
#include "GUIDialogSubMenu.h"
#include "Application.h"
#ifdef HAS_CREDITS
#include "Credits.h"
#endif
#include "Util.h"


#define CONTROL_BTN_SHUTDOWN  10
#define CONTROL_BTN_DASHBOARD  11
#define CONTROL_BTN_REBOOT   12
#define CONTROL_BTN_CREDITS   13
#define CONTROL_BTN_ONLINE   14

CGUIDialogSubMenu::CGUIDialogSubMenu(void)
    : CGUIDialog(WINDOW_DIALOG_SUB_MENU, "DialogSubMenu.xml")
{
  ON_CLICK_MESSAGE(CONTROL_BTN_SHUTDOWN, CGUIDialogSubMenu, OnClickShutdown);
  ON_CLICK_MESSAGE(CONTROL_BTN_DASHBOARD, CGUIDialogSubMenu, OnClickDashboard);
  ON_CLICK_MESSAGE(CONTROL_BTN_REBOOT, CGUIDialogSubMenu, OnClickReboot);
  ON_CLICK_MESSAGE(CONTROL_BTN_CREDITS, CGUIDialogSubMenu, OnClickCredits);
  ON_CLICK_MESSAGE(CONTROL_BTN_ONLINE, CGUIDialogSubMenu, OnClickOnlineGaming);
}

CGUIDialogSubMenu::~CGUIDialogSubMenu(void)
{}

bool CGUIDialogSubMenu::OnMessage(CGUIMessage &message)
{
  bool bRet = CGUIDialog::OnMessage(message);
  if (message.GetMessage() == GUI_MSG_CLICKED)
  {
    // someone has been clicked - deinit...
    Close();
    return true;
  }
  return bRet;
}

void CGUIDialogSubMenu::OnClickShutdown(CGUIMessage& aMessage)
{
  g_applicationMessenger.Shutdown();
}

void CGUIDialogSubMenu::OnClickDashboard(CGUIMessage& aMessage)
{
#ifdef HAS_XBOX_HARDWARE
  CUtil::RunXBE(g_guiSettings.GetString("myprograms.dashboard").c_str());
#endif
}

void CGUIDialogSubMenu::OnClickReboot(CGUIMessage& aMessage)
{
  bool bCanceled;
  bool bResult = CGUIDialogYesNo::ShowAndGetInput(13313, 13308, 13309, 0, bCanceled);
  if (bCanceled)
    return;

  if (bResult)
    g_applicationMessenger.Restart();
  else
    g_applicationMessenger.RestartApp();
}

void CGUIDialogSubMenu::OnClickCredits(CGUIMessage& aMessage)
{
#ifdef HAS_CREDITS
  RunCredits();
#endif
}

void CGUIDialogSubMenu::OnClickOnlineGaming(CGUIMessage& aMessage)
{
  m_gWindowManager.ActivateWindow( WINDOW_BUDDIES );
}
