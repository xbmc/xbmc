// OptionsGSSPage.cpp : implementation file
//

#include "stdafx.h"
#include "filezilla server.h"
#include "OptionsGSSPage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// COptionsGSSPage property page


COptionsGSSPage::COptionsGSSPage(CWnd* pParent /*=NULL*/) 
: CSAPrefsSubDlg(COptionsGSSPage::IDD, pParent)
{
	//{{AFX_DATA_INIT(COptionsGSSPage)
	m_bPromptPassword = FALSE;
	m_bUseGSS = FALSE;
	//}}AFX_DATA_INIT
}


void COptionsGSSPage::DoDataExchange(CDataExchange* pDX)
{
	CSAPrefsSubDlg::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(COptionsGSSPage)
	DDX_Check(pDX, IDC_PROMPTPASSWORD, m_bPromptPassword);
	DDX_Check(pDX, IDC_USEGSS, m_bUseGSS);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(COptionsGSSPage, CSAPrefsSubDlg)
	//{{AFX_MSG_MAP(COptionsGSSPage)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// COptionsGSSPage message handlers

