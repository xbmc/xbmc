//////////////////////////////////////////////
// View.cpp
//  Definitions for the CView class

#include "view.h"


CView::CView()
{
}

void CView::DrawLine(int x, int y)
{
	CDC* pDC = GetDC();
	pDC->MoveTo(m_OldPt.x, m_OldPt.y);
	pDC->LineTo(x, y);
}

void CView::OnLButtonDown(LPARAM lParam)
{
 	// Capture mouse input.
 	SetCapture();

	m_OldPt.x = GET_X_LPARAM(lParam);
	m_OldPt.y = GET_Y_LPARAM(lParam);
}

void CView::OnLButtonUp(LPARAM lParam)
{
	{
		//Release the capture on the mouse
		ReleaseCapture();
	}
}

void CView::OnMouseMove(WPARAM wParam, LPARAM lParam)
{
	// hold down the left mouse button and move mouse to draw lines.
	if (wParam & MK_LBUTTON)
	{
		DrawLine(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		m_OldPt.x = GET_X_LPARAM(lParam);
		m_OldPt.y = GET_Y_LPARAM(lParam);
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

	case WM_DESTROY:
		//End the program when window is destroyed
		::PostQuitMessage(0);
		break;
	}

	//Use the default message handling for remaining messages
	return WndProcDefault(uMsg, wParam, lParam);
}
