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

// GroupsDlgSpeedLimit.cpp: Implementierungsdatei
//

#include "stdafx.h"
#include "filezilla server.h"
#include "GroupsDlgSpeedLimit.h"
#include "SpeedLimitRuleDlg.h"

/////////////////////////////////////////////////////////////////////////////
// CGroupsDlgSpeedLimit dialog


CGroupsDlgSpeedLimit::CGroupsDlgSpeedLimit(CGroupsDlg* pOwner) 
	: CSAPrefsSubDlg(IDD)
{
	m_pOwner = pOwner;
	
	//{{AFX_DATA_INIT(CGroupsDlgSpeedLimit)
	m_DownloadSpeedLimitType = -1;
	m_UploadSpeedLimitType = -1;
	m_DownloadValue = 10;
	m_UploadValue = 10;
	//}}AFX_DATA_INIT
}

CGroupsDlgSpeedLimit::~CGroupsDlgSpeedLimit()
{
}

void CGroupsDlgSpeedLimit::DoDataExchange(CDataExchange* pDX)
{
	CSAPrefsSubDlg::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGroupsDlgSpeedLimit)
	DDX_Control(pDX, IDC_SPEEDLIMIT_DOWNLOAD_UP, m_DownloadUpCtrl);
	DDX_Control(pDX, IDC_SPEEDLIMIT_DOWNLOAD_RULES_LIST, m_DownloadRulesListCtrl);
	DDX_Control(pDX, IDC_SPEEDLIMIT_DOWNLOAD_REMOVE, m_DownloadRemoveCtrl);
	DDX_Control(pDX, IDC_SPEEDLIMIT_DOWNLOAD_DOWN, m_DownloadDownCtrl);
	DDX_Control(pDX, IDC_SPEEDLIMIT_DOWNLOAD_ADD, m_DownloadAddCtrl);
	DDX_Control(pDX, IDC_SPEEDLIMIT_UPLOAD_UP, m_UploadUpCtrl);
	DDX_Control(pDX, IDC_SPEEDLIMIT_UPLOAD_RULES_LIST, m_UploadRulesListCtrl);
	DDX_Control(pDX, IDC_SPEEDLIMIT_UPLOAD_REMOVE, m_UploadRemoveCtrl);
	DDX_Control(pDX, IDC_SPEEDLIMIT_UPLOAD_DOWN, m_UploadDownCtrl);
	DDX_Control(pDX, IDC_SPEEDLIMIT_UPLOAD_ADD, m_UploadAddCtrl);
	DDX_Control(pDX, IDC_SPEEDLIMIT_UPLOAD_VALUE, m_UploadValueCtrl);
	DDX_Control(pDX, IDC_SPEEDLIMIT_DOWNLOAD_VALUE, m_DownloadValueCtrl);
	DDX_Radio(pDX, IDC_SPEEDLIMIT_DOWNLOAD1, m_DownloadSpeedLimitType);
	DDX_Radio(pDX, IDC_SPEEDLIMIT_UPLOAD1, m_UploadSpeedLimitType);
	DDX_Text(pDX, IDC_SPEEDLIMIT_DOWNLOAD_VALUE, m_DownloadValue);
	DDV_MinMaxInt(pDX, m_DownloadValue, 1, 1000000000);
	DDX_Text(pDX, IDC_SPEEDLIMIT_UPLOAD_VALUE, m_UploadValue);
	DDV_MinMaxInt(pDX, m_UploadValue, 1, 1000000000);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CGroupsDlgSpeedLimit, CSAPrefsSubDlg)
	//{{AFX_MSG_MAP(CGroupsDlgSpeedLimit)
	ON_BN_CLICKED(IDC_SPEEDLIMIT_DOWNLOAD1, OnRadio)
	ON_BN_CLICKED(IDC_SPEEDLIMIT_DOWNLOAD_ADD, OnSpeedlimitDownloadAdd)
	ON_BN_CLICKED(IDC_SPEEDLIMIT_DOWNLOAD_REMOVE, OnSpeedlimitDownloadRemove)
	ON_BN_CLICKED(IDC_SPEEDLIMIT_DOWNLOAD_UP, OnSpeedlimitDownloadUp)
	ON_BN_CLICKED(IDC_SPEEDLIMIT_DOWNLOAD_DOWN, OnSpeedlimitDownloadDown)
	ON_LBN_DBLCLK(IDC_SPEEDLIMIT_DOWNLOAD_RULES_LIST, OnDblclkSpeedlimitDownloadRulesList)
	ON_BN_CLICKED(IDC_SPEEDLIMIT_UPLOAD_ADD, OnSpeedlimitUploadAdd)
	ON_BN_CLICKED(IDC_SPEEDLIMIT_UPLOAD_REMOVE, OnSpeedlimitUploadRemove)
	ON_BN_CLICKED(IDC_SPEEDLIMIT_UPLOAD_UP, OnSpeedlimitUploadUp)
	ON_BN_CLICKED(IDC_SPEEDLIMIT_UPLOAD_DOWN, OnSpeedlimitUploadDown)
	ON_BN_CLICKED(IDC_SPEEDLIMIT_UPLOAD1, OnRadio)
	ON_BN_CLICKED(IDC_SPEEDLIMIT_DOWNLOAD2, OnRadio)
	ON_BN_CLICKED(IDC_SPEEDLIMIT_UPLOAD2, OnRadio)
	ON_BN_CLICKED(IDC_SPEEDLIMIT_DOWNLOAD3, OnRadio)
	ON_BN_CLICKED(IDC_SPEEDLIMIT_UPLOAD3, OnRadio)
	ON_BN_CLICKED(IDC_SPEEDLIMIT_DOWNLOAD4, OnRadio)
	ON_BN_CLICKED(IDC_SPEEDLIMIT_UPLOAD4, OnRadio)
	ON_LBN_DBLCLK(IDC_SPEEDLIMIT_UPLOAD_RULES_LIST, OnDblclkSpeedlimitUploadRulesList)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGroupsDlgSpeedLimit message handlers

