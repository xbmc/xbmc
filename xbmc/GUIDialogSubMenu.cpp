
#include "stdafx.h"
#include "guidialogsubmenu.h"
#include "guiWindowManager.h"
#include "localizeStrings.h"
#include "application.h"
#include "Credits.h"
#include "util.h"

#define CONTROL_BTN_SHUTDOWN		10
#define CONTROL_BTN_DASHBOARD		11
#define CONTROL_BTN_REBOOT			12
#define CONTROL_BTN_CREDITS			13
#define CONTROL_BTN_ONLINE			14

CGUIDialogSubMenu::CGUIDialogSubMenu(void)
:CGUIDialog(0)
{
	ON_CLICK_MESSAGE(CONTROL_BTN_SHUTDOWN,	CGUIDialogSubMenu, OnClickShutdown);	
	ON_CLICK_MESSAGE(CONTROL_BTN_DASHBOARD,	CGUIDialogSubMenu, OnClickDashboard);	
	ON_CLICK_MESSAGE(CONTROL_BTN_REBOOT,	CGUIDialogSubMenu, OnClickReboot);	
	ON_CLICK_MESSAGE(CONTROL_BTN_CREDITS,	CGUIDialogSubMenu, OnClickCredits);	
	ON_CLICK_MESSAGE(CONTROL_BTN_ONLINE,	CGUIDialogSubMenu, OnClickOnlineGaming);	
}

CGUIDialogSubMenu::~CGUIDialogSubMenu(void)
{
}

void CGUIDialogSubMenu::OnAction(const CAction &action)
{
	if (action.wID == ACTION_CLOSE_DIALOG || action.wID == ACTION_PREVIOUS_MENU)
  {
		Close();
		return;
  }
	CGUIDialog::OnAction(action);
}

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
	CGUIDialogYesNo* dlgYesNo = (CGUIDialogYesNo*)m_gWindowManager.GetWindow(WINDOW_DIALOG_YES_NO);
	if (dlgYesNo)
	{
		dlgYesNo->SetHeading(13307);
		dlgYesNo->SetLine(0, 13308);
		dlgYesNo->SetLine(1, 13309);
		dlgYesNo->SetLine(2, "");
		dlgYesNo->DoModal(GetID());

    if(dlgYesNo->IsConfirmed())
      g_applicationMessenger.Restart();
    else
      g_applicationMessenger.Reset();
  }
}

void CGUIDialogSubMenu::OnClickCredits(CGUIMessage& aMessage)
{
	RunCredits();
}

void CGUIDialogSubMenu::OnClickOnlineGaming(CGUIMessage& aMessage)
{
	m_gWindowManager.ActivateWindow( WINDOW_BUDDIES );
}
