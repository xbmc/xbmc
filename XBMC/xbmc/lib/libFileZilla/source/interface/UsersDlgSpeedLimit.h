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

#if !defined(AFX_USERSDLGSPEEDLIMIT_H__C47564E9_A44A_4103_A810_109ECD6215F8__INCLUDED_)
#define AFX_USERSDLGSPEEDLIMIT_H__C47564E9_A44A_4103_A810_109ECD6215F8__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// UsersDlgSpeedLimit.h : Header-Datei
//

/////////////////////////////////////////////////////////////////////////////
// Dialogfeld CUsersDlgSpeedLimit 
#include "UsersDlg.h"

class CUsersDlgSpeedLimit : public CSAPrefsSubDlg
{
// Konstruktion
public:
	CUsersDlgSpeedLimit(CUsersDlg *pOwner);   // Standardkonstruktor
	~CUsersDlgSpeedLimit();
	
	void SetCtrlState();
	BOOL DisplayUser(const t_user *pUser);
	BOOL SaveUser(t_user *pUser);
	
protected:
	void ShowSpeedLimit( CListBox &listBox, SPEEDLIMITSLIST &list);

	SPEEDLIMITSLIST m_DownloadSpeedLimits;
	SPEEDLIMITSLIST m_UploadSpeedLimits;

	CUsersDlg *m_pOwner;

// Dialogfelddaten
	//{{AFX_DATA(CUsersDlgSpeedLimit)
	enum { IDD = IDD_USERS_SPEEDLIMIT };
	CButton	m_DownloadUpCtrl;
	CListBox	m_DownloadRulesListCtrl;
	CButton	m_DownloadRemoveCtrl;
	CButton	m_DownloadDownCtrl;
	CButton	m_DownloadAddCtrl;
	CButton	m_UploadUpCtrl;
	CListBox	m_UploadRulesListCtrl;
	CButton	m_UploadRemoveCtrl;
	CButton	m_UploadDownCtrl;
	CButton	m_UploadAddCtrl;
	CEdit	m_UploadValueCtrl;
	CEdit	m_DownloadValueCtrl;
	int		m_DownloadSpeedLimitType;
	int		m_UploadSpeedLimitType;
	int		m_DownloadValue;
	int		m_UploadValue;
	//}}AFX_DATA


// Überschreibungen
	// Vom Klassen-Assistenten generierte virtuelle Funktionsüberschreibungen
	//{{AFX_VIRTUAL(CUsersDlgSpeedLimit)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV-Unterstützung
	//}}AFX_VIRTUAL

// Implementierung
protected:

	// Generierte Nachrichtenzuordnungsfunktionen
	//{{AFX_MSG(CUsersDlgSpeedLimit)
	virtual BOOL OnInitDialog();
	afx_msg void OnRadio();
	afx_msg void OnSpeedlimitDownloadAdd();
	afx_msg void OnSpeedlimitDownloadRemove();
	afx_msg void OnSpeedlimitDownloadUp();
	afx_msg void OnSpeedlimitDownloadDown();
	afx_msg void OnDblclkSpeedlimitDownloadRulesList();
	afx_msg void OnSpeedlimitUploadAdd();
	afx_msg void OnSpeedlimitUploadRemove();
	afx_msg void OnSpeedlimitUploadUp();
	afx_msg void OnSpeedlimitUploadDown();
	afx_msg void OnDblclkSpeedlimitUploadRulesList();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ fügt unmittelbar vor der vorhergehenden Zeile zusätzliche Deklarationen ein.

#endif // AFX_USERSDLGSPEEDLIMIT_H__C47564E9_A44A_4103_A810_109ECD6215F8__INCLUDED_
