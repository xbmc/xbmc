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

// GroupsDlg.cpp: Implementierungsdatei
//

#include "stdafx.h"
#include "FileZilla server.h"
#include "GroupsDlg.h"
#include "misc\md5.h"
#include "misc\sbdestination.h"
#include "entersomething.h"
#include "GroupsDlgGeneral.h"
#include "GroupsDlgSpeedLimit.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialogfeld CGroupsDlg 

CGroupsDlg::CGroupsDlg(CWnd* pParent /*=NULL*/)
	: CSAPrefsDialog(CGroupsDlg::IDD, pParent)
{
	m_pGeneralPage = new CGroupsDlgGeneral(this);
	m_pSpeedLimitPage = new CGroupsDlgSpeedLimit(this);
	
	AddPage(*m_pGeneralPage, "General");
	AddPage(*m_pSpeedLimitPage, "Speed Limits");
}

CGroupsDlg::~CGroupsDlg()
{
	delete m_pGeneralPage;
	delete m_pSpeedLimitPage;
}

void CGroupsDlg::DoDataExchange(CDataExchange* pDX)
{
	CSAPrefsDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGroupsDlg)
	DDX_Control(pDX, IDC_GROUPS_GROUPLIST, m_cGrouplist);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CGroupsDlg, CSAPrefsDialog)
	//{{AFX_MSG_MAP(CGroupsDlg)
	ON_LBN_SELCHANGE(IDC_GROUPS_GROUPLIST, OnSelchangeGrouplist)
	ON_WM_CONTEXTMENU()
	ON_COMMAND(ID_GROUPMENU_ADD, OnGroupAdd)
	ON_COMMAND(ID_GROUPMENU_COPY, OnGroupCopy)
	ON_COMMAND(ID_GROUPMENU_REMOVE, OnGroupRemove)
	ON_COMMAND(ID_GROUPMENU_RENAME, OnGroupRename)
	ON_BN_CLICKED(IDC_GROUPS_ADDGROUP, OnGroupAdd)
	ON_BN_CLICKED(IDC_GROUPS_COPYGROUP, OnGroupCopy)
	ON_BN_CLICKED(IDC_GROUPS_REMOVEGROUP, OnGroupRemove)
	ON_BN_CLICKED(IDC_GROUPS_RENAMEGROUP, OnGroupRename)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Behandlungsroutinen f�r Nachrichten CGroupsDlg 

BOOL CGroupsDlg::OnInitDialog() 
{
	CSAPrefsDialog::OnInitDialog();
	
	m_oldgroup = LB_ERR;
	m_cGrouplist.ResetContent();	
	UpdateData(FALSE);
	for (unsigned int i=0; i<m_GroupsList.size(); i++)
	{
		int index = m_cGrouplist.AddString(m_GroupsList[i].group);
		m_cGrouplist.SetItemData(index, i);
	}
				
	if (m_GroupsList.size())
	{
		m_cGrouplist.SetCurSel(0);
		OnSelchangeGrouplist();
	}

	SetCtrlState();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX-Propertypages should return FALSE
}

BOOL CGroupsDlg::Validate()
{
	CString res = m_pGeneralPage->Validate();
	if (res != _T(""))
	{
		ShowPage(m_pGeneralPage);
		m_cGrouplist.SetCurSel(m_oldgroup);
		MessageBox(res);
		return FALSE;
	}
	
	return TRUE;
}

void CGroupsDlg::OnSelchangeGrouplist() 
{
	if (!Validate())
		return;
	if (m_oldgroup!=LB_ERR)
	{
		int oldindex = m_cGrouplist.GetItemData(m_oldgroup);
		VERIFY(m_pGeneralPage->SaveGroup(&m_GroupsList[oldindex]));
		VERIFY(m_pSpeedLimitPage->SaveGroup(&m_GroupsList[oldindex]));
	}
	int nItem = m_cGrouplist.GetCurSel();
	if (nItem!=LB_ERR)
	{
		m_oldgroup = nItem;
		int index = m_cGrouplist.GetItemData(nItem);
		VERIFY(m_pGeneralPage->DisplayGroup(&m_GroupsList[index]));
		VERIFY(m_pSpeedLimitPage->DisplayGroup(&m_GroupsList[index]));
	}
	else
	{
		VERIFY(m_pGeneralPage->DisplayGroup(NULL));
		VERIFY(m_pSpeedLimitPage->DisplayGroup(NULL));
	}

	SetCtrlState();
}

void CGroupsDlg::OnOK() 
{
	if (!Validate())
		return;
	m_cGrouplist.SetCurSel(-1);
	OnSelchangeGrouplist();
	
	CSAPrefsDialog::OnOK();
}

