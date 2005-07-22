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

// UsersDlgGeneral.cpp: Implementierungsdatei
//

#include "stdafx.h"
#include "FileZilla server.h"
#include "UsersDlgGeneral.h"
#include "misc\md5.h"
#include "misc\sbdestination.h"
#include "entersomething.h"
#include "NewUserDlg.h"
#include "UsersDlg.h"
#include "UsersDlgSpeedLimit.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialogfeld CUsersDlgGeneral 

CUsersDlgGeneral::CUsersDlgGeneral(CUsersDlg* pOwner) 
	: CSAPrefsSubDlg(IDD)
{
	m_pOwner = pOwner;
	
	//{{AFX_DATA_INIT(CUsersDlgGeneral)
	m_bNeedpass = FALSE;
	m_Pass = _T("");
	m_bDirsCreate = FALSE;
	m_bDirsDelete = FALSE;
	m_bDirsList = FALSE;
	m_bDirsSubdirs = FALSE;
	m_bFilesAppend = FALSE;
	m_bFilesDelete = FALSE;
	m_bFilesRead = FALSE;
	m_bFilesWrite = FALSE;
	m_nLnk = FALSE;
	m_nRelative = FALSE;
	m_nMaxUsersBypass = FALSE;
	m_MaxConnCount = _T("");
	m_IpLimit = _T("");
	//}}AFX_DATA_INIT
}

CUsersDlgGeneral::~CUsersDlgGeneral()
{
}


void CUsersDlgGeneral::DoDataExchange(CDataExchange* pDX)
{
	CSAPrefsSubDlg::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CUsersDlgGeneral)
	DDX_Control(pDX, IDC_MAXCONNCOUNT, m_cMaxConnCount);
	DDX_Control(pDX, IDC_MAXUSERBYPASS, m_cMaxUsersBypass);
	DDX_Control(pDX, IDC_RELATIVE, m_cRelative);
	DDX_Control(pDX, IDC_LNK, m_cLnk);
	DDX_Control(pDX, IDC_NEEDPASS, m_cNeedpass);
	DDX_Control(pDX, IDC_GROUP, m_cGroup);
	DDX_Control(pDX, IDC_DIRS_CREATE, m_cDirsCreate);
	DDX_Control(pDX, IDC_DIRS_DELETE, m_cDirsDelete);
	DDX_Control(pDX, IDC_DIRS_LIST, m_cDirsList);
	DDX_Control(pDX, IDC_DIRS_SUBDIRS, m_cDirsSubdirs);
	DDX_Control(pDX, IDC_FILES_READ, m_cFilesRead);
	DDX_Control(pDX, IDC_FILES_WRITE, m_cFilesWrite);
	DDX_Control(pDX, IDC_FILES_DELETE, m_cFilesDelete);
	DDX_Control(pDX, IDC_FILES_APPEND, m_cFilesAppend);
	DDX_Control(pDX, IDC_DIRS, m_cDirs);
	DDX_Control(pDX, IDC_PASS, m_cPass);
	DDX_Control(pDX, IDC_IPLIMIT, m_cIpLimit);
	DDX_Check(pDX, IDC_NEEDPASS, m_bNeedpass);
	DDX_Text(pDX, IDC_PASS, m_Pass);
	DDX_Check(pDX, IDC_DIRS_CREATE, m_bDirsCreate);
	DDX_Check(pDX, IDC_DIRS_DELETE, m_bDirsDelete);
	DDX_Check(pDX, IDC_DIRS_LIST, m_bDirsList);
	DDX_Check(pDX, IDC_DIRS_SUBDIRS, m_bDirsSubdirs);
	DDX_Check(pDX, IDC_FILES_APPEND, m_bFilesAppend);
	DDX_Check(pDX, IDC_FILES_DELETE, m_bFilesDelete);
	DDX_Check(pDX, IDC_FILES_READ, m_bFilesRead);
	DDX_Check(pDX, IDC_FILES_WRITE, m_bFilesWrite);
	DDX_Check(pDX, IDC_LNK, m_nLnk);
	DDX_Check(pDX, IDC_RELATIVE, m_nRelative);
	DDX_Check(pDX, IDC_MAXUSERBYPASS, m_nMaxUsersBypass);
	DDX_Text(pDX, IDC_MAXCONNCOUNT, m_MaxConnCount);
	DDV_MaxChars(pDX, m_MaxConnCount, 9);
	DDX_Text(pDX, IDC_IPLIMIT, m_IpLimit);
	DDV_MaxChars(pDX, m_IpLimit, 9);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CUsersDlgGeneral, CSAPrefsSubDlg)
	//{{AFX_MSG_MAP(CUsersDlgGeneral)
	ON_BN_CLICKED(IDC_NEEDPASS, OnNeedpass)
	ON_WM_CONTEXTMENU()
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_DIRS, OnItemchangedDirs)
	ON_NOTIFY(LVN_ITEMCHANGING, IDC_DIRS, OnItemchangingDirs)
	ON_COMMAND(ID_DIRMENU_ADD, OnDirmenuAdd)
	ON_COMMAND(ID_DIRMENU_REMOVE, OnDirmenuRemove)
	ON_COMMAND(ID_DIRMENU_RENAME, OnDirmenuRename)
	ON_COMMAND(ID_DIRMENU_SETASHOMEDIR, OnDirmenuSetashomedir)
	ON_NOTIFY(LVN_ENDLABELEDIT, IDC_DIRS, OnEndlabeleditDirs)
	ON_NOTIFY(NM_DBLCLK, IDC_DIRS, OnDblclkDirs)
	ON_BN_CLICKED(IDC_FILES_WRITE, OnFilesWrite)
	ON_BN_CLICKED(IDC_DIRADD, OnDiradd)
	ON_BN_CLICKED(IDC_DIRREMOVE, OnDirremove)
	ON_BN_CLICKED(IDC_DIRRENAME, OnDirrename)
	ON_BN_CLICKED(IDC_DIRSETASHOME, OnDirsetashome)
	ON_CBN_SELCHANGE(IDC_GROUP, OnSelchangeGroup)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Behandlungsroutinen für Nachrichten CUsersDlgGeneral 

