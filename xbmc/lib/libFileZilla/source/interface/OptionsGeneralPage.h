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

#if !defined(AFX_OPTIONSGENERALPAGE_H__2198594C_33A4_4A87_A753_76FE8507A8F8__INCLUDED_)
#define AFX_OPTIONSGENERALPAGE_H__2198594C_33A4_4A87_A753_76FE8507A8F8__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// OptionsGeneralPage.h : Header-Datei
//

/////////////////////////////////////////////////////////////////////////////
// Dialogfeld COptionsGeneralPage 

class COptionsDlg;
class COptionsGeneralPage : public COptionsPage
{
// Konstruktion
public:
	COptionsGeneralPage(COptionsDlg *pOptionsDlg, CWnd* pParent = NULL);   // Standardkonstruktor

	virtual BOOL IsDataValid();
	virtual void SaveData();
	virtual void LoadData();

	// Dialogfelddaten
	//{{AFX_DATA(COptionsGeneralPage)
	enum { IDD = IDD_OPTIONS_GENERAL };
	CString	m_MaxUsers;
	CString	m_Port;
	CString	m_Threadnum;
	CString	m_Timeout;
	CString	m_NoTransferTimeout;
	CString	m_LoginTimeout;
	//}}AFX_DATA


// Überschreibungen
	// Vom Klassen-Assistenten generierte virtuelle Funktionsüberschreibungen
	//{{AFX_VIRTUAL(COptionsGeneralPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV-Unterstützung
	//}}AFX_VIRTUAL

// Implementierung
protected:

	// Generierte Nachrichtenzuordnungsfunktionen
	//{{AFX_MSG(COptionsGeneralPage)
		// HINWEIS: Der Klassen-Assistent fügt hier Member-Funktionen ein
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ fügt unmittelbar vor der vorhergehenden Zeile zusätzliche Deklarationen ein.

#endif // AFX_OPTIONSGENERALPAGE_H__2198594C_33A4_4A87_A753_76FE8507A8F8__INCLUDED_
