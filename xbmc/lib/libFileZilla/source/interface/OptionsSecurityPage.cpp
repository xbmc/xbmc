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

// OptionsSecurityPage.cpp: Implementierungsdatei
//

#include "stdafx.h"
#include "filezilla server.h"
#include "OptionsSecurityPage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialogfeld COptionsSecurityPage 


COptionsSecurityPage::COptionsSecurityPage(CWnd* pParent /*=NULL*/)
	: CSAPrefsSubDlg(COptionsSecurityPage::IDD, pParent)
{
	//{{AFX_DATA_INIT(COptionsSecurityPage)
	m_bInFxp = FALSE;
	m_bInFxpStrict = FALSE;
	m_bOutFxp = FALSE;
	m_bOutFxpStrict = FALSE;
	//}}AFX_DATA_INIT
}


void COptionsSecurityPage::DoDataExchange(CDataExchange* pDX)
{
	CSAPrefsSubDlg::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(COptionsSecurityPage)
	DDX_Control(pDX, IDC_OUTFXPSTRICT, m_cOutFxpStrict);
	DDX_Control(pDX, IDC_INFXPSTRICT, m_cInFxpStrict);
	DDX_Check(pDX, IDC_INFXP, m_bInFxp);
	DDX_Check(pDX, IDC_INFXPSTRICT, m_bInFxpStrict);
	DDX_Check(pDX, IDC_OUTFXP, m_bOutFxp);
	DDX_Check(pDX, IDC_OUTFXPSTRICT, m_bOutFxpStrict);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(COptionsSecurityPage, CSAPrefsSubDlg)
	//{{AFX_MSG_MAP(COptionsSecurityPage)
	ON_BN_CLICKED(IDC_INFXP, OnInfxp)
	ON_BN_CLICKED(IDC_OUTFXP, OnOutfxp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Behandlungsroutinen für Nachrichten COptionsSecurityPage 

void COptionsSecurityPage::OnInfxp() 
{
	UpdateData(TRUE);
	m_cInFxpStrict.EnableWindow(m_bInFxp);
	
}

void COptionsSecurityPage::OnOutfxp() 
{
	UpdateData(TRUE);
	m_cOutFxpStrict.EnableWindow(m_bOutFxp);		
}

BOOL COptionsSecurityPage::OnInitDialog() 
{
	CSAPrefsSubDlg::OnInitDialog();
	
	m_cInFxpStrict.EnableWindow(m_bInFxp);
	m_cOutFxpStrict.EnableWindow(m_bOutFxp);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX-Eigenschaftenseiten sollten FALSE zurückgeben
}
