//////////////////////////////////////////////
// View.cpp
//  Definitions for the CView class

#include "view.h"
#include "GDI.h"
#include "file.h"
#include "resource.h"

using namespace std;

CView::CView() : m_PenColor(RGB(0,0,0))
{
	m_Brush.CreateSolidBrush(RGB(255,255,230));
}

CView::~CView()
{
}

void CView::ClearPoints()
{
	m_points.clear();
	Invalidate();
}

void CView::DrawLine(int x, int y)
{
	CClientDC dcClient(this);
	dcClient.CreatePen(PS_SOLID, 1, m_points.back().color);
	dcClient.MoveTo(m_points.back().x, m_points.back().y);
	dcClient.LineTo(x, y);
}

void CView::OnDraw(CDC* pDC)
{
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

void CView::PreCreate(CREATESTRUCT &cs)
{
	// Set the extra style to provide a sunken effect
	cs.dwExStyle = WS_EX_CLIENTEDGE;
}

void CView::PreRegisterClass(WNDCLASS &wc)
{
	// Set the background brush, class name and cursor
	wc.hbrBackground = m_Brush;
	wc.lpszClassName = _T("Scribble Window");
	wc.hCursor = ::LoadCursor(GetApp()->GetInstanceHandle(), MAKEINTRESOURCE(IDC_CURSOR1));
}

BOOL CView::FileOpen(LPCTSTR szFilename)
{
	// empty the PlotPoint vector
	m_points.clear();
	DWORD nBytesRead;
	BOOL bResult = FALSE;

	// Create a handle to the file
	CFile File;
	if (File.Open(szFilename, OPEN_EXISTING))
	{
		do
		{
			PlotPoint pp;
			nBytesRead = File.Read(&pp, sizeof(PlotPoint));
			if (nBytesRead == sizeof(PlotPoint))
				m_points.push_back(pp);	

		} while (nBytesRead == sizeof(PlotPoint));

		if ((0 != nBytesRead) || (m_points.empty()))
		{
			// Failed to read all of the file
			m_points.clear();
			::MessageBox (0, _T("Invalid data in file"), _T("Error"), MB_ICONEXCLAMATION | MB_OK);
		}
		else
			bResult = TRUE;

	}
	else
	{
		tString tsErrMsg = _T("Failed to open file ");
		tsErrMsg += szFilename;
		::MessageBox (0, tsErrMsg.c_str(), _T("Error"), MB_ICONEXCLAMATION | MB_OK);
	}

	Invalidate();
	return bResult;
}

BOOL CView::FileSave(LPCTSTR szFilename)
{
	BOOL bResult = TRUE;
	CFile hFile;
	if (!hFile.Open(szFilename, CREATE_ALWAYS))
	{
		::MessageBox (0, _T("Failed to open file for writing"), _T("Error"), MB_ICONEXCLAMATION | MB_OK);
		bResult = FALSE;
	}
	
	if (bResult)
	{
		// Write the file
		for (size_t i = 0; i < m_points.size(); ++i)
		{
			if (!hFile.Write(&m_points[i], sizeof(PlotPoint)))
			{
				::MessageBox (0, _T("Error while writing to file"), _T("Error"), MB_ICONEXCLAMATION | MB_OK);
				bResult = FALSE;
				break;
			}
		}

		// Verify file length
		if (hFile.GetLength() != m_points.size() * sizeof(PlotPoint))
		{
			::MessageBox (0, _T("Error while writing to file"), _T("Error"), MB_ICONEXCLAMATION | MB_OK);
			bResult = FALSE;
		}
	}

	return bResult;
}

void CView::SetPen(COLORREF color)
{
	m_PenColor = color;
}

void CView::StorePoint(int x, int y, bool PenDown)
{
	PlotPoint P1;
	P1.x = x;
	P1.y = y;
	P1.PenDown = PenDown;
	P1.color = m_PenColor;

	m_points.push_back(P1); //Add the point to the vector
}

void CView::OnLButtonDown(LPARAM lParam)
{
 	// Capture mouse input.
 	SetCapture();

	StorePoint(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), true);
}

void CView::OnLButtonUp(LPARAM lParam)
{
	{
		//Release the capture on the mouse
		ReleaseCapture();

		StorePoint(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), false);
	}
}

void CView::OnMouseMove(WPARAM wParam, LPARAM lParam)
{
	// hold down the left mouse button and move mouse to draw lines.
	if ( (wParam & MK_LBUTTON) && (GetCapture() == this) )
	{
		DrawLine(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		StorePoint(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), true);
	}
}

LRESULT CView::WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_LBUTTONDOWN:
		OnLButtonDown(lParam);
		break;

	case WM_MOUSEMOVE:
		OnMouseMove(wParam, lParam);
        break;

    case WM_LBUTTONUP:
		OnLButtonUp(lParam);
		break;
	}

	//Use the default message handling for remaining messages
	return WndProcDefault(uMsg, wParam, lParam);
}

