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

#include "options.h"
#include "misc\MarkupSTL.h"
#include "permissions.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialogfeld COptions 

COptions::COptions(HWND hParentWnd)
{
	m_hParentWnd = hParentWnd;
}

COptions::~COptions()
{
}

void COptions::SetOptionVal(int nOptionID)
{
	CMarkupSTL xml;
	if (!xml.Load(m_lpOutputFile))
		xml.AddElem( _T("FileZillaServer") );
		
	if (!xml.FindElem( _T("FileZillaServer") ))
		xml.AddElem( _T("FileZillaServer") );

	if (!xml.FindChildElem( _T("Settings") ))
		xml.AddChildElem( _T("Settings") );
	
	CStdString valuestr;
	valuestr.Format( _T("%d"), m_OptionsCache[nOptionID].value);
	xml.IntoElem();
	BOOL res=xml.FindChildElem();
	while (res)
	{
		CStdString name=xml.GetChildAttrib( _T("name"));
		if (!_tcscmp(name, m_Options[nOptionID].name))
		{
			xml.SetChildAttrib(_T("name"), m_Options[nOptionID].name);
			xml.SetChildAttrib(_T("type"), _T("numeric"));
			xml.SetChildData(valuestr);
			break;
		}
		res=xml.FindChildElem();
	}
	if (!res)
	{
		xml.InsertChildElem(_T("Item"), valuestr);
		xml.SetChildAttrib(_T("name"), m_Options[nOptionID].name);
		xml.SetChildAttrib(_T("type"), _T("numeric"));
	}
	if (AskSave())
		if (!xml.Save(m_lpOutputFile))
			MessageBox(m_hParentWnd, "Failed to write to settings file, file may be corrupt", "Converting settings", MB_ICONEXCLAMATION|MB_OK);
}

void COptions::SetOption(int nOptionID)
{
	CMarkupSTL xml;
	if (!xml.Load(m_lpOutputFile))
		xml.AddElem( _T("FileZillaServer") );
		
	if (!xml.FindElem( _T("FileZillaServer") ))
		xml.AddElem( _T("FileZillaServer") );

	if (!xml.FindChildElem( _T("Settings") ))
		xml.AddChildElem( _T("Settings") );
	
	xml.IntoElem();
	BOOL res=xml.FindChildElem();
	while (res)
	{
		CStdString name=xml.GetChildAttrib( _T("name"));
		if (!_tcscmp(name, m_Options[nOptionID].name))
		{
			xml.SetChildAttrib(_T("name"), m_Options[nOptionID].name);
			xml.SetChildAttrib(_T("type"), _T("string"));
			xml.SetChildData(m_OptionsCache[nOptionID].str);
			break;
		}
		res=xml.FindChildElem();
	}
	if (!res)
	{
		xml.InsertChildElem( _T("Item"), m_OptionsCache[nOptionID].str );
		xml.SetChildAttrib(_T("name"), m_Options[nOptionID].name);
		xml.SetChildAttrib(_T("type"), _T("string"));
	}
	if (AskSave())
		if (!xml.Save(m_lpOutputFile))
			MessageBox(m_hParentWnd, "Failed to write to settings file, file may be corrupt", "Converting settings", MB_ICONEXCLAMATION|MB_OK);
}

