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

// EnterSomething.cpp: Implementierungsdatei
//

#include "stdafx.h"
#include "filezilla server.h"
#include "EnterSomething.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialogfeld CEnterSomething 


//CEnterSomething::CEnterSomething(CWnd* pParent /*=NULL*/)
/*	: CDialog(CEnterSomething::IDD, pParent)
{
}*/

CEnterSomething::CEnterSomething(UINT nID):CDialog(CEnterSomething::IDD, 0)
{
	//{{AFX_DATA_INIT(CEnterSomething)
	m_String = _T("");
	//}}AFX_DATA_INIT
	m_nID=nID;
	
}

void CEnterSomething::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CEnterSomething)
	DDX_Control(pDX, IDC_TEXT, m_cText);
	DDX_Control(pDX, IDOK, m_OkCtrl);
	DDX_Text(pDX, IDC_EDIT1, m_String);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CEnterSomething, CDialog)
	//{{AFX_MSG_MAP(CEnterSomething)
	ON_EN_CHANGE(IDC_EDIT1, OnChangeEdit1)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Behandlungsroutinen für Nachrichten CEnterSomething 

void CEnterSomething::OnOK() 
{
	UpdateData(TRUE);
	//Check if a value was entered
	if (m_String=="")
		AfxMessageBox(IDS_ERRORMSG_ENTERSTRING, MB_ICONEXCLAMATION);
	else
		CDialog::OnOK();
}

void CEnterSomething::OnChangeEdit1() 
{
	//Disable the OK button if the edit field is empty
	UpdateData(TRUE);
	m_OkCtrl.EnableWindow(m_String!=""?TRUE:FALSE);
}

BOOL CEnterSomething::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	//Load and set the window title
	CString title;
	title.LoadString(m_nID);
	int pos=title.Find(_T("|"));
	SetWindowText(title.Left(pos));
	m_cText.SetWindowText(title.Mid(pos+1));
	//Disable the OK button if the edit field is empty
	m_OkCtrl.EnableWindow(m_String!=""?TRUE:FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX-Eigenschaftenseiten sollten FALSE zurückgeben
}