BOOL CUsersDlgGeneral::OnInitDialog() 
{
	CSAPrefsSubDlg::OnInitDialog();
	
	m_bNeedpass = FALSE;
	m_Pass = "";
	m_cDirs.InsertColumn(0,"Directories",LVCFMT_LEFT,142);
	m_cDirs.InsertColumn(1,"Attributes",LVCFMT_LEFT,57);
	UpdateData(FALSE);
	
	m_imagelist.Create( 16, 16, ILC_MASK, 3, 3 );
	HICON icon;
	icon = AfxGetApp()->LoadIcon(IDI_EMPTY);
	m_imagelist.Add(icon);
	DestroyIcon(icon);
	icon = AfxGetApp()->LoadIcon(IDI_HOME);
	m_imagelist.Add(icon);
	DestroyIcon(icon);
	
	m_cDirs.SetImageList(&m_imagelist, LVSIL_SMALL);

	m_cGroup.AddString("<none>");
	for (CUsersDlg::t_GroupsList::iterator iter = m_pOwner->m_GroupsList.begin(); iter != m_pOwner->m_GroupsList.end(); iter++)
		m_cGroup.AddString(iter->group);

	SetCtrlState();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX-Eigenschaftenseiten sollten FALSE zurückgeben
}

void CUsersDlgGeneral::OnNeedpass() 
{
	UpdateData(TRUE);
	m_cPass.EnableWindow(m_bNeedpass);
}

CString CUsersDlgGeneral::Validate()
{
	//TODO: check for homedir
	
	UpdateData(TRUE);
	if (m_bNeedpass && m_Pass=="")
	{
		m_cPass.SetFocus();
		return _T("Empty passwords are not allowed. Please enter a password!");
	}
	if (atoi(m_MaxConnCount)<0 || atoi(m_MaxConnCount)>999999999)
	{
		m_cMaxConnCount.SetFocus();
		return _T("The maximum user count has to be between 0 and 999999999!");
	}
	if (atoi(m_MaxConnCount)<0 || atoi(m_MaxConnCount)>999999999)
	{
		m_cIpLimit.SetFocus();
		return _T("The maximum user limit per IP has to be between 0 and 999999999!");
	}
	return _T("");
}

