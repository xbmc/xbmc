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
#include "GroupsDlgGeneral.h"
#include "GroupsDlg.h"
#include "misc\sbdestination.h"
#include "entersomething.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialogfeld CGroupsDlgGeneral 

CGroupsDlgGeneral::CGroupsDlgGeneral(CGroupsDlg *pOwner)
	: CSAPrefsSubDlg(CGroupsDlgGeneral::IDD)
{
	ASSERT(pOwner);
	m_pOwner = pOwner;
	
	//{{AFX_DATA_INIT(CGroupsDlgGeneral)
	m_bDirsCreate = FALSE;
	m_bDirsDelete = FALSE;
	m_bDirsList = FALSE;
	m_bDirsSubdirs = FALSE;
	m_bFilesAppend = FALSE;
	m_bFilesDelete = FALSE;
	m_bFilesRead = FALSE;
	m_bFilesWrite = FALSE;
	m_bAutoCreate = FALSE;
	m_nLnk = FALSE;
	m_nRelative = FALSE;
	m_nMaxUsersBypass = FALSE;
	m_MaxConnCount = _T("");
	m_IpLimit = _T("");
	//}}AFX_DATA_INIT
}

CGroupsDlgGeneral::~CGroupsDlgGeneral()
{
}


void CGroupsDlgGeneral::DoDataExchange(CDataExchange* pDX)
{
	CSAPrefsSubDlg::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGroupsDlgGeneral)
	DDX_Control(pDX, IDC_GROUPS_MAXCONNCOUNT, m_cMaxConnCount);
	DDX_Control(pDX, IDC_GROUPS_MAXUSERBYPASS, m_cMaxUsersBypass);
	DDX_Control(pDX, IDC_RELATIVE, m_cRelative);
	DDX_Control(pDX, IDC_LNK, m_cLnk);
	DDX_Control(pDX, IDC_DIRS_CREATE, m_cDirsCreate);
	DDX_Control(pDX, IDC_DIRS_DELETE, m_cDirsDelete);
	DDX_Control(pDX, IDC_DIRS_LIST, m_cDirsList);
	DDX_Control(pDX, IDC_DIRS_SUBDIRS, m_cDirsSubdirs);
	DDX_Control(pDX, IDC_FILES_READ, m_cFilesRead);
	DDX_Control(pDX, IDC_FILES_WRITE, m_cFilesWrite);
	DDX_Control(pDX, IDC_FILES_DELETE, m_cFilesDelete);
	DDX_Control(pDX, IDC_FILES_APPEND, m_cFilesAppend);
	DDX_Control(pDX, IDC_GROUPS_AUTOCREATE, m_cAutoCreate);
	DDX_Control(pDX, IDC_DIRS, m_cDirs);
	DDX_Control(pDX, IDC_GROUPS_IPLIMIT, m_cIpLimit);
	DDX_Check(pDX, IDC_DIRS_CREATE, m_bDirsCreate);
	DDX_Check(pDX, IDC_DIRS_DELETE, m_bDirsDelete);
	DDX_Check(pDX, IDC_DIRS_LIST, m_bDirsList);
	DDX_Check(pDX, IDC_DIRS_SUBDIRS, m_bDirsSubdirs);
	DDX_Check(pDX, IDC_FILES_APPEND, m_bFilesAppend);
	DDX_Check(pDX, IDC_FILES_DELETE, m_bFilesDelete);
	DDX_Check(pDX, IDC_FILES_READ, m_bFilesRead);
	DDX_Check(pDX, IDC_FILES_WRITE, m_bFilesWrite);
	DDX_Check(pDX, IDC_GROUPS_AUTOCREATE, m_bAutoCreate);
	DDX_Check(pDX, IDC_LNK, m_nLnk);
	DDX_Check(pDX, IDC_RELATIVE, m_nRelative);
	DDX_Check(pDX, IDC_GROUPS_MAXUSERBYPASS, m_nMaxUsersBypass);
	DDX_Text(pDX, IDC_GROUPS_MAXCONNCOUNT, m_MaxConnCount);
	DDV_MaxChars(pDX, m_MaxConnCount, 9);
	DDX_Text(pDX, IDC_GROUPS_IPLIMIT, m_IpLimit);
	DDV_MaxChars(pDX, m_IpLimit, 9);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CGroupsDlgGeneral, CSAPrefsSubDlg)
	//{{AFX_MSG_MAP(CGroupsDlgGeneral)
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
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Behandlungsroutinen für Nachrichten CGroupsDlgGeneral 