void COptions::GetOption(int nOptionID)
{
	ASSERT(!m_OptionsCache[nOptionID].bCached);
	
	CStdString res = "";
	
	HKEY key;
	unsigned long tmp=2000;
	unsigned char *buffer=new unsigned char[2000];
	BOOL exists = FALSE;
	
	if (RegOpenKey(m_bCU?HKEY_CURRENT_USER:HKEY_LOCAL_MACHINE, _T("Software\\FileZilla Server"), &key)==ERROR_SUCCESS)
	{
		memset(buffer,0,2000);
		
		if (RegQueryValueEx(key, m_Options[nOptionID].name, NULL, NULL, buffer, &tmp)==ERROR_SUCCESS) 
		{
			exists = TRUE;
			res=(LPTSTR)buffer;
		}
		RegCloseKey(key);
	}
	
	delete [] buffer;

	if (!exists)
		return;
	
	//Verification

	switch (nOptionID+1)
	{
	case OPTION_WELCOMEMESSAGE:
		{
			std::vector<CStdString> msgLines;
			int oldpos=0;
			res.Replace("\r\n", "\n");
			int pos=res.Find("\n");
			CStdString line;
			while (pos!=-1)
			{
				if (pos)
				{
					line = res.Mid(oldpos, pos-oldpos);
					line = line.Left(70);
					line.TrimRight(" ");
					if (msgLines.size() || line!="")
						msgLines.push_back(line);
				}
				oldpos=pos+1;
				pos=res.Find("\n", oldpos);
			}
			line=res.Mid(oldpos);
			if (line!="")
			{
				line=line.Left(70);
				msgLines.push_back(line);
			}
			res="";
			for (int i=0;i<msgLines.size();i++)
				res+=msgLines[i]+"\r\n";
			res.TrimRight("\r\n");
			if (res=="")
			{
				res="%v";
				res+="\r\nwritten by Tim Kosse (Tim.Kosse@gmx.de)";
				res+="\r\nPlease visit http://sourceforge.net/projects/filezilla/";
			}
		}
		break;
	}
	
	m_OptionsCache[nOptionID].bCached = TRUE;
	m_OptionsCache[nOptionID].str = res;
	m_OptionsCache[nOptionID].nType = 0;
}

void COptions::GetOptionVal(int nOptionID)
{
	ASSERT(!m_OptionsCache[nOptionID].bCached);

	int val=0;
	
	int error=1;
	
	HKEY key;
	unsigned char *buffer=new unsigned char[100];
	unsigned long tmp=100;
				
	if (RegOpenKey(m_bCU?HKEY_CURRENT_USER:HKEY_LOCAL_MACHINE, _T("Software\\FileZilla Server") ,&key)!=ERROR_SUCCESS)
	{
		delete [] buffer;
		return;
	}

	if (RegQueryValueEx(key, m_Options[nOptionID].name, NULL, NULL, buffer, &tmp)!=ERROR_SUCCESS) 
	{
		RegCloseKey(key);
		delete [] buffer;
		return;
	}
	RegCloseKey(key);
	if (tmp>4) 
		return;
			
	memcpy(&val,buffer,tmp);
	delete [] buffer;
	
	switch (nOptionID+1)
	{
		case OPTION_SERVERPORT:
			if (val>65535)
				val=21;
			else if (val<1)
				val=21;
			break;
		case OPTION_MAXUSERS:
			if (val<0)
				val=0;
			break;
		case OPTION_THREADNUM:
			if (val<1)
				val=2;
			else if (val>50)
				val=2;
			break;
		case OPTION_TIMEOUT:
		case OPTION_NOTRANSFERTIMEOUT:
		case OPTION_LOGINTIMEOUT:
			if (val<0)
				val=120;
			else if (val>9999)
				val=120;
			break;
	}

	m_OptionsCache[nOptionID].bCached = TRUE;
	m_OptionsCache[nOptionID].value = val;
	m_OptionsCache[nOptionID].nType = 1;
}

