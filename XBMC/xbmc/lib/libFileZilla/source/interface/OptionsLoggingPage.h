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

#if !defined(AFX_OPTIONSLOGGINGPAGE_H__658D35D4_AB51_4911_920F_33A027528848__INCLUDED_)
#define AFX_OPTIONSLOGGINGPAGE_H__658D35D4_AB51_4911_920F_33A027528848__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// OptionsLoggingPage.h : Header-Datei
//

/////////////////////////////////////////////////////////////////////////////
// Dialogfeld COptionsLoggingPage 

class COptionsDlg;
class COptionsLoggingPage : public COptionsPage
{
// Konstruktion
public:
	COptionsLoggingPage(COptionsDlg *pOptionsDlg, CWnd* pParent = NULL);   // Standardkonstruktor

	virtual void SaveData();
	virtual void LoadData();

// Dialogfelddaten
	//{{AFX_DATA(COptionsLoggingPage)
	enum { IDD = IDD_OPTIONS_LOGGING };
	CButton	m_cLogtype;
	CButton	m_cLogtype2;
	CEdit	m_cLimitSize;
	CButton	m_cLimit;
	CEdit	m_cDeleteTime;
	CButton	m_cDelete;
	BOOL	m_bDelete;
	CString	m_DeleteTime;
	BOOL	m_bLimit;
	CString	m_LimitSize;
	BOOL	m_bEnable;
	int		m_nLogtype;
	//}}AFX_DATA


// Überschreibungen
	// Vom Klassen-Assistenten generierte virtuelle Funktionsüberschreibungen
	//{{AFX_VIRTUAL(COptionsLoggingPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV-Unterstützung
	//}}AFX_VIRTUAL

// Implementierung
protected:

	// Generierte Nachrichtenzuordnungsfunktionen
	//{{AFX_MSG(COptionsLoggingPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnOptionsLoggingChangedOption();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ fügt unmittelbar vor der vorhergehenden Zeile zusätzliche Deklarationen ein.

#endif // AFX_OPTIONSLOGGINGPAGE_H__658D35D4_AB51_4911_920F_33A027528848__INCLUDED_
