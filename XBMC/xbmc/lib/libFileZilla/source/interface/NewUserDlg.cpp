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

// NewUserDlg.cpp: Implementierungsdatei
//

#include "stdafx.h"
#include "filezilla server.h"
#include "NewUserDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialogfeld CNewUserDlg 


CNewUserDlg::CNewUserDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CNewUserDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CNewUserDlg)
	m_Name = _T("");
	//}}AFX_DATA_INIT
}


void CNewUserDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNewUserDlg)
	DDX_Control(pDX, IDOK, m_cOkCtrl);
	DDX_Control(pDX, IDC_NEWUSER_GROUP, m_cGroup);
	DDX_Text(pDX, IDC_NEWUSER_NAME, m_Name);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CNewUserDlg, CDialog)
	//{{AFX_MSG_MAP(CNewUserDlg)
	ON_EN_CHANGE(IDC_NEWUSER_NAME, OnChangeNewuserName)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Behandlungsroutinen für Nachrichten CNewUserDlg 

void CNewUserDlg::OnChangeNewuserName() 
{
	//Disable the OK button if the edit field is empty
	UpdateData(TRUE);
	m_cOkCtrl.EnableWindow(m_Name!=""?TRUE:FALSE);
}

BOOL CNewUserDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	m_cOkCtrl.EnableWindow(m_Name!=""?TRUE:FALSE);	

	m_cGroup.AddString("<none>");
	m_cGroup.SetCurSel(0);
	for (std::list<CString>::iterator iter=m_GroupList.begin(); iter!=m_GroupList.end(); iter++)
		m_cGroup.AddString(*iter);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX-Eigenschaftenseiten sollten FALSE zurückgeben
}

void CNewUserDlg::OnOK() 
{
	if (m_cGroup.GetCurSel() > 0)
		m_cGroup.GetLBText(m_cGroup.GetCurSel(), m_Group);
	CDialog::OnOK();
}
