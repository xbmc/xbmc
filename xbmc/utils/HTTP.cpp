
#include "../stdafx.h"
// HTTP.cpp: implementation of the CHTTP class.
//
//////////////////////////////////////////////////////////////////////

#include "HTTP.h"
#include "stdstring.h"
#ifdef _XBOX
#include "../dnsnamecache.h"
#endif

#include "../settings.h"
#include "log.h"
#include "../util.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


						
CHTTP::CHTTP(const string& strProxyServer, int iProxyPort)
:m_strProxyServer(strProxyServer)
,m_iProxyPort(iProxyPort)
,m_socket(INVALID_SOCKET)
{
	m_strCookie="";
	m_iHTTPver=1;
	hEvent = WSA_INVALID_EVENT;
}


CHTTP::CHTTP()
:m_socket(INVALID_SOCKET)
{
	m_strProxyServer=g_stSettings.m_szHTTPProxy;
	m_iProxyPort=g_stSettings.m_iHTTPProxyPort;
	m_strCookie="";
	hEvent = WSA_INVALID_EVENT;
}


CHTTP::~CHTTP()
{
	Close();
}

//------------------------------------------------------------------------------------------------------------------

int CHTTP::FindLength(const string& strHeaders)
{
	string::size_type n = strHeaders.find("Content-Length: ");
	if (n == string::npos)
	{
		return -1;
	}
	else
	{
		return atol(strHeaders.c_str() + n+16);
	}
}

//------------------------------------------------------------------------------------------------------------------

bool CHTTP::Get(string& strURL, string& strHTML)
{
  CLog::Log("Get URL: %s", strURL.c_str());

	string strHeaders;
	strHTML.clear();
	int status = Open(strURL, "GET", strHeaders, strHTML);

	if (status != 200)
	{
		Close();
		return false;
	}

	bool bGotLength = false;
	int Length = FindLength(strHeaders);
	if (Length < 0)
		Length = 65536;
	else
		bGotLength = true;
	Length -= strHTML.size();

	if (Length > 0)
	{
		char* buf = new char[Length];
		int iRead;
		if (!Recv(buf, Length, iRead, true))
		{
			if (bGotLength)
			{
				delete [] buf;
				Close();
				strHTML.clear();
				return false;
			}
		}
		buf[iRead]=0;
		strHTML.append(buf, buf+iRead);
		delete [] buf;
	}
	
	Close();
	return true;
}

//------------------------------------------------------------------------------------------------------------------
bool CHTTP::Connect()
{
	if (!CUtil::IsNetworkUp())
		return false;

	sockaddr_in service;
	service.sin_family = AF_INET;

	if (m_strProxyServer.size())
	{
		// connect to proxy server
		service.sin_addr.s_addr = inet_addr(m_strProxyServer.c_str());
		service.sin_port = htons(m_iProxyPort);
	}
	else
	{
		// connect to site directly
		if(inet_addr(m_strHostName.c_str())==INADDR_NONE)
		{
			CStdString strIpAddress="";
			CDNSNameCache::Lookup(m_strHostName,strIpAddress);
			service.sin_addr.s_addr = inet_addr(strIpAddress.c_str());
			if (strIpAddress=="")
			{
				if (strcmp(m_strHostName.c_str(),"ia.imdb.com")==0)
					service.sin_addr.s_addr = inet_addr("193.108.152.15");
				else if (strcmp(m_strHostName.c_str(),"us.imdb.com")==0)
					service.sin_addr.s_addr = inet_addr("207.171.166.140");
				else if (strcmp(m_strHostName.c_str(),"www.imdb.com")==0)
					service.sin_addr.s_addr = inet_addr("207.171.166.140");
				else if (strcmp(m_strHostName.c_str(),"www.allmusic.com")==0)
					service.sin_addr.s_addr = inet_addr("64.152.71.2");
				else if (strcmp(m_strHostName.c_str(),"image.allmusic.com")==0)
					service.sin_addr.s_addr = inet_addr("64.152.70.67");
				else if (strcmp(m_strHostName.c_str(),"xoap.weather.com")==0)
					service.sin_addr.s_addr = inet_addr("63.111.24.34");
			}
		}
		else
		{
			service.sin_addr.s_addr = inet_addr(m_strHostName.c_str());
		}
		service.sin_port = htons(m_iPort);
	}
	m_socket.attach(socket(AF_INET,SOCK_STREAM,IPPROTO_TCP));
	
	// attempt to connection
	if (connect((SOCKET)m_socket,(sockaddr*) &service,sizeof(struct sockaddr)) == SOCKET_ERROR)
	{
		Close();
		return false;
	}
	hEvent = WSACreateEvent();
	return true;
}