void CGroupsDlgSpeedLimit::SetCtrlState()
{
	UpdateData(TRUE);

	ShowSpeedLimit(m_DownloadRulesListCtrl, m_DownloadSpeedLimits);
	ShowSpeedLimit(m_UploadRulesListCtrl, m_UploadSpeedLimits);

	if (!m_pOwner->GetCurrentGroup())
	{
		m_DownloadValueCtrl.EnableWindow(FALSE);
		m_DownloadUpCtrl.EnableWindow(FALSE);
		m_DownloadRulesListCtrl.EnableWindow(FALSE);
		m_DownloadRemoveCtrl.EnableWindow(FALSE);
		m_DownloadDownCtrl.EnableWindow(FALSE);
		m_DownloadAddCtrl.EnableWindow(FALSE);
		
		m_UploadValueCtrl.EnableWindow(FALSE);
		m_UploadUpCtrl.EnableWindow(FALSE);
		m_UploadRulesListCtrl.EnableWindow(FALSE);
		m_UploadRemoveCtrl.EnableWindow(FALSE);
		m_UploadDownCtrl.EnableWindow(FALSE);
		m_UploadAddCtrl.EnableWindow(FALSE);

		GetDlgItem(IDC_SPEEDLIMIT_DOWNLOAD1)->EnableWindow(FALSE);
		GetDlgItem(IDC_SPEEDLIMIT_DOWNLOAD2)->EnableWindow(FALSE);
		GetDlgItem(IDC_SPEEDLIMIT_DOWNLOAD3)->EnableWindow(FALSE);
		GetDlgItem(IDC_SPEEDLIMIT_UPLOAD1)->EnableWindow(FALSE);
		GetDlgItem(IDC_SPEEDLIMIT_UPLOAD2)->EnableWindow(FALSE);
		GetDlgItem(IDC_SPEEDLIMIT_UPLOAD3)->EnableWindow(FALSE);

		((CButton *)GetDlgItem(IDC_GROUPS_SPEEDLIMIT_SERVERBYPASS_DOWNLOAD))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_GROUPS_SPEEDLIMIT_SERVERBYPASS_UPLOAD))->EnableWindow(FALSE);
	}
	else
	{
		m_DownloadValueCtrl.EnableWindow(m_DownloadSpeedLimitType == 1);
		m_DownloadUpCtrl.EnableWindow(m_DownloadSpeedLimitType == 2);
		m_DownloadRulesListCtrl.EnableWindow(m_DownloadSpeedLimitType == 2);
		m_DownloadRemoveCtrl.EnableWindow(m_DownloadSpeedLimitType == 2);
		m_DownloadDownCtrl.EnableWindow(m_DownloadSpeedLimitType == 2);
		m_DownloadAddCtrl.EnableWindow(m_DownloadSpeedLimitType == 2);
		
		m_UploadValueCtrl.EnableWindow(m_UploadSpeedLimitType == 1);
		m_UploadUpCtrl.EnableWindow(m_UploadSpeedLimitType == 2);
		m_UploadRulesListCtrl.EnableWindow(m_UploadSpeedLimitType == 2);
		m_UploadRemoveCtrl.EnableWindow(m_UploadSpeedLimitType == 2);
		m_UploadDownCtrl.EnableWindow(m_UploadSpeedLimitType == 2);
		m_UploadAddCtrl.EnableWindow(m_UploadSpeedLimitType == 2);

		GetDlgItem(IDC_SPEEDLIMIT_DOWNLOAD1)->EnableWindow(TRUE);
		GetDlgItem(IDC_SPEEDLIMIT_DOWNLOAD2)->EnableWindow(TRUE);
		GetDlgItem(IDC_SPEEDLIMIT_DOWNLOAD3)->EnableWindow(TRUE);
		GetDlgItem(IDC_SPEEDLIMIT_UPLOAD1)->EnableWindow(TRUE);
		GetDlgItem(IDC_SPEEDLIMIT_UPLOAD2)->EnableWindow(TRUE);
		GetDlgItem(IDC_SPEEDLIMIT_UPLOAD3)->EnableWindow(TRUE);
	
		((CButton *)GetDlgItem(IDC_GROUPS_SPEEDLIMIT_SERVERBYPASS_DOWNLOAD))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_GROUPS_SPEEDLIMIT_SERVERBYPASS_UPLOAD))->EnableWindow(TRUE);
	}
}

