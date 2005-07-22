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

// UsersDlg.cpp: Implementierungsdatei
//

#include "stdafx.h"
#include "FileZilla server.h"
#include "UsersDlg.h"
#include "misc\md5.h"
#include "misc\sbdestination.h"
#include "entersomething.h"
#include "NewUserDlg.h"
#include "UsersDlgGeneral.h"
#include "UsersDlgSpeedLimit.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialogfeld CUsersDlg 

CUsersDlg::CUsersDlg(CWnd* pParent /*=NULL*/)
	: CSAPrefsDialog(IDD, pParent)
{
	m_pGeneralPage = new CUsersDlgGeneral(this);
	m_pSpeedLimitPage = new CUsersDlgSpeedLimit(this);

	AddPage(*m_pGeneralPage, "General");
	AddPage(*m_pSpeedLimitPage, "Speed Limits");
}

CUsersDlg::~CUsersDlg()
{
	delete m_pGeneralPage;
	delete m_pSpeedLimitPage;
}


void CUsersDlg::DoDataExchange(CDataExchange* pDX)
{
	CSAPrefsDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CUsersDlg)
	DDX_Control(pDX, IDC_USERLIST, m_cUserlist);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CUsersDlg, CSAPrefsDialog)
	//{{AFX_MSG_MAP(CUsersDlg)
	ON_LBN_SELCHANGE(IDC_USERLIST, OnSelchangeUserlist)
	ON_WM_CONTEXTMENU()
	ON_COMMAND(ID_USERMENU_ADD, OnUsermenuAdd)
	ON_COMMAND(ID_USERMENU_COPY, OnUsermenuCopy)
	ON_COMMAND(ID_USERMENU_REMOVE, OnUsermenuRemove)
	ON_COMMAND(ID_USERMENU_RENAME, OnUsermenuRename)
	ON_BN_CLICKED(IDC_USERADD, OnUseradd)
	ON_BN_CLICKED(IDC_USERCOPY, OnUsercopy)
	ON_BN_CLICKED(IDC_USERREMOVE, OnUserremove)
	ON_BN_CLICKED(IDC_USERRENAME, OnUserrename)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Behandlungsroutinen für Nachrichten CUsersDlg 

BOOL CUsersDlg::OnInitDialog() 
{
	CSAPrefsDialog::OnInitDialog();

	m_olduser = LB_ERR;
	
	m_cUserlist.ResetContent();	

	for (unsigned int i=0;i<m_UsersList.size();i++)
	{
		int index=m_cUserlist.AddString(m_UsersList[i].user);
		m_cUserlist.SetItemData(index, i);
	}

	if (m_UsersList.size())
	{
		m_cUserlist.SetCurSel(0);
		OnSelchangeUserlist();
	}
	
	SetCtrlState();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX-Eigenschaftenseiten sollten FALSE zurückgeben
}

BOOL CUsersDlg::Validate()
{
	CString res = m_pGeneralPage->Validate();
	if (res != _T(""))
	{
		ShowPage(m_pGeneralPage);
		m_cUserlist.SetCurSel(m_olduser);
		MessageBox(res);
		return FALSE;
	}
	return TRUE;
}

void CUsersDlg::OnSelchangeUserlist() 
{
	if (!Validate())
		return;
	if (m_olduser!=LB_ERR)
	{
		int oldindex = m_cUserlist.GetItemData(m_olduser);
		VERIFY(m_pGeneralPage->SaveUser(&m_UsersList[oldindex]));
		VERIFY(m_pSpeedLimitPage->SaveUser(&m_UsersList[oldindex]));
	}
	int nItem = m_cUserlist.GetCurSel();
	if (nItem!=LB_ERR)
	{
		m_olduser = nItem;
		int index = m_cUserlist.GetItemData(nItem);
		VERIFY(m_pGeneralPage->DisplayUser(&m_UsersList[index]));
		VERIFY(m_pSpeedLimitPage->DisplayUser(&m_UsersList[index]));
	}
	else
	{
		VERIFY(m_pGeneralPage->DisplayUser(NULL));
		VERIFY(m_pSpeedLimitPage->DisplayUser(NULL));
	}
	m_pGeneralPage->UpdateData(FALSE);

	SetCtrlState();
}

