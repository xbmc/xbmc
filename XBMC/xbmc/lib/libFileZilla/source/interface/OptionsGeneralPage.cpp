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

// OptionsGeneralPage.cpp: Implementierungsdatei
//

#include "stdafx.h"
#include "filezilla server.h"
#include "OptionsGeneralPage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialogfeld COptionsGeneralPage 


COptionsGeneralPage::COptionsGeneralPage(CWnd* pParent /*=NULL*/)
	: CSAPrefsSubDlg(COptionsGeneralPage::IDD, pParent)
{
	//{{AFX_DATA_INIT(COptionsGeneralPage)
	m_MaxUsers = _T("");
	m_Port = _T("");
	m_Threadnum = _T("");
	m_Timeout = _T("");
	m_NoTransferTimeout = _T("");
	m_LoginTimeout = _T("");
	//}}AFX_DATA_INIT
}


void COptionsGeneralPage::DoDataExchange(CDataExchange* pDX)
{
	CSAPrefsSubDlg::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(COptionsGeneralPage)
	DDX_Text(pDX, IDC_MAXUSERS, m_MaxUsers);
	DDV_MaxChars(pDX, m_MaxUsers, 9);
	DDX_Text(pDX, IDC_PORT, m_Port);
	DDV_MaxChars(pDX, m_Port, 5);
	DDX_Text(pDX, IDC_THREADNUM, m_Threadnum);
	DDV_MaxChars(pDX, m_Threadnum, 2);
	DDX_Text(pDX, IDC_TIMEOUT, m_Timeout);
	DDV_MaxChars(pDX, m_Timeout, 4);
	DDX_Text(pDX, IDC_TRANSFERTIMEOUT, m_NoTransferTimeout);
	DDV_MaxChars(pDX, m_NoTransferTimeout, 4);
	DDX_Text(pDX, IDC_LOGINTIMEOUT, m_LoginTimeout);
	DDV_MaxChars(pDX, m_LoginTimeout, 4);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(COptionsGeneralPage, CSAPrefsSubDlg)
	//{{AFX_MSG_MAP(COptionsGeneralPage)
		// HINWEIS: Der Klassen-Assistent fügt hier Zuordnungsmakros für Nachrichten ein
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Behandlungsroutinen für Nachrichten COptionsGeneralPage 