BOOL CGroupsDlgSpeedLimit::OnInitDialog() 
{
	CSAPrefsSubDlg::OnInitDialog();
	
	// TODO: Add extra initialization here
	SetCtrlState();

	ShowSpeedLimit( m_DownloadRulesListCtrl, m_DownloadSpeedLimits);
	ShowSpeedLimit( m_UploadRulesListCtrl, m_UploadSpeedLimits);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CGroupsDlgSpeedLimit::OnRadio() 
{
	SetCtrlState();
}

void CGroupsDlgSpeedLimit::OnSpeedlimitDownloadAdd() 
{
	// TODO: Add your control notification handler code here
	CSpeedLimitRuleDlg dlg;

	if ( dlg.DoModal() == IDOK)
	{
		m_DownloadSpeedLimits.push_back(dlg.GetSpeedLimit());

		ShowSpeedLimit( m_DownloadRulesListCtrl, m_DownloadSpeedLimits);
	}
}

void CGroupsDlgSpeedLimit::ShowSpeedLimit(CListBox &listBox, SPEEDLIMITSLIST &list)
{
	listBox.ResetContent();

	for (unsigned int i = 0; i < list.size(); i++)
	{
		CString str;
		
		str.Format(_T( "%dkB/s"), list[i].m_Speed);

		CString help;
		
		if (list[i].m_DateCheck)
		{
			CTime t(list[i].m_Date.y, list[i].m_Date.m, list[i].m_Date.d, 0, 0, 0);
			help += t.Format(_T("%x"));
		}
		
		if (list[i].m_FromCheck)
		{
			if (help.GetLength() > 0)
				help += _T( "; ");
			
			CTime t(2003, 1, 1, list[i].m_FromTime.h, list[i].m_FromTime.m, list[i].m_FromTime.s);
			help += _T("F:") + t.Format( _T("%X"));
		}
		
		if (list[i].m_ToCheck)
		{
			if (help.GetLength() > 0)
				help += _T( "; ");
			
			CTime t(2003, 1, 1, list[i].m_ToTime.h, list[i].m_ToTime.m, list[i].m_ToTime.s);
			help += _T("T:") + t.Format( _T("%X"));
		}
		
		for (int j = 0; j < 7; j++)
			if (!(list[i].m_Day & (1<<j)))
			{
				if (help.GetLength() > 0)
					help += _T( "; ");
				
				bool was = false;
				for ( int k = 0; k < 7; k++)
				{
					if ((list[i].m_Day & (1<<k)))
					{
						if (was)
							help += _T(", ");
						
						was = true;
						
						CTime time(2001, 1, k + 1, 0, 0, 0);
						
						help += time.Format(_T("%a"));
					}
				}
				
				break;
			}
		
		str += _T(" [") + help + _T("]");
		
		listBox.AddString(str);
	}
}

void CGroupsDlgSpeedLimit::OnSpeedlimitDownloadRemove() 
{
	int curSel = m_DownloadRulesListCtrl.GetCurSel();

	if ( ( curSel >= 0) && ( curSel < m_DownloadSpeedLimits.size()))
	{
		for (SPEEDLIMITSLIST::iterator iter=m_DownloadSpeedLimits.begin(); iter!=m_DownloadSpeedLimits.end(); iter++)
		{
			if (!curSel)
			{
				m_DownloadSpeedLimits.erase(iter);
				break;
			}
			curSel--;
		}

		ShowSpeedLimit( m_DownloadRulesListCtrl, m_DownloadSpeedLimits);
	}
}

void CGroupsDlgSpeedLimit::OnSpeedlimitDownloadUp() 
{
	// TODO: Add your control notification handler code here
	int curSel = m_DownloadRulesListCtrl.GetCurSel();

	if ( ( curSel >= 1) && ( curSel < m_DownloadSpeedLimits.size()))
	{
		CSpeedLimit sl = m_DownloadSpeedLimits[curSel];
		m_DownloadSpeedLimits[curSel] = m_DownloadSpeedLimits[curSel - 1];
		m_DownloadSpeedLimits[curSel - 1] = sl;

		ShowSpeedLimit( m_DownloadRulesListCtrl, m_DownloadSpeedLimits);

		m_DownloadRulesListCtrl.SetCurSel( curSel - 1);
	}
}

void CGroupsDlgSpeedLimit::OnSpeedlimitDownloadDown() 
{
	// TODO: Add your control notification handler code here
	int curSel = m_DownloadRulesListCtrl.GetCurSel();

	if ( ( curSel >= 0) && ( curSel < m_DownloadSpeedLimits.size() - 1))
	{
		CSpeedLimit sl = m_DownloadSpeedLimits[curSel];
		m_DownloadSpeedLimits[curSel] = m_DownloadSpeedLimits[curSel + 1];
		m_DownloadSpeedLimits[curSel + 1] = sl;

		ShowSpeedLimit( m_DownloadRulesListCtrl, m_DownloadSpeedLimits);

		m_DownloadRulesListCtrl.SetCurSel( curSel + 1);
	}
}

void CGroupsDlgSpeedLimit::OnDblclkSpeedlimitDownloadRulesList() 
{
	// TODO: Add your control notification handler code here
	int curSel = m_DownloadRulesListCtrl.GetCurSel();

	if ( ( curSel >= 0) && ( curSel < m_DownloadSpeedLimits.size()))
	{
		CSpeedLimitRuleDlg dlg;

		dlg.FillFromSpeedLimit(m_DownloadSpeedLimits[curSel]);

		if ( dlg.DoModal() == IDOK)
		{
			m_DownloadSpeedLimits[curSel] = dlg.GetSpeedLimit();

			ShowSpeedLimit(m_DownloadRulesListCtrl, m_DownloadSpeedLimits);
			m_DownloadRulesListCtrl.SetCurSel(curSel);
		}
	}
}

void CGroupsDlgSpeedLimit::OnSpeedlimitUploadRemove() 
{
	// TODO: Add your control notification handler code here
	int curSel = m_UploadRulesListCtrl.GetCurSel();

	if ( ( curSel >= 0) && ( curSel < m_UploadSpeedLimits.size()))
	{
		for (SPEEDLIMITSLIST::iterator iter=m_UploadSpeedLimits.begin(); iter!=m_UploadSpeedLimits.end(); iter++)
		{
			if (!curSel)
			{
				m_UploadSpeedLimits.erase(iter);
				break;
			}
			curSel--;
		}

		ShowSpeedLimit( m_UploadRulesListCtrl, m_UploadSpeedLimits);
	}
}

void CGroupsDlgSpeedLimit::OnSpeedlimitUploadUp() 
{
	int curSel = m_UploadRulesListCtrl.GetCurSel();

	if ( ( curSel >= 1) && ( curSel < m_UploadSpeedLimits.size()))
	{
		CSpeedLimit sl = m_UploadSpeedLimits[curSel];
		m_UploadSpeedLimits[curSel] = m_UploadSpeedLimits[curSel - 1];
		m_UploadSpeedLimits[curSel - 1] = sl;

		ShowSpeedLimit( m_UploadRulesListCtrl, m_UploadSpeedLimits);

		m_UploadRulesListCtrl.SetCurSel( curSel - 1);
	}
}

void CGroupsDlgSpeedLimit::OnSpeedlimitUploadDown() 
{
	// TODO: Add your control notification handler code here
	int curSel = m_UploadRulesListCtrl.GetCurSel();

	if ( ( curSel >= 0) && ( curSel < m_UploadSpeedLimits.size() - 1))
	{
		CSpeedLimit sl = m_UploadSpeedLimits[curSel];
		m_UploadSpeedLimits[curSel] = m_UploadSpeedLimits[curSel + 1];
		m_UploadSpeedLimits[curSel + 1] = sl;

		ShowSpeedLimit( m_UploadRulesListCtrl, m_UploadSpeedLimits);

		m_UploadRulesListCtrl.SetCurSel( curSel + 1);
	}
}

void CGroupsDlgSpeedLimit::OnSpeedlimitUploadAdd() 
{
	// TODO: Add your control notification handler code here
	CSpeedLimitRuleDlg dlg;

	if ( dlg.DoModal() == IDOK)
	{
		m_UploadSpeedLimits.push_back( dlg.GetSpeedLimit());

		ShowSpeedLimit( m_UploadRulesListCtrl, m_UploadSpeedLimits);
	}
}

void CGroupsDlgSpeedLimit::OnDblclkSpeedlimitUploadRulesList() 
{
	// TODO: Add your control notification handler code here
	int curSel = m_UploadRulesListCtrl.GetCurSel();

	if ( ( curSel >= 0) && ( curSel < m_UploadSpeedLimits.size()))
	{
		CSpeedLimitRuleDlg dlg;

		dlg.FillFromSpeedLimit( m_UploadSpeedLimits[curSel]);

		if (dlg.DoModal() == IDOK)
		{
			m_UploadSpeedLimits[curSel] = dlg.GetSpeedLimit();

			ShowSpeedLimit( m_UploadRulesListCtrl, m_UploadSpeedLimits);
			m_UploadRulesListCtrl.SetCurSel( curSel);
		}
	}
}

BOOL CGroupsDlgSpeedLimit::DisplayGroup(const t_group *pGroup)
{
	if (!pGroup)
	{
		m_DownloadValue = 10;
		m_UploadValue = 10;
		m_DownloadSpeedLimitType = -1;
		m_UploadSpeedLimitType = -1;

		m_DownloadSpeedLimits.clear();
		m_UploadSpeedLimits.clear();

		((CButton *)GetDlgItem(IDC_GROUPS_SPEEDLIMIT_SERVERBYPASS_DOWNLOAD))->SetCheck(0);
		((CButton *)GetDlgItem(IDC_GROUPS_SPEEDLIMIT_SERVERBYPASS_UPLOAD))->SetCheck(0);
	}
	else
	{
		m_DownloadValue = pGroup->nDownloadSpeedLimit;
		m_UploadValue = pGroup->nUploadSpeedLimit;
		m_DownloadSpeedLimitType = pGroup->nDownloadSpeedLimitType - 1;
		if (m_DownloadSpeedLimitType == -1)
			m_DownloadSpeedLimitType = 0;
		m_UploadSpeedLimitType = pGroup->nUploadSpeedLimitType - 1;
		if (m_UploadSpeedLimitType == -1)
			m_UploadSpeedLimitType = 0;
		
		m_DownloadSpeedLimits = pGroup->DownloadSpeedLimits;
		m_UploadSpeedLimits = pGroup->UploadSpeedLimits;
	
		((CButton *)GetDlgItem(IDC_GROUPS_SPEEDLIMIT_SERVERBYPASS_DOWNLOAD))->SetCheck(pGroup->nBypassServerDownloadSpeedLimit ? 1 : 0);
		((CButton *)GetDlgItem(IDC_GROUPS_SPEEDLIMIT_SERVERBYPASS_UPLOAD))->SetCheck(pGroup->nBypassServerUploadSpeedLimit ? 1 : 0);
	}

	UpdateData(FALSE);

	return TRUE;
}

BOOL CGroupsDlgSpeedLimit::SaveGroup(t_group *pGroup)
{
	UpdateData(TRUE);
	pGroup->nDownloadSpeedLimit = m_DownloadValue;
	pGroup->nUploadSpeedLimit = m_UploadValue;
	pGroup->nDownloadSpeedLimitType = m_DownloadSpeedLimitType + 1;
	pGroup->nUploadSpeedLimitType = m_UploadSpeedLimitType + 1;

	pGroup->DownloadSpeedLimits = m_DownloadSpeedLimits;
	pGroup->UploadSpeedLimits = m_UploadSpeedLimits;

	pGroup->nBypassServerDownloadSpeedLimit = ((CButton *)GetDlgItem(IDC_GROUPS_SPEEDLIMIT_SERVERBYPASS_DOWNLOAD))->GetCheck() ? 1 : 0;
	pGroup->nBypassServerUploadSpeedLimit = ((CButton *)GetDlgItem(IDC_GROUPS_SPEEDLIMIT_SERVERBYPASS_UPLOAD))->GetCheck() ? 1 : 0;
	return TRUE;
}