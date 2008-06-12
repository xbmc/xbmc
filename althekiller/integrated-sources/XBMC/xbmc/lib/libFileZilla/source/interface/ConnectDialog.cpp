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

// ConnectDialog.cpp: Implementierungsdatei
//

#include "stdafx.h"
#include "filezilla server.h"
#include "ConnectDialog.h"
#include "Options.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialogfeld CConnectDialog 


CConnectDialog::CConnectDialog(COptions *pOptions)
	: CDialog(CConnectDialog::IDD, NULL)
{
	ASSERT(pOptions);
	m_pOptions = pOptions;
	//{{AFX_DATA_INIT(CConnectDialog)
	m_ServerAddress = _T("");
	m_ServerPassword = _T("");
	m_ServerPort = _T("");
	m_bAlways = FALSE;
	//}}AFX_DATA_INIT
}


void CConnectDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CConnectDialog)
	DDX_Text(pDX, IDC_CONNECTDIALOG_ADDRESS, m_ServerAddress);
	DDX_Text(pDX, IDC_CONNECTDIALOG_PASSWORD, m_ServerPassword);
	DDX_Text(pDX, IDC_CONNECTDIALOG_PORT, m_ServerPort);
	DDV_MaxChars(pDX, m_ServerPort, 5);
	DDX_Check(pDX, IDC_CONNECTDIALOG_ALWAYS, m_bAlways);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CConnectDialog, CDialog)
	//{{AFX_MSG_MAP(CConnectDialog)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Behandlungsroutinen für Nachrichten CConnectDialog 

BOOL CConnectDialog::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	m_ServerAddress	= m_pOptions->GetOption(IOPTION_LASTSERVERADDRESS);
	m_ServerPort.Format(_T("%d"), m_pOptions->GetOptionVal(IOPTION_LASTSERVERPORT));
	m_ServerPassword	= m_pOptions->GetOption(IOPTION_LASTSERVERPASS);
	m_bAlways = m_pOptions->GetOptionVal(IOPTION_ALWAYS);
	UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX-Eigenschaftenseiten sollten FALSE zurückgeben
}

void CConnectDialog::OnOK() 
{
	UpdateData();

	if (m_ServerAddress == _T(""))
	{
		AfxMessageBox(_T("Please enter a server address."), MB_ICONEXCLAMATION);
		return;
	}

	if (_ttoi(m_ServerPort)<1 || _ttoi(m_ServerPort)>65535)
	{
		AfxMessageBox(_T("Please enter a server port in the range from 1 to 65535."), MB_ICONEXCLAMATION);
		return;
	}

	m_pOptions->SetOption(IOPTION_LASTSERVERADDRESS, m_ServerAddress);
	m_pOptions->SetOption(IOPTION_LASTSERVERPORT, _ttoi(m_ServerPort));
	m_pOptions->SetOption(IOPTION_LASTSERVERPASS, m_ServerPassword);
	m_pOptions->SetOption(IOPTION_ALWAYS, m_bAlways);

	CDialog::OnOK();
}