//------------------------------------------------------------------------------------------------------------------
bool CHTTP::BreakURL(const string &strURL, string &strHostName, int& iPort, string &strFile)
{
  int pos1, pos2;
  char *ptr1, *ptr2, *ptr3;
	char *url=(char*)strURL.c_str();
	char szProtocol[128];
    // extract the protocol
	strFile="";
	iPort=80;
	strHostName="";
	
  ptr1 = strstr(url, "://");
  if( ptr1==NULL )
  {
		//g_dialog.SetCaption(0,"invalid URL");
		//g_dialog.SetMessage(0,strURL.c_str());
		//g_dialog.DoModal();
		return false;
  }
		    
	pos1 = ptr1-url;
	strncpy(szProtocol, url, pos1);
	szProtocol[pos1] = '\0';

	// jump the "://"
	ptr1 += 3;
	pos1 += 3;
		
    
  // look if the port is given
  ptr2 = strstr(ptr1, ":");
  // If the : is after the first / it isn't the port
  ptr3 = strstr(ptr1, "/");
  if(ptr3 && ptr3 - ptr2 < 0)
      ptr2 = NULL;
  if( ptr2==NULL )
  {
    // No port is given
    // Look if a path is given
		iPort=80;
    ptr2 = strstr(ptr1, "/");
    if( ptr2==NULL )
    {
        // No path/filename
        // So we have an URL like http://www.hostname.com
        pos2 = strlen(url);
    }
    else
    {
        // We have an URL like http://www.hostname.com/file.txt
        pos2 = ptr2-url;
    }
  }
  else
  {
    // We have an URL beginning like http://www.hostname.com:1212
    // Get the port number
    iPort = atoi(ptr2+1);
    pos2 = ptr2-url;
  }
	char szHostName[128];
  strncpy(szHostName, ptr1, pos2-pos1);
  szHostName[pos2-pos1] = '\0';
	strHostName=szHostName;

  // Look if a path is given
  ptr2 = strstr(ptr1, "/");
  if( ptr2!=NULL )
  {
    // A path/filename is given
    // check if it's not a trailing '/'
    if( strlen(ptr2)>1 )
    {
        // copy the path/filename in the URL container
        strFile=ptr2;
    }
  }
  // Check if a filenme was given or set, else set it with '/'
  if( !strFile.size() )
  {
    strFile="/";
  }
	return true;
}

//*********************************************************************************************

#define TIMEOUT (30*1000)

bool CHTTP::Send(char* pBuffer, int iLen)
{
	int iPos=0;
	int iOrgLen=iLen;
	WSABUF buf;
	WSAOVERLAPPED ovl;
	DWORD n, flags;

	while (iLen>0)
	{
		buf.buf = &pBuffer[iPos];
		buf.len = iLen;
		ovl.hEvent = hEvent;
		WSAResetEvent(hEvent);
		int ret = WSASend(m_socket, &buf, 1, &n, 0, &ovl, NULL);
		if (ret == SOCKET_ERROR)
		{
			if (WSAGetLastError() == WSA_IO_PENDING)
			{
				if (WSAWaitForMultipleEvents(1, &hEvent, FALSE, TIMEOUT, FALSE) == WSA_WAIT_EVENT_0)
				{
					if (!WSAGetOverlappedResult(m_socket, &ovl, &n, FALSE, &flags))
						return false;
				}
				else
				{
					WSACancelOverlappedIO(m_socket);
					WSASetLastError(WSAETIMEDOUT);
					return false;
				}
			}
		}
		iPos+=n;
		iLen-=n;
	}
	return true;
}

//*********************************************************************************************
bool CHTTP::Recv(char* pBuffer, int iLen, int& iRead, bool bFill)
{
	iRead = 0;
	int iOrgLen=iLen;
	WSABUF buf;
	WSAOVERLAPPED ovl;
	DWORD n, flags;

	while (iLen > 0)
	{
		buf.buf = &pBuffer[iRead];
		buf.len = iLen;
		flags = 0;
		ovl.hEvent = hEvent;
		WSAResetEvent(hEvent);
		int ret = WSARecv(m_socket, &buf, 1, &n, &flags, &ovl, NULL);
		if (ret == SOCKET_ERROR)
		{
			if (WSAGetLastError() == WSA_IO_PENDING)
			{
				if (WSAWaitForMultipleEvents(1, &hEvent, FALSE, TIMEOUT, FALSE) == WSA_WAIT_EVENT_0)
				{
					if (!WSAGetOverlappedResult(m_socket, &ovl, &n, FALSE, &flags))
						return false;
				}
				else
				{
					WSACancelOverlappedIO(m_socket);
					WSASetLastError(WSAETIMEDOUT);
					return false;
				}
			}
		}
		if (!n)
		{
			return false; // graceful close
		}
		iRead+=n;
		iLen-=n;

		if (!bFill)
			break;
	}
	return true;
}

