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

// UsersListCtrl.cpp: Implementierungsdatei
//

#include "stdafx.h"
#include "filezilla server.h"
#include "UsersListCtrl.h"
#include "mainfrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CUsersListCtrl

CUsersListCtrl::CUsersListCtrl(CMainFrame *pOwner)
{
	ASSERT(pOwner);
	m_pOwner = pOwner;
}

CUsersListCtrl::~CUsersListCtrl()
{
	for (int i=0;i<GetItemCount();i++)
		delete (t_connectiondata *)GetItemData(i);
}


BEGIN_MESSAGE_MAP(CUsersListCtrl, CListCtrl)
	//{{AFX_MSG_MAP(CUsersListCtrl)
	ON_WM_CREATE()
	ON_COMMAND(ID_USERVIEWCONTEXT_KICK, OnContextmenuKick)
	ON_WM_CONTEXTMENU()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Behandlungsroutinen für Nachrichten CUsersListCtrl 

int CUsersListCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CListCtrl::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	InsertColumn(0,"Connected users",LVCFMT_LEFT,1000);
	return 0;
}

void CUsersListCtrl::ProcessConnOp(int op, const t_connectiondata &connectionData)
{
	if (op==USERCONTROL_CONNOP_ADD)
	{
		CString str;
		if (connectionData.user=="")
			str.Format("(not logged in) (%06d)",connectionData.userid);
		else
			str.Format("%s (%06d)",connectionData.user,connectionData.userid);
		int index=InsertItem(GetItemCount(),str);
		t_connectiondata *pData = new t_connectiondata;
		*pData = connectionData;
		SetItemData(index, (DWORD)pData);
	}
	else if (op==USERCONTROL_CONNOP_MODIFY)
	{
		for (int i=0;i<GetItemCount();i++)
		{
			t_connectiondata *pData=(t_connectiondata *)GetItemData(i);
			if (pData->userid==connectionData.userid)
			{
				*pData = connectionData;

				CString str;
				if (connectionData.user=="")
					str.Format("(not logged in) (%06d)",connectionData.userid);
				else
					str.Format("%s (%06d)",connectionData.user,connectionData.userid);
				SetItemText(i,0,str);
				break;
			}
		}
	}
	else if (op==USERCONTROL_CONNOP_REMOVE)
	{
		for (int i=0;i<GetItemCount();i++)
		{
			t_connectiondata *pData=(t_connectiondata *)GetItemData(i);
			if (pData->userid==connectionData.userid)
			{
				delete pData;
				DeleteItem(i);
				break;
			}
		}
	}
}

void CUsersListCtrl::OnContextmenuKick() 
{
	if (AfxMessageBox("Do you really want to kick the selected user?", MB_ICONQUESTION|MB_YESNO)!=IDYES)
		return;
	POSITION pos=GetFirstSelectedItemPosition();
	while (pos)
	{
		int nItem=GetNextSelectedItem(pos);
		
		t_connectiondata *data=(t_connectiondata *)GetItemData(nItem);
		
		unsigned char buffer[5];
		buffer[0]=USERCONTROL_KICK;
		memcpy(buffer+1, &data->userid, 5);
		m_pOwner->SendCommand(3, &buffer, 5);
	}	
}

void CUsersListCtrl::OnContextMenu(CWnd* pWnd, CPoint point) 
{
	CMenu menu;
	menu.LoadMenu(IDR_USERVIEWCONTEXT);

	CMenu* pPopup = menu.GetSubMenu(0);
	ASSERT(pPopup != NULL);
	CWnd* pWndPopupOwner = this;
	//while (pWndPopupOwner->GetStyle() & WS_CHILD)
	//	pWndPopupOwner = pWndPopupOwner->GetParent();

	POSITION pos=GetFirstSelectedItemPosition();
	if (!pos)
	{
		pPopup->EnableMenuItem(ID_USERVIEWCONTEXT_KICK,MF_GRAYED);
	}
		
	pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y,
		pWndPopupOwner);
}

BOOL CUsersListCtrl::ParseUserControlCommand(unsigned char *pData, DWORD dwDataLength)
{
	int type=*pData;
	if (type<0 || type>4)
	{
		m_pOwner->ShowStatus(_T("Protocol error: Invalid data"), 1);
		return FALSE;
	}
	switch (type)
	{
	case USERCONTROL_GETLIST:
		if (dwDataLength < 3)
			return FALSE;
		else
		{
			int i;
			for (i=0;i<GetItemCount();i++)
				delete (t_connectiondata *)GetItemData(i);
			DeleteAllItems();
			int num=pData[1]*256 + pData[2];
			int pos = 3;
			for (i=0; i<num; i++)
			{
				t_connectiondata connectionData;
				memcpy(&connectionData.userid, pData+pos, 4);
				pos += 4;
				int len = pData[pos] * 256 + pData[pos+1];
				pos+=2;
				if (pos+len>dwDataLength)
					return FALSE;
				

				memcpy(connectionData.ip.GetBuffer(len), pData + pos, len);
				connectionData.ip.ReleaseBuffer(len);
				pos+=len;
				
				if ((pos+6) > dwDataLength)
					return FALSE;
				memcpy(&connectionData.port, pData+pos, 4);
				
				pos+=4;
				
				len = pData[pos] * 256 + pData[pos+1];
				pos+=2;
				if (pos+len>dwDataLength)
					return FALSE;

				memcpy(connectionData.user.GetBuffer(len), pData + pos, len);
				connectionData.user.ReleaseBuffer(len);
				pos+=len;

				ProcessConnOp(USERCONTROL_CONNOP_ADD, connectionData);
			}
		}
		break;
	case USERCONTROL_CONNOP:
		if (pData[1]>=0 && pData[1]<3)
		{
			if (dwDataLength < 8)
				return FALSE;
			t_connectiondata connectionData;
			memcpy(&connectionData.userid, pData+2, 4);
			int pos = 6;
			int len = pData[pos] * 256 + pData[pos+1];
			pos+=2;
			if (pos+len>dwDataLength)
				return FALSE;
			
			memcpy(connectionData.ip.GetBuffer(len), pData + pos, len);
			connectionData.ip.ReleaseBuffer(len);
			pos+=len;
			
			if ((pos+6) > dwDataLength)
				return FALSE;
			memcpy(&connectionData.port, pData+pos, 4);
			
			pos+=4;
			
			len = pData[pos] * 256 + pData[pos+1];
			pos+=2;
			if (pos+len>dwDataLength)
				return FALSE;

			memcpy(connectionData.user.GetBuffer(len), pData + pos, len);
			connectionData.user.ReleaseBuffer(len);
			pos+=len;
			ProcessConnOp(pData[1], connectionData);
		}
		else
			return FALSE;		
		break;
	case USERCONTROL_KICK:
		break;
	default:
		m_pOwner->ShowStatus(_T("Protocol error: Specified usercontrol option not implemented"), 1);
		return FALSE;
		break;
	}
	return TRUE;
}