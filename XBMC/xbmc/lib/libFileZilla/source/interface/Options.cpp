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

// Options.cpp: Implementierungsdatei
//

#include "stdafx.h"
#include "options.h"
#include "version.h"
#include "filezilla server.h"
#include "misc\MarkupSTL.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

BOOL COptions::m_bInitialized=FALSE;

/////////////////////////////////////////////////////////////////////////////
// Dialogfeld COptions 

COptions::COptions()
{
	for (int i=0;i<IOPTIONS_NUM;i++)
		m_OptionsCache[i].bCached=FALSE;
}

COptions::~COptions()
{
}

struct t_Option
{
	TCHAR name[30];
	int nType;
};

static const t_Option m_Options[IOPTIONS_NUM]={	_T("Start Minimized"),			1,
												_T("Last Server Address"),		0,
												_T("Last Server Port"),			1,
												_T("Last Server Password"),		0,
												_T("Always use last server"),	1
												};

void COptions::SetOption(int nOptionID, int value)
{
	Init();

	m_OptionsCache[nOptionID-1].bCached = TRUE;
	m_OptionsCache[nOptionID-1].nType = 1;
	m_OptionsCache[nOptionID-1].value = value;

	TCHAR buffer[MAX_PATH + 1000]; //Make it large enough
	GetModuleFileName( 0, buffer, MAX_PATH );
	LPTSTR pos=_tcsrchr(buffer, '\\');
	if (pos)
		*++pos=0;
	_tcscat(buffer, _T("FileZilla Server Interface.xml"));
	CMarkupSTL xml;
	if (!xml.Load(buffer))
		return;
	
	if (!xml.FindElem( _T("FileZillaServer") ))
		return;

	if (!xml.FindChildElem( _T("Settings") ))
		xml.AddChildElem( _T("Settings") );
	
	CString valuestr;
	valuestr.Format( _T("%d"), value);
	xml.IntoElem();
	BOOL res=xml.FindChildElem();
	while (res)
	{
		CString name=xml.GetChildAttrib( _T("name"));
		if (!_tcscmp(name, m_Options[nOptionID-1].name))
		{
			xml.SetChildAttrib(_T("name"), m_Options[nOptionID-1].name);
			xml.SetChildAttrib(_T("type"), _T("numeric"));
			xml.SetChildData(valuestr);
			break;
		}
		res=xml.FindChildElem();
	}
	if (!res)
	{
		xml.InsertChildElem(_T("Item"), valuestr);
		xml.SetChildAttrib(_T("name"), m_Options[nOptionID-1].name);
		xml.SetChildAttrib(_T("type"), _T("numeric"));
	}
	xml.Save(buffer);
}

void COptions::SetOption(int nOptionID, CString value)
{
	Init();

	m_OptionsCache[nOptionID-1].bCached = TRUE;
	m_OptionsCache[nOptionID-1].nType = 0;
	m_OptionsCache[nOptionID-1].str = value;

	TCHAR buffer[MAX_PATH + 1000]; //Make it large enough
	GetModuleFileName( 0, buffer, MAX_PATH );
	LPTSTR pos=_tcsrchr(buffer, '\\');
	if (pos)
		*++pos=0;
	_tcscat(buffer, _T("FileZilla Server Interface.xml"));
	CMarkupSTL xml;
	if (!xml.Load(buffer))
		return;
	
	if (!xml.FindElem( _T("FileZillaServer") ))
		return;

	if (!xml.FindChildElem( _T("Settings") ))
		xml.AddChildElem( _T("Settings") );
	
	xml.IntoElem();
	BOOL res=xml.FindChildElem();
	while (res)
	{
		CString name=xml.GetChildAttrib( _T("name"));
		if (!_tcscmp(name, m_Options[nOptionID-1].name))
		{
			xml.SetChildAttrib(_T("name"), m_Options[nOptionID-1].name);
			xml.SetChildAttrib(_T("type"), _T("string"));
			xml.SetChildData(value);
			break;
		}
		res=xml.FindChildElem();
	}
	if (!res)
	{
		xml.InsertChildElem( _T("Item"), value );
		xml.SetChildAttrib(_T("name"), m_Options[nOptionID-1].name);
		xml.SetChildAttrib(_T("type"), _T("string"));
	}
	xml.Save(buffer);
}

CString COptions::GetOption(int nOptionID)
{
	ASSERT(nOptionID>0 && nOptionID<=IOPTIONS_NUM);
	ASSERT(!m_Options[nOptionID-1].nType);
	Init();
	
	if (m_OptionsCache[nOptionID-1].bCached)
		return m_OptionsCache[nOptionID-1].str;

	//Verification
	switch (nOptionID)
	{
	case IOPTION_LASTSERVERADDRESS:
		m_OptionsCache[nOptionID-1].str = _T("127.0.0.1");
		break;
	default:
		m_OptionsCache[nOptionID-1].str="";
		break;
	}
	m_OptionsCache[nOptionID-1].bCached=TRUE;
	m_OptionsCache[nOptionID-1].nType=0;
	return m_OptionsCache[nOptionID-1].str;
}