BOOL CGroupsDlgGeneral::OnInitDialog() 
{
	CSAPrefsSubDlg::OnInitDialog();
	
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
			
	SetCtrlState();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX-Propertypages should return FALSE
}

CString CGroupsDlgGeneral::Validate()
{
	//TODO: check for homedir
	
	UpdateData(TRUE);
	t_group *pGroup = m_pOwner->GetCurrentGroup();
	if (pGroup)
	{
		BOOL bFoundHome = TRUE;
		for (std::vector<t_directory>::iterator permissioniter = pGroup->permissions.begin(); permissioniter!=pGroup->permissions.end(); permissioniter++)
		{
			if (permissioniter->bIsHome)
			{
				bFoundHome = TRUE;
				break;
			}
		}
		if (!bFoundHome)
		{
			m_cDirs.SetFocus();
			return _T("Please select a home dir");
		}
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

void CGroupsDlgGeneral::OnOK() 
{
	if (!Validate())
		return;
	
	CSAPrefsSubDlg::OnOK();
}

void CGroupsDlgGeneral::OnContextMenu(CWnd* pWnd, CPoint point) 
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

void CGroupsDlgGeneral::OnItemchangedDirs(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	
	t_group *pGroup = m_pOwner->GetCurrentGroup();
	if (!pGroup)
		return;
	
	int nItem = pNMListView->iItem;
	POSITION selpos = m_cDirs.GetFirstSelectedItemPosition();
	if (selpos)
	{
		if (m_cDirs.GetNextSelectedItem(selpos)!=nItem)
			return;
	}
	int index = pNMListView->lParam;
	if (nItem!=-1)
	{
		m_bFilesRead = pGroup->permissions[index].bFileRead;
		m_bFilesWrite = pGroup->permissions[index].bFileWrite;
		m_bFilesDelete = pGroup->permissions[index].bFileDelete;
		m_bFilesAppend = pGroup->permissions[index].bFileAppend;
		m_bDirsCreate = pGroup->permissions[index].bDirCreate;
		m_bDirsDelete = pGroup->permissions[index].bDirDelete;
		m_bDirsList = pGroup->permissions[index].bDirList;
		m_bDirsSubdirs = pGroup->permissions[index].bDirSubdirs;
		m_bAutoCreate = pGroup->permissions[index].bAutoCreate;
	}
	UpdateData(FALSE);

	SetCtrlState();	
	*pResult = 0;
}

void CGroupsDlgGeneral::OnItemchangingDirs(NMHDR* pNMHDR, LRESULT* pResult) 
{
	*pResult = 0;
	UpdateData(TRUE);
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	t_group *pGroup = m_pOwner->GetCurrentGroup();
	if (!pGroup)
		return;

		
	POSITION selpos=m_cDirs.GetFirstSelectedItemPosition();
	if (selpos)
	{
		int item = m_cDirs.GetNextSelectedItem(selpos);
		int index = m_cDirs.GetItemData(item);
		pGroup->permissions[index].bFileRead = m_bFilesRead;
		pGroup->permissions[index].bFileWrite = m_bFilesWrite;
		pGroup->permissions[index].bFileDelete = m_bFilesDelete;
		pGroup->permissions[index].bFileAppend = m_bFilesAppend;
		pGroup->permissions[index].bDirCreate = m_bDirsCreate;
		pGroup->permissions[index].bDirDelete = m_bDirsDelete;
		pGroup->permissions[index].bDirList = m_bDirsList;
		pGroup->permissions[index].bDirSubdirs = m_bDirsSubdirs;
		pGroup->permissions[index].bAutoCreate = m_bAutoCreate;
	}
}

void CGroupsDlgGeneral::SetCtrlState()
{
	t_group *pGroup = m_pOwner->GetCurrentGroup();
	if (!pGroup)
	{
		m_cDirs.EnableWindow(FALSE);
		m_cFilesRead.EnableWindow(FALSE);
		m_cFilesWrite.EnableWindow(FALSE);
		m_cFilesDelete.EnableWindow(FALSE);
		m_cFilesAppend.EnableWindow(FALSE);
		m_cDirsCreate.EnableWindow(FALSE);
		m_cDirsDelete.EnableWindow(FALSE);
		m_cDirsList.EnableWindow(FALSE);
		m_cDirsSubdirs.EnableWindow(FALSE);
		m_cAutoCreate.EnableWindow(FALSE);
		m_cLnk.EnableWindow(FALSE);
		m_cRelative.EnableWindow(FALSE);
		m_cMaxUsersBypass.EnableWindow(FALSE);
		m_cMaxConnCount.EnableWindow(FALSE);
		m_cIpLimit.EnableWindow(FALSE);
		GetDlgItem(IDC_DIRADD)->EnableWindow(FALSE);
		GetDlgItem(IDC_DIRREMOVE)->EnableWindow(FALSE);
		GetDlgItem(IDC_DIRRENAME)->EnableWindow(FALSE);
		GetDlgItem(IDC_DIRSETASHOME)->EnableWindow(FALSE);

		m_bFilesAppend = m_bFilesDelete = m_bFilesRead = m_bFilesWrite = FALSE;
		m_bDirsCreate = m_bDirsDelete = m_bDirsList = m_bDirsSubdirs = FALSE;
		m_bAutoCreate = FALSE;
		UpdateData(FALSE);
	}
	else
	{
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
			m_cAutoCreate.EnableWindow(TRUE);
			GetDlgItem(IDC_DIRREMOVE)->EnableWindow(TRUE);
			GetDlgItem(IDC_DIRRENAME)->EnableWindow(TRUE);
			GetDlgItem(IDC_DIRSETASHOME)->EnableWindow(TRUE);
		}
		else
		{
			m_bFilesAppend = m_bFilesDelete = m_bFilesRead = m_bFilesWrite = FALSE;
			m_bDirsCreate = m_bDirsDelete = m_bDirsList = m_bDirsSubdirs = FALSE;
			m_bAutoCreate = FALSE;
			UpdateData(FALSE);
			m_cFilesRead.EnableWindow(FALSE);
			m_cFilesWrite.EnableWindow(FALSE);
			m_cFilesDelete.EnableWindow(FALSE);
			m_cFilesAppend.EnableWindow(FALSE);
			m_cDirsCreate.EnableWindow(FALSE);
			m_cDirsDelete.EnableWindow(FALSE);
			m_cDirsList.EnableWindow(FALSE);
			m_cDirsSubdirs.EnableWindow(FALSE);
			m_cAutoCreate.EnableWindow(FALSE);
			GetDlgItem(IDC_DIRREMOVE)->EnableWindow(FALSE);
			GetDlgItem(IDC_DIRRENAME)->EnableWindow(FALSE);
			GetDlgItem(IDC_DIRSETASHOME)->EnableWindow(FALSE);
		}
	}
}

void CGroupsDlgGeneral::OnDirmenuAdd() 
{
	t_group *pGroup = m_pOwner->GetCurrentGroup();
	if (!pGroup)
		return;

	t_directory dir;
	dir.bDirCreate=dir.bDirDelete=dir.bDirList=
		dir.bDirSubdirs=dir.bFileAppend=
		dir.bFileDelete=dir.bFileRead=
		dir.bFileWrite=dir.bIsHome =
		dir.bAutoCreate = FALSE;
	dir.dir = "";
	dir.bIsHome = m_cDirs.GetItemCount()?FALSE:TRUE;

	pGroup->permissions.push_back(dir);
	int nItem = m_cDirs.InsertItem(LVIF_TEXT |LVIF_PARAM|LVIF_IMAGE, 0, "<new directory>", 0, 0, dir.bIsHome?1:0, pGroup->permissions.size()-1);
	m_cDirs.SetItemState(nItem, LVIS_SELECTED,LVIS_SELECTED);
	m_cDirs.SetItemState(nItem, LVIS_SELECTED,LVIS_SELECTED);
	OnDblclkDirs(0, 0);	
}

void CGroupsDlgGeneral::OnDirmenuRemove() 
{
t_group *pGroup = m_pOwner->GetCurrentGroup();
	if (!pGroup)
		return;
	
	POSITION selpos;
	selpos=m_cDirs.GetFirstSelectedItemPosition();
	if (!selpos)
		return;
	int nItem=m_cDirs.GetNextSelectedItem(selpos);
	int index=m_cDirs.GetItemData(nItem);
	m_cDirs.DeleteItem(nItem);
	int i=0;
	for (std::vector<t_directory>::iterator iter=pGroup->permissions.begin(); iter!=pGroup->permissions.end(); iter++, i++)
		if (i==index)
		{
			pGroup->permissions.erase(iter);
			break;
		}
	for (i=0;i<m_cDirs.GetItemCount();i++)
	{
		int data=m_cDirs.GetItemData(i);
		if (data>index)
		{
			m_cDirs.SetItemData(i,data-1);
		}
	}
	SetCtrlState();
}

void CGroupsDlgGeneral::OnDirmenuRename() 
{
	t_group *pGroup = m_pOwner->GetCurrentGroup();
	if (!pGroup)
		return;
	
	POSITION selpos = m_cDirs.GetFirstSelectedItemPosition();
	if (!selpos)
		return;
	int nItem = m_cDirs.GetNextSelectedItem(selpos);
	
	m_cDirs.SetFocus();
	m_cDirs.EditLabel(nItem);
}

void CGroupsDlgGeneral::OnDirmenuSetashomedir() 
{
	t_group *pGroup = m_pOwner->GetCurrentGroup();
	if (!pGroup)
		return;
	
	POSITION selpos;
	selpos=m_cDirs.GetFirstSelectedItemPosition();
	if (!selpos)
		return;
	int nItem=m_cDirs.GetNextSelectedItem(selpos);
	
	for (unsigned int j=0; j<pGroup->permissions.size(); j++)
	{
		LVITEM item;
		memset(&item,0,sizeof(item));
		item.mask=LVIF_IMAGE|LVIF_PARAM;
		item.iItem=j;
		m_cDirs.GetItem(&item);
		item.iImage = (j==(unsigned int)nItem)?1:0;
		pGroup->permissions[item.lParam].bIsHome=0;
		m_cDirs.SetItem(&item);
	}
	int index=m_cDirs.GetItemData(nItem);
	pGroup->permissions[index].bIsHome=1;
}

void CGroupsDlgGeneral::OnEndlabeleditDirs(NMHDR* pNMHDR, LRESULT* pResult) 
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
			t_group *pGroup = m_pOwner->GetCurrentGroup();
			if (!pGroup)
				return;
						
			pGroup->permissions[pDispInfo->item.lParam].dir=pDispInfo->item.pszText;
			*pResult=TRUE;
		}
	
	}
	else
	{
		if (m_cDirs.GetItemText(pDispInfo->item.iItem,0)=="")
		{
			t_group *pGroup = m_pOwner->GetCurrentGroup();
			if (!pGroup)
				return;
			
			m_cDirs.DeleteItem(pDispInfo->item.iItem);
			int i=0;
			for (std::vector<t_directory>::iterator iter=pGroup->permissions.begin(); iter!=pGroup->permissions.end(); iter++, i++)
				if (i==pDispInfo->item.lParam)
				{
					pGroup->permissions.erase(iter);
					break;
				}
		}
	}
}

