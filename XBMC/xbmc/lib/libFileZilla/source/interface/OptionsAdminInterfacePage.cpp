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

// OptionsAdminInterfacePage.cpp: Implementierungsdatei
//

#include "stdafx.h"
#include "filezilla server.h"
#include "OptionsAdminInterfacePage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialogfeld COptionsAdminInterfacePage 


COptionsAdminInterfacePage::COptionsAdminInterfacePage(CWnd* pParent /*=NULL*/)
	: CSAPrefsSubDlg(COptionsAdminInterfacePage::IDD, pParent)
{
	//{{AFX_DATA_INIT(COptionsAdminInterfacePage)
	m_Port = _T("");
	m_IpAddresses = _T("");
	m_IpBindings = _T("");
	m_NewPass = _T("");
	m_NewPass2 = _T("");
	m_bChangePass = FALSE;
	//}}AFX_DATA_INIT
}


void COptionsAdminInterfacePage::DoDataExchange(CDataExchange* pDX)
{
	CSAPrefsSubDlg::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(COptionsAdminInterfacePage)
	DDX_Control(pDX, IDC_OPTIONS_ADMININTERFACE_CHANGEPASS, m_cChangePass);
	DDX_Control(pDX, IDC_OPTIONS_ADMININTERFACE_NEWPASS2, m_cNewPass2);
	DDX_Control(pDX, IDC_OPTIONS_ADMININTERFACE_NEWPASS, m_cNewPass);
	DDX_Text(pDX, IDC_OPTIONS_ADMININTERFACE_PORT, m_Port);
	DDV_MaxChars(pDX, m_Port, 5);
	DDX_Text(pDX, IDC_OPTIONS_ADMININTERFACE_IPADDRESSES, m_IpAddresses);
	DDX_Text(pDX, IDC_OPTIONS_ADMININTERFACE_IPBINDINGS, m_IpBindings);
	DDX_Text(pDX, IDC_OPTIONS_ADMININTERFACE_NEWPASS, m_NewPass);
	DDX_Text(pDX, IDC_OPTIONS_ADMININTERFACE_NEWPASS2, m_NewPass2);
	DDX_Check(pDX, IDC_OPTIONS_ADMININTERFACE_CHANGEPASS, m_bChangePass);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(COptionsAdminInterfacePage, CSAPrefsSubDlg)
	//{{AFX_MSG_MAP(COptionsAdminInterfacePage)
	ON_BN_CLICKED(IDC_OPTIONS_ADMININTERFACE_CHANGEPASS, OnOptionsAdmininterfaceChangepass)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Behandlungsroutinen für Nachrichten COptionsAdminInterfacePage 

void COptionsAdminInterfacePage::OnOptionsAdmininterfaceChangepass() 
{
	UpdateData(TRUE);

	m_cNewPass.EnableWindow(m_bChangePass);
	m_cNewPass2.EnableWindow(m_bChangePass);	
}

BOOL COptionsAdminInterfacePage::OnInitDialog() 
{
	CSAPrefsSubDlg::OnInitDialog();
	
	if (m_NewPass != "")
	{
		m_NewPass = "";
		m_bChangePass = TRUE;
		UpdateData(FALSE);

		m_cChangePass.EnableWindow(FALSE);
	}
	m_cNewPass.EnableWindow(m_bChangePass);
	m_cNewPass2.EnableWindow(m_bChangePass);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX-Eigenschaftenseiten sollten FALSE zurückgeben
}
