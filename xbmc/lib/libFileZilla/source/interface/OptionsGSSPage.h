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

#if !defined(AFX_OPTIONSGSSPAGE_H__E3184429_C324_46EF_85F7_5F12C16A6FA6__INCLUDED_)
#define AFX_OPTIONSGSSPAGE_H__E3184429_C324_46EF_85F7_5F12C16A6FA6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// OptionsGSSPage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// COptionsGSSPage dialog

class COptionsDlg;
class COptionsGSSPage : public COptionsPage
{

// Construction
public:
	COptionsGSSPage(COptionsDlg *pOptionsDlg, CWnd* pParent = NULL);

	virtual void SaveData();
	virtual void LoadData();

// Dialog Data
	//{{AFX_DATA(COptionsGSSPage)
	enum { IDD = IDD_OPTIONS_GSS };
	BOOL	m_bPromptPassword;
	BOOL	m_bUseGSS;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(COptionsGSSPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(COptionsGSSPage)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OPTIONSGSSPAGE_H__E3184429_C324_46EF_85F7_5F12C16A6FA6__INCLUDED_)
