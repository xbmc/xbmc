#include "stdafx.h"
#include "GUIDialogSubMenu.h"
#include "Application.h"
#include "Credits.h"
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
  CUtil::RunXBE(g_stSettings.szDashboard);
}

void CGUIDialogSubMenu::OnClickReboot(CGUIMessage& aMessage)
{
  if (CGUIDialogYesNo::ShowAndGetInput(13313, 13308, 13309, 0))
    g_applicationMessenger.Restart();
  else
    g_applicationMessenger.RestartApp();
}

void CGUIDialogSubMenu::OnClickCredits(CGUIMessage& aMessage)
{
  RunCredits();
}

void CGUIDialogSubMenu::OnClickOnlineGaming(CGUIMessage& aMessage)
{
  m_gWindowManager.ActivateWindow( WINDOW_BUDDIES );
}
