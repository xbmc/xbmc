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

// OptionsMisc.cpp: Implementierungsdatei
//

#include "stdafx.h"
#include "filezilla server.h"
#include "OptionsMiscPage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialogfeld COptionsMisc 


COptionsMiscPage::COptionsMiscPage(CWnd* pParent /*=NULL*/)
	: CSAPrefsSubDlg(COptionsMiscPage::IDD, pParent)
{
	//{{AFX_DATA_INIT(COptionsMiscPage)
	m_bDontShowPass = FALSE;
	m_bStartMinimized = FALSE;
	m_bEnableCustomPASV = FALSE;
	m_sCustomPASVString = "";
	m_sCustomPASVHelp = CString((LPCTSTR)IDC_OPTIONS_MISC_CUSTOM_PASV_TEXT);
	m_CustomPasvMinPort = _T("");
	m_CustomPasvMaxPort = _T("");
	m_TransferBufferSize = _T("");
	//}}AFX_DATA_INIT
}


void COptionsMiscPage::DoDataExchange(CDataExchange* pDX)
{
	CSAPrefsSubDlg::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(COptionsMiscPage)
	DDX_Control(pDX, IDC_OPTIONS_MISC_CUSTOM_PASV_MIN_PORT, m_cCustomPASVMinPort);
	DDX_Control(pDX, IDC_OPTIONS_MISC_CUSTOM_PASV_MAX_PORT, m_cCustomPASVMaxPort);
	DDX_Control(pDX, IDC_OPTIONS_MISC_CUSTOM_PASV_IP, m_cCustomPASVString);
	DDX_Check(pDX, IDC_OPTIONS_MISC_DONTSHOWPASS, m_bDontShowPass);
	DDX_Check(pDX, IDC_OPTIONS_MISC_STARTMINIMIZED, m_bStartMinimized);
	DDX_Check(pDX, IDC_OPTIONS_MISC_CUSTOM_PASV, m_bEnableCustomPASV);
	DDX_Text(pDX, IDC_OPTIONS_MISC_CUSTOM_PASV_IP, m_sCustomPASVString);
	DDX_Text(pDX, IDC_OPTIONS_MISC_CUSTOM_PASV_TEXT, m_sCustomPASVHelp);
	DDX_Text(pDX, IDC_OPTIONS_MISC_CUSTOM_PASV_MIN_PORT, m_CustomPasvMinPort);
	DDV_MaxChars(pDX, m_CustomPasvMinPort, 5);
	DDX_Text(pDX, IDC_OPTIONS_MISC_CUSTOM_PASV_MAX_PORT, m_CustomPasvMaxPort);
	DDV_MaxChars(pDX, m_CustomPasvMaxPort, 5);
	DDX_Text(pDX, IDC_OPTIONS_TRANSFERBUFFERSIZE, m_TransferBufferSize);
	DDV_MaxChars(pDX, m_TransferBufferSize, 6);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(COptionsMiscPage, CSAPrefsSubDlg)
	//{{AFX_MSG_MAP(COptionsMiscPage)
	ON_BN_CLICKED(IDC_OPTIONS_MISC_CUSTOM_PASV, OnOptionsMiscCustomPasv)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Behandlungsroutinen für Nachrichten COptionsMiscPage

BOOL COptionsMiscPage::OnInitDialog() 
{
	CSAPrefsSubDlg::OnInitDialog();
	
	if (m_bEnableCustomPASV)
	{
		m_cCustomPASVString.EnableWindow(TRUE);
		m_cCustomPASVMinPort.EnableWindow(TRUE);
		m_cCustomPASVMaxPort.EnableWindow(TRUE);
	}
	else
	{
		m_cCustomPASVString.EnableWindow(FALSE);
		m_cCustomPASVMinPort.EnableWindow(FALSE);
		m_cCustomPASVMaxPort.EnableWindow(FALSE);
	}	
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX-Eigenschaftenseiten sollten FALSE zurückgeben
}

void COptionsMiscPage::OnOptionsMiscCustomPasv() 
{
	UpdateData(TRUE);
	if (m_bEnableCustomPASV)
	{
		m_cCustomPASVString.EnableWindow(TRUE);
		m_cCustomPASVMinPort.EnableWindow(TRUE);
		m_cCustomPASVMaxPort.EnableWindow(TRUE);
	}
	else
	{
		m_cCustomPASVString.EnableWindow(FALSE);
		m_cCustomPASVMinPort.EnableWindow(FALSE);
		m_cCustomPASVMaxPort.EnableWindow(FALSE);
	}	
}
