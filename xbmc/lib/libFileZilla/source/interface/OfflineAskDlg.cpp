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

// OfflineAskDlg.cpp: Implementierungsdatei
//

#include "stdafx.h"
#include "filezilla server.h"
#include "OfflineAskDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialogfeld COfflineAskDlg 


COfflineAskDlg::COfflineAskDlg(CWnd* pParent /*=NULL*/)
	: CDialog(COfflineAskDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(COfflineAskDlg)
	m_nRadio = -1;
	//}}AFX_DATA_INIT
}


void COfflineAskDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(COfflineAskDlg)
	DDX_Radio(pDX, IDC_RADIO1, m_nRadio);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(COfflineAskDlg, CDialog)
	//{{AFX_MSG_MAP(COfflineAskDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Behandlungsroutinen für Nachrichten COfflineAskDlg 

BOOL COfflineAskDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	m_nRadio=0;
	UpdateData(FALSE);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX-Eigenschaftenseiten sollten FALSE zurückgeben
}