void CGroupsDlgGeneral::OnDblclkDirs(NMHDR* pNMHDR, LRESULT* pResult) 
{
	t_group *pGroup = m_pOwner->GetCurrentGroup();
	if (!pGroup)
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
		pGroup->permissions[index].dir=sb.GetSelectedFolder();
	}
	
	if (pResult)
		*pResult = 0;
}

void CGroupsDlgGeneral::OnFilesWrite() 
{
	UpdateData(TRUE);
	SetCtrlState();	
}

void CGroupsDlgGeneral::OnDiradd() 
{
	OnDirmenuAdd();	
}

void CGroupsDlgGeneral::OnDirremove() 
{
	OnDirmenuRemove();	
}

void CGroupsDlgGeneral::OnDirrename() 
{
	OnDirmenuRename();	
}

void CGroupsDlgGeneral::OnDirsetashome() 
{
	OnDirmenuSetashomedir();	
}

BOOL CGroupsDlgGeneral::PreTranslateMessage(MSG* pMsg) 
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

BOOL CGroupsDlgGeneral::DisplayGroup(const t_group *pGroup)
{
	if (!pGroup)
	{
		m_cDirs.DeleteAllItems();
		m_nRelative = 0;
		m_nLnk = 0;
		m_nMaxUsersBypass = 0;
		m_IpLimit = "";
		m_MaxConnCount = "";
		m_bFilesRead = m_bFilesWrite = m_bFilesDelete = m_bFilesAppend = FALSE;
		m_bDirsCreate = m_bDirsList = m_bDirsDelete = m_bDirsSubdirs = FALSE;
		return TRUE;
	}
	
	m_nLnk = pGroup->nLnk;
	m_nRelative = pGroup->nRelative;
	m_nMaxUsersBypass = pGroup->nBypassUserLimit;
	CString str;
	str.Format("%d", pGroup->nUserLimit);
	m_MaxConnCount = str;
	str.Format("%d", pGroup->nIpLimit);
	m_IpLimit = str;
	UpdateData(FALSE);
	
	//Fill the dirs list
	m_cDirs.DeleteAllItems();
	for (unsigned int j=0; j<pGroup->permissions.size(); j++)
	{
		int nItem = m_cDirs.InsertItem(j, pGroup->permissions[j].dir);
		LVITEM item;
		memset(&item,0,sizeof(item));
		item.mask=LVIF_IMAGE|LVIF_PARAM;
		item.iItem = nItem;
		m_cDirs.GetItem(&item);
		item.lParam=j;
		item.iImage = pGroup->permissions[j].bIsHome?1:0;
		m_cDirs.SetItem(&item);
	}

	return TRUE;
}

