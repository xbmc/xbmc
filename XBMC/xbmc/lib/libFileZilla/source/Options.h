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
#include "SpeedLimit.h"

class CMarkupSTL;
class COptionsHelperWindow;
class COptions
{
	friend COptionsHelperWindow;
// Konstruktion
public:
	BOOL GetAsCommand(char **pBuffer, DWORD *nBufferLength);
	CStdString GetOption(int nOptionID);
	_int64 GetOptionVal(int nOptionID);
	COptions();   // Standardkonstruktor
	virtual ~COptions();
	static CMarkupSTL *GetXML();
	static BOOL FreeXML(CMarkupSTL *pXML);
	BOOL ParseOptionsCommand(unsigned char *pData, DWORD dwDataLength, BOOL bFromLocal = FALSE);
	void SetOption(int nOptionID, LPCTSTR value);
	void SetOption(int nOptionID, _int64 value);
	int GetCurrentSpeedLimit(int nMode);
	void ReloadConfig();
	
protected:
	static CCriticalSectionWrapper m_Sync;
	static std::list<COptions *> m_InstanceList;
	static bool IsNumeric(LPCTSTR str);

	BOOL ReadSpeedLimits(CMarkupSTL *pXML);
	BOOL SaveSpeedLimits();

	static SPEEDLIMITSLIST m_sDownloadSpeedLimits;
	static SPEEDLIMITSLIST m_sUploadSpeedLimits;
	SPEEDLIMITSLIST m_DownloadSpeedLimits;
	SPEEDLIMITSLIST m_UploadSpeedLimits;
	
	struct t_OptionsCache
	{
		BOOL bCached;
		int nType;
		CStdString str;
		_int64 value;
	} m_OptionsCache[OPTIONS_NUM];
	static t_OptionsCache m_sOptionsCache[OPTIONS_NUM];
	void Init();
	static BOOL m_bInitialized;

	static void UpdateInstances();
	COptionsHelperWindow *m_pOptionsHelperWindow;
};


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ fügt unmittelbar vor der vorhergehenden Zeile zusätzliche Deklarationen ein.

#endif // AFX_OPTIONS_H__3E60F2D3_99F3_4271_92A3_2CF71AF62731__INCLUDED_
