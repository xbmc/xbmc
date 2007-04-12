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

#include "stdafx.h"
#include "filezilla server.h"
#include "OptionsDlg.h"
#include "OptionsPage.h"
#include "OptionsLoggingPage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialogfeld COptionsLoggingPage 


COptionsLoggingPage::COptionsLoggingPage(COptionsDlg *pOptionsDlg, CWnd* pParent /*=NULL*/)
	: COptionsPage(pOptionsDlg, COptionsLoggingPage::IDD, pParent)
{
	//{{AFX_DATA_INIT(COptionsLoggingPage)
	m_bDelete = FALSE;
	m_DeleteTime = _T("14");
	m_bLimit = FALSE;
	m_LimitSize = _T("100");
	m_bEnable = FALSE;
	m_nLogtype = 0;
	//}}AFX_DATA_INIT
}


void COptionsLoggingPage::DoDataExchange(CDataExchange* pDX)
{
	COptionsPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(COptionsLoggingPage)
	DDX_Control(pDX, IDC_OPTIONS_LOGGING_LOGTYPE, m_cLogtype);
	DDX_Control(pDX, IDC_OPTIONS_LOGGING_LOGTYPE2, m_cLogtype2);
	DDX_Control(pDX, IDC_OPTIONS_LOGGING_LIMITSIZE, m_cLimitSize);
	DDX_Control(pDX, IDC_OPTIONS_LOGGING_LIMIT, m_cLimit);
	DDX_Control(pDX, IDC_OPTIONS_LOGGING_DELETETIME, m_cDeleteTime);
	DDX_Control(pDX, IDC_OPTIONS_LOGGING_DELETE, m_cDelete);
	DDX_Check(pDX, IDC_OPTIONS_LOGGING_DELETE, m_bDelete);
	DDX_Text(pDX, IDC_OPTIONS_LOGGING_DELETETIME, m_DeleteTime);
	DDV_MaxChars(pDX, m_DeleteTime, 3);
	DDX_Check(pDX, IDC_OPTIONS_LOGGING_LIMIT, m_bLimit);
	DDX_Text(pDX, IDC_OPTIONS_LOGGING_LIMITSIZE, m_LimitSize);
	DDV_MaxChars(pDX, m_LimitSize, 6);
	DDX_Check(pDX, IDC_OPTIONS_LOGGING_ENABLE, m_bEnable);
	DDX_Radio(pDX, IDC_OPTIONS_LOGGING_LOGTYPE, m_nLogtype);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(COptionsLoggingPage, COptionsPage)
	//{{AFX_MSG_MAP(COptionsLoggingPage)
	ON_BN_CLICKED(IDC_OPTIONS_LOGGING_ENABLE, OnOptionsLoggingChangedOption)
	ON_BN_CLICKED(IDC_OPTIONS_LOGGING_DELETE, OnOptionsLoggingChangedOption)
	ON_BN_CLICKED(IDC_OPTIONS_LOGGING_LIMIT, OnOptionsLoggingChangedOption)
	ON_BN_CLICKED(IDC_OPTIONS_LOGGING_LOGTYPE, OnOptionsLoggingChangedOption)
	ON_BN_CLICKED(IDC_OPTIONS_LOGGING_LOGTYPE2, OnOptionsLoggingChangedOption)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Behandlungsroutinen für Nachrichten COptionsLoggingPage 

BOOL COptionsLoggingPage::OnInitDialog() 
{
	COptionsPage::OnInitDialog();
	
	//Set ctrl states
	OnOptionsLoggingChangedOption();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX-Eigenschaftenseiten sollten FALSE zurückgeben
}

void COptionsLoggingPage::OnOptionsLoggingChangedOption() 
{
	UpdateData(TRUE);

	if (m_bEnable)
	{
		m_cLimit.EnableWindow(TRUE);
		m_cLimitSize.EnableWindow(m_bLimit);
		m_cLogtype.EnableWindow(TRUE);
		m_cLogtype2.EnableWindow(TRUE);

		if (m_nLogtype == 1)
		{
			m_cDelete.EnableWindow(TRUE);
			m_cDeleteTime.EnableWindow(m_bDelete);
		}
		else
		{
			m_cDelete.EnableWindow(FALSE);
			m_cDeleteTime.EnableWindow(FALSE);
		}
	}
	else
	{
		m_cLimit.EnableWindow(FALSE);
		m_cLimitSize.EnableWindow(FALSE);
		m_cLogtype.EnableWindow(FALSE);
		m_cLogtype2.EnableWindow(FALSE);
		m_cDelete.EnableWindow(FALSE);
		m_cDeleteTime.EnableWindow(FALSE);
	}
	
}

void COptionsLoggingPage::LoadData()
{
	m_bEnable = m_pOptionsDlg->GetOptionVal(OPTION_ENABLELOGGING);

	int nLimit = m_pOptionsDlg->GetOptionVal(OPTION_LOGLIMITSIZE);
	m_bLimit = nLimit ? TRUE : FALSE;
	if (nLimit)
		m_LimitSize.Format("%d", nLimit);

	m_nLogtype = m_pOptionsDlg->GetOptionVal(OPTION_LOGTYPE);

	int nDelete = m_pOptionsDlg->GetOptionVal(OPTION_LOGDELETETIME);
	m_bDelete = nDelete ? TRUE : FALSE;
	if (nDelete)
		m_DeleteTime.Format("%d", nDelete);

}

void COptionsLoggingPage::SaveData()
{
	m_pOptionsDlg->SetOption(OPTION_ENABLELOGGING, m_bEnable);
	m_pOptionsDlg->SetOption(OPTION_LOGLIMITSIZE, m_bLimit ? _ttoi(m_LimitSize) : 0);
	m_pOptionsDlg->SetOption(OPTION_LOGTYPE, m_nLogtype);
	m_pOptionsDlg->SetOption(OPTION_LOGDELETETIME, m_bDelete ? _ttoi(m_DeleteTime) : 0);
}