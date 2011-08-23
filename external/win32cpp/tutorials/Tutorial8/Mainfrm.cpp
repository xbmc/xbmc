////////////////////////////////////////////////////
// Mainfrm.cpp  - definitions for the CMainFrame class

#include "mainfrm.h"
#include "resource.h"


CMainFrame::CMainFrame()
{
	// Set m_View as the view window of the frame
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
	case IDM_FILE_NEW:
		m_View.ClearPoints();
		m_PathName = _T("");
		return TRUE;
	case IDM_FILE_OPEN:
		OnFileOpen();
		return TRUE;
	case IDM_FILE_SAVE:
		OnFileSave();
		return TRUE;
	case IDM_FILE_SAVEAS:
		OnFileSaveAs();
		return TRUE;
	case IDM_FILE_PRINT:
		::MessageBox(NULL, _T("File Print Implemented Later"), _T("Menu"), MB_OK);
		return TRUE;
	case IDM_PEN_RED:
		m_View.SetPen(RGB(255,0,0));
		return TRUE;
	case IDM_PEN_BLUE:
		m_View.SetPen(RGB(0,0,255));
		return TRUE;
	case IDM_PEN_GREEN:
		m_View.SetPen(RGB(0,196,0));
		return TRUE;
	case IDM_PEN_BLACK:
		m_View.SetPen(RGB(0,0,0));
		return TRUE;
	case IDM_HELP_ABOUT:
		OnHelp();
		return TRUE;
	case IDM_FILE_EXIT:
		::PostMessage(m_hWnd, WM_CLOSE, 0, 0);
		return TRUE;
	}

	return FALSE;
}

void CMainFrame::OnFileOpen()
{
	CFile File;
	CString str = File.OpenFileDialog(0, OFN_FILEMUSTEXIST, _T("Scribble Files (*.dat)\0*.dat\0\0"), this);

	if (!str.IsEmpty())
	{
		// Retrieve the PlotPoint data
		if (m_View.FileOpen(str))
		{
			// Save the filename
			m_PathName = str;
			AddMRUEntry(str);
		}
		else
			m_PathName=_T("");
	}
}

void CMainFrame::OnFileSave()
{
	if (m_PathName == _T(""))
		OnFileSaveAs();
	else
		m_View.FileSave(m_PathName.c_str());
}

void CMainFrame::OnFileSaveAs()
{
	CFile File;
	CString str = File.SaveFileDialog(0, OFN_OVERWRITEPROMPT, _T("Scribble Files (*.dat)\0*.dat\0\0"), _T("dat"), this);

	// Store the PlotPoint data in the file
	if (!str.IsEmpty())
	{
		m_PathName = str;

		// Save the file name
		m_View.FileSave(str);
		AddMRUEntry(str);
	}
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
	AddToolBarButton ( IDM_PEN_RED    );	
	AddToolBarButton ( IDM_PEN_BLUE   );
	AddToolBarButton ( IDM_PEN_GREEN  );
	AddToolBarButton ( IDM_PEN_BLACK  );
	AddToolBarButton ( IDM_HELP_ABOUT );
}

LRESULT CMainFrame::WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
//	switch (uMsg)
//	{

//	} // switch (uMsg)

	return WndProcDefault(uMsg, wParam, lParam);
} // LRESULT CMainFrame::WndProc(...)

