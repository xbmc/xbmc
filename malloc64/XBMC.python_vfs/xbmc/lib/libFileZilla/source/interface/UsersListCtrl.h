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

#if !defined(AFX_USERSLISTCTRL_H__C939FF91_7A57_4E36_927B_00B917F6ECED__INCLUDED_)
#define AFX_USERSLISTCTRL_H__C939FF91_7A57_4E36_927B_00B917F6ECED__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// UsersListCtrl.h : Header-Datei
//

/////////////////////////////////////////////////////////////////////////////
// Fenster CUsersListCtrl 

class CMainFrame;
class CUsersListCtrl : public CListCtrl
{
// Konstruktion
public:
	CUsersListCtrl(CMainFrame *pOwner);
	virtual ~CUsersListCtrl();

// Attribute
public:

// Operationen
public:
	BOOL ParseUserControlCommand(unsigned char *pData, DWORD dwDataLength);

// Überschreibungen
	// Vom Klassen-Assistenten generierte virtuelle Funktionsüberschreibungen
	//{{AFX_VIRTUAL(CUsersListCtrl)
	//}}AFX_VIRTUAL

// Implementierung
protected:
	void ProcessConnOp(int op, const t_connectiondata &connectionData);

	CMainFrame *m_pOwner;

	// Generierte Nachrichtenzuordnungsfunktionen
protected:
	//{{AFX_MSG(CUsersListCtrl)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnContextmenuKick();
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ fügt unmittelbar vor der vorhergehenden Zeile zusätzliche Deklarationen ein.

#endif // AFX_USERSLISTCTRL_H__C939FF91_7A57_4E36_927B_00B917F6ECED__INCLUDED_
