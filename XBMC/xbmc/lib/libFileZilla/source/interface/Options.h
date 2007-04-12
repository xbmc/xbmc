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

#if !defined(AFX_OPTIONS_H__3E60F2D3_99F3_4271_92A3_2CF71AF62731__INCLUDED_)
#define AFX_OPTIONS_H__3E60F2D3_99F3_4271_92A3_2CF71AF62731__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Options.h : Header-Datei
//

/////////////////////////////////////////////////////////////////////////////
// Dialogfeld COptions 

#define IOPTION_STARTMINIMIZED 1
#define IOPTION_LASTSERVERADDRESS 2
#define IOPTION_LASTSERVERPORT 3
#define IOPTION_LASTSERVERPASS 4
#define IOPTION_ALWAYS 5

#define IOPTIONS_NUM 5

class CMarkupSTL;
class COptionsDlg;
class COptions
{
	friend COptionsDlg;
// Konstruktion
public:
	CString GetOption(int nOptionID);
	int GetOptionVal(int nOptionID);
	COptions();   // Standardkonstruktor
	virtual ~COptions();
	static CMarkupSTL *GetXML();
	static BOOL FreeXML(CMarkupSTL *pXML);
	void SetOption(int nOptionID, CString value);
	void SetOption(int nOptionID, int value);
	
protected:
	static bool IsNumeric(LPCTSTR str);
	
	struct t_OptionsCache
	{
		BOOL bCached;
		CTime createtime;
		int nType;
		CString str;
		_int64 value;
	} m_OptionsCache[IOPTIONS_NUM];
	void Init();
	static BOOL m_bInitialized;
};


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ fügt unmittelbar vor der vorhergehenden Zeile zusätzliche Deklarationen ein.

#endif // AFX_OPTIONS_H__3E60F2D3_99F3_4271_92A3_2CF71AF62731__INCLUDED_