void CGroupsDlg::OnContextMenu(CWnd* pWnd, CPoint point) 
{
	if (pWnd==&m_cGrouplist)
	{
		CMenu menu;
		menu.LoadMenu(IDR_GROUPCONTEXT);

		CMenu* pPopup = menu.GetSubMenu(0);
		ASSERT(pPopup != NULL);
		CWnd* pWndPopupOwner = this;
		while (pWndPopupOwner->GetStyle() & WS_CHILD)
			pWndPopupOwner = pWndPopupOwner->GetParent();
		
		if (m_cGrouplist.GetCurSel()==LB_ERR)
		{
			pPopup->EnableMenuItem(ID_GROUPMENU_COPY, MF_GRAYED);
			pPopup->EnableMenuItem(ID_GROUPMENU_REMOVE, MF_GRAYED);
			pPopup->EnableMenuItem(ID_GROUPMENU_RENAME, MF_GRAYED);
		}
		if (point.x==-1)
			GetCursorPos(&point);
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y,
			pWndPopupOwner);
	}
}

void CGroupsDlg::OnGroupAdd() 
{
	if (!Validate())
		return;

	CEnterSomething dlg(IDS_ADDGROUPDIALOG);
	if (dlg.DoModal()==IDOK)
	{
		CString newname=dlg.m_String;
		newname.MakeLower();
		for (int i=0; i<m_cGrouplist.GetCount(); i++)
		{
			CString str;
			m_cGrouplist.GetText(i, str);
			str.MakeLower();
			if (str==newname)
			{
				AfxMessageBox(IDS_ERRORMSG_GROUPALREADYEXISTS);
				return;
			}
		}

		t_group group;
		group.group = dlg.m_String;
		group.nBypassUserLimit = 0;
		group.nLnk = 0;
		group.nRelative = 0;
		group.nIpLimit = 0;
		group.nUserLimit = 0;
		int nItem = m_cGrouplist.AddString(group.group);
		if (nItem <= m_oldgroup)
			m_oldgroup++;
		m_GroupsList.push_back(group);
		m_cGrouplist.SetItemData(nItem, m_GroupsList.size()-1);
		m_cGrouplist.SetCurSel(nItem);	
		OnSelchangeGrouplist();
	}
}

void CGroupsDlg::OnGroupCopy() 
{
	if (!Validate())
		return;

	int pos = m_cGrouplist.GetCurSel();
	if (pos==LB_ERR)
		return;
	int index = m_cGrouplist.GetItemData(pos);

	CEnterSomething dlg(IDS_COPYGROUPDIALOG);
	if (dlg.DoModal()==IDOK)
	{
		int i;
		CString newname = dlg.m_String;
		newname.MakeLower();
		for (i=0;i<m_cGrouplist.GetCount();i++)
		{
			CString str;
			m_cGrouplist.GetText(i,str);
			str.MakeLower();
			if (str==newname)
			{
				AfxMessageBox(IDS_ERRORMSG_GROUPALREADYEXISTS);
				return;
			}
		}
		
		t_group group;
		group.group = dlg.m_String;
		group.nBypassUserLimit = m_GroupsList[index].nBypassUserLimit;
		group.nLnk = m_GroupsList[index].nLnk;
		group.nRelative = m_GroupsList[index].nRelative;
		group.nIpLimit = m_GroupsList[index].nIpLimit;
		group.nUserLimit = m_GroupsList[index].nUserLimit;
		for (std::vector<t_directory>::const_iterator iter=m_GroupsList[index].permissions.begin(); iter!=m_GroupsList[index].permissions.end(); iter++)
			group.permissions.push_back(*iter);
		
		int nItem=m_cGrouplist.AddString(group.group);
		if (nItem<=m_oldgroup)
			m_oldgroup++;
		m_GroupsList.push_back(group);
		m_cGrouplist.SetItemData(nItem, m_GroupsList.size()-1);
		m_cGrouplist.SetCurSel(nItem);	

		OnSelchangeGrouplist();
	}	
}


void CGroupsDlg::OnGroupRemove() 
{
	int pos = m_cGrouplist.GetCurSel();
	if (pos==LB_ERR)
		return;
	int index = m_cGrouplist.GetItemData(pos);
	m_oldgroup = LB_ERR;
	int i=0;
	for (t_GroupsList::iterator iter=m_GroupsList.begin(); iter!=m_GroupsList.end(); iter++, i++)
		if (i==index)
		{
			m_GroupsList.erase(iter);
			break;
		}
	for (i=0; i<m_cGrouplist.GetCount(); i++)
	{
		int data = m_cGrouplist.GetItemData(i);
		if (data>index)
			m_cGrouplist.SetItemData(i, data-1);
	}
	m_cGrouplist.DeleteString(pos);
	OnSelchangeGrouplist();
}

