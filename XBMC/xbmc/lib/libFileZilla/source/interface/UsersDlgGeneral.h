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

#if !defined(AFX_USERSDLGGENERAL_H__5348112C_F36E_42D1_B073_78272B897DDA__INCLUDED_)
#define AFX_USERSDLGGENERAL_H__5348112C_F36E_42D1_B073_78272B897DDA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// UsersDlg.h : Header-Datei
//

#include "UsersDlg.h"

/////////////////////////////////////////////////////////////////////////////
// Dialogfeld CUsersDlgGeneral 
class CUsersDlgGeneral : public CSAPrefsSubDlg
{
	friend CUsersDlg;
// Konstruktion
public:
	CUsersDlgGeneral(CUsersDlg* pOwner = NULL);   // Standardkonstruktor
	~CUsersDlgGeneral();

	BOOL DisplayUser(const t_user *pUser);
	BOOL SaveUser(t_user *pUser);

protected:
// Dialogfelddaten
	//{{AFX_DATA(CUsersDlgGeneral)
	enum { IDD = IDD_USERS_GENERAL };
	CEdit	m_cMaxConnCount;
	CEdit	m_cIpLimit;
	CButton	m_cMaxUsersBypass;
	CButton	m_cRelative;
	CButton	m_cLnk;
	CButton	m_cNeedpass;
	CComboBox	m_cGroup;
	CButton	m_cDirsCreate;
	CButton	m_cDirsDelete;
	CButton	m_cDirsList;
	CButton	m_cDirsSubdirs;
	CButton	m_cFilesRead;
	CButton	m_cFilesWrite;
	CButton	m_cFilesDelete;
	CButton	m_cFilesAppend;
	CListCtrl	m_cDirs;
	CEdit	m_cPass;
	BOOL	m_bNeedpass;
	CString	m_Pass;
	BOOL	m_bDirsCreate;
	BOOL	m_bDirsDelete;
	BOOL	m_bDirsList;
	BOOL	m_bDirsSubdirs;
	BOOL	m_bFilesAppend;
	BOOL	m_bFilesDelete;
	BOOL	m_bFilesRead;
	BOOL	m_bFilesWrite;
	int		m_nLnk;
	int		m_nRelative;
	int		m_nMaxUsersBypass;
	CString	m_MaxConnCount;
	CString	m_IpLimit;
	//}}AFX_DATA


// Überschreibungen
	// Vom Klassen-Assistenten generierte virtuelle Funktionsüberschreibungen
	//{{AFX_VIRTUAL(CUsersDlgGeneral)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV-Unterstützung
	//}}AFX_VIRTUAL

// Implementierung
protected:
	CUsersDlg *m_pOwner;
	CImageList m_imagelist;
	void SetCtrlState();
	CString Validate();


/*	typedef std::vector<t_group> t_GroupsList; 
	typedef std::vector<t_user> t_UsersList; 
	t_GroupsList m_GroupsList;
	t_UsersList m_UsersList;
*/
	// Generierte Nachrichtenzuordnungsfunktionen
	//{{AFX_MSG(CUsersDlgGeneral)
	virtual BOOL OnInitDialog();
	afx_msg void OnNeedpass();
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnItemchangedDirs(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnItemchangingDirs(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDirmenuAdd();
	afx_msg void OnDirmenuRemove();
	afx_msg void OnDirmenuRename();
	afx_msg void OnDirmenuSetashomedir();
	afx_msg void OnEndlabeleditDirs(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDblclkDirs(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnFilesWrite();
	afx_msg void OnDiradd();
	afx_msg void OnDirremove();
	afx_msg void OnDirrename();
	afx_msg void OnDirsetashome();
	afx_msg void OnSelchangeGroup();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ fügt unmittelbar vor der vorhergehenden Zeile zusätzliche Deklarationen ein.

#endif // AFX_USERSDLGGENERAL_H__5348112C_F36E_42D1_B073_78272B897DDA__INCLUDED_
