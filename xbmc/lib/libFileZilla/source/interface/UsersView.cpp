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

// UsersView.cpp: Implementierungsdatei
//

#include "stdafx.h"
#include "filezilla server.h"
#include "UsersView.h"
#include "userslistctrl.h"
#include "mainfrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CUsersView

IMPLEMENT_DYNCREATE(CUsersView, CView)

CUsersView::CUsersView()
{
}

CUsersView::~CUsersView()
{
}


BEGIN_MESSAGE_MAP(CUsersView, CView)
	//{{AFX_MSG_MAP(CUsersView)
	ON_WM_SIZE()
	ON_WM_CREATE()
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Zeichnung CUsersView 

void CUsersView::OnDraw(CDC* pDC)
{
	CDocument* pDoc = GetDocument();
	// ZU ERLEDIGEN: Code zum Zeichnen hier einfügen
}

/////////////////////////////////////////////////////////////////////////////
// Diagnose CUsersView

#ifdef _DEBUG
void CUsersView::AssertValid() const
{
	CView::AssertValid();
}

void CUsersView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// Behandlungsroutinen für Nachrichten CUsersView 

void CUsersView::OnSize(UINT nType, int cx, int cy) 
{
	CView::OnSize(nType, cx, cy);
		
    m_pListCtrl->SetWindowPos( NULL, 0, 0, cx, cy, SWP_NOZORDER | SWP_NOMOVE );
	
}

int CUsersView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	CMainFrame *pMainFrame = (CMainFrame *)AfxGetMainWnd();
	m_pListCtrl=new CUsersListCtrl(pMainFrame);
	m_pListCtrl->Create(LVS_NOSORTHEADER|LVS_REPORT|WS_CHILD | WS_VISIBLE | LVS_SHOWSELALWAYS|LVS_SHAREIMAGELISTS|WS_VSCROLL|LVS_NOCOLUMNHEADER,CRect(0,0,0,0),this,0);
	
	return 0;
}

void CUsersView::OnDestroy() 
{
	delete m_pListCtrl;
	m_pListCtrl=0;
	CView::OnDestroy();
	
	// TODO: Code für die Behandlungsroutine für Nachrichten hier einfügen
	
}
