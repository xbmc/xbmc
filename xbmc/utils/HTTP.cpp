// HTTP.cpp: implementation of the CHTTP class.
//
//////////////////////////////////////////////////////////////////////

#include "HTTP.h"
#include "stdstring.h"
#ifdef _XBOX
#include "../dnsnamecache.h"
#endif

#include "../settings.h"

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
}


CHTTP::CHTTP()
:m_socket(INVALID_SOCKET)
{
	m_strProxyServer=g_stSettings.m_szHTTPProxy;
	m_iProxyPort=g_stSettings.m_iHTTPProxyPort;
	m_strCookie="";
}


CHTTP::~CHTTP()
{

}

//------------------------------------------------------------------------------------------------------------------

bool CHTTP::Get(string& strURL, string& strHTML)
{
	string strFile="";
	m_strHostName="";
	if (!BreakURL(strURL, m_strHostName, m_iPort, strFile))
	{
//		printf("invalid url\n");
		OutputDebugString("Invalid url.\n");
		return false;
	}	

	if ( !Connect() ) 
	{
		//printf("unable to connect\n");
		OutputDebugString("Unable to connect.\n");
		return false;
	}
	// send request...
	char szGet[1024];
	char szHTTPHEADER[1024];
  strcpy(szHTTPHEADER,"Accept: image/gif, image/x-xbitmap, image/jpeg, image/pjpeg, application/msword, */*\r\n");
	strcat(szHTTPHEADER, "Accept-Language: en-us\r\n");
	strcat(szHTTPHEADER, "Host:");
	strcat(szHTTPHEADER,m_strHostName.c_str());
	strcat(szHTTPHEADER, "\r\n");
	strcat(szHTTPHEADER, "User-Agent: Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1)\r\n");
	if (m_strCookie.size() )
	{
		strcat(szHTTPHEADER,"Cookie: ");
		strcat(szHTTPHEADER,m_strCookie.c_str());
		strcat(szHTTPHEADER, "\r\n");
	}
	
	
	if (m_strProxyServer.size())
	{
		sprintf(szGet,"GET %s HTTP/1.1\r%s\r\n",strURL.c_str(),szHTTPHEADER);
	}
	else
	{
		sprintf(szGet,"GET %s HTTP/1.1\r\n%s\r\n",strFile.c_str(),szHTTPHEADER);
	}
	//printf("send %s", szGet);
	if ( !Send((unsigned char*)szGet,strlen(szGet) ) )
	{
		//printf("send failed\n");
		
		char strError[128];
		sprintf(strError,"send failed error:%i %i\n", GetLastError(), WSAGetLastError());
    OutputDebugString(strError);

		m_socket.reset();
		return false;
	}

	char* pszBuffer = new char[5000];
	long lReadTotal=0;
	do
	{
		long lenRead=recv((SOCKET)m_socket,(char*)&pszBuffer[lReadTotal],5000-lReadTotal,0);
		if (lenRead <=0)
		{
			//printf("receive failed\n");
			char strError[128];			
			sprintf(strError,"recv error:%i %i", GetLastError(), WSAGetLastError());
      OutputDebugString(strError);
			m_socket.reset();
		}
		lReadTotal+=lenRead;
	} while (strstr(pszBuffer,"\r\n\r\n")==NULL);

	/*
	lReadTotal=0;
	memset(szBuffer,0,sizeof(szBuffer));
	FILE *fdt=fopen("E:\\log.txt","r");
	lReadTotal=fread(szBuffer,1,sizeof(szBuffer),fdt);
	fclose(fdt);
*/
	char* pResult=strstr(pszBuffer,"HTTP/1.0 200");
	if (!pResult)
		pResult=strstr(pszBuffer,"HTTP/1.1 200");
	if (!pResult)
		pResult=strstr(pszBuffer,"HTTP/1.1 100");

	if (!pResult)
	{
		// were we redirected?
		pResult=strstr(pszBuffer,"HTTP/1.0 302");
		if (!pResult)
			pResult=strstr(pszBuffer,"HTTP/1.1 302");

		if (pResult)
		{
      OutputDebugString("Redirected\n");
			m_socket.reset();
			
			char* pNewLocation=strstr(pszBuffer,"Location: ");
			if (pNewLocation)
			{
				pNewLocation += strlen("Location: ");
				char* pEndLocation=strstr(pNewLocation,"\r\n");
				*pEndLocation=0;

				strURL = pNewLocation;
				delete [] pszBuffer;
        
        if (strstr(strURL.c_str(),"http:") ==NULL && strstr(strURL.c_str(),"HTTP:") ==NULL)
        {
          char szNewLocation[1024];
          sprintf(szNewLocation,"http://%s:%i%s", m_strHostName.c_str(),m_iPort,strURL.c_str());
          strURL=szNewLocation;
        }
        OutputDebugString("to:");
        OutputDebugString(strURL.c_str());
        OutputDebugString("\n");
				return Get(strURL, strHTML);
			}

			
			char strError[128];
			sprintf(strError,"redirect error:%i %i\n", GetLastError(), WSAGetLastError());
			OutputDebugString(strError);
      delete [] pszBuffer;
			return false;
		}

		OutputDebugString("Website returned an error code: ");
		OutputDebugString(pszBuffer);
		OutputDebugString("\n");

		//printf("website didn't return 200\n");
		pszBuffer[lReadTotal]=0;
		//printf("%s",szBuffer);
		m_socket.reset();
		
		pszBuffer[40]=0;
		char strError[128];
		sprintf(strError,"error:%i %i size:%i", GetLastError(), WSAGetLastError(),lReadTotal);
		//g_dialog.SetCaption(0,"GET didnt return 200");
		//g_dialog.SetMessage(0,szBuffer);
		//g_dialog.SetMessage(1,strError);
		//g_dialog.DoModal();
		delete [] pszBuffer;
		return false;
	}

	DWORD dwLength;
	bool bGotLength(false);
	char* pLength=strstr(pszBuffer,"Content-Length: ");
	if (!pLength)
	{
		//printf("website didn't return content-length\n");
		dwLength=65535;
	}
	else
	{
		pLength += strlen("Content-Length: ");
		char szLength[10];
		int ipos=0;
		while (isdigit(*pLength) )
		{
			szLength[ipos++]=*pLength;
			szLength[ipos]=0;
			pLength++;
		}

		dwLength=atol(szLength);
		bGotLength=true;
	}
	if (dwLength > 0)
	{
		char* pBody=strstr(pszBuffer,"\r\n\r\n");
		if (!pBody)
		{
			//printf("website returned empty document\n");
			OutputDebugString("Website returned empty document\n");
			m_socket.reset();
			
			char strError[128];
			sprintf(strError,"error:%i %i", GetLastError(), WSAGetLastError());
			//g_dialog.SetCaption(0,"GET returned empty document");
			//g_dialog.SetMessage(0,m_strHostName.c_str());
			//g_dialog.SetMessage(1,strError);
			//g_dialog.DoModal();
			delete [] pszBuffer;
			return false;
		}
		pBody+=4;
		
		unsigned char *pBuffer= new unsigned char[dwLength+2];
		if (!pBuffer)
		{
			//printf("failed to allocate space\n");
			m_socket.reset();
				
			char strError[128];
			sprintf(strError,"error:%i %i", GetLastError(), WSAGetLastError());
			//g_dialog.SetCaption(0,"out of memory");
			//g_dialog.SetMessage(0,m_strHostName.c_str());
			//g_dialog.SetMessage(1,strError);
			//g_dialog.DoModal();
			delete [] pszBuffer;
			return false;
		}
		int iPos   =lReadTotal - (pBody-&pszBuffer[0]);
		int iBytesLeftToRead = dwLength - iPos;
		
		memcpy(pBuffer, pBody, iPos);
		if (iBytesLeftToRead >0)
		{
			int iRead;
			if (!Recv(&pBuffer[iPos], iBytesLeftToRead, iRead))
			{
				if (bGotLength)
				{
					OutputDebugString("Failed to read body\n");
					//printf("failed to read body\n");
					delete [] pBuffer;
					m_socket.reset();
					
					char strError[128];
					sprintf(strError,"error:%i %i", GetLastError(), WSAGetLastError());
					//g_dialog.SetCaption(0,"recv() failed");
					//g_dialog.SetMessage(0,m_strHostName.c_str());
					//g_dialog.SetMessage(1,strError);
					//g_dialog.DoModal();
					delete [] pszBuffer;
					return false;
				}
				else
				{
					dwLength=iPos+iRead;
				}
			}
		}
		pBuffer[dwLength]=0;
		
		strHTML = (char*)pBuffer;
		delete [] pBuffer;
		delete [] pszBuffer;
	}
	else strHTML="";
	
	//printf("%s\n", strHTML.c_str());
	m_socket.reset();
	return true;
}