void CUsersDlg::OnOK() 
{
	if (!Validate())
		return;
	m_cUserlist.SetCurSel(-1);
	OnSelchangeUserlist();
	
	CSAPrefsDialog::OnOK();
}

void CUsersDlg::OnContextMenu(CWnd* pWnd, CPoint point) 
{
	if (pWnd==&m_cUserlist)
	{
		CMenu menu;
		menu.LoadMenu(IDR_USERCONTEXT);

		CMenu* pPopup = menu.GetSubMenu(0);
		ASSERT(pPopup != NULL);
		CWnd* pWndPopupOwner = this;
		while (pWndPopupOwner->GetStyle() & WS_CHILD)
			pWndPopupOwner = pWndPopupOwner->GetParent();
		
		if (m_cUserlist.GetCurSel()==LB_ERR)
		{
			pPopup->EnableMenuItem(ID_USERMENU_COPY,MF_GRAYED);
			pPopup->EnableMenuItem(ID_USERMENU_REMOVE,MF_GRAYED);
			pPopup->EnableMenuItem(ID_USERMENU_RENAME,MF_GRAYED);
		}
		if (point.x==-1)
			GetCursorPos(&point);
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y,
			pWndPopupOwner);
	}
}

void CUsersDlg::OnUsermenuAdd() 
{
	if (!Validate())
		return;

	CNewUserDlg dlg;
	for (t_GroupsList::iterator iter=m_GroupsList.begin(); iter!=m_GroupsList.end(); iter++)
		dlg.m_GroupList.push_back(iter->group);
	if (dlg.DoModal()==IDOK)
	{
		CString newname = dlg.m_Name;
		newname.MakeLower();
		for (int i=0; i<m_cUserlist.GetCount(); i++)
		{
			CString str;
			m_cUserlist.GetText(i,str);
			str.MakeLower();
			if (str==newname)
			{
				AfxMessageBox(IDS_ERRORMSG_USERALREADYEXISTS);
				return;
			}
		}

		t_user user;
		user.user = dlg.m_Name;
		if (dlg.m_Group != "")
		{
			user.group = dlg.m_Group;
			user.nBypassUserLimit = 2;
			user.nLnk = 2;
			user.nRelative = 2;
			user.nBypassServerDownloadSpeedLimit = 2;
			user.nBypassServerUploadSpeedLimit = 2;
		}
		else
		{
			user.nBypassUserLimit = 0;
			user.nLnk = 0;
			user.nRelative = 0;
			user.nBypassServerDownloadSpeedLimit = 0;
			user.nBypassServerUploadSpeedLimit = 0;
		}
		user.nIpLimit = 0;
		user.nUserLimit = 0;
		user.password = "";
		int nItem = m_cUserlist.AddString(user.user);
		if (nItem <= m_olduser)
			m_olduser++;
		m_UsersList.push_back(user);
		m_cUserlist.SetItemData(nItem, m_UsersList.size()-1);
		m_cUserlist.SetCurSel(nItem);	
		OnSelchangeUserlist();
	}
}

void CUsersDlg::OnUsermenuCopy() 
{
	if (!Validate())
		return;

	int pos=m_cUserlist.GetCurSel();
	if (pos==LB_ERR)
		return;
	int index=m_cUserlist.GetItemData(pos);

	CEnterSomething dlg(IDS_COPYUSERDIALOG);
	if (dlg.DoModal()==IDOK)
	{
		int i;
		CString newname=dlg.m_String;
		newname.MakeLower();
		for (i=0;i<m_cUserlist.GetCount();i++)
		{
			CString str;
			m_cUserlist.GetText(i,str);
			str.MakeLower();
			if (str==newname)
			{
				AfxMessageBox(IDS_ERRORMSG_USERALREADYEXISTS);
				return;
			}
		}
		
		t_user user;
		user.user=dlg.m_String;
		user.nBypassUserLimit = m_UsersList[index].nBypassUserLimit;
		user.nLnk = m_UsersList[index].nLnk;
		user.nRelative = m_UsersList[index].nRelative;
		user.nIpLimit = m_UsersList[index].nIpLimit;
		user.nUserLimit = m_UsersList[index].nUserLimit;
		user.password = m_UsersList[index].password;
		for (std::vector<t_directory>::const_iterator iter=m_UsersList[index].permissions.begin(); iter!=m_UsersList[index].permissions.end(); iter++)
			user.permissions.push_back(*iter);
		
		int nItem=m_cUserlist.AddString(user.user);
		if (nItem<=m_olduser)
			m_olduser++;
		m_UsersList.push_back(user);
		m_cUserlist.SetItemData(nItem, m_UsersList.size()-1);
		m_cUserlist.SetCurSel(nItem);	

		OnSelchangeUserlist();
	}	
}


