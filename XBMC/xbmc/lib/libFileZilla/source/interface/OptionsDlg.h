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

#if !defined(AFX_OPTIONSDLG_H__3E60F2D3_99F3_4271_92A3_2CF71AF62731__INCLUDED_)
#define AFX_OPTIONSDLG_H__3E60F2D3_99F3_4271_92A3_2CF71AF62731__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Options.h : Header-Datei
//
#include "OptionsSecurityPage.h"
#include "OptionsGeneralPage.h"
#include "OptionsGeneralWelcomemessagePage.h"
#include "OptionsMiscPage.h"
#include "OptionsAdminInterfacePage.h"
#include "OptionsLoggingPage.h"
#include "OptionsGSSPage.h"
#include "OptionsSpeedLimitPage.h"
#include "..\OptionTypes.h"

/////////////////////////////////////////////////////////////////////////////
// Dialogfeld COptions 

class COptions;
class COptionsDlg : public CSAPrefsDialog
{
// Konstruktion
public:
	CString GetOption(int nOptionID);
	int GetOptionVal(int nOptionID);
	BOOL Show();
	COptionsDlg(COptions *pInterfaceOptions);
	virtual ~COptionsDlg();
	COptionsGeneralPage m_OptionsGeneralPage;
	COptionsSecurityPage m_OptionsSecurityPage;
	COptionsMiscPage m_OptionsMiscPage;
	COptionsGeneralWelcomemessagePage m_OptionsGeneralWelcomemessagePage;
	COptionsAdminInterfacePage m_OptionsAdminInterfacePage;
	COptionsLoggingPage m_OptionsLoggingPage;
	COptionsGSSPage m_OptionsGSSPage;
	COptionsSpeedLimitPage m_OptionsSpeedLimitPage;
	BOOL Init(unsigned char *pData, DWORD dwDataLength);
	BOOL GetAsCommand(char **pBuffer, DWORD *nBufferLength);
	
protected:
	COptions *m_pInterfaceOptions;
	static bool IsNumeric(LPCTSTR str);

	void SetOption(int nOptionID, CString value);
	void SetOption(int nOptionID, int value);
	
	struct t_OptionsCache
	{
		int nType;
		CString str;
		_int64 value;
	} m_OptionsCache[OPTIONS_NUM];

	// add a page (page, page title, optional parent)
	bool AddPage(CSAPrefsSubDlg &page, UINT nCaptionID, CSAPrefsSubDlg *pDlgParent = NULL);

	CSAPrefsSubDlg* GetCurPage();
	
// Dialogfelddaten
	//{{AFX_DATA(COptions)
	enum { IDD = IDD_SAPREFS };
		// HINWEIS: Der Klassen-Assistent fügt hier Datenelemente ein
	//}}AFX_DATA


// Überschreibungen
	// Vom Klassen-Assistenten generierte virtuelle Funktionsüberschreibungen
	//{{AFX_VIRTUAL(COptions)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV-Unterstützung
	//}}AFX_VIRTUAL

// Implementierung

	// Generierte Nachrichtenzuordnungsfunktionen
	//{{AFX_MSG(COptions)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ fügt unmittelbar vor der vorhergehenden Zeile zusätzliche Deklarationen ein.

#endif // AFX_OPTIONSDLG_H__3E60F2D3_99F3_4271_92A3_2CF71AF62731__INCLUDED_