//------------------------------------------------------------------------------------------------------------------
bool CHTTP::Connect()
{
	sockaddr_in service;
	service.sin_family = AF_INET;


	OutputDebugString("Connect:");
	OutputDebugString(m_strHostName.c_str());
	OutputDebugString("\n");

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
			CStdString strIpAdres="";
			CDNSNameCache::Lookup(m_strHostName,strIpAdres);
			service.sin_addr.s_addr = inet_addr(strIpAdres.c_str());
			if (strIpAdres=="")
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
	m_socket.attach( socket(AF_INET,SOCK_STREAM,IPPROTO_TCP));
	
	// attempt to connection
	if (connect((SOCKET)m_socket,(sockaddr*) &service,sizeof(struct sockaddr)) == SOCKET_ERROR)
	{
		char strError[128];
		sprintf(strError,"error:%i %i", GetLastError(), WSAGetLastError());
		//g_dialog.SetCaption(0,"Unable to connect");
		//g_dialog.SetMessage(0,m_strHostName.c_str());
		//g_dialog.SetMessage(1,strError);
		//g_dialog.DoModal();
		m_socket.reset();
		return false;
	}
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
bool CHTTP::Send(unsigned char* pBuffer, int iLen)
{
	int iPos=0;
	int iOrgLen=iLen;
	while (iLen>0)
	{
		int iErr=send((SOCKET)m_socket,(const char*)&pBuffer[iPos],iLen,0);
		if (iErr>0)
		{
			iPos+=iLen;
			iLen-=iPos;
		}
		else
		{
			iErr=WSAGetLastError();
			if (iErr != WSAEINPROGRESS) 
			{
				return false;
			}
		}
	}
	return true;
}

//*********************************************************************************************
bool CHTTP::Recv(unsigned char* pBuffer, int iLen, int& iRead)
{
	int iLenOrg=iLen;
	long bytesRead = 0;
	while (iLen>0)
	{
		long lenRead=recv((SOCKET)m_socket,(char*)&pBuffer[bytesRead],iLen,0);
		if (lenRead > 0)
		{
			bytesRead+=lenRead;
			iLen-=lenRead;
		}
		else
		{
			iRead=bytesRead;
			return false;
		}
	}
	return true;
	
}

//------------------------------------------------------------------------------------------------------------------
bool CHTTP::Download(const string &strURL, const string &strFileName)
{
	string strFile="";
	m_strHostName="";
	DeleteFile(strFileName.c_str());	
	OutputDebugString("Download:");
	OutputDebugString(strURL.c_str());
	
	OutputDebugString("\nto:");
	OutputDebugString(strFileName.c_str());
	OutputDebugString("\n:");
	if (!BreakURL(strURL, m_strHostName, m_iPort, strFile))
	{
		OutputDebugString("invalid url\n");
		return false;
	}	

	if ( !Connect() ) 
	{
		OutputDebugString("unable to connect\n");
		return false;
	}
	// send request...
	char szGet[1024];
	char szHTTPHEADER[1024];
  strcpy(szHTTPHEADER,"Accept: image/gif, image/x-xbitmap, image/jpeg, image/pjpeg, application/msword, */*\r\n");
	strcat(szHTTPHEADER, "Accept-Language: en-us\r\n");
	strcat(szHTTPHEADER, "Host:");
	strcat(szHTTPHEADER,m_strHostName.c_str());
	strcat(szHTTPHEADER, "\r\n");
	strcat(szHTTPHEADER, "User-Agent: Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1)\r\n");
	
	
	if (m_strProxyServer.size())
	{
		sprintf(szGet,"GET %s HTTP/1.%i\r%s\r\n",strURL.c_str(),m_iHTTPver,szHTTPHEADER);
	}
	else
	{
		sprintf(szGet,"GET %s HTTP/1.%i\r\n%s\r\n",strFile.c_str(),m_iHTTPver,szHTTPHEADER);
	}
	//printf("send %s", szGet);
	if ( !Send((unsigned char*)szGet,strlen(szGet) ) )
	{
		char strError[128];
		sprintf(strError,"error:%i %i", GetLastError(), WSAGetLastError());
		//g_dialog.SetCaption(0,"Send failed");
		//g_dialog.SetMessage(0,m_strHostName.c_str());
		//g_dialog.SetMessage(1,strError);
		//g_dialog.DoModal();

		m_socket.reset();
		return false;
	}

	char* pszBuffer = new char[5000];
	long lReadTotal=0;
	do
	{
		long lenRead=recv((SOCKET)m_socket,(char*)&pszBuffer[lReadTotal],5000-lReadTotal,0);
		if (lenRead <=0)
		{
			char strError[128];
			sprintf(strError,"error:%i %i", GetLastError(), WSAGetLastError());
			//g_dialog.SetCaption(0,"receive failed");
			//g_dialog.SetMessage(0,m_strHostName.c_str());
			//g_dialog.SetMessage(1,strError);
			//g_dialog.DoModal();

			m_socket.reset();
		}
		lReadTotal+=lenRead;
	} while (strstr(pszBuffer,"\r\n\r\n")==NULL);

	char* pResult=strstr(pszBuffer,"HTTP/1.0 200");
	if (!pResult) pResult=strstr(pszBuffer,"HTTP/1.1 200");
	if (!pResult) pResult=strstr(pszBuffer,"HTTP/1.1 100");
	if (!pResult)
	{
		pResult=strstr(pszBuffer,"HTTP/1.0 302");
		if (pResult)
		{
			m_socket.reset();
			
			char* pNewLocation=strstr(pszBuffer,"Location: ");
			if (pNewLocation)
			{
				pNewLocation += strlen("Location: ");
				char* pEndLocation=strstr(pNewLocation,"\r\n");
				*pEndLocation=0;
        string strURL=pNewLocation;
        if (strstr(pNewLocation,"http:") ==NULL && strstr(pNewLocation,"HTTP:") ==NULL)
        {
          char szNewLocation[1024];
          sprintf(szNewLocation,"http://%s:%i%s", m_strHostName.c_str(),m_iPort,pNewLocation);
          strURL=szNewLocation;
        }
				bool bResult=Download(strURL.c_str(), strFileName);
				delete[] pszBuffer;
				return bResult;
			}
			char strError[128];
			sprintf(strError,"error:%i %i", GetLastError(), WSAGetLastError());
			//g_dialog.SetCaption(0,"relocation failed");
			//g_dialog.SetMessage(0,m_strHostName.c_str());
			//g_dialog.SetMessage(1,strError);
			//g_dialog.DoModal();
			delete[] pszBuffer;
			return false;
		}
		OutputDebugString("website didn't return 200\n");
		pszBuffer[lReadTotal]=0;
		//printf("%s",pszBuffer);
		m_socket.reset();
		char strError[128];
		sprintf(strError,"error:%i %i", GetLastError(), WSAGetLastError());
		//g_dialog.SetCaption(0,"GET didn't return 200");
		//g_dialog.SetMessage(0,m_strHostName.c_str());
		//g_dialog.SetMessage(1,strError);
		//g_dialog.DoModal();
		delete[] pszBuffer;
		return false;
	}
	DWORD dwLength;
	bool bGotLength(false);
	char* pLength=strstr(pszBuffer,"Content-Length: ");
	if (!pLength)
	{
		//printf("website didn't return content-length\n");
		dwLength=65535;
	}
	else
	{
		pLength += strlen("Content-Length: ");
		char szLength[10];
		int ipos=0;
		while (isdigit(*pLength) )
		{
			szLength[ipos++]=*pLength;
			szLength[ipos]=0;
			pLength++;
		}

		dwLength=atol(szLength);
		bGotLength=true;
	}
	if (dwLength > 0)
	{
		char* pBody=strstr(pszBuffer,"\r\n\r\n");
		if (!pBody)
		{
			char strError[128];
			sprintf(strError,"error:%i %i", GetLastError(), WSAGetLastError());
			//g_dialog.SetCaption(0,"empty document");
			//g_dialog.SetMessage(0,m_strHostName.c_str());
			//g_dialog.SetMessage(1,strError);
			//g_dialog.DoModal();

			m_socket.reset();
			delete[] pszBuffer;
			return false;
		}
		pBody+=4;
		
		unsigned char *pBuffer= new unsigned char[dwLength+2];
		if (!pBuffer)
		{
			OutputDebugString("failed to allocate space\n");
			m_socket.reset();
			char strError[128];
			sprintf(strError,"error:%i %i", GetLastError(), WSAGetLastError());
			//g_dialog.SetCaption(0,"Out of memory");
			//g_dialog.SetMessage(0,m_strHostName.c_str());
			//g_dialog.SetMessage(1,strError);
			//g_dialog.DoModal();
			delete[] pszBuffer;
			return false;
		}
		int iPos   =lReadTotal - (pBody-&pszBuffer[0]);
		int iBytesLeftToRead = dwLength - iPos;
		
		memcpy(pBuffer, pBody, iPos);
		if (iBytesLeftToRead >0)
		{
			int iRead;
			if (!Recv(&pBuffer[iPos], iBytesLeftToRead, iRead))
			{
				if (bGotLength)
				{
					char strError[128];
					sprintf(strError,"error:%i %i", GetLastError(), WSAGetLastError());
					//g_dialog.SetCaption(0,"Recv failed");
					//g_dialog.SetMessage(0,m_strHostName.c_str());
					//g_dialog.SetMessage(1,strError);
					//g_dialog.DoModal();

					delete [] pBuffer;
					m_socket.reset();
					delete[] pszBuffer;
					return false;
				}
				else
				{
					dwLength=iPos+iRead;
				}
			}
		}
		pBuffer[dwLength]=0;
		
		FILE* fd = fopen(strFileName.c_str(),"wb");
		if (fd==NULL)
		{
			char strError[128];
			sprintf(strError,"error:%i %i", GetLastError(), WSAGetLastError());
			//g_dialog.SetCaption(0,"failed to create file");
			//g_dialog.SetMessage(0,m_strHostName.c_str());
			//g_dialog.SetMessage(1,strError);
			//g_dialog.DoModal();

			delete [] pBuffer;
			delete[] pszBuffer;
			return false;
		}
		fwrite(pBuffer,1, dwLength, fd);
		fclose(fd);

		delete [] pBuffer;
		delete[] pszBuffer;
	}
	//printf("%s\n", strHTML.c_str());
	m_socket.reset();
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
	string strFile="";
	m_strHostName="";
	if (!BreakURL(strURL, m_strHostName, m_iPort, strFile))
	{
//		printf("invalid url\n");
		OutputDebugString("Invalid url.\n");
		return false;
	}	

	if ( !Connect() ) 
	{
		//printf("unable to connect\n");
		OutputDebugString("Unable to connect.\n");
		return false;
	}
	// send request...
	char szGet[1024];
	char szHTTPHEADER[1024];
	char szLength[128];
  strcpy(szHTTPHEADER,"Accept: image/gif, image/x-xbitmap, image/jpeg, image/pjpeg, application/msword, */*\r\n");
	strcat(szHTTPHEADER, "Accept-Language: en-us\r\n");
	strcat(szHTTPHEADER, "Host:");
	strcat(szHTTPHEADER,m_strHostName.c_str());
	strcat(szHTTPHEADER, "\r\n");
	strcat(szHTTPHEADER, "User-Agent: Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1)\r\n");
	
	sprintf(szLength, "Content-Length: %i\r\n",strPostData.size());
	strcat(szHTTPHEADER,szLength);
	if (m_strCookie.size() )
	{
		strcat(szHTTPHEADER,"Cookie: ");
		strcat(szHTTPHEADER,m_strCookie.c_str());
		strcat(szHTTPHEADER, "\r\n");
	}
	

	strcat(szHTTPHEADER, "\r\n");
	strcat(szHTTPHEADER,strPostData.c_str());
	
	if (m_strProxyServer.size())
	{
		sprintf(szGet,"POST %s HTTP/1.1\r%s",strURL.c_str(),szHTTPHEADER);
	}
	else
	{
		sprintf(szGet,"POST %s HTTP/1.1\r\n%s",strFile.c_str(),szHTTPHEADER);
	}
	//printf("send %s", szGet);
	if ( !Send((unsigned char*)szGet,strlen(szGet) ) )
	{
		//printf("send failed\n");
		
		char strError[128];
		sprintf(strError,"error:%i %i", GetLastError(), WSAGetLastError());
		//g_dialog.SetCaption(0,"Send failed");
		//g_dialog.SetMessage(0,m_strHostName.c_str());
		//g_dialog.SetMessage(1,strError);
		//g_dialog.DoModal();

		m_socket.reset();
		return false;
	}

	char* pszBuffer = new char[5000];
	memset(&pszBuffer[0],0,5000);
	long lReadTotal=0;
	do
	{
		long lenRead=recv((SOCKET)m_socket,(char*)&pszBuffer[lReadTotal],5000-lReadTotal,0);
		if (lenRead <=0)
		{
			//printf("receive failed\n");
			char strError[128];			
			sprintf(strError,"error:%i %i", GetLastError(), WSAGetLastError());
			//g_dialog.SetCaption(0,"Recv failed");
			//g_dialog.SetMessage(0,m_strHostName.c_str());
			//g_dialog.SetMessage(1,strError);
			//g_dialog.DoModal();
			m_socket.reset();
		}
		lReadTotal+=lenRead;
	} while (strstr(pszBuffer,"\r\n\r\n")==NULL && lReadTotal < 1024);

	/*
	lReadTotal=0;
	memset(szBuffer,0,sizeof(szBuffer));
	FILE *fdt=fopen("E:\\log.txt","r");
	lReadTotal=fread(szBuffer,1,sizeof(szBuffer),fdt);
	fclose(fdt);
*/
	
	
	char* pResult=strstr(pszBuffer,"HTTP/1.0 200");
	if (!pResult)
		pResult=strstr(pszBuffer,"HTTP/1.1 200");
	if (!pResult)
		pResult=strstr(pszBuffer,"HTTP/1.1 100");

	if (!pResult)
	{
		/*
		// were we redirected?
		pResult=strstr(pszBuffer,"HTTP/1.0 302");
		if (!pResult)
			pResult=strstr(pszBuffer,"HTTP/1.1 302");

		if (pResult)
		{
			m_socket.reset();
			
			char* pNewLocation=strstr(pszBuffer,"Location: ");
			if (pNewLocation)
			{
				pNewLocation += strlen("Location: ");
				char* pEndLocation=strstr(pNewLocation,"\r\n");
				*pEndLocation=0;

				strURL = pNewLocation;
				return Get(strURL, strHTML);
			}

			
			char strError[128];
			sprintf(strError,"error:%i %i", GetLastError(), WSAGetLastError());
			//g_dialog.SetCaption(0,"redirect failed");
			//g_dialog.SetMessage(0,m_strHostName.c_str());
			//g_dialog.SetMessage(1,strError);
			//g_dialog.DoModal();
			return false;
		}*/

		OutputDebugString("Website returned an error code: ");
		OutputDebugString(pszBuffer);
		OutputDebugString("\n");

		//printf("website didn't return 200\n");
		pszBuffer[lReadTotal]=0;
		//printf("%s",pszBuffer);
		m_socket.reset();
		
		pszBuffer[40]=0;
		char strError[128];
		sprintf(strError,"error:%i %i size:%i", GetLastError(), WSAGetLastError(),lReadTotal);
		//g_dialog.SetCaption(0,"GET didnt return 200");
		//g_dialog.SetMessage(0,pszBuffer);
		//g_dialog.SetMessage(1,strError);
		//g_dialog.DoModal();
		delete [] pszBuffer;
		return false;
	}

	DWORD dwLength;
	bool bGotLength(false);
	char* pLength=strstr(pszBuffer,"Content-Length: ");
	if (!pLength)
	{
		//printf("website didn't return content-length\n");
		dwLength=65535;
	}
	else
	{
		pLength += strlen("Content-Length: ");
		char szLength[10];
		int ipos=0;
		while (isdigit(*pLength) )
		{
			szLength[ipos++]=*pLength;
			szLength[ipos]=0;
			pLength++;
		}

		dwLength=atol(szLength);
		bGotLength=true;
	}
	if (dwLength > 0)
	{
		char* pBody=strstr(pszBuffer,"\r\n\r\n");
		if (!pBody)
		{
			//printf("website returned empty document\n");
			OutputDebugString("Website returned empty document\n");
			m_socket.reset();
			
			char strError[128];
			sprintf(strError,"error:%i %i", GetLastError(), WSAGetLastError());
			//g_dialog.SetCaption(0,"GET returned empty document");
			//g_dialog.SetMessage(0,m_strHostName.c_str());
			//g_dialog.SetMessage(1,strError);
			//g_dialog.DoModal();
			delete [] pszBuffer;
			return false;
		}
		pBody+=4;
		
		unsigned char *pBuffer= new unsigned char[dwLength+2];
		if (!pBuffer)
		{
			//printf("failed to allocate space\n");
			m_socket.reset();
				
			char strError[128];
			sprintf(strError,"error:%i %i", GetLastError(), WSAGetLastError());
			//g_dialog.SetCaption(0,"out of memory");
			//g_dialog.SetMessage(0,m_strHostName.c_str());
			//g_dialog.SetMessage(1,strError);
			//g_dialog.DoModal();
			delete [] pszBuffer;
			return false;
		}
		int iPos   =lReadTotal - (pBody-&pszBuffer[0]);
		int iBytesLeftToRead = dwLength - iPos;
		
		memcpy(pBuffer, pBody, iPos);
		if (iBytesLeftToRead >0)
		{
			int iRead;
			if (!Recv(&pBuffer[iPos], iBytesLeftToRead, iRead))
			{
				if (bGotLength)
				{
					OutputDebugString("Failed to read body\n");
					//printf("failed to read body\n");
					delete [] pBuffer;
					delete [] pszBuffer;
					m_socket.reset();
					
					char strError[128];
					sprintf(strError,"error:%i %i", GetLastError(), WSAGetLastError());
					//g_dialog.SetCaption(0,"recv() failed");
					//g_dialog.SetMessage(0,m_strHostName.c_str());
					//g_dialog.SetMessage(1,strError);
					//g_dialog.DoModal();
					return false;
				}
				else
				{
					dwLength=iPos+iRead;
				}
			}
		}
		pBuffer[dwLength]=0;
		
		strHTML = (char*)pBuffer;
		delete [] pBuffer;
		delete [] pszBuffer;
	}
	else strHTML="";
	
	//printf("%s\n", strHTML.c_str());
	m_socket.reset();
	return true;
}
//------------------------------------------------------------------------------------------------------------------

bool CHTTP::Open(const string& strURL)
{
	string strFile="";
	m_strHostName="";
	if (!BreakURL(strURL, m_strHostName, m_iPort, strFile))
	{
		OutputDebugString("Invalid url.\n");
		return false;
	}	

	if ( !Connect() ) 
	{
		//printf("unable to connect\n");
		OutputDebugString("Unable to connect.\n");
		return false;
	}
	// send request...
	char szGet[1024];
	char szHTTPHEADER[1024];
  strcpy(szHTTPHEADER,"Accept: image/gif, image/x-xbitmap, image/jpeg, image/pjpeg, application/msword, */*\r\n");
	strcat(szHTTPHEADER, "Accept-Language: en-us\r\n");
	strcat(szHTTPHEADER, "Host:");
	strcat(szHTTPHEADER,m_strHostName.c_str());
	strcat(szHTTPHEADER, "\r\n");
	strcat(szHTTPHEADER, "User-Agent: Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1)\r\n");
	if (m_strCookie.size() )
	{
		strcat(szHTTPHEADER,"Cookie: ");
		strcat(szHTTPHEADER,m_strCookie.c_str());
		strcat(szHTTPHEADER, "\r\n");
	}
	
	
	if (m_strProxyServer.size())
	{
		sprintf(szGet,"GET %s HTTP/1.1\r%s\r\n",strURL.c_str(),szHTTPHEADER);
	}
	else
	{
		sprintf(szGet,"GET %s HTTP/1.1\r\n%s\r\n",strFile.c_str(),szHTTPHEADER);
	}
	//printf("send %s", szGet);
	if ( !Send((unsigned char*)szGet,strlen(szGet) ) )
	{
		//printf("send failed\n");
		
		char strError[128];
		sprintf(strError,"error:%i %i", GetLastError(), WSAGetLastError());
		//g_dialog.SetCaption(0,"Send failed");
		//g_dialog.SetMessage(0,m_strHostName.c_str());
		//g_dialog.SetMessage(1,strError);
		//g_dialog.DoModal();

		m_socket.reset();
		return false;
	}

	char* pszBuffer = new char[5000];
	long lReadTotal=0;
	do
	{
		long lenRead=recv((SOCKET)m_socket,(char*)&pszBuffer[lReadTotal],5000-lReadTotal,0);
		if (lenRead <=0)
		{
			//printf("receive failed\n");
			char strError[128];			
			sprintf(strError,"error:%i %i", GetLastError(), WSAGetLastError());
			//g_dialog.SetCaption(0,"Recv failed");
			//g_dialog.SetMessage(0,m_strHostName.c_str());
			//g_dialog.SetMessage(1,strError);
			//g_dialog.DoModal();
			m_socket.reset();
		}
		lReadTotal+=lenRead;
	} while (strstr(pszBuffer,"\r\n\r\n")==NULL);
	char* pResult=strstr(pszBuffer,"HTTP/1.0 200");
	if (!pResult)
		pResult=strstr(pszBuffer,"HTTP/1.1 200");


	delete [] pszBuffer;
	if (!pResult)
	{
		return false;
	}
	return true;
}
//------------------------------------------------------------------------------------------------------------------

void CHTTP::Close()
{
	m_socket.reset();
}