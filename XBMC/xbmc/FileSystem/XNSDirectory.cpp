#include "xnsdirectory.h"
#include "tinyxml/tinyxml.h"
#include "../DNSNameCache.h"
#include "../url.h"
#include "../util.h"

CXNSDirectory::CXNSDirectory(void)
{
}

CXNSDirectory::~CXNSDirectory(void)
{
}



bool  CXNSDirectory::GetDirectory(const CStdString& strPath,VECFILEITEMS &items)
{
	CURL url(strPath);
	int iport					 = 1400;
	if (url.HasPort()) iport=url.GetPort();

	CStdString strHostName;
	if ( !CDNSNameCache::Lookup(url.GetHostName(), strHostName) )
	{
		return false;
	}

	CStdString strFile=url.GetFileName();

	SOCKET s = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	sockaddr_in service;

	unsigned long ulHostIp = inet_addr( strHostName.c_str() );

	service.sin_family		  = AF_INET;
	service.sin_port				= htons(iport);
	service.sin_addr.s_addr = ulHostIp;
		
	// attempt to connection
	int err = connect(s,(sockaddr*) &service,sizeof(struct sockaddr));
	if (err == SOCKET_ERROR) 
	{
		closesocket(s);
		return false;
	}

	// validate greeting
	char buff[1024];
	memset(buff,0,256);
	send(s,"HELLO XSTREAM 6.0\r\n",19,0);
	recv(s,buff,11,0);
	if (strcmp(buff,"HELLO XBOX!")!=0)
	{
		closesocket(s);
		return false;
	}

	// format the
	char szResult[32];

	if (strFile.size() > 0)
	{
		sprintf(buff,"*CAT,%s",strFile.c_str() );
	}
	else
	{
		strcpy(buff,"*CAT");
	}

	// evalute the response, positive is the amount of bytes to get, negative is error.
	send(s,buff,strlen(buff),0);
	recv(s,szResult,32,0);
	int catalogueSize = atoi(szResult);
	if (catalogueSize<=0)
	{
		closesocket(s);
		return false;
	}
	
	// allocate sufficient memory for the document
	char* lpszXml = new char[catalogueSize+1];
	
	// start pulling it down from the server
	int bytesRead=0;
	while(bytesRead<catalogueSize)
	{
		bytesRead+=recv(s,(lpszXml+bytesRead),(catalogueSize-bytesRead),0);
	}

	// terminate the xml document (CStdString) and close the socket
	lpszXml[catalogueSize]=0x00;
	closesocket(s);

	// Create a tag which identifies the remote server hosting this file
	// we will prefix this tag to each item we enumerate
	char szPrefix[1024];
	sprintf(szPrefix,"$%s:%i:",strHostName.c_str(),iport);
	int nPathOffset = strlen(szPrefix);
 
	
	// Start at the very beginning a very good place to start!
	TiXmlDocument xmlDoc;
	
  if ( !xmlDoc.Parse( lpszXml ) ) 
	{
		delete [] lpszXml;
		return false;
	}
	TiXmlElement* pRootElement =xmlDoc.RootElement();
	if (!pRootElement)
	{
		delete [] lpszXml;
		return false;
	}
	
	const TiXmlNode *pChild = pRootElement->FirstChild();
	while (pChild>0)
	{
		CStdString strTagName=pChild->Value();

		if ( !CUtil::cmpnocase(strTagName.c_str(),"ITEM") )		
		{
			const TiXmlNode *pathNode=pChild->FirstChild("PATH");
			const TiXmlNode *atrbNode=pChild->FirstChild("ATTRIB");

			int attrib = 0;
			if (atrbNode)
			{
				// is the item a file
				attrib = atoi( atrbNode->FirstChild()->Value() );
			}

			const char* szPath = NULL;
			if (pathNode)
			{
				szPath = pathNode->FirstChild()->Value() ;
			}
			else
			{
				// no path? how can this be?
				delete [] lpszXml;
				return false;
			}

      CStdString strLabel=szPath;
      char *szLabel=strrchr(szPath, '\\');
      if (!szLabel) szLabel=strrchr(szPath, '/');
      if (szLabel) strLabel = (++szLabel);

      strcpy(&szPrefix[nPathOffset],szPath);
      bool bIsFolder(false);
      if (attrib & FILE_ATTRIBUTE_DIRECTORY)
				bIsFolder=true;

			if ( bIsFolder || IsAllowed( szPath) )
			{
				CFileItem*  pItem = new CFileItem(strLabel);
				char szBuf[1024];
				sprintf(szBuf,"xns://%s:%i/%s", url.GetHostName().c_str(), iport, szPath);
				pItem->m_strPath=szBuf;
				pItem->m_bIsFolder = bIsFolder;
				if (attrib & FILE_ATTRIBUTE_DIRECTORY)
					pItem->m_bIsFolder=true;

				// file creation time is not supported by relax
				memset(&pItem->m_stTime,0,sizeof(pItem->m_stTime));
				pItem->m_dwSize=0;
				items.push_back(pItem);
			}
		}

		pChild=pChild->NextSibling();
	}

	if (lpszXml) delete [] lpszXml;
	return true;
}
