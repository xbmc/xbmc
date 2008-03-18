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
#include "misc\MarkupSTL.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#pragma warning (disable:4244)
#pragma warning (disable:4800)
std::list<COptions *> COptions::m_InstanceList;
CCriticalSectionWrapper COptions::m_Sync;
COptions::t_OptionsCache COptions::m_sOptionsCache[OPTIONS_NUM];
BOOL COptions::m_bInitialized=FALSE;



#if defined(_XBOX)
CStdString GetDefaultWelcomeMessage()
{
  return "\r\n---------------------------------------------------------------------------"
         "\r\n%v\r\n"
         "\r\nhttp://sourceforge.net/projects/xbmc/"
         "\r\n---------------------------------------------------------------------------\r\n\r\n";
}
#endif

SPEEDLIMITSLIST COptions::m_sDownloadSpeedLimits;
SPEEDLIMITSLIST COptions::m_sUploadSpeedLimits;

/////////////////////////////////////////////////////////////////////////////
// COptionsHelperWindow

class COptionsHelperWindow
{
public:
	COptionsHelperWindow(COptions *pOptions)
	{
		ASSERT(pOptions);
		m_pOptions=pOptions;
		
		//Create window
		WNDCLASSEX wndclass; 
		wndclass.cbSize=sizeof wndclass; 
		wndclass.style=0; 
		wndclass.lpfnWndProc=WindowProc; 
		wndclass.cbClsExtra=0; 
		wndclass.cbWndExtra=0; 
		wndclass.hInstance=GetModuleHandle(0); 
		wndclass.hIcon=0; 
		wndclass.hCursor=0; 
		wndclass.hbrBackground=0; 
		wndclass.lpszMenuName=0; 
		wndclass.lpszClassName=_T("COptions Helper Window"); 
		wndclass.hIconSm=0; 
		
		RegisterClassEx(&wndclass);
		
		m_hWnd=CreateWindow(_T("COptions Helper Window"), _T("COptions Helper Window"), 0, 0, 0, 0, 0, 0, 0, 0, GetModuleHandle(0));
		ASSERT(m_hWnd);
		SetWindowLong(m_hWnd, GWL_USERDATA, (LONG)this);
	};
	
	virtual ~COptionsHelperWindow()
	{
		//Destroy window
		if (m_hWnd)
		{
			DestroyWindow(m_hWnd);
			m_hWnd=0;
		}
	}
	
	HWND GetHwnd()
	{
		return m_hWnd;
	}
	
protected:
	static LRESULT CALLBACK WindowProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
	{
		if (message==WM_USER)
		{
			COptionsHelperWindow *pWnd=(COptionsHelperWindow *)GetWindowLong(hWnd, GWL_USERDATA);
			if (!pWnd)
				return 0;
			ASSERT(pWnd);
			ASSERT(pWnd->m_pOptions);
			for (int i=0;i<OPTIONS_NUM;i++)
				pWnd->m_pOptions->m_OptionsCache[i].bCached=FALSE;	
			COptions::m_Sync.Lock();
			pWnd->m_pOptions->m_DownloadSpeedLimits = COptions::m_sDownloadSpeedLimits;
			pWnd->m_pOptions->m_UploadSpeedLimits = COptions::m_sUploadSpeedLimits;
			COptions::m_Sync.Unlock();
		}
		return ::DefWindowProc(hWnd, message, wParam, lParam);
	}
	COptions *m_pOptions;
	
private:
	HWND m_hWnd;
};

/////////////////////////////////////////////////////////////////////////////
// Dialogfeld COptions 

COptions::COptions()
{
	for (int i=0;i<OPTIONS_NUM;i++)
		m_OptionsCache[i].bCached=FALSE;
	m_pOptionsHelperWindow=new COptionsHelperWindow(this);
	m_Sync.Lock();
#ifdef _DEBUG 
	for (std::list<COptions *>::iterator iter=m_InstanceList.begin(); iter!=m_InstanceList.end(); iter++)
		ASSERT(*iter!=this);
#endif _DEBUG
	m_InstanceList.push_back(this);
	m_Sync.Unlock();
}

COptions::~COptions()
{
	m_Sync.Lock();
	std::list<COptions *>::iterator iter;
	for (iter=m_InstanceList.begin(); iter!=m_InstanceList.end(); iter++)
		if (*iter==this)
			break;

	ASSERT(iter!=m_InstanceList.end());
	m_InstanceList.erase(iter);
	m_Sync.Unlock();

	if (m_pOptionsHelperWindow)
		delete m_pOptionsHelperWindow;
	m_pOptionsHelperWindow=0;
}