void CUsersDlg::OnUsermenuRemove() 
{
	int pos=m_cUserlist.GetCurSel();
	if (pos==LB_ERR)
		return;
	int index=m_cUserlist.GetItemData(pos);
	m_olduser=LB_ERR;
	int i=0;
	for (t_UsersList::iterator iter=m_UsersList.begin(); iter!=m_UsersList.end(); iter++, i++)
		if (i==index)
		{
			m_UsersList.erase(iter);
			break;
		}
	for (i=0;i<m_cUserlist.GetCount();i++)
	{
		int data=m_cUserlist.GetItemData(i);
		if (data>index)
			m_cUserlist.SetItemData(i, data-1);
	}
	m_cUserlist.DeleteString(pos);
	OnSelchangeUserlist();
}

void CUsersDlg::OnUsermenuRename() 
{
	if (!Validate())
		return;

	int pos=m_cUserlist.GetCurSel();
	if (pos==LB_ERR)
		return;
	int index=m_cUserlist.GetItemData(pos);

	CEnterSomething dlg(IDS_INPUTDIALOGTEXT_RENAME);
	if (dlg.DoModal()==IDOK)
	{
		CString newname=dlg.m_String;
		newname.MakeLower();
		for (int i=0;i<m_cUserlist.GetCount();i++)
		{
			CString str;
			m_cUserlist.GetText(i,str);
			str.MakeLower();
			if (str==newname)
			{
				AfxMessageBox(IDS_ERRORMSG_USERALREADYEXISTS);
				return;
			}
		}
		
		m_cUserlist.DeleteString(pos);
		pos=m_cUserlist.AddString(dlg.m_String);
		m_cUserlist.SetItemData(pos,index);
		m_cUserlist.SetCurSel(pos);	
		m_olduser=pos;
		m_UsersList[index].user=dlg.m_String;
		OnSelchangeUserlist();
	}
}

void CUsersDlg::SetCtrlState()
{
	if (m_cUserlist.GetCurSel()==LB_ERR)
	{
		GetDlgItem(IDC_USERREMOVE)->EnableWindow(FALSE);
		GetDlgItem(IDC_USERRENAME)->EnableWindow(FALSE);
		GetDlgItem(IDC_USERCOPY)->EnableWindow(FALSE);
	}
	else
	{
		GetDlgItem(IDC_USERREMOVE)->EnableWindow(TRUE);
		GetDlgItem(IDC_USERRENAME)->EnableWindow(TRUE);
		GetDlgItem(IDC_USERCOPY)->EnableWindow(TRUE);	
	}
	m_pGeneralPage->SetCtrlState();
	m_pSpeedLimitPage->SetCtrlState();
}

void CUsersDlg::OnUseradd() 
{
	OnUsermenuAdd();
}

void CUsersDlg::OnUsercopy() 
{
	OnUsermenuCopy();
}

void CUsersDlg::OnUserremove() 
{
	OnUsermenuRemove();
}

void CUsersDlg::OnUserrename() 
{
	OnUsermenuRename();
}

BOOL CUsersDlg::GetAsCommand(char **pBuffer, DWORD *nBufferLength)
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

BOOL CUsersDlg::Init(unsigned char *pData, DWORD dwDataLength)
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

BOOL CUsersDlg::PreTranslateMessage(MSG* pMsg) 
{
	if (pMsg->message==WM_KEYDOWN)
	{
		if (pMsg->wParam==VK_F2)
		{
			if (GetFocus() == &m_cUserlist)
			{
				if (m_cUserlist.GetCurSel() == LB_ERR)
					return TRUE;
				OnUsermenuRename();
			}
			return TRUE;
		}
	}
	return CSAPrefsDialog::PreTranslateMessage(pMsg);
}

t_user* CUsersDlg::GetCurrentUser()
{
	if (m_cUserlist.GetCurSel() == LB_ERR)
		return NULL;
	else
		return &m_UsersList[m_cUserlist.GetItemData(m_cUserlist.GetCurSel())];
}

