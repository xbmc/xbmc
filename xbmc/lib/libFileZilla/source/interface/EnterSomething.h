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

#if !defined(AFX_ENTERSOMETHING_H__007790E4_A6FA_4F06_96A0_56539DCC4963__INCLUDED_)
#define AFX_ENTERSOMETHING_H__007790E4_A6FA_4F06_96A0_56539DCC4963__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// EnterSomething.h : Header-Datei
//

/////////////////////////////////////////////////////////////////////////////
// Dialogfeld CEnterSomething 

class CEnterSomething : public CDialog
{
// Konstruktion
public:
	//CEnterSomething(CWnd* pParent = NULL);   // Standardkonstruktor
	CEnterSomething(UINT nID);

// Dialogfelddaten
	//{{AFX_DATA(CEnterSomething)
	enum { IDD = IDD_ENTERSOMETHING };
	CStatic	m_cText;
	CButton	m_OkCtrl;
	CString	m_String;
	//}}AFX_DATA


// Überschreibungen
	// Vom Klassen-Assistenten generierte virtuelle Funktionsüberschreibungen
	//{{AFX_VIRTUAL(CEnterSomething)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV-Unterstützung
	//}}AFX_VIRTUAL

// Implementierung
protected:
	UINT m_nID;

	// Generierte Nachrichtenzuordnungsfunktionen
	//{{AFX_MSG(CEnterSomething)
	virtual void OnOK();
	afx_msg void OnChangeEdit1();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ fügt unmittelbar vor der vorhergehenden Zeile zusätzliche Deklarationen ein.

#endif // AFX_ENTERSOMETHING_H__007790E4_A6FA_4F06_96A0_56539DCC4963__INCLUDED_