void CUsersDlgGeneral::OnContextMenu(CWnd* pWnd, CPoint point) 
{
	if (pWnd==&m_cDirs)
	{
		CMenu menu;
		menu.LoadMenu(IDR_DIRCONTEXT);

		CMenu* pPopup = menu.GetSubMenu(0);
		ASSERT(pPopup != NULL);
		CWnd* pWndPopupOwner = this;
		while (pWndPopupOwner->GetStyle() & WS_CHILD)
			pWndPopupOwner = pWndPopupOwner->GetParent();
		
		if (!m_cDirs.GetFirstSelectedItemPosition())
		{
			pPopup->EnableMenuItem(ID_DIRMENU_REMOVE,MF_GRAYED);
			pPopup->EnableMenuItem(ID_DIRMENU_RENAME,MF_GRAYED);
			pPopup->EnableMenuItem(ID_DIRMENU_SETASHOMEDIR,MF_GRAYED);
		}
		if (point.x==-1)
			GetCursorPos(&point);
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y,
			pWndPopupOwner);
	}
}

void CUsersDlgGeneral::OnItemchangedDirs(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	t_user *pUser = m_pOwner->GetCurrentUser();
	if (!pUser)
		return;
	
	int nItem = pNMListView->iItem;
	POSITION selpos=m_cDirs.GetFirstSelectedItemPosition();
	if (selpos)
	{
		if (m_cDirs.GetNextSelectedItem(selpos) != nItem)
			return;
	}
	int index=pNMListView->lParam;
	if (nItem!=-1)
	{
		m_bFilesRead = pUser->permissions[index].bFileRead;
		m_bFilesWrite = pUser->permissions[index].bFileWrite;
		m_bFilesDelete = pUser->permissions[index].bFileDelete;
		m_bFilesAppend = pUser->permissions[index].bFileAppend;
		m_bDirsCreate = pUser->permissions[index].bDirCreate;
		m_bDirsDelete = pUser->permissions[index].bDirDelete;
		m_bDirsList = pUser->permissions[index].bDirList;
		m_bDirsSubdirs = pUser->permissions[index].bDirSubdirs;
	}
	UpdateData(FALSE);

	SetCtrlState();	
	*pResult = 0;
}

void CUsersDlgGeneral::OnItemchangingDirs(NMHDR* pNMHDR, LRESULT* pResult) 
{
	*pResult = 0;
	UpdateData(TRUE);
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	t_user *pUser = m_pOwner->GetCurrentUser();
	if (!pUser)
		return;
		
	POSITION selpos=m_cDirs.GetFirstSelectedItemPosition();
	if (selpos)
	{
		int item=m_cDirs.GetNextSelectedItem(selpos);
		int index=m_cDirs.GetItemData(item);
		pUser->permissions[index].bFileRead=m_bFilesRead;
		pUser->permissions[index].bFileWrite=m_bFilesWrite;
		pUser->permissions[index].bFileDelete=m_bFilesDelete;
		pUser->permissions[index].bFileAppend=m_bFilesAppend;
		pUser->permissions[index].bDirCreate=m_bDirsCreate;
		pUser->permissions[index].bDirDelete=m_bDirsDelete;
		pUser->permissions[index].bDirList=m_bDirsList;
		pUser->permissions[index].bDirSubdirs=m_bDirsSubdirs;
	}
}

