////////////////////////////////////////////////////
// Mainfrm.cpp  - definitions for the CMainFrame class

#include "mainfrm.h"
#include "resource.h"


CMainFrame::CMainFrame()
{
	//Set m_View as the view window of the frame
	SetView(m_View);
}

CMainFrame::~CMainFrame()
{
}

BOOL CMainFrame::OnCommand(WPARAM wParam, LPARAM lParam)
{
	// Process the messages from the Menu and Tool Bar
	switch (LOWORD(wParam))
	{
	case IDM_FILE_EXIT:
		// End the application
		::PostMessage(m_hWnd, WM_CLOSE, 0, 0);
		return TRUE;
	case IDM_HELP_ABOUT:
		// Display the help dialog
		OnHelp();
		return TRUE;
	case IDM_FILE_NEW:
		::MessageBox(NULL, _T("File New  ... Implemented later"), _T("Menu"), MB_OK);
		return TRUE;
	case IDM_FILE_OPEN:
		::MessageBox(NULL, _T("File Open  ... Implemented later"), _T("Menu"), MB_OK);
		return TRUE;
	case IDM_FILE_SAVE:
		::MessageBox(NULL, _T("File Save  ... Implemented later"), _T("Menu"), MB_OK);
		return TRUE;
	case IDM_FILE_SAVEAS:
		::MessageBox(NULL, _T("File SaveAs  ... Implemented later"), _T("Menu"), MB_OK);
		return TRUE;
	case IDM_FILE_PRINT:
		::MessageBox(NULL, _T("File Print  ... Implemented later"), _T("Menu"), MB_OK);
		return TRUE;
	}

	return FALSE;
}

void CMainFrame::SetupToolBar()
{
	// Set the Resource IDs for the toolbar buttons
	AddToolBarButton( IDM_FILE_NEW   );
	AddToolBarButton( IDM_FILE_OPEN  );
	AddToolBarButton( IDM_FILE_SAVE  );
	
	AddToolBarButton( 0 );				// Separator
	AddToolBarButton( IDM_EDIT_CUT );
	AddToolBarButton( IDM_EDIT_COPY );
	AddToolBarButton( IDM_EDIT_PASTE );
	
	AddToolBarButton( 0 );				// Separator
	AddToolBarButton( IDM_FILE_PRINT );
	
	AddToolBarButton( 0 );				// Separator
	AddToolBarButton( IDM_HELP_ABOUT );
}

LRESULT CMainFrame::WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
//	switch (uMsg)
//	{

//	} // switch (uMsg)

	return WndProcDefault(uMsg, wParam, lParam);
} // LRESULT CMainFrame::WndProc(...)