//------------------------------------------------------------------------------------------------------------------
bool CHTTP::Download(const string &strURL, const string &strFileName)
{
	CLog::Log("Download: %s->%s",strURL.c_str(),strFileName.c_str());

	string strHeaders, strData;
	int status = Open(strURL, "GET", strHeaders, strData);

	if (status != 200)
	{
		Close();
		return false;
	}

	bool bGotLength = false;
	int Length = FindLength(strHeaders);
	if (Length < 0)
		Length = 65536;
	else
		bGotLength = true;

	if (Length > 0)
	{
		char* buf = new char[Length];
		memcpy(buf, strData.c_str(), strData.size());
		int iRead;
		if (!Recv(buf + strData.size(), Length - strData.size(), iRead, true))
		{
			if (bGotLength)
			{
				delete [] buf;
				Close();
				return false;
			}
		}

		HANDLE hFile = CreateFile(strFileName.c_str(), GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
		if (hFile == INVALID_HANDLE_VALUE)
		{
			CLog::Log("Unable to open file %s: %d", strFileName.c_str(), GetLastError());
			delete [] buf;
			Close();
			return false;
		}
		SetFilePointer(hFile, Length, 0, FILE_BEGIN);
		SetEndOfFile(hFile);
		SetFilePointer(hFile, 0, 0, FILE_BEGIN);
		DWORD n;
		WriteFile(hFile, buf, Length, &n, 0);
		CloseHandle(hFile);
		delete [] buf;
	}
	else
		DeleteFile(strFileName.c_str());

	Close();
	return true;
}


//------------------------------------------------------------------------------------------------------------------
void CHTTP::SetHTTPVer(unsigned int iVer)
{
	if(iVer == 0)
		m_iHTTPver = 0;
	else
		m_iHTTPver = 1;
}
//------------------------------------------------------------------------------------------------------------------
void CHTTP::SetCookie(const string &strCookie)
{
	m_strCookie=strCookie;
}


//------------------------------------------------------------------------------------------------------------------
bool CHTTP::Post(const string &strURL, const string &strPostData, string &strHTML)
{
  CLog::Log("Post URL:%s", strURL.c_str());

	string strHeaders;
	strHTML = strPostData;
	int status = Open(strURL, "POST", strHeaders, strHTML);

	if (status != 200)
	{
		Close();
		return false;
	}

	bool bGotLength = false;
	int Length = FindLength(strHeaders);
	if (Length < 0)
		Length = 65536;
	else
		bGotLength = true;
	Length -= strHTML.size();

	if (Length > 0)
	{
		char* buf = new char[Length];
		int iRead;
		if (!Recv(buf, Length, iRead, true))
		{
			if (bGotLength)
			{
				delete [] buf;
				Close();
				strHTML.clear();
				return false;
			}
		}
		buf[iRead]=0;
		strHTML.append(buf, buf+iRead);
		delete [] buf;
	}

	Close();
	return true;
}
//------------------------------------------------------------------------------------------------------------------

int CHTTP::Open(const string& strURL, const char* verb, string& strHeaders, string& strData)
{
	string strFile="";
	m_strHostName="";
	if (!BreakURL(strURL, m_strHostName, m_iPort, strFile))
	{
    CLog::Log("Invalid url: %s\n",strURL.c_str());
		return 0;
	}	

	if (!Connect())
	{
		CLog::Log("Unable to connect to %s: %d\n",m_strHostName.c_str(), WSAGetLastError());
		return 0;
	}

	// send request...
	char* szHTTPHEADER = (char*)_alloca(300 + m_strHostName.size() + m_strCookie.size() + strData.size());
  strcpy(szHTTPHEADER,"Accept: image/gif, image/x-xbitmap, image/jpeg, image/pjpeg, application/msword, */*\r\n"
											"Accept-Language: en-us\r\n"
											"Host:");
	strcat(szHTTPHEADER,m_strHostName.c_str());
	strcat(szHTTPHEADER,"\r\n"
											"User-Agent: Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1)\r\n");
	if (m_strCookie.size())
	{
		strcat(szHTTPHEADER,"Cookie: ");
		strcat(szHTTPHEADER,m_strCookie.c_str());
		strcat(szHTTPHEADER, "\r\n");
	}
	if (!strcmp(verb, "POST"))
	{
		strcat(szHTTPHEADER, "\r\n");
		strcat(szHTTPHEADER,strData.c_str());
	}

	char* szGet;
	if (m_strProxyServer.size())
	{
		szGet = (char*)_alloca(strlen(szHTTPHEADER) + strURL.size() + 20);
		sprintf(szGet,"%s %s HTTP/1.1\r\n%s\r\n",verb, strURL.c_str(),szHTTPHEADER);
	}
	else
	{
		szGet = (char*)_alloca(strlen(szHTTPHEADER) + strFile.size() + 20);
		sprintf(szGet,"%s %s HTTP/1.1\r\n%s\r\n",verb, strFile.c_str(),szHTTPHEADER);
	}

	if (!Send(szGet,strlen(szGet)))
	{
		CLog::Log("Send failed: %d", WSAGetLastError());
		Close();
		return 0;
	}

	char* pszBuffer = (char*)_alloca(5000), *HeaderEnd;
	long lReadTotal=0;
	do
	{
		if (lReadTotal >= 5000)
		{
			CLog::Log("Invalid reply from server");
			Close();
			return 0;
		}
		int lenRead;
		if (!Recv(&pszBuffer[lReadTotal],5000-lReadTotal,lenRead, false))
		{
			CLog::Log("Recv failed: %d", WSAGetLastError());
			Close();
			return 0;
		}
		lReadTotal+=lenRead;
	} while ((HeaderEnd = strstr(pszBuffer,"\r\n\r\n"))==NULL);
	HeaderEnd += 4;

	// expected return:
	// HTTP-Version SP Status-Code SP Reason-Phrase CRLF
	if (strnicmp(pszBuffer, "HTTP/", 5) || pszBuffer[5] != '1' || pszBuffer[6] != '.' || pszBuffer[7] < '0' || pszBuffer[7] > '1')
	{
		CLog::Log("Invalid reply from server");
		Close();
		return 0; // malformed reply
	}
	int status = atoi(pszBuffer + 9);
	char* end = strchr(pszBuffer + 13, '\r');
	string strReason(pszBuffer + 13, end);
	strHeaders.assign(end+2, HeaderEnd);

	if (status < 100)
	{
		CLog::Log("Invalid reply from server");
		Close();
		return 0; // malformed reply
	}
	else if (status < 300)
	{
		strData.assign(HeaderEnd, pszBuffer + lReadTotal);
		return status; // successful
	}
	else if (status < 400)
	{
		// redirect
		bool CanHandle = false;
		switch (status)
		{
		case 302:
			// 302 Found - auto redirect if this is a GET
			CanHandle = !stricmp(verb, "GET");
			break;
		case 303:
			// 303 See Other - perform GET on the new resource
			verb = "GET";
			CanHandle = true;
			break;
		case 307:
			// Temporary Redirect - auto redirect if NOT a GET
			CanHandle = !!stricmp(verb, "GET");
			break;
		}

		Close();

		if (!CanHandle)
		{
			CLog::Log("Server returned: %d %s", status, strReason.c_str());
			return status; // unhandlable
		}

		const char* pNewLocation = strstr(pszBuffer, "Location:");
		if (pNewLocation)
		{
			pNewLocation += 10;
			string strURL(pNewLocation, strchr(pNewLocation,'\r'));
			if (strnicmp(pNewLocation,"http:", 5))
			{
				char portstr[8];
				sprintf(portstr, ":%d", m_iPort);
				strURL.insert(0, portstr);
				strURL.insert(0, m_strHostName.c_str());
				strURL.insert(0, "http://");
			}
			return Open(strURL, verb, strHeaders, strData);
		}
		else
		{
			CLog::Log("Invalid reply from server");
			Close();
			return 0; // malformed reply
		}
	}
	else
	{
		CLog::Log("Server returned: %d %s", status, strReason.c_str());
		Close();
		return status; // error
	}
}
//------------------------------------------------------------------------------------------------------------------

void CHTTP::Close()
{
	m_socket.reset();
	if (hEvent != WSA_INVALID_EVENT)
		WSACloseEvent(hEvent);
	hEvent = WSA_INVALID_EVENT;
}