void CGroupsDlg::OnGroupRename() 
{
	if (!Validate())
		return;

	int pos=m_cGrouplist.GetCurSel();
	if (pos==LB_ERR)
		return;
	int index=m_cGrouplist.GetItemData(pos);

	CEnterSomething dlg(IDS_INPUTDIALOGTEXT_RENAME);
	if (dlg.DoModal()==IDOK)
	{
		CString newname=dlg.m_String;
		newname.MakeLower();
		for (int i=0;i<m_cGrouplist.GetCount();i++)
		{
			CString str;
			m_cGrouplist.GetText(i,str);
			str.MakeLower();
			if (str==newname)
			{
				AfxMessageBox(IDS_ERRORMSG_GROUPALREADYEXISTS);
				return;
			}
		}
		
		m_cGrouplist.DeleteString(pos);
		pos = m_cGrouplist.AddString(dlg.m_String);
		m_cGrouplist.SetItemData(pos,index);
		m_cGrouplist.SetCurSel(pos);	
		m_oldgroup = pos;
		m_GroupsList[index].group = dlg.m_String;
		OnSelchangeGrouplist();
	}
}

void CGroupsDlg::SetCtrlState()
{
	if (m_cGrouplist.GetCurSel() == LB_ERR)
	{
		GetDlgItem(IDC_GROUPS_REMOVEGROUP)->EnableWindow(FALSE);
		GetDlgItem(IDC_GROUPS_RENAMEGROUP)->EnableWindow(FALSE);
		GetDlgItem(IDC_GROUPS_COPYGROUP)->EnableWindow(FALSE);
	}
	else
	{
		GetDlgItem(IDC_GROUPS_REMOVEGROUP)->EnableWindow(TRUE);
		GetDlgItem(IDC_GROUPS_RENAMEGROUP)->EnableWindow(TRUE);
		GetDlgItem(IDC_GROUPS_COPYGROUP)->EnableWindow(TRUE);	
	}
	m_pGeneralPage->SetCtrlState();
	m_pSpeedLimitPage->SetCtrlState();
}

BOOL CGroupsDlg::GetAsCommand(char **pBuffer, DWORD *nBufferLength)
{
	if (!pBuffer)
		return FALSE;

	DWORD len = 4;

	t_GroupsList::iterator groupiter;
	for (groupiter=m_GroupsList.begin(); groupiter!=m_GroupsList.end(); groupiter++)
		len += groupiter->GetRequiredBufferLen();

	t_UsersList::iterator iter;
	for (iter=m_UsersList.begin(); iter!=m_UsersList.end(); iter++)
		len += iter->GetRequiredBufferLen();

	*pBuffer=new char[len];
	char *p=*pBuffer;

	*p++ = m_GroupsList.size()/256;
	*p++ = m_GroupsList.size()%256;
	for (groupiter=m_GroupsList.begin(); groupiter!=m_GroupsList.end(); groupiter++)
	{
		p = groupiter->FillBuffer(p);
		if (!p)
		{
			delete [] *pBuffer;
			*pBuffer = NULL;
			return FALSE;
		}		
	}

	*p++ = m_UsersList.size()/256;
	*p++ = m_UsersList.size()%256;
	for (iter=m_UsersList.begin(); iter!=m_UsersList.end(); iter++)
	{
		p = iter->FillBuffer(p);
		if (!p)
		{
			delete [] *pBuffer;
			*pBuffer = NULL;
			return FALSE;
		}
	}
		
	*nBufferLength = len;

	return TRUE;
}

BOOL CGroupsDlg::Init(unsigned char *pData, DWORD dwDataLength)
{
	unsigned char *p=pData;
	unsigned int i;

	if (dwDataLength < 2)
		return FALSE;
	unsigned int num = *p * 256 + p[1];
	p+=2;
	for (i=0; i<num; i++)
	{
		t_group group;
		
		p = group.ParseBuffer(p, dwDataLength-(p-pData));
		if (!p)
			return FALSE;
		m_GroupsList.push_back(group);
	}

	if ((UINT)(p-pData+2)>dwDataLength)
		return FALSE;
	num = *p * 256 + p[1];
	p+=2;
	for (i=0; i<num; i++)
	{
		t_user user;
		
		p = user.ParseBuffer(p, dwDataLength-(p-pData));
		if (!p)
			return FALSE;
		m_UsersList.push_back(user);
	}
	return TRUE;
}

BOOL CGroupsDlg::PreTranslateMessage(MSG* pMsg) 
{
	if (pMsg->message==WM_KEYDOWN)
	{
		if (pMsg->wParam==VK_F2)
		{
			if (GetFocus() == &m_cGrouplist)
			{
				if (m_cGrouplist.GetCurSel() == LB_ERR)
					return TRUE;
				OnGroupRename();
			}
			return TRUE;
		}
	}
	return CSAPrefsDialog::PreTranslateMessage(pMsg);
}

t_group* CGroupsDlg::GetCurrentGroup()
{
	if (m_cGrouplist.GetCurSel() == LB_ERR)
		return NULL;
	else
		return &m_GroupsList[m_cGrouplist.GetItemData(m_cGrouplist.GetCurSel())];
}
