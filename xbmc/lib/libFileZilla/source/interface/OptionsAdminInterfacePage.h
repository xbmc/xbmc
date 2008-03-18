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

#if !defined(AFX_OPTIONSADMININTERFACEPAGE_H__F3823923_0389_411A_A3F2_516F04AC4665__INCLUDED_)
#define AFX_OPTIONSADMININTERFACEPAGE_H__F3823923_0389_411A_A3F2_516F04AC4665__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// OptionsAdminInterfacePage.h : Header-Datei
//

/////////////////////////////////////////////////////////////////////////////
// Dialogfeld COptionsAdminInterfacePage 

class COptionsDlg;
class COptionsAdminInterfacePage : public COptionsPage
{
// Konstruktion
public:
	COptionsAdminInterfacePage(COptionsDlg *pOptionsDlg, CWnd* pParent = NULL);   // Standardkonstruktor

	virtual BOOL IsDataValid();
	virtual void SaveData();
	virtual void LoadData();

	CString m_IpBindingsResult;
	
	// Dialogfelddaten
	//{{AFX_DATA(COptionsAdminInterfacePage)
	enum { IDD = IDD_OPTIONS_ADMININTERFACE };
	CButton	m_cChangePass;
	CEdit	m_cNewPass2;
	CEdit	m_cNewPass;
	CString	m_Port;
	CString	m_IpAddresses;
	CString	m_IpBindings;
	CString	m_NewPass;
	CString	m_NewPass2;
	BOOL	m_bChangePass;
	//}}AFX_DATA

// Überschreibungen
	// Vom Klassen-Assistenten generierte virtuelle Funktionsüberschreibungen
	//{{AFX_VIRTUAL(COptionsAdminInterfacePage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV-Unterstützung
	//}}AFX_VIRTUAL

// Implementierung
protected:

	// Generierte Nachrichtenzuordnungsfunktionen
	//{{AFX_MSG(COptionsAdminInterfacePage)
	afx_msg void OnOptionsAdmininterfaceChangepass();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ fügt unmittelbar vor der vorhergehenden Zeile zusätzliche Deklarationen ein.

#endif // AFX_OPTIONSADMININTERFACEPAGE_H__F3823923_0389_411A_A3F2_516F04AC4665__INCLUDED_