CMarkupSTL *COptions::GetXML()
{
	CMarkupSTL *pXML=new CMarkupSTL;
	if (!pXML->Load(m_lpOutputFile))
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

BOOL COptions::FreeXML(CMarkupSTL *pXML, BOOL bSave)
{
	ASSERT(pXML);
	if (!pXML)
		return FALSE;
	if (bSave && AskSave())
		if (!pXML->Save(m_lpOutputFile))
		{
			MessageBox(m_hParentWnd, "Failed to write to settings file, file may be corrupt", "Converting settings", MB_ICONEXCLAMATION|MB_OK);
			return FALSE;
		}
	delete pXML;
	return TRUE;
}

BOOL COptions::Convert(BOOL bCU, LPCTSTR lpOutputFile)
{
	m_bCU = bCU;
	m_lpOutputFile = lpOutputFile;
	m_nAskedConvert = m_nAskedOverwrite = 0;

	for (int i=0; i<OPTIONS_NUM; i++)
	{
		m_OptionsCache[i].bCached = FALSE;
		if (!m_Options[i].nType)
		{
			GetOption(i);
			if (m_OptionsCache[i].bCached)
				SetOption(i);
		}
		else
		{
			GetOptionVal(i);
			if (m_OptionsCache[i].bCached)
				SetOptionVal(i);
		}
	}
	CPermissions permissions(bCU);
	CMarkupSTL *pXML = GetXML();
	if (pXML)
	{
		BOOL res = permissions.Convert(pXML);
		FreeXML(pXML, res);
	}

	Delete();
	return TRUE;
}

BOOL COptions::AskSave()
{
	if (!m_nAskedConvert)
	{
		if (m_bCU)
		{
			char str[] = "Settings from previous versions of FileZilla Server (0.7.4 and earlier) have been found in the registry under 'HKEY_CURRENT_USER\\Software\\FileZilla'.\nFrom version 0.8.0 on, FileZilla Server stores the settings in an XML file.\nDo you want to convert the old settings?";
			if (MessageBox(0, str, "Converting settings", MB_ICONQUESTION|MB_YESNO)==IDYES)
				m_nAskedConvert = 1;
			else
			{
				m_nAskedConvert = 2;
				return FALSE;
			}
		}
		else
		{
			char str[] = "Settings from previous versions of FileZilla Server (0.7.4 and earlier) have been found in the registry under 'HKEY_LOCAL_MACHINE\\Software\\FileZilla'.\nFrom version 0.8.0 on, FileZilla Server stores the settings in an XML file.\nDo you want to convert the old settings?";
			if (MessageBox(0, str, "Converting settings", MB_ICONQUESTION|MB_YESNO)==IDYES)
				m_nAskedConvert = 1;
			else
			{
				m_nAskedConvert = 2;
				return FALSE;
			}
		}
	}
	else if (m_nAskedConvert == 2)
		return FALSE;

	if (!m_nAskedOverwrite)
	{
		HANDLE hFile = CreateFile(m_lpOutputFile, 0, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, 0, OPEN_EXISTING, 0);
		if (hFile)
		{
			CloseHandle(hFile);
			char str[] = "A settings file already exists in the FileZilla Server folder. Do you want to continue?\nNew settings and user accounts with the same name will be overwritten with the converted ones.";
			if (MessageBox(0, str, "Converting settings", MB_ICONQUESTION|MB_YESNO)==IDYES)
				m_nAskedOverwrite = 1;
			else
			{
				m_nAskedOverwrite = 2;
				return FALSE;
			}
		}
		else
			m_nAskedOverwrite = 1;
	}
	else if (m_nAskedOverwrite == 2)
		return FALSE;
	return TRUE;
}

BOOL COptions::Delete()
{
	if (!m_nAskedConvert)
		return FALSE;

	if (m_bCU)
	{
		if (MessageBox(m_hParentWnd, "Do you want to delete the settings under 'HKEY_CURRENT_USER\\Software\\FileZilla Server' from the registry?", "Converting settings", MB_ICONQUESTION|MB_YESNO)!=IDYES)
			return FALSE;
		RegDeleteKeyEx(HKEY_CURRENT_USER, "Software\\FileZilla Server");
	}
	else
	{
		if (MessageBox(m_hParentWnd, "Do you want to delete the settings under 'HKEY_LOCAL_MACHINE\\Software\\FileZilla Server' from the registry?", "Converting settings", MB_ICONQUESTION|MB_YESNO)!=IDYES)
			return FALSE;
		RegDeleteKeyEx(HKEY_LOCAL_MACHINE, "Software\\FileZilla Server");
	}	
	
	return TRUE;
}


int COptions::RegDeleteKeyEx(HKEY thiskey, const char *lpSubKey)
{
	HKEY key;
	int retval=RegOpenKeyEx(thiskey,lpSubKey,0,KEY_ALL_ACCESS,&key);
	if (retval==ERROR_SUCCESS)
	{
		char buffer[MAX_PATH+1];
		while (RegEnumKey(key,0,buffer,MAX_PATH+1)==ERROR_SUCCESS)
		{
			if ((retval=RegDeleteKeyEx(key,buffer)) != ERROR_SUCCESS) break;
		}
		RegCloseKey(key);
		retval=RegDeleteKey(thiskey,lpSubKey);
	}
	return retval;
}