void CUsersDlgGeneral::SetCtrlState()
{
	if (!m_pOwner->GetCurrentUser())
	{
		m_cNeedpass.EnableWindow(FALSE);
		m_cPass.EnableWindow(FALSE);
		m_cGroup.EnableWindow(FALSE);
		m_cDirs.EnableWindow(FALSE);
		m_cFilesRead.EnableWindow(FALSE);
		m_cFilesWrite.EnableWindow(FALSE);
		m_cFilesDelete.EnableWindow(FALSE);
		m_cFilesAppend.EnableWindow(FALSE);
		m_cDirsCreate.EnableWindow(FALSE);
		m_cDirsDelete.EnableWindow(FALSE);
		m_cDirsList.EnableWindow(FALSE);
		m_cDirsSubdirs.EnableWindow(FALSE);
		m_cLnk.EnableWindow(FALSE);
		m_cRelative.EnableWindow(FALSE);
		m_cMaxUsersBypass.EnableWindow(FALSE);
		m_cMaxConnCount.EnableWindow(FALSE);
		m_cIpLimit.EnableWindow(FALSE);
		GetDlgItem(IDC_DIRADD)->EnableWindow(FALSE);
		GetDlgItem(IDC_DIRREMOVE)->EnableWindow(FALSE);
		GetDlgItem(IDC_DIRRENAME)->EnableWindow(FALSE);
		GetDlgItem(IDC_DIRSETASHOME)->EnableWindow(FALSE);

		m_cGroup.SetCurSel(CB_ERR);

		m_bFilesAppend = m_bFilesDelete = m_bFilesRead = m_bFilesWrite = FALSE;
		m_bDirsCreate = m_bDirsDelete = m_bDirsList = m_bDirsSubdirs = FALSE;
		UpdateData(FALSE);
	}
	else
	{
		m_cNeedpass.EnableWindow(TRUE);
		m_cPass.EnableWindow(TRUE);
		m_cGroup.EnableWindow(TRUE);
		m_cDirs.EnableWindow(TRUE);
		m_cLnk.EnableWindow(TRUE);
		m_cRelative.EnableWindow(TRUE);
		m_cMaxUsersBypass.EnableWindow(TRUE);
		m_cMaxConnCount.EnableWindow(TRUE);
		m_cIpLimit.EnableWindow(TRUE);
		GetDlgItem(IDC_DIRADD)->EnableWindow(TRUE);
		
		if (m_cDirs.GetFirstSelectedItemPosition())
		{
			m_cFilesRead.EnableWindow(TRUE);
			m_cFilesWrite.EnableWindow(TRUE);
			m_cFilesDelete.EnableWindow(TRUE);
			if (m_bFilesWrite)
				m_cFilesAppend.EnableWindow(TRUE);
			else
				m_cFilesAppend.EnableWindow(FALSE);
			m_cDirsCreate.EnableWindow(TRUE);
			m_cDirsDelete.EnableWindow(TRUE);
			m_cDirsList.EnableWindow(TRUE);
			m_cDirsSubdirs.EnableWindow(TRUE);
			GetDlgItem(IDC_DIRREMOVE)->EnableWindow(TRUE);
			GetDlgItem(IDC_DIRRENAME)->EnableWindow(TRUE);
			GetDlgItem(IDC_DIRSETASHOME)->EnableWindow(TRUE);
		}
		else
		{
			m_bFilesAppend = m_bFilesDelete = m_bFilesRead = m_bFilesWrite = FALSE;
			m_bDirsCreate = m_bDirsDelete = m_bDirsList = m_bDirsSubdirs = FALSE;
			UpdateData(FALSE);
			m_cFilesRead.EnableWindow(FALSE);
			m_cFilesWrite.EnableWindow(FALSE);
			m_cFilesDelete.EnableWindow(FALSE);
			m_cFilesAppend.EnableWindow(FALSE);
			m_cDirsCreate.EnableWindow(FALSE);
			m_cDirsDelete.EnableWindow(FALSE);
			m_cDirsList.EnableWindow(FALSE);
			m_cDirsSubdirs.EnableWindow(FALSE);
			GetDlgItem(IDC_DIRREMOVE)->EnableWindow(FALSE);
			GetDlgItem(IDC_DIRRENAME)->EnableWindow(FALSE);
			GetDlgItem(IDC_DIRSETASHOME)->EnableWindow(FALSE);
		}
		OnNeedpass();
	}
}

void CUsersDlgGeneral::OnDirmenuAdd() 
{
	t_user *pUser = m_pOwner->GetCurrentUser();
	if (!pUser)
		return;
	
	t_directory dir;
	dir.bDirCreate=dir.bDirDelete=dir.bDirList=
		dir.bDirSubdirs=dir.bFileAppend=
		dir.bFileDelete=dir.bFileRead=
		dir.bFileWrite=dir.bIsHome=FALSE;
	dir.dir="";
	if (m_cGroup.GetCurSel()<=0 && !m_cDirs.GetItemCount())
		dir.bIsHome = TRUE;
	else
		dir.bIsHome = FALSE;

	pUser->permissions.push_back(dir);
	int nItem=m_cDirs.InsertItem(LVIF_TEXT |LVIF_PARAM|LVIF_IMAGE, 0, "<new directory>", 0, 0, dir.bIsHome?1:0, pUser->permissions.size()-1);
	m_cDirs.SetItemState(nItem, LVIS_SELECTED, LVIS_SELECTED);
	m_cDirs.SetItemState(nItem, LVIS_SELECTED, LVIS_SELECTED);
	m_cDirs.SetFocus();
	OnDblclkDirs(0, 0);	
}

