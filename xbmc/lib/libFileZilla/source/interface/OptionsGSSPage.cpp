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

// OptionsGSSPage.cpp : implementation file
//

#include "stdafx.h"
#include "filezilla server.h"
#include "OptionsDlg.h"
#include "OptionsPage.h"
#include "OptionsGSSPage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// COptionsGSSPage property page


COptionsGSSPage::COptionsGSSPage(COptionsDlg *pOptionsDlg, CWnd* pParent /*=NULL*/) 
: COptionsPage(pOptionsDlg, COptionsGSSPage::IDD, pParent)
{
	//{{AFX_DATA_INIT(COptionsGSSPage)
	m_bPromptPassword = FALSE;
	m_bUseGSS = FALSE;
	//}}AFX_DATA_INIT
}


void COptionsGSSPage::DoDataExchange(CDataExchange* pDX)
{
	COptionsPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(COptionsGSSPage)
	DDX_Check(pDX, IDC_PROMPTPASSWORD, m_bPromptPassword);
	DDX_Check(pDX, IDC_USEGSS, m_bUseGSS);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(COptionsGSSPage, COptionsPage)
	//{{AFX_MSG_MAP(COptionsGSSPage)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// COptionsGSSPage message handlers

void COptionsGSSPage::LoadData()
{
	m_bUseGSS = m_pOptionsDlg->GetOptionVal(OPTION_USEGSS);
	m_bPromptPassword = m_pOptionsDlg->GetOptionVal(OPTION_GSSPROMPTPASSWORD);
}

void COptionsGSSPage::SaveData()
{
	m_pOptionsDlg->SetOption(OPTION_USEGSS, m_bUseGSS);
	m_pOptionsDlg->SetOption(OPTION_GSSPROMPTPASSWORD, m_bPromptPassword);
}