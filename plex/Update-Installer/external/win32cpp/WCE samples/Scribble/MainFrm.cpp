#include "MainFrm.h"
#include "resource.h"



CMainFrame::CMainFrame() : m_PenColor(RGB(0,0,0))
{
	// Set the Resource IDs for the toolbar buttons
	AddToolBarButton( 0 );			// Separator
	AddToolBarButton( IDM_RED  );
	AddToolBarButton( IDM_BLUE );
	AddToolBarButton( IDM_GREEN );
	AddToolBarButton( IDM_BLACK );
}

void CMainFrame::DrawLine(short x, short y)
{
	CDC* pDC = GetDC();
	pDC->CreatePen(PS_SOLID, 1, m_points.back().color);
	pDC->MoveTo(m_points.back().x, m_points.back().y);
	pDC->LineTo(x, y);
}

BOOL CMainFrame::OnCommand(WPARAM wParam, LPARAM /*lParam*/)
{
	// Respond to menu and toolbar selections
	switch (LOWORD(wParam))
	{
	// Respond to menu items
	case IDM_NEW:
		m_points.clear();
		Invalidate();
		return TRUE;
	case IDM_HELP_ABOUT:
		{
			CDialog HelpDialog(IDW_ABOUT, this);
			HelpDialog.DoModal();
		}
		return TRUE;

	// Respond to ToolBar buttons
	case IDM_RED:
		m_PenColor = RGB(255, 0, 0);
		TRACE(_T("Red Pen Selected \n"));
		return TRUE;
	case IDM_BLUE:
		m_PenColor = RGB(0, 0, 255);
		TRACE(_T("Blue Pen Selected \n"));
		return TRUE;
	case IDM_GREEN:
		m_PenColor = RGB(0, 191, 0);
		TRACE(_T("Green Pen Selected \n"));
		return TRUE;
	case IDM_BLACK:
		m_PenColor = RGB(0, 0, 0);
		TRACE(_T("Black Pen Selected \n"));
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
	// Redraw our client area
	if (m_points.size() > 0)
	{
		bool bDraw = false;  //Start with the pen up
		for (unsigned int i = 0 ; i < m_points.size(); i++)
		{
			pDC->CreatePen(PS_SOLID, 1, m_points[i].color);
			if (bDraw)
				pDC->LineTo(m_points[i].x, m_points[i].y);
			else
				pDC->MoveTo(m_points[i].x, m_points[i].y);
			
			bDraw = m_points[i].PenDown;
		}
	}
}

void CMainFrame::OnInitialUpdate()
{
	// Startup code goes here
}

void CMainFrame::OnLButtonDown(WPARAM /*wParam*/, LPARAM lParam)
{
	// Capture mouse input.
	SetCapture();

	StorePoint(LOWORD(lParam), HIWORD(lParam), true);
}

void CMainFrame::OnLButtonUp(WPARAM /*wParam*/, LPARAM lParam)
{
	//Release the capture on the mouse
	ReleaseCapture();

	StorePoint(LOWORD(lParam), HIWORD(lParam), false);
}

void CMainFrame::OnMouseMove(WPARAM wParam, LPARAM lParam)
{
	// hold down the left mouse button and move mouse to draw lines.
	if (wParam & MK_LBUTTON)
	{	
		TCHAR str[80];
		::wsprintf(str, TEXT("Draw Point:  %hd, %hd\n"), LOWORD(lParam), HIWORD(lParam));
		TRACE(str);

		DrawLine(LOWORD(lParam), HIWORD(lParam));
		StorePoint(LOWORD(lParam), HIWORD(lParam), true);
	}
}

void CMainFrame::SetPen(COLORREF color)
{
	m_PenColor = color;
}

void CMainFrame::StorePoint(int x, int y, bool PenDown)
{
	PlotPoint P1;
	P1.x = x;
	P1.y = y;
	P1.PenDown = PenDown;
	P1.color = m_PenColor;

	m_points.push_back(P1); //Add the point to the vector
}

LRESULT CMainFrame::WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// handle left mouse button up/down and mouse move messages
	// a seperate function for each case keeps the code tidy.
	switch (uMsg)
	{
	case WM_LBUTTONDOWN:
		OnLButtonDown(wParam, lParam);
		break;

	case WM_MOUSEMOVE:
		OnMouseMove(wParam, lParam);
        break;

    case WM_LBUTTONUP:
		OnLButtonUp(wParam, lParam);
		break;
	}

	// Pass unhandled messages on to WndProcDefault
	return WndProcDefault(uMsg, wParam, lParam);
}


