// FileZilla Server - a Windows ftp server

// Copyright (C) 2002 - Tim Kosse <tim.kosse@gmx.de>

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

// StatusView.cpp: Implementierungsdatei
//

#include "stdafx.h"
#include "FileZilla server.h"
#include "StatusView.h"
#include "StatusCtrl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CStatusView

IMPLEMENT_DYNCREATE(CStatusView, CView)

CStatusView::CStatusView()
{
	m_pStatusCtrl = new CStatusCtrl();
}

CStatusView::~CStatusView()
{
	delete m_pStatusCtrl;
}


BEGIN_MESSAGE_MAP(CStatusView, CView)
	//{{AFX_MSG_MAP(CStatusView)
	ON_WM_CREATE()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Zeichnung CStatusView 

void CStatusView::OnDraw(CDC* pDC)
{
	// ZU ERLEDIGEN: Code zum Zeichnen hier einfügen
}

/////////////////////////////////////////////////////////////////////////////
// Diagnose CStatusView

#ifdef _DEBUG
void CStatusView::AssertValid() const
{
	CView::AssertValid();
}

void CStatusView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}
#endif //_DEBUG




int CStatusView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{

	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;

		// Create the style
	DWORD dwStyle = WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_READONLY | WS_VSCROLL | ES_AUTOVSCROLL | ES_NOHIDESEL;
	
	// Create the list control.  Don't worry about specifying
	// correct coordinates.  That will be handled in OnSize()
	VERIFY(AfxInitRichEdit());
	BOOL bResult = m_pStatusCtrl->Create(dwStyle, CRect(1, 1, 10, 10), this, 0);

	return (bResult ? 0 : -1);
}


// Override OnSize to resize the control to match the view
void CStatusView::OnSize(UINT nType, int cx, int cy) 
{
	CView::OnSize(nType, cx, cy);
	
	if (::IsWindow(m_pStatusCtrl->m_hWnd))
	m_pStatusCtrl->MoveWindow(0, 0, cx, cy, TRUE);
}//OnSize


void CStatusView::ShowStatus(CString status,int type)
{
	m_pStatusCtrl->ShowStatus(status, type);
}