void CUsersDlgGeneral::OnDirmenuRemove() 
{
	t_user *pUser = m_pOwner->GetCurrentUser();
	if (!pUser)
		return;

	POSITION selpos;
	selpos = m_cDirs.GetFirstSelectedItemPosition();
	if (!selpos)
		return;
	int nItem=m_cDirs.GetNextSelectedItem(selpos);
	int index=m_cDirs.GetItemData(nItem);
	m_cDirs.DeleteItem(nItem);
	int i=0;
	for (std::vector<t_directory>::iterator iter = pUser->permissions.begin(); iter!=pUser->permissions.end(); iter++, i++)
		if (i==index)
		{
			pUser->permissions.erase(iter);
			break;
		}
	for (i=0;i<m_cDirs.GetItemCount();i++)
	{
		int data=m_cDirs.GetItemData(i);
		if (data>index)
		{
			m_cDirs.SetItemData(i, data-1);
		}
	}
	SetCtrlState();
}

void CUsersDlgGeneral::OnDirmenuRename() 
{
	t_user *pUser = m_pOwner->GetCurrentUser();
	if (!pUser)
		return;

	POSITION selpos = m_cDirs.GetFirstSelectedItemPosition();
	if (!selpos)
		return;
	int nItem = m_cDirs.GetNextSelectedItem(selpos);
	
	m_cDirs.SetFocus();
	m_cDirs.EditLabel(nItem);
}

void CUsersDlgGeneral::OnDirmenuSetashomedir() 
{
	t_user *pUser = m_pOwner->GetCurrentUser();
	if (!pUser)
		return;

	POSITION selpos;
	selpos=m_cDirs.GetFirstSelectedItemPosition();
	if (!selpos)
		return;
	int nItem=m_cDirs.GetNextSelectedItem(selpos);
	
	for (unsigned int j=0; j<pUser->permissions.size(); j++)
	{
		LVITEM item;
		memset(&item,0,sizeof(item));
		item.mask=LVIF_IMAGE|LVIF_PARAM;
		item.iItem=j;
		m_cDirs.GetItem(&item);
		item.iImage = (j==(unsigned int)nItem)?1:0;
		pUser->permissions[item.lParam].bIsHome=0;
		m_cDirs.SetItem(&item);
	}
	int index = m_cDirs.GetItemData(nItem);
	pUser->permissions[index].bIsHome = 1;
}

void CUsersDlgGeneral::OnEndlabeleditDirs(NMHDR* pNMHDR, LRESULT* pResult) 
{
	LV_DISPINFO* pDispInfo = (LV_DISPINFO*)pNMHDR;
	if (pDispInfo->item.pszText)
	{
		if (pDispInfo->item.pszText[0]==0)
		{
			AfxMessageBox("Please select a folder!");
			*pResult=FALSE;
		}
		else
		{
			t_user *pUser = m_pOwner->GetCurrentUser();
			if (!pUser)
				return;
			pUser->permissions[pDispInfo->item.lParam].dir = pDispInfo->item.pszText;
			*pResult=TRUE;
		}	
	}
	else
	{
		if (m_cDirs.GetItemText(pDispInfo->item.iItem,0)=="")
		{
			t_user *pUser = m_pOwner->GetCurrentUser();
			if (!pUser)
				return;
			
			m_cDirs.DeleteItem(pDispInfo->item.iItem);
			int i=0;
			for (std::vector<t_directory>::iterator iter=pUser->permissions.begin(); iter!=pUser->permissions.end(); iter++, i++)
				if (i==pDispInfo->item.lParam)
				{
					pUser->permissions.erase(iter);
					break;
				}
		}
	}
}