int COptions::GetOptionVal(int nOptionID)
{
	ASSERT(nOptionID>0 && nOptionID<=IOPTIONS_NUM);
	ASSERT(m_Options[nOptionID-1].nType == 1);
	Init();
	
	if (m_OptionsCache[nOptionID-1].bCached)
		return m_OptionsCache[nOptionID-1].value;

	switch (nOptionID)
	{
	case IOPTION_LASTSERVERPORT:
		m_OptionsCache[nOptionID-1].value=14147;
		break;
	default:
		m_OptionsCache[nOptionID-1].value=0;
	}
	m_OptionsCache[nOptionID-1].bCached=TRUE;
	m_OptionsCache[nOptionID-1].nType=0;
	return m_OptionsCache[nOptionID-1].value;
}

void COptions::Init()
{
	if (m_bInitialized)
		return;
	m_bInitialized=TRUE;
	CFileStatus status;
	CMarkupSTL xml;
	TCHAR buffer[MAX_PATH + 1000]; //Make it large enough
	GetModuleFileName( 0, buffer, MAX_PATH );
	LPTSTR pos=_tcsrchr(buffer, '\\');
	if (pos)
		*++pos=0;
	_tcscat(buffer, _T("FileZilla Server Interface.xml"));
	
	for (int i=0; i<IOPTIONS_NUM; i++)
		m_OptionsCache[i].bCached=FALSE;
	
	if (!CFile::GetStatus(buffer, status) )
	{
		xml.AddElem( _T("FileZillaServer") );
		if (!xml.Save(buffer))
			return;
	}
	else if (status.m_attribute&FILE_ATTRIBUTE_DIRECTORY)
		return;
		
	if (xml.Load(buffer))
	{
		if (xml.FindElem( _T("FileZillaServer") ))
		{
			if (!xml.FindChildElem( _T("Settings") ))
				xml.AddChildElem( _T("Settings") );
			
			CString str;
			xml.IntoElem();
			str=xml.GetTagName();
			while (xml.FindChildElem())
			{
				CString value=xml.GetChildData();
				CString name=xml.GetChildAttrib( _T("name") );
				CString type=xml.GetChildAttrib( _T("type") );
				for (int i=0;i<IOPTIONS_NUM;i++)
				{
					if (!_tcscmp(name, m_Options[i].name))
					{
						if (m_OptionsCache[i].bCached)
						{
							::AfxTrace( _T("Item '%s' is already in cache, ignoring item\n"), name);
							break;
						}
						else
						{
							if (type=="numeric")
							{
								if (m_Options[i].nType!=1)
								{
									::AfxTrace( _T("Type mismatch for option '%s'. Type is %d, should be %d"), name, m_Options[i].nType, 1);
									break;
								}
								m_OptionsCache[i].bCached=TRUE;
								m_OptionsCache[i].nType=1;
								_int64 value64=_ttoi64(value);

								switch(i+1) {
								case IOPTION_LASTSERVERPORT:
									if (value64<1 || value64>65535)
										value64 = 14147;
									break;
								default:
									break;
								}

								m_OptionsCache[i].value=value64;
							}
							else
							{
								if (type!="string")
									::AfxTrace( _T("Unknown option type '%s' for item '%s', assuming string\n"), type, name);
								if (m_Options[i].nType!=0)
								{
									::AfxTrace( _T("Type mismatch for option '%s'. Type is %d, should be %d"), name, m_Options[i].nType, 0);
									break;
								}
								m_OptionsCache[i].bCached=TRUE;
								m_OptionsCache[i].nType=0;
								
								m_OptionsCache[i].str=value;
							}
						}
						break;
					}
				}
			}
			return;
		}
	}
}

bool COptions::IsNumeric(LPCTSTR str)
{
	if (!str)
		return false;
	LPCTSTR p=str;
	while(*p)
	{
		if (*p<'0' || *p>'9')
		{
			return false;
		}
		p++;
	}
	return true;
}

CMarkupSTL *COptions::GetXML()
{
	TCHAR buffer[MAX_PATH + 1000]; //Make it large enough
	GetModuleFileName( 0, buffer, MAX_PATH );
	LPTSTR pos=_tcsrchr(buffer, '\\');
	if (pos)
		*++pos=0;
	_tcscat(buffer, _T("FileZilla Server Interface.xml"));
	CMarkupSTL *pXML=new CMarkupSTL;
	if (!pXML->Load(buffer))
	{
		delete pXML;
		return NULL;
	}
	
	if (!pXML->FindElem( _T("FileZillaServer") ))
	{
		delete pXML;
		return NULL;
	}
	return pXML;
}

BOOL COptions::FreeXML(CMarkupSTL *pXML)
{
	ASSERT(pXML);
	if (!pXML)
		return FALSE;
	TCHAR buffer[MAX_PATH + 1000]; //Make it large enough
	GetModuleFileName( 0, buffer, MAX_PATH );
	LPTSTR pos=_tcsrchr(buffer, '\\');
	if (pos)
		*++pos=0;
	_tcscat(buffer, _T("FileZilla Server Interface.xml"));
	if (!pXML->Save(buffer))
		return FALSE;
	delete pXML;
	return TRUE;
}