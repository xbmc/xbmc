#include "MainFrm.h"
#include "resource.h"


CMainFrame::CMainFrame()
{
	// Set the Resource IDs for the toolbar buttons
	AddToolBarButton( 0 );				// Separator
	AddToolBarButton( IDM_BOLD  );
	AddToolBarButton( IDM_ITALIC);
	AddToolBarButton( IDM_UNDERLINE);
}

CMainFrame::~CMainFrame()
{
}

BOOL CMainFrame::OnCommand(WPARAM wParam, LPARAM lParam)
{
	// Respond to menu and toolbar selections
	switch (LOWORD(wParam))
	{
	// Respond to menu items
	case IDM_EDIT_CUT:
		TRACE(TEXT("Cut ... Menu item\n"));
		return TRUE;
	case IDM_EDIT_COPY:
		TRACE(TEXT("Copy ... Menu item\n"));
		return TRUE;
    case IDM_EDIT_PASTE:
		TRACE(TEXT("PASTE ... Menu item\n"));
		return TRUE;
	case IDM_EDIT_CLEAR:
		TRACE(TEXT("CLEAR ... Menu item\n"));
		return TRUE;
	case IDM_HELP_ABOUT:
		{
			CDialog HelpDialog(IDW_ABOUT, this);
			HelpDialog.DoModal();
		}
		return TRUE;

	// Respond to ToolBar buttons
	case IDM_BOLD:
		TRACE(TEXT("Bold button pressed\n"));
		return TRUE;
	case IDM_ITALIC:
		TRACE(TEXT("Italic button pressed\n"));
		return TRUE;
	case IDM_UNDERLINE:
		TRACE(TEXT("Underline button pressed\n"));
		return TRUE;

	// Respond to the accelerator key
	case IDW_QUIT:
		SendMessage(WM_CLOSE, 0L, 0L);
		return TRUE;
	}

	return FALSE;
}

void CMainFrame::OnDraw(CDC* pDC)
{
	CRect rc = GetViewRect();
	tString tsHello = LoadString(IDS_HELLO);
	pDC->DrawText(tsHello.c_str(), tsHello.length(), rc, DT_SINGLELINE | DT_VCENTER | DT_CENTER);
}


void CMainFrame::OnInitialUpdate()
{
	// Startup code goes here
	TRACE(TEXT("Frame Created\n"));
}

LRESULT CMainFrame::WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
//	switch (uMsg)
//	{
//
//	}

	// Pass unhandled messages on for default processing
	return WndProcDefault(uMsg, wParam, lParam);
}
