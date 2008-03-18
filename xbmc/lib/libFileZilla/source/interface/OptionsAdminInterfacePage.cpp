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
#include "OptionsDlg.h"
#include "OptionsPage.h"
#include "OptionsAdminInterfacePage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialogfeld COptionsAdminInterfacePage 


COptionsAdminInterfacePage::COptionsAdminInterfacePage(COptionsDlg *pOptionsDlg, CWnd* pParent /*=NULL*/)
	: COptionsPage(pOptionsDlg, COptionsAdminInterfacePage::IDD, pParent)
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
	COptionsPage::DoDataExchange(pDX);
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


BEGIN_MESSAGE_MAP(COptionsAdminInterfacePage, COptionsPage)
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
	COptionsPage::OnInitDialog();
	
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

BOOL COptionsAdminInterfacePage::IsDataValid()
{
	if (!UpdateData(TRUE))
		return FALSE;

	if (_ttoi(m_Port) < 1 || _ttoi(m_Port) > 65535)
	{
		m_pOptionsDlg->ShowPage(this);
		GetDlgItem(IDC_OPTIONS_ADMININTERFACE_PORT)->SetFocus();
		AfxMessageBox(_T("The port for the admin interface has to be in the range from 1 to 65535."));
		return FALSE;
	}

	CString bindIPs = m_IpBindings;
	CString sub;
	std::list<CString> ipBindList;
	for (int i = 0; i<bindIPs.GetLength(); i++)
	{
		char cur = bindIPs[i];
		if ((cur<'0' || cur>'9') && cur!='.')
		{
			if (sub=="" && cur=='*')
			{
				ipBindList.clear();
				ipBindList.push_back("*");
				break;
			}

			if (sub != "")
			{
				//Parse IP
				SOCKADDR_IN sockAddr;
				memset(&sockAddr,0,sizeof(sockAddr));
			
				sockAddr.sin_family = AF_INET;
				sockAddr.sin_addr.s_addr = inet_addr(sub);
			
				if (sockAddr.sin_addr.s_addr != INADDR_NONE)
				{
					sub = inet_ntoa(sockAddr.sin_addr);
					std::list<CString>::iterator iter;
					for (iter = ipBindList.begin(); iter!=ipBindList.end(); iter++)
						if (*iter==sub)
							break;
					if (iter == ipBindList.end())
						ipBindList.push_back(sub);
				}
				sub = "";
			}
		}
		else
			sub += cur;
	}
	if (sub != "")
	{
		//Parse IP
		SOCKADDR_IN sockAddr;
		memset(&sockAddr,0,sizeof(sockAddr));
		
		sockAddr.sin_family = AF_INET;
		sockAddr.sin_addr.s_addr = inet_addr(sub);
		
		if (sockAddr.sin_addr.s_addr != INADDR_NONE)
		{
			sub = inet_ntoa(sockAddr.sin_addr);
			std::list<CString>::iterator iter;
			for (iter = ipBindList.begin(); iter!=ipBindList.end(); iter++)
				if (*iter==sub)
					break;
			if (iter == ipBindList.end())
				ipBindList.push_back(sub);
		}
		sub = "";
	}
	bindIPs = "";
	for (std::list<CString>::iterator iter = ipBindList.begin(); iter!=ipBindList.end(); iter++)
		if (*iter != "127.0.0.1")
			bindIPs += *iter + " ";

	bindIPs.TrimRight(" ");
					
	m_IpBindingsResult = bindIPs;

	if (m_bChangePass && m_IpAddresses!="")
	{
		if (m_NewPass.GetLength() < 6)
		{
			m_pOptionsDlg->ShowPage(this);
			GetDlgItem(IDC_OPTIONS_ADMININTERFACE_NEWPASS)->SetFocus();
			AfxMessageBox(_T("The admin password has to be at least 6 characters long,"));
			return FALSE;
		}
		if (m_NewPass != m_NewPass2)
		{
			m_pOptionsDlg->ShowPage(this);
			GetDlgItem(IDC_OPTIONS_ADMININTERFACE_NEWPASS)->SetFocus();
			AfxMessageBox(_T("Admin passwords do not match."));
			return FALSE;
		}
	}
	return TRUE;
}

void COptionsAdminInterfacePage::SaveData()
{
	m_pOptionsDlg->SetOption(OPTION_ADMINPORT, _ttoi(m_Port));
	m_pOptionsDlg->SetOption(OPTION_ADMINIPBINDINGS, m_IpBindingsResult);
	m_pOptionsDlg->SetOption(OPTION_ADMINIPADDRESSES, m_IpAddresses);
	if (m_bChangePass)
		m_pOptionsDlg->SetOption(OPTION_ADMINPASS, m_NewPass);
}

void COptionsAdminInterfacePage::LoadData()
{
	m_Port.Format(_T("%d"), m_pOptionsDlg->GetOptionVal(OPTION_ADMINPORT));
	m_IpBindings = m_pOptionsDlg->GetOption(OPTION_ADMINIPBINDINGS);
	m_IpAddresses = m_pOptionsDlg->GetOption(OPTION_ADMINIPADDRESSES);
	m_NewPass = m_pOptionsDlg->GetOption(OPTION_ADMINPASS);
}
