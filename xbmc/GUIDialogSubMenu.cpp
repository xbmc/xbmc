
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
	char* szBackslash = strrchr(g_stSettings.szDashboard,'\\');
	*szBackslash=0x00;
	char* szXbe = &szBackslash[1];

	char* szColon = strrchr(g_stSettings.szDashboard,':');
	*szColon=0x00;
	char* szDrive = g_stSettings.szDashboard;
	char* szDirectory = &szColon[1];

	char szDevicePath[1024];
	char szXbePath[1024];
	CIoSupport helper;
	helper.GetPartition( (LPCSTR) szDrive, szDevicePath);

	strcat(szDevicePath,szDirectory);
	wsprintf(szXbePath,"d:\\%s",szXbe);

	g_application.Stop();

	CUtil::LaunchXbe(szDevicePath,szXbePath,NULL);
}

void CGUIDialogSubMenu::OnClickReboot(CGUIMessage& aMessage)
{
  g_applicationMessenger.Restart();
}

void CGUIDialogSubMenu::OnClickCredits(CGUIMessage& aMessage)
{
	RunCredits();
}

void CGUIDialogSubMenu::OnClickOnlineGaming(CGUIMessage& aMessage)
{
	m_gWindowManager.ActivateWindow( WINDOW_BUDDIES );
}