BOOL CGroupsDlgGeneral::SaveGroup(t_group *pGroup)
{
	if (!pGroup)
		return FALSE;
		
	pGroup->nLnk = m_nLnk;
	pGroup->nRelative = m_nRelative;
	pGroup->nBypassUserLimit = m_nMaxUsersBypass;
	pGroup->nUserLimit = atoi(m_MaxConnCount);
	pGroup->nIpLimit = atoi(m_IpLimit);
	
	POSITION selpos = m_cDirs.GetFirstSelectedItemPosition();
	if (selpos)
	{
		int item = m_cDirs.GetNextSelectedItem(selpos);
		int index = m_cDirs.GetItemData(item);
		pGroup->permissions[index].bFileRead = m_bFilesRead;
		pGroup->permissions[index].bFileWrite = m_bFilesWrite;
		pGroup->permissions[index].bFileDelete = m_bFilesDelete;
		pGroup->permissions[index].bFileAppend = m_bFilesAppend;
		pGroup->permissions[index].bDirCreate = m_bDirsCreate;
		pGroup->permissions[index].bDirDelete = m_bDirsDelete;
		pGroup->permissions[index].bDirList = m_bDirsList;
		pGroup->permissions[index].bDirSubdirs = m_bDirsSubdirs;
		pGroup->permissions[index].bAutoCreate = m_bAutoCreate;
	}		
	return TRUE;
}
