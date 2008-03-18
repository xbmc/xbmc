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

#if !defined(AFX_NEWUSERDLG_H__0487F0AE_1C8C_4658_A466_5EF412B12982__INCLUDED_)
#define AFX_NEWUSERDLG_H__0487F0AE_1C8C_4658_A466_5EF412B12982__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// NewUserDlg.h : Header-Datei
//

/////////////////////////////////////////////////////////////////////////////
// Dialogfeld CNewUserDlg 

class CNewUserDlg : public CDialog
{
// Konstruktion
public:
	CNewUserDlg(CWnd* pParent = NULL);   // Standardkonstruktor
	std::list<CString> m_GroupList;

// Dialogfelddaten
	//{{AFX_DATA(CNewUserDlg)
	enum { IDD = IDD_NEWUSER };
	CButton	m_cOkCtrl;
	CComboBox	m_cGroup;
	CString	m_Name;
	CString m_Group;
	//}}AFX_DATA


// Überschreibungen
	// Vom Klassen-Assistenten generierte virtuelle Funktionsüberschreibungen
	//{{AFX_VIRTUAL(CNewUserDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV-Unterstützung
	//}}AFX_VIRTUAL

// Implementierung
protected:

	// Generierte Nachrichtenzuordnungsfunktionen
	//{{AFX_MSG(CNewUserDlg)
	afx_msg void OnChangeNewuserName();
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ fügt unmittelbar vor der vorhergehenden Zeile zusätzliche Deklarationen ein.

#endif // AFX_NEWUSERDLG_H__0487F0AE_1C8C_4658_A466_5EF412B12982__INCLUDED_