void CUsersDlgGeneral::OnDblclkDirs(NMHDR* pNMHDR, LRESULT* pResult) 
{
	t_user *pUser = m_pOwner->GetCurrentUser();
	if (!pUser)
		return;
	
	POSITION selpos=m_cDirs.GetFirstSelectedItemPosition();
	if (!selpos)
		return;
	int nItem=m_cDirs.GetNextSelectedItem(selpos);
	int index=m_cDirs.GetItemData(nItem);
	CSBDestination sb(m_hWnd, IDS_BROWSEFORFOLDER);
	sb.SetFlags(BIF_RETURNONLYFSDIRS | BIF_STATUSTEXT);
	sb.SetInitialSelection(m_cDirs.GetItemText(nItem,0));
	if (sb.SelectFolder())
	{
		m_cDirs.SetItemText(nItem,0,sb.GetSelectedFolder());
		pUser->permissions[index].dir=sb.GetSelectedFolder();
	}
	
	if (pResult)
		*pResult = 0;
}

void CUsersDlgGeneral::OnFilesWrite() 
{
	UpdateData(TRUE);
	SetCtrlState();	
}

void CUsersDlgGeneral::OnDiradd() 
{
	OnDirmenuAdd();	
}

void CUsersDlgGeneral::OnDirremove() 
{
	OnDirmenuRemove();	
}

void CUsersDlgGeneral::OnDirrename() 
{
	OnDirmenuRename();	
}

void CUsersDlgGeneral::OnDirsetashome() 
{
	OnDirmenuSetashomedir();	
}

BOOL CUsersDlgGeneral::PreTranslateMessage(MSG* pMsg) 
{
	if (pMsg->message==WM_KEYDOWN)
	{
		if (pMsg->wParam==VK_F2)
		{
			if (GetFocus() == &m_cDirs)
			{
				if (m_cDirs.GetEditControl())
					return TRUE;
				OnDirmenuRename();
			}
			return TRUE;
		}
	}
	return CSAPrefsSubDlg::PreTranslateMessage(pMsg);
}

void CUsersDlgGeneral::OnSelchangeGroup() 
{
	if (m_cGroup.GetCurSel()<=0)
	{
		UpdateData(TRUE);
		if (m_nLnk == 2)
			m_nLnk = 0;
		if (m_nRelative == 2)
			m_nRelative = 1;
		if (m_nMaxUsersBypass == 2)
			m_nMaxUsersBypass = 0;
		UpdateData(FALSE);
		m_cLnk.SetButtonStyle(BS_AUTOCHECKBOX);
		m_cRelative.SetButtonStyle(BS_AUTOCHECKBOX);
		m_cMaxUsersBypass.SetButtonStyle(BS_AUTOCHECKBOX);
		
		
		m_pOwner->m_pSpeedLimitPage->UpdateData(TRUE);
		CButton *pButton = reinterpret_cast<CButton *>(m_pOwner->m_pSpeedLimitPage->GetDlgItem(IDC_USERS_SPEEDLIMIT_SERVERBYPASS_DOWNLOAD));
		if (pButton->GetCheck() == 2)
			pButton->SetCheck(0);
		pButton->SetButtonStyle(BS_AUTOCHECKBOX);

		pButton = reinterpret_cast<CButton *>(m_pOwner->m_pSpeedLimitPage->GetDlgItem(IDC_USERS_SPEEDLIMIT_SERVERBYPASS_DOWNLOAD));
		if (pButton->GetCheck() == 2)
			pButton->SetCheck(0);
		pButton->SetButtonStyle(BS_AUTOCHECKBOX);
		m_pOwner->m_pSpeedLimitPage->UpdateData(FALSE);
	}
	else
	{
		m_cLnk.SetButtonStyle(BS_AUTO3STATE);
		m_cRelative.SetButtonStyle(BS_AUTO3STATE);
		m_cMaxUsersBypass.SetButtonStyle(BS_AUTO3STATE);
		((CButton *)m_pOwner->m_pSpeedLimitPage->GetDlgItem(IDC_USERS_SPEEDLIMIT_SERVERBYPASS_DOWNLOAD))->SetButtonStyle(BS_AUTO3STATE);
		((CButton *)m_pOwner->m_pSpeedLimitPage->GetDlgItem(IDC_USERS_SPEEDLIMIT_SERVERBYPASS_UPLOAD))->SetButtonStyle(BS_AUTO3STATE);
	}
}