void COptions::SetOption(int nOptionID, _int64 value)
{
	switch (nOptionID)
	{
	case OPTION_SERVERPORT:
		if (value>65535)
			value=21;
		else if (value<1)
			value=21;
		break;
	case OPTION_MAXUSERS:
		if (value<0)
			value=0;
		break;
	case OPTION_THREADNUM:
		if (value<1)
			value=2;
		else if (value>50)
			value=2;
		break;
	case OPTION_TIMEOUT:
	case OPTION_NOTRANSFERTIMEOUT:
		if (value<0)
			value=120;
		else if (value>9999)
			value=120;
		break;
	case OPTION_LOGINTIMEOUT:
		if (value<0)
			value=60;
		else if (value>9999)
			value=60;
		break;
	case OPTION_ADMINPORT:
		if (value>65535)
			value=14147;
		else if (value<1)
			value=14147;
		break;
	case OPTION_LOGTYPE:
		if (value!=0 && value!=1)
			value = 0;
		break;
	case OPTION_LOGLIMITSIZE:
		if ((value > 999999 || value < 10) && value!=0)
			value = 100;
		break;
	case OPTION_LOGDELETETIME:
		if (value > 999 || value < 0)
			value = 14;
		break;
	case OPTION_DOWNLOADSPEEDLIMITTYPE:
	case OPTION_UPLOADSPEEDLIMITTYPE:
		if (value < 0 || value > 2)
			value = 0;
		break;
	case OPTION_DOWNLOADSPEEDLIMIT:
	case OPTION_UPLOADSPEEDLIMIT:
		if (value > 65535 || value < 1)
			value = 10;
		break;
	case OPTION_BUFFERSIZE:
		if (value < 256 || value > (1024*1024))
			value = 4096;
		break;
	case OPTION_CUSTOMPASVIPTYPE:
		if (value < 0 || value > 2)
			value = 0;
		break;
	}
		
	Init();

	m_Sync.Lock();
	m_sOptionsCache[nOptionID-1].bCached = TRUE;
	m_sOptionsCache[nOptionID-1].nType = 1;
	m_sOptionsCache[nOptionID-1].value = value;
	m_OptionsCache[nOptionID-1]=m_sOptionsCache[nOptionID-1];
	
	m_Sync.Unlock();

	TCHAR buffer[MAX_PATH + 1000]; //Make it large enough
	GetModuleFileName( 0, buffer, MAX_PATH );
	LPTSTR pos=_tcsrchr(buffer, '\\');
	if (pos)
		*++pos=0;
	_tcscat(buffer, _T("FileZilla Server.xml"));
	CMarkupSTL xml;
	if (!xml.Load(buffer))
		return;
	
	if (!xml.FindElem( _T("FileZillaServer") ))
		return;

	if (!xml.FindChildElem( _T("Settings") ))
		xml.AddChildElem( _T("Settings") );
	
	CStdString valuestr;
	valuestr.Format( _T("%I64d"), value);
	xml.IntoElem();
	BOOL res=xml.FindChildElem();
	while (res)
	{
		CStdString name=xml.GetChildAttrib( _T("name"));
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

void COptions::SetOption(int nOptionID, LPCTSTR value)
{
	CStdString str = value;
	Init();

	switch (nOptionID)
	{
	case OPTION_WELCOMEMESSAGE:
		{
			std::vector<CStdString> msgLines;
			int oldpos=0;
			str.Replace("\r\n", "\n");
			int pos=str.Find("\n");
			CStdString line;
			while (pos!=-1)
			{
				if (pos)
				{
					line = str.Mid(oldpos, pos-oldpos);
					line = line.Left(70);
					line.TrimRight(" ");
					if (msgLines.size() || line!="")
						msgLines.push_back(line);
				}
				oldpos=pos+1;
				pos=str.Find("\n", oldpos);
			}
			line=str.Mid(oldpos);
			if (line!="")
			{
				line=line.Left(70);
				msgLines.push_back(line);
			}
			str="";
			for (unsigned int i=0;i<msgLines.size();i++)
				str+=msgLines[i]+"\r\n";
			str.TrimRight("\r\n");
			if (str=="")
			{
#if defined(_XBOX)
				str = GetDefaultWelcomeMessage();
#else
				str="%v";
				str+="\r\nwritten by Tim Kosse (Tim.Kosse@gmx.de)";
				str+="\r\nPlease visit http://sourceforge.net/projects/filezilla/";
#endif
			}
		}
		break;
	case OPTION_ADMINIPBINDINGS:
		{
			CStdString sub;
			std::list<CStdString> ipBindList;
			for (unsigned int i = 0; i<_tcslen(value); i++)
			{
				char cur = value[i];
				if ((cur<'0' || cur>'9') && cur!='.')
				{
					if (sub=="" && cur=='*')
					{
						ipBindList.clear();
						ipBindList.push_back("*");
						break;
					}

					if (sub != "")
					{
						//Parse IP
						SOCKADDR_IN sockAddr;
						memset(&sockAddr,0,sizeof(sockAddr));
			
						sockAddr.sin_family = AF_INET;
						sockAddr.sin_addr.s_addr = inet_addr(sub);
			
						if (sockAddr.sin_addr.s_addr != INADDR_NONE)
						{
							sub = inet_ntoa(sockAddr.sin_addr);
							std::list<CStdString>::iterator iter;
							for (iter = ipBindList.begin(); iter!=ipBindList.end(); iter++)
								if (*iter==sub)
									break;
							if (iter == ipBindList.end())
								ipBindList.push_back(sub);
						}
						sub = "";
					}
				}
				else
					sub += cur;
			}
			if (sub != "")
			{
				//Parse IP
				SOCKADDR_IN sockAddr;
				memset(&sockAddr,0,sizeof(sockAddr));
				
				sockAddr.sin_family = AF_INET;
				sockAddr.sin_addr.s_addr = inet_addr(sub);
		
				if (sockAddr.sin_addr.s_addr != INADDR_NONE)
				{
					sub = inet_ntoa(sockAddr.sin_addr);
					std::list<CStdString>::iterator iter;
					for (iter = ipBindList.begin(); iter!=ipBindList.end(); iter++)
						if (*iter==sub)
							break;
					if (iter == ipBindList.end())
						ipBindList.push_back(sub);
				}
				sub = "";
			}
			str = "";
			for (std::list<CStdString>::iterator iter = ipBindList.begin(); iter!=ipBindList.end(); iter++)
				if (*iter != "127.0.0.1")
					str += *iter + " ";
	
			str.TrimRight(" ");
		}
		break;
	case OPTION_ADMINIPADDRESSES:
		{
			CStdString sub;
			int dotCount = 0;
			std::list<CStdString> ipList;
			for (unsigned int i = 0; i<_tcslen(value); i++)
			{
				char cur = value[i];
				if ((cur<'0' || cur>'9') && cur!='.' && cur!='*' && cur!='?')
				{
					if (sub!="" && dotCount==3)
					{
						while (sub.Replace("**", "*"));
						BOOL bError = FALSE;
						CStdString ip;
						for (int j=0; j<3; j++)
						{
							int pos = sub.Find(".");
							if (pos<1 || pos>3)
							{
								bError = TRUE;
								break;
							}
							ip += sub.Left(pos) + ".";
							sub = sub.Mid(pos + 1);
						}
						if (!bError && sub.GetLength() != 0)
						{
							ip += sub;
							ipList.push_back(ip);
						}
					}
					sub = "";
					dotCount = 0;
				}
				else
				{
					sub+=cur;
					if (cur == '.')
						dotCount++;
				}
			}
			if (sub!="" && dotCount==3)
			{
				while (sub.Replace("**", "*"));
				BOOL bError = FALSE;
				CStdString ip;
				for (int j=0; j<3; j++)
				{
					int pos = sub.Find(".");
					if (pos<1 || pos>3)
					{
						bError = TRUE;
						break;
					}
					ip += sub.Left(pos) + ".";
					sub = sub.Mid(pos + 1);
				}
				if (!bError && sub.GetLength() != 0)
				{
					ip += sub;
					ipList.push_back(ip);
				}
			}

			str = "";
			for (std::list<CStdString>::iterator iter = ipList.begin(); iter!=ipList.end(); iter++)
				if (*iter != "127.0.0.1")
					str += *iter + " ";
			str.TrimRight(" ");
		}
		break;
	case OPTION_ADMINPASS:
		if (str.GetLength() < 6)
			return;
		break;
	}
	m_Sync.Lock();
	m_sOptionsCache[nOptionID-1].bCached = TRUE;
	m_sOptionsCache[nOptionID-1].nType = 0;
	m_sOptionsCache[nOptionID-1].str = str;
	m_OptionsCache[nOptionID-1]=m_sOptionsCache[nOptionID-1];
	m_Sync.Unlock();

	TCHAR buffer[MAX_PATH + 1000]; //Make it large enough
	GetModuleFileName( 0, buffer, MAX_PATH );
	LPTSTR pos=_tcsrchr(buffer, '\\');
	if (pos)
		*++pos=0;
	_tcscat(buffer, _T("FileZilla Server.xml"));
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
		CStdString name=xml.GetChildAttrib( _T("name"));
		if (!_tcscmp(name, m_Options[nOptionID-1].name))
		{
			xml.SetChildAttrib(_T("name"), m_Options[nOptionID-1].name);
			xml.SetChildAttrib(_T("type"), _T("string"));
			xml.SetChildData(str);
			break;
		}
		res=xml.FindChildElem();
	}
	if (!res)
	{
		xml.InsertChildElem( _T("Item"), str );
		xml.SetChildAttrib(_T("name"), m_Options[nOptionID-1].name);
		xml.SetChildAttrib(_T("type"), _T("string"));
	}
	xml.Save(buffer);
}

CStdString COptions::GetOption(int nOptionID)
{
	ASSERT(nOptionID>0 && nOptionID<=OPTIONS_NUM);
	ASSERT(!m_Options[nOptionID-1].nType);
	Init();
	
	if (m_OptionsCache[nOptionID-1].bCached)
		return m_OptionsCache[nOptionID-1].str;

	m_Sync.Lock();

	if (!m_sOptionsCache[nOptionID-1].bCached)
	{
		//Default values
		switch (nOptionID)
		{
		case OPTION_WELCOMEMESSAGE:
#if defined(_XBOX)
			m_sOptionsCache[nOptionID-1].str = GetDefaultWelcomeMessage();
#else
			m_sOptionsCache[nOptionID-1].str ="%v";
			m_sOptionsCache[nOptionID-1].str+="\r\nwritten by Tim Kosse (Tim.Kosse@gmx.de)";
			m_sOptionsCache[nOptionID-1].str+="\r\nPlease visit http://sourceforge.net/projects/filezilla/";
#endif
			break;
		case OPTION_CUSTOMPASVIPSERVER:
			m_sOptionsCache[nOptionID-1].str = "http://filezilla.sourceforge.net/misc/ip.php";
			break;
		default:
			m_sOptionsCache[nOptionID-1].str="";
			m_sOptionsCache[nOptionID-1].bCached=TRUE;
			break;
		}
		m_sOptionsCache[nOptionID-1].bCached=TRUE;
		m_sOptionsCache[nOptionID-1].nType=0;
	}
	m_OptionsCache[nOptionID-1]=m_sOptionsCache[nOptionID-1];
	m_Sync.Unlock();
	return m_OptionsCache[nOptionID-1].str;
}

_int64 COptions::GetOptionVal(int nOptionID)
{
	ASSERT(nOptionID>0 && nOptionID<=OPTIONS_NUM);
	ASSERT(m_Options[nOptionID-1].nType == 1);
	Init();
	
	if (m_OptionsCache[nOptionID-1].bCached)
		return m_OptionsCache[nOptionID-1].value;

	m_Sync.Lock();

	if (!m_sOptionsCache[nOptionID-1].bCached)
	{
		//Default values
		switch (nOptionID)
		{
			case OPTION_SERVERPORT:
				m_sOptionsCache[nOptionID-1].value = 21;
				break;
			case OPTION_MAXUSERS:
				m_sOptionsCache[nOptionID-1].value = 0;
				break;
			case OPTION_THREADNUM:
				m_sOptionsCache[nOptionID-1].value = 2;
				break;
			case OPTION_TIMEOUT:
			case OPTION_NOTRANSFERTIMEOUT:
				m_sOptionsCache[nOptionID-1].value = 120;
				break;
			case OPTION_LOGINTIMEOUT:
				m_sOptionsCache[nOptionID-1].value = 60;
				break;
			case OPTION_ADMINPORT:
				m_sOptionsCache[nOptionID-1].value = 14147;
				break;
			case OPTION_DOWNLOADSPEEDLIMIT:
			case OPTION_UPLOADSPEEDLIMIT:
				m_sOptionsCache[nOptionID-1].value = 10;
				break;
			case OPTION_BUFFERSIZE:
				m_sOptionsCache[nOptionID-1].value = 4096;
				break;
			case OPTION_CUSTOMPASVIPTYPE:
				m_sOptionsCache[nOptionID-1].value = 0;
				break;
			default:
				m_sOptionsCache[nOptionID-1].value=0;
		}
		m_sOptionsCache[nOptionID-1].bCached=TRUE;
		m_sOptionsCache[nOptionID-1].nType=1;
	}
	m_OptionsCache[nOptionID-1]=m_sOptionsCache[nOptionID-1];
	m_Sync.Unlock();
	return m_OptionsCache[nOptionID-1].value;
}

void COptions::UpdateInstances()
{
	m_Sync.Lock();
	for (std::list<COptions *>::iterator iter=m_InstanceList.begin(); iter!=m_InstanceList.end(); iter++)
	{
		ASSERT((*iter)->m_pOptionsHelperWindow);
		::PostMessage((*iter)->m_pOptionsHelperWindow->GetHwnd(), WM_USER, 0, 0);
	}
	m_Sync.Unlock();
}

void COptions::Init()
{
	m_Sync.Lock();
	if (m_bInitialized)
	{
		m_Sync.Unlock();
		return;
	}
	m_bInitialized=TRUE;
	CFileStatus64 status;
	CMarkupSTL xml;
	TCHAR buffer[MAX_PATH + 1000]; //Make it large enough
	GetModuleFileName( 0, buffer, MAX_PATH );
	LPTSTR pos=_tcsrchr(buffer, '\\');
	if (pos)
		*++pos=0;
	_tcscat(buffer, _T("FileZilla Server.xml"));
	
	for (int i=0; i<OPTIONS_NUM; i++)
		m_sOptionsCache[i].bCached=FALSE;
	
	if (!GetStatus64(buffer, status) )
	{
		xml.AddElem( _T("FileZillaServer") );
		if (!xml.Save(buffer))
		{
			m_Sync.Unlock();
			return;
		}
	}
	else if (status.m_attribute&FILE_ATTRIBUTE_DIRECTORY)
	{
		m_Sync.Unlock();
		return;
	}
		
	if (xml.Load(buffer))
	{
		if (xml.FindElem( _T("FileZillaServer") ))
		{
			if (!xml.FindChildElem( _T("Settings") ))
				xml.AddChildElem( _T("Settings") );
			
			CStdString str;
			xml.IntoElem();
			str=xml.GetTagName();
			while (xml.FindChildElem())
			{
				CStdString value=xml.GetChildData();
				CStdString name=xml.GetChildAttrib( _T("name") );
				CStdString type=xml.GetChildAttrib( _T("type") );
				for (int i=0;i<OPTIONS_NUM;i++)
				{
					if (!_tcscmp(name, m_Options[i].name))
					{
						if (m_sOptionsCache[i].bCached)
							break;
						else
						{
							if (type=="numeric")
							{
								if (m_Options[i].nType!=1)
									break;
								_int64 value64=_ttoi64(value);
								if (IsNumeric(value))
									SetOption(i+1, value64);
							}
							else
							{
								if (m_Options[i].nType!=0)
									break;
								SetOption(i+1, value);
							}
						}
						break;
					}
				}
			}
			ReadSpeedLimits(&xml);
			
			m_Sync.Unlock();
			UpdateInstances();
			return;
		}
	}
	m_Sync.Unlock();
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
	m_Sync.Lock();
	TCHAR buffer[MAX_PATH + 1000]; //Make it large enough
	GetModuleFileName( 0, buffer, MAX_PATH );
	LPTSTR pos=_tcsrchr(buffer, '\\');
	if (pos)
		*++pos=0;
	_tcscat(buffer, _T("FileZilla Server.xml"));
	CMarkupSTL *pXML=new CMarkupSTL;
	if (!pXML->Load(buffer))
	{
		m_Sync.Unlock();
		delete pXML;
		return NULL;
	}
	
	if (!pXML->FindElem( _T("FileZillaServer") ))
	{
		m_Sync.Unlock();
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
	_tcscat(buffer, _T("FileZilla Server.xml"));
	if (!pXML->Save(buffer))
		return FALSE;
	delete pXML;
	m_Sync.Unlock();
	return TRUE;
}

BOOL COptions::GetAsCommand(char **pBuffer, DWORD *nBufferLength)
{
	int i;
	DWORD len = 2;

	m_Sync.Lock();
	for (i=0; i<OPTIONS_NUM; i++)
	{
		len+=1;
		if (!m_Options[i].nType)
			if ((i+1)!=OPTION_ADMINPASS)
				len+=GetOption(i+1).GetLength()+2;
			else if (GetOption(i+1).GetLength() >= 6)
				len+=GetOption(i+1).GetLength()+2;
			else
				len+=3;
		else
			len+=8;
	}

	len +=4;
	SPEEDLIMITSLIST::const_iterator iter;
	for (iter = m_sDownloadSpeedLimits.begin(); iter != m_sDownloadSpeedLimits.end(); iter++)
		len += iter->GetRequiredBufferLen();
	for (iter = m_sUploadSpeedLimits.begin(); iter != m_sUploadSpeedLimits.end(); iter++)
		len += iter->GetRequiredBufferLen();

	*pBuffer=new char[len];
	char *p=*pBuffer;
	*p++ = OPTIONS_NUM/256;
	*p++ = OPTIONS_NUM%256;
	for (i=0; i<OPTIONS_NUM; i++)
	{
		*p++ = m_Options[i].nType;
		switch(m_Options[i].nType) {
		case 0:
			{
				CStdString str = GetOption(i+1);
				if ((i+1)==OPTION_ADMINPASS) //Do NOT send admin password, 
											 //instead send empty string if admin pass is set
											 //and send a single char if admin pass is invalid (len < 6)
				if (str.GetLength() >= 6)
					str="";
				else
					str="*";
				*p++ = str.GetLength() / 256;
				*p++ = str.GetLength() % 256;
				memcpy(p, str, str.GetLength());
				p+=str.GetLength();
			}
			break;
		case 1:
			{
				_int64 value = GetOptionVal(i+1);
				memcpy(p, &value, 8);
				p+=8;
			}
			break;
		default:
			ASSERT(FALSE);
		}
	}

	*p++ = m_sDownloadSpeedLimits.size() << 8;
	*p++ = m_sDownloadSpeedLimits.size() %256;
	for (iter = m_sDownloadSpeedLimits.begin(); iter != m_sDownloadSpeedLimits.end(); iter++)
		p = iter->FillBuffer(p);

	*p++ = m_sUploadSpeedLimits.size() << 8;
	*p++ = m_sUploadSpeedLimits.size() %256;
	for (iter = m_sUploadSpeedLimits.begin(); iter != m_sUploadSpeedLimits.end(); iter++)
		p = iter->FillBuffer(p);

	m_Sync.Unlock();

	*nBufferLength = len;

	return TRUE;
}

BOOL COptions::ParseOptionsCommand(unsigned char *pData, DWORD dwDataLength, BOOL bFromLocal /*=FALSE*/)
{
	unsigned char *p=pData;
	int num = *p * 256 + p[1];
	p+=2;
	if (num!=OPTIONS_NUM)
		return FALSE;
	
	int i;
	for (i = 0; i < num; i++)
	{
		if ((DWORD)(p-pData)>=dwDataLength)
			return FALSE;
		int nType = *p++;
		if (!nType)
		{
			if ((DWORD)(p-pData+2) >= dwDataLength)
				return 2;
			int len= *p * 256 + p[1];
			p+=2;
			if ((DWORD)(p-pData+len)>dwDataLength)
				return FALSE;
			char *pBuffer = new char[len+1];
			memcpy(pBuffer, p, len);
			pBuffer[len]=0;
			if (!m_Options[i].bOnlyLocal || bFromLocal) //Do not change admin interface settings from remote connections
				SetOption(i+1, pBuffer);
			delete [] pBuffer;
			p+=len;
		}
		else if (nType == 1)
		{
			if ((DWORD)(p-pData+8)>dwDataLength)
				return FALSE;
			if (!m_Options[i].bOnlyLocal || bFromLocal) //Do not change admin interface settings from remote connections
				SetOption(i+1, *(_int64 *)p);
			p+=8;
		}
		else
			return FALSE;
	}

	SPEEDLIMITSLIST dl;
	SPEEDLIMITSLIST ul;

	if ((p-pData+2)>dwDataLength)
		return FALSE;
	num = *p++ << 8;
	num |= *p++;
	m_Sync.Lock();
	for (i=0; i<num; i++)
	{
		CSpeedLimit limit;
		p = limit.ParseBuffer(p, dwDataLength - (p - pData));
		if (!p)
		{
			m_Sync.Unlock();
			return FALSE;
		}
		dl.push_back(limit);
	}
	
	if ((p-pData+2)>dwDataLength)
	{
		m_Sync.Unlock();
		return FALSE;
	}
	num = *p++ << 8;
	num |= *p++;
	for (i=0; i<num; i++)
	{
		CSpeedLimit limit;
		p = limit.ParseBuffer(p, dwDataLength - (p - pData));
		if (!p)
		{
			m_Sync.Unlock();
			return FALSE;
		}
		ul.push_back(limit);
	}

	m_sDownloadSpeedLimits = dl;
	m_sUploadSpeedLimits = ul;
	VERIFY(SaveSpeedLimits());

	m_Sync.Unlock();

	UpdateInstances();
	return TRUE;
}

BOOL COptions::SaveSpeedLimits()
{
	TCHAR buffer[MAX_PATH + 1000]; //Make it large enough
	GetModuleFileName( 0, buffer, MAX_PATH );
	LPTSTR pos=_tcsrchr(buffer, '\\');
	if (pos)
		*++pos=0;
	_tcscat(buffer, _T("FileZilla Server.xml"));
	CMarkupSTL xml;
	if (!xml.Load(buffer))
		return FALSE;
	
	if (!xml.FindElem( _T("FileZillaServer") ))
		return FALSE;

	m_Sync.Lock();

	if (!xml.FindChildElem( _T("Settings") ))
		xml.AddChildElem( _T("Settings") );
	
	xml.IntoElem();

	while (xml.FindChildElem(_T("SpeedLimits")))
		xml.RemoveChildElem();
	xml.AddChildElem(_T("SpeedLimits"));

	CStdString str;

	xml.IntoElem();

	int i;

	xml.AddChildElem("Download");
	xml.IntoElem();
	for (i=0; i<m_sDownloadSpeedLimits.size(); i++)
	{
		CSpeedLimit limit = m_sDownloadSpeedLimits[i];
		xml.AddChildElem(_T("Rule"));

		xml.SetChildAttrib(_T("Speed"), limit.m_Speed);

		xml.IntoElem();

		str.Format("%d", limit.m_Day);
		xml.AddChildElem(_T("Days"), str);

		if (limit.m_DateCheck)
		{
			xml.AddChildElem(_T("Date"));
			xml.SetChildAttrib(_T("Year"), limit.m_Date.y);
			xml.SetChildAttrib(_T("Month"), limit.m_Date.m);
			xml.SetChildAttrib(_T("Day"), limit.m_Date.d);
		}

		if (limit.m_FromCheck)
		{
			xml.AddChildElem(_T("From"));
			xml.SetChildAttrib(_T("Hour"), limit.m_FromTime.h);
			xml.SetChildAttrib(_T("Minute"), limit.m_FromTime.m);
			xml.SetChildAttrib(_T("Second"), limit.m_FromTime.s);
		}
		
		if (limit.m_ToCheck)
		{
			xml.AddChildElem(_T("To"));
			xml.SetChildAttrib(_T("Hour"), limit.m_ToTime.h);
			xml.SetChildAttrib(_T("Minute"), limit.m_ToTime.m);
			xml.SetChildAttrib(_T("Second"), limit.m_ToTime.s);
		}
	
		xml.OutOfElem();
	}
	xml.OutOfElem();

	xml.AddChildElem("Upload");
	xml.IntoElem();
	for (i=0; i<m_sUploadSpeedLimits.size(); i++)
	{
		CSpeedLimit limit = m_sUploadSpeedLimits[i];
		xml.AddChildElem(_T("Rule"));

		xml.SetChildAttrib(_T("Speed"), limit.m_Speed);

		xml.IntoElem();

		str.Format("%d", limit.m_Day);
		xml.AddChildElem(_T("Days"), str);

		if (limit.m_DateCheck)
		{
			xml.AddChildElem(_T("Date"));
			xml.SetChildAttrib(_T("Year"), limit.m_Date.y);
			xml.SetChildAttrib(_T("Month"), limit.m_Date.m);
			xml.SetChildAttrib(_T("Day"), limit.m_Date.d);
		}

		if (limit.m_FromCheck)
		{
			xml.AddChildElem(_T("From"));
			xml.SetChildAttrib(_T("Hour"), limit.m_FromTime.h);
			xml.SetChildAttrib(_T("Minute"), limit.m_FromTime.m);
			xml.SetChildAttrib(_T("Second"), limit.m_FromTime.s);
		}
		
		if (limit.m_ToCheck)
		{
			xml.AddChildElem(_T("To"));
			xml.SetChildAttrib(_T("Hour"), limit.m_ToTime.h);
			xml.SetChildAttrib(_T("Minute"), limit.m_ToTime.m);
			xml.SetChildAttrib(_T("Second"), limit.m_ToTime.s);
		}
	
		xml.OutOfElem();
	}
	xml.OutOfElem();
	
	xml.OutOfElem();

	xml.OutOfElem();
	
	xml.Save(buffer);

	m_Sync.Unlock();
	return TRUE;
}

BOOL COptions::ReadSpeedLimits(CMarkupSTL *pXML)
{
	m_Sync.Lock();
	pXML->ResetChildPos();
				
	while (pXML->FindChildElem(_T("SpeedLimits")))
	{
		CStdString str;
		int n;

		pXML->IntoElem();

		while (pXML->FindChildElem(_T("Download")))
		{
			pXML->IntoElem();

			while (pXML->FindChildElem(_T("Rule")))
			{
				CSpeedLimit limit;
				str = pXML->GetChildAttrib("Speed");
				n = _ttoi(str);
				if (n < 0 || n > 65535)
					n = 10;
				limit.m_Speed = n;
				
				pXML->IntoElem();
				
				if (pXML->FindChildElem("Days"))
				{
					str = pXML->GetChildData();
					if (str != "")
						n = _ttoi(str);
					else
						n = 0x7F;
					limit.m_Day = n & 0x7F;
				}
				pXML->ResetChildPos();
				
				limit.m_DateCheck = FALSE;
				if (pXML->FindChildElem("Date"))
				{
					limit.m_DateCheck = TRUE;
					str = pXML->GetChildAttrib("Year");
					n = _ttoi(str);
					if (n < 1900 || n > 3000)
						n = 2003;
					limit.m_Date.y = n;
					str = pXML->GetChildAttrib("Month");
					n = _ttoi(str);
					if (n < 1 || n > 12)
						n = 1;
					limit.m_Date.m = n;
					str = pXML->GetChildAttrib("Day");
					n = _ttoi(str);
					if (n < 1 || n > 31)
						n = 1;
					limit.m_Date.d = n;
				}
				pXML->ResetChildPos();
				
				limit.m_FromCheck = FALSE;
				if (pXML->FindChildElem("From"))
				{
					limit.m_FromCheck = TRUE;
					str = pXML->GetChildAttrib("Hour");
					n = _ttoi(str);
					if (n < 0 || n > 23)
						n = 0;
					limit.m_FromTime.h = n;
					str = pXML->GetChildAttrib("Minute");
					n = _ttoi(str);
					if (n < 0 || n > 59)
						n = 0;
					limit.m_FromTime.m = n;
					str = pXML->GetChildAttrib("Second");
					n = _ttoi(str);
					if (n < 0 || n > 59)
						n = 0;
					limit.m_FromTime.s = n;
				}
				pXML->ResetChildPos();
				
				limit.m_ToCheck = FALSE;
				if (pXML->FindChildElem("To"))
				{
					limit.m_ToCheck = TRUE;
					str = pXML->GetChildAttrib("Hour");
					n = _ttoi(str);
					if (n < 0 || n > 23)
						n = 0;
					limit.m_ToTime.h = n;
					str = pXML->GetChildAttrib("Minute");
					n = _ttoi(str);
					if (n < 0 || n > 59)
						n = 0;
					limit.m_ToTime.m = n;
					str = pXML->GetChildAttrib("Second");
					n = _ttoi(str);
					if (n < 0 || n > 59)
						n = 0;
					limit.m_ToTime.s = n;
				}
				pXML->ResetChildPos();
				
				pXML->OutOfElem();

				m_sDownloadSpeedLimits.push_back(limit);
			}
			pXML->OutOfElem();
		}
		pXML->ResetChildPos();

		while (pXML->FindChildElem(_T("Upload")))
		{
			pXML->IntoElem();

			while (pXML->FindChildElem(_T("Rule")))
			{
				CSpeedLimit limit;
				str = pXML->GetChildAttrib("Speed");
				n = _ttoi(str);
				if (n < 0 || n > 65535)
					n = 10;
				limit.m_Speed = n;
				
				pXML->IntoElem();
				
				if (pXML->FindChildElem("Days"))
				{
					str = pXML->GetChildData();
					if (str != "")
						n = _ttoi(str);
					else
						n = 0x7F;
					limit.m_Day = n & 0x7F;
				}
				pXML->ResetChildPos();
				
				limit.m_DateCheck = FALSE;
				if (pXML->FindChildElem("Date"))
				{
					limit.m_DateCheck = TRUE;
					str = pXML->GetChildAttrib("Year");
					n = _ttoi(str);
					if (n < 1900 || n > 3000)
						n = 2003;
					limit.m_Date.y = n;
					str = pXML->GetChildAttrib("Month");
					n = _ttoi(str);
					if (n < 1 || n > 12)
						n = 1;
					limit.m_Date.m = n;
					str = pXML->GetChildAttrib("Day");
					n = _ttoi(str);
					if (n < 1 || n > 31)
						n = 1;
					limit.m_Date.d = n;
				}
				pXML->ResetChildPos();
				
				limit.m_FromCheck = FALSE;
				if (pXML->FindChildElem("From"))
				{
					limit.m_FromCheck = TRUE;
					str = pXML->GetChildAttrib("Hour");
					n = _ttoi(str);
					if (n < 0 || n > 23)
						n = 0;
					limit.m_FromTime.h = n;
					str = pXML->GetChildAttrib("Minute");
					n = _ttoi(str);
					if (n < 0 || n > 59)
						n = 0;
					limit.m_FromTime.m = n;
					str = pXML->GetChildAttrib("Second");
					n = _ttoi(str);
					if (n < 0 || n > 59)
						n = 0;
					limit.m_FromTime.s = n;
				}
				pXML->ResetChildPos();
				
				limit.m_ToCheck = FALSE;
				if (pXML->FindChildElem("To"))
				{
					limit.m_ToCheck = TRUE;
					str = pXML->GetChildAttrib("Hour");
					n = _ttoi(str);
					if (n < 0 || n > 23)
						n = 0;
					limit.m_ToTime.h = n;
					str = pXML->GetChildAttrib("Minute");
					n = _ttoi(str);
					if (n < 0 || n > 59)
						n = 0;
					limit.m_ToTime.m = n;
					str = pXML->GetChildAttrib("Second");
					n = _ttoi(str);
					if (n < 0 || n > 59)
						n = 0;
					limit.m_ToTime.s = n;
				}
				pXML->ResetChildPos();
				
				pXML->OutOfElem();

				m_sUploadSpeedLimits.push_back(limit);
			}
			pXML->OutOfElem();
		}

		pXML->OutOfElem();
	}
	m_Sync.Unlock();

	return TRUE;
}

int COptions::GetCurrentSpeedLimit(int nMode)
{
	Init();
	if (nMode)
	{
		int nType = GetOptionVal(OPTION_UPLOADSPEEDLIMITTYPE);
		switch (nType)
		{
			case 0:
				return -1;
			case 1:
				return GetOptionVal(OPTION_UPLOADSPEEDLIMIT);
			default:
				{
					SYSTEMTIME s;
					GetSystemTime(&s);
					for (SPEEDLIMITSLIST::const_iterator iter = m_UploadSpeedLimits.begin(); iter != m_UploadSpeedLimits.end(); iter++)
						if (iter->IsItActive(s))
							return iter->m_Speed;
					return -1;
				}
		}
	}
	else
	{
		int nType = GetOptionVal(OPTION_DOWNLOADSPEEDLIMITTYPE);
		switch (nType)
		{
			case 0:
				return -1;
			case 1:
				return GetOptionVal(OPTION_DOWNLOADSPEEDLIMIT);
			default:
				{
					SYSTEMTIME s;
					GetSystemTime(&s);
					for (SPEEDLIMITSLIST::const_iterator iter = m_DownloadSpeedLimits.begin(); iter != m_DownloadSpeedLimits.end(); iter++)
						if (iter->IsItActive(s))
							return iter->m_Speed;
					return -1;
				}
		}
	}
}

void COptions::ReloadConfig()
{
	m_Sync.Lock();

	m_bInitialized = TRUE;
	CFileStatus64 status;
	CMarkupSTL xml;
	TCHAR buffer[MAX_PATH + 1000]; //Make it large enough
	GetModuleFileName( 0, buffer, MAX_PATH );
	LPTSTR pos=_tcsrchr(buffer, '\\');
	if (pos)
		*++pos=0;
	_tcscat(buffer, _T("FileZilla Server.xml"));
	
	for (int i=0; i<OPTIONS_NUM; i++)
		m_sOptionsCache[i].bCached = FALSE;
	
	if (!GetStatus64(buffer, status) )
	{
		xml.AddElem( _T("FileZillaServer") );
		if (!xml.Save(buffer))
		{
			m_Sync.Unlock();
			return;
		}
	}
	else if (status.m_attribute&FILE_ATTRIBUTE_DIRECTORY)
	{
		m_Sync.Unlock();
		return;
	}
		
	if (xml.Load(buffer))
	{
		if (xml.FindElem( _T("FileZillaServer") ))
		{
			if (!xml.FindChildElem( _T("Settings") ))
				xml.AddChildElem( _T("Settings") );
			
			CStdString str;
			xml.IntoElem();
			str=xml.GetTagName();
			while (xml.FindChildElem())
			{
				CStdString value=xml.GetChildData();
				CStdString name=xml.GetChildAttrib( _T("name") );
				CStdString type=xml.GetChildAttrib( _T("type") );
				for (int i=0;i<OPTIONS_NUM;i++)
				{
					if (!_tcscmp(name, m_Options[i].name))
					{
						if (m_sOptionsCache[i].bCached)
							break;
						else
						{
							if (type=="numeric")
							{
								if (m_Options[i].nType!=1)
									break;
								_int64 value64=_ttoi64(value);
								if (IsNumeric(value))
									SetOption(i+1, value64);
							}
							else
							{
								if (m_Options[i].nType!=0)
									break;
								SetOption(i+1, value);
							}
						}
						break;
					}
				}
			}
			ReadSpeedLimits(&xml);
			
			m_Sync.Unlock();
			UpdateInstances();
			return;
		}
	}
	m_Sync.Unlock();
}