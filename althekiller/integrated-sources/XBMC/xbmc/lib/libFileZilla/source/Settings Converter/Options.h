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

#include "OptionTypes.h"
#include "windows.h"

#include "misc\StdString.h"
#include <vector>
#include <map>

class CMarkupSTL;
class COptions
{
// Konstruktion
public:
	void GetOption(int nOptionID);
	void GetOptionVal(int nOptionID);
	COptions(HWND hParentWnd);   // Standardkonstruktor
	virtual ~COptions();
	BOOL Convert(BOOL bCU, LPCTSTR lpOutputFile);
	
protected:
	int RegDeleteKeyEx(HKEY thiskey, const char* lpSubKey);
	BOOL Delete();
	BOOL AskSave();
	CMarkupSTL *GetXML();
	BOOL FreeXML(CMarkupSTL *pXML, BOOL bSave);
	void SetOption(int nOptionID);
	void SetOptionVal(int nOptionID);
	
	HWND m_hParentWnd;
	LPCTSTR m_lpOutputFile;
	BOOL m_bCU;
	int m_nAskedConvert;
	int m_nAskedOverwrite;
	
	struct t_OptionsCache
	{
		BOOL bCached;
		int nType;
		CStdString str;
		_int64 value;
	} m_OptionsCache[OPTIONS_NUM];
};


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ fügt unmittelbar vor der vorhergehenden Zeile zusätzliche Deklarationen ein.

#endif // AFX_OPTIONS_H__3E60F2D3_99F3_4271_92A3_2CF71AF62731__INCLUDED_