BOOL CUsersDlgGeneral::DisplayUser(const t_user *pUser)
{
	if (!pUser)
	{
		m_cDirs.DeleteAllItems();
		m_bNeedpass = FALSE;
		m_Pass = "";
		m_nRelative = 0;
		m_nLnk = 0;
		m_nMaxUsersBypass = 0;
		m_IpLimit = "";
		m_MaxConnCount = "";
		m_bFilesRead = m_bFilesWrite = m_bFilesDelete = m_bFilesAppend = FALSE;
		m_bDirsCreate = m_bDirsList = m_bDirsDelete = m_bDirsSubdirs = FALSE;
		return TRUE;
	}
	
	m_Pass = pUser->password;
	m_cPass.SetModify(FALSE);
	m_bNeedpass = pUser->password!="";
	
	if (pUser->group=="" || m_cGroup.SelectString(-1, pUser->group) == CB_ERR)
	{
		m_cLnk.SetButtonStyle(BS_AUTOCHECKBOX);
		m_cRelative.SetButtonStyle(BS_AUTOCHECKBOX);
		m_cMaxUsersBypass.SetButtonStyle(BS_AUTOCHECKBOX);
		m_cGroup.SetCurSel(0);
	}
	else
	{
		m_cLnk.SetButtonStyle(BS_AUTO3STATE);
		m_cRelative.SetButtonStyle(BS_AUTO3STATE);
		m_cMaxUsersBypass.SetButtonStyle(BS_AUTO3STATE);
	}
	m_nLnk = pUser->nLnk;
	m_nRelative = pUser->nRelative;
	m_nMaxUsersBypass = pUser->nBypassUserLimit;
	CString str;
	str.Format("%d", pUser->nUserLimit);
	m_MaxConnCount = str;
	str.Format("%d", pUser->nIpLimit);
	m_IpLimit = str;
	UpdateData(FALSE);
	
	//Fill the dirs list
	m_cDirs.DeleteAllItems();
	for (unsigned int j=0; j<pUser->permissions.size(); j++)
	{
		int nItem = m_cDirs.InsertItem(j, pUser->permissions[j].dir);
		LVITEM item;
		memset(&item,0,sizeof(item));
		item.mask=LVIF_IMAGE|LVIF_PARAM;
		item.iItem = nItem;
		m_cDirs.GetItem(&item);
		item.lParam=j;
		item.iImage = pUser->permissions[j].bIsHome?1:0;
		m_cDirs.SetItem(&item);
	}

	return TRUE;
}

BOOL CUsersDlgGeneral::SaveUser(t_user *pUser)
{
	if (!pUser)
		return FALSE;
	
	pUser->password = m_Pass;
	if (!m_bNeedpass)
		pUser->password = "";
	else if (m_cPass.GetModify() && m_Pass != "")
	{
		const char *tmp = pUser->password;
		MD5 md5;
		md5.update((unsigned char *)tmp,pUser->password.GetLength());
		md5.finalize();
		char *res = md5.hex_digest();
		CString hash = res;
		delete [] res;
		pUser->password = hash;	
	}
	
	pUser->nLnk = m_nLnk;
	pUser->nRelative = m_nRelative;
	pUser->nBypassUserLimit = m_nMaxUsersBypass;
	pUser->nUserLimit = atoi(m_MaxConnCount);
	pUser->nIpLimit = atoi(m_IpLimit);
	if (m_cGroup.GetCurSel()<=0)
		pUser->group = "";
	else
		m_cGroup.GetLBText(m_cGroup.GetCurSel(), pUser->group);
	
	POSITION selpos = m_cDirs.GetFirstSelectedItemPosition();
	if (selpos)
	{
		int item = m_cDirs.GetNextSelectedItem(selpos);
		int index = m_cDirs.GetItemData(item);
		pUser->permissions[index].bFileRead = m_bFilesRead;
		pUser->permissions[index].bFileWrite = m_bFilesWrite;
		pUser->permissions[index].bFileDelete = m_bFilesDelete;
		pUser->permissions[index].bFileAppend = m_bFilesAppend;
		pUser->permissions[index].bDirCreate = m_bDirsCreate;
		pUser->permissions[index].bDirDelete = m_bDirsDelete;
		pUser->permissions[index].bDirList = m_bDirsList;
		pUser->permissions[index].bDirSubdirs = m_bDirsSubdirs;
	}		
	return TRUE;
}
