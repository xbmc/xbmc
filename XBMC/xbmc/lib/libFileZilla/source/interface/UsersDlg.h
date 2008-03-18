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

#if !defined(AFX_USERSDLG_H__5348112C_F36E_42D1_B073_78272B897DDA__INCLUDED_)
#define AFX_USERSDLG_H__5348112C_F36E_42D1_B073_78272B897DDA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// UsersDlg.h : Header-Datei
//

#include "..\Accounts.h"

class CUsersDlgGeneral;
class CUsersDlgSpeedLimit;

/////////////////////////////////////////////////////////////////////////////
// Dialogfeld CUsersDlg
class CUsersDlg : public CSAPrefsDialog
{
	friend CUsersDlgGeneral;
// Konstruktion
public:
	CUsersDlg(CWnd* pParent = NULL);   // Standardkonstruktor
	~CUsersDlg();
	BOOL GetAsCommand(char **pBuffer, DWORD *nBufferLength);
	BOOL Init(unsigned char *pData, DWORD dwDataLength);
	t_user* GetCurrentUser();

protected:
// Dialogfelddaten
	//{{AFX_DATA(CUsersDlg)
	enum { IDD = IDD_USERS };
	CListBox	m_cUserlist;
	//}}AFX_DATA


// Überschreibungen
	// Vom Klassen-Assistenten generierte virtuelle Funktionsüberschreibungen
	//{{AFX_VIRTUAL(CUsersDlg)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV-Unterstützung
	//}}AFX_VIRTUAL

// Implementierung
protected:

	CUsersDlgGeneral *m_pGeneralPage;
	CUsersDlgSpeedLimit *m_pSpeedLimitPage;

	void SetCtrlState();
	int m_olduser;
	BOOL Validate();
	
	typedef std::vector<t_group> t_GroupsList; 
	typedef std::vector<t_user> t_UsersList; 
	t_GroupsList m_GroupsList;
	t_UsersList m_UsersList;

	// Generierte Nachrichtenzuordnungsfunktionen
	//{{AFX_MSG(CUsersDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangeUserlist();
	virtual void OnOK();
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnUsermenuAdd();
	afx_msg void OnUsermenuCopy();
	afx_msg void OnUsermenuRemove();
	afx_msg void OnUsermenuRename();
	afx_msg void OnUseradd();
	afx_msg void OnUsercopy();
	afx_msg void OnUserremove();
	afx_msg void OnUserrename();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ fügt unmittelbar vor der vorhergehenden Zeile zusätzliche Deklarationen ein.

#endif // AFX_USERSDLG_H__5348112C_F36E_42D1_B073_78272B897DDA__INCLUDED_
