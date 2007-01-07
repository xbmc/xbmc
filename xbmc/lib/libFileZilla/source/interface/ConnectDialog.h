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

#if !defined(AFX_CONNECTDIALOG_H__384ED51B_C9D8_419A_9A8F_FC3024D53735__INCLUDED_)
#define AFX_CONNECTDIALOG_H__384ED51B_C9D8_419A_9A8F_FC3024D53735__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ConnectDialog.h : Header-Datei
//

/////////////////////////////////////////////////////////////////////////////
// Dialogfeld CConnectDialog 

class COptions;
class CConnectDialog : public CDialog
{
// Konstruktion
public:
	CConnectDialog(COptions *pOptions);   // Standardkonstruktor

// Dialogfelddaten
	//{{AFX_DATA(CConnectDialog)
	enum { IDD = IDD_CONNECT_DIALOG };
	CString	m_ServerAddress;
	CString	m_ServerPassword;
	CString	m_ServerPort;
	BOOL	m_bAlways;
	//}}AFX_DATA


// Überschreibungen
	// Vom Klassen-Assistenten generierte virtuelle Funktionsüberschreibungen
	//{{AFX_VIRTUAL(CConnectDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV-Unterstützung
	//}}AFX_VIRTUAL

// Implementierung
protected:
	COptions *m_pOptions;

	// Generierte Nachrichtenzuordnungsfunktionen
	//{{AFX_MSG(CConnectDialog)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ fügt unmittelbar vor der vorhergehenden Zeile zusätzliche Deklarationen ein.

#endif // AFX_CONNECTDIALOG_H__384ED51B_C9D8_419A_9A8F_FC3024D53735__INCLUDED_
