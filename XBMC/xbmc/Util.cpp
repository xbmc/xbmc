#include "util.h"
#include "xbox/iosupport.h"
#include "crc32.h"
#include "settings.h"
#include "xbox/undocumented.h"
#include "lib/common/xbnet.h"
#include "url.h"

char g_szTitleIP[32];

CUtil::CUtil(void)
{
  memset(g_szTitleIP,0,sizeof(g_szTitleIP));
}

CUtil::~CUtil(void)
{
}

char* CUtil::GetExtension(const CStdString& strFileName) 
{
  char* extension = strrchr(strFileName.c_str(),'.');
  return extension ;
}

bool CUtil::IsXBE(const CStdString& strFileName)
{
   char* pExtension=GetExtension(strFileName);
   if (!pExtension) return false;
   if (CUtil::cmpnocase(pExtension,".xbe")==0) return true;
   return false;
}

bool CUtil::IsShortCut(const CStdString& strFileName)
{
   char* pExtension=GetExtension(strFileName);
   if (!pExtension) return false;
   if (CUtil::cmpnocase(pExtension,".cut")==0) return true;
   return false;
}

int CUtil::cmpnocase(const char* str1,const char* str2)
{
	int iLen;
	if ( strlen(str1) != strlen(str2) ) return 1;
	
	iLen=strlen(str1);
	for (int i=0; i < iLen;i++ )
	{
		if (tolower((unsigned char)str1[i]) != tolower((unsigned char)str2[i]) ) return 1;
	}
	return 0;
}


char* CUtil::GetFileName(const CStdString& strFileNameAndPath)
{

  char* extension = strrchr(strFileNameAndPath.c_str(),'\\');
  if (!extension)
  {
    extension = strrchr(strFileNameAndPath.c_str(),'/');
    if (!extension) return (char*)strFileNameAndPath.c_str();
  }

  extension++;
  return extension;

}


bool CUtil::GetParentPath(const CStdString& strPath, CStdString& strParent)
{
	strParent="";

	CURL url(strPath);
	CStdString strFile=url.GetFileName();
	if (strFile.size()==0) return false;

	if (HasSlashAtEnd(strFile) )
	{
		strFile=strFile.Left(strFile.size()-1);
	}

	int iPos=strFile.ReverseFind('/');
	if (iPos < 0)
	{
		iPos=strFile.ReverseFind('\\');
	}
	if (iPos < 0)
	{
		url.SetFileName("");
		url.GetURL(strParent);
		return true;
	}

	strFile=strFile.Left(iPos);
	url.SetFileName(strFile);
	url.GetURL(strParent);
  return true;
}

//*********************************************************************************************
void CUtil::LaunchXbe(char* szPath, char* szXbe, char* szParameters)
{
	OutputDebugString("Mounting ");
	OutputDebugString(szPath);
	OutputDebugString(" as D:\n");

	CIoSupport helper;
	helper.Unmount("D:");
	helper.Mount("D:",szPath);

	OutputDebugString("Launching ");
	OutputDebugString( (szXbe!=NULL) ? szXbe:"default title" );
	OutputDebugString("\n");				

	if (szParameters==NULL)
	{
		XLaunchNewImage(szXbe, NULL );
	}
	else
	{
		LAUNCH_DATA LaunchData;
		strcpy((char*)LaunchData.Data,szParameters);

		XLaunchNewImage(szXbe, &LaunchData );
	}
}

bool CUtil::FileExists(const CStdString& strFileName)
{
  FILE *fd;
  fd=fopen(strFileName.c_str(),"rb");
  if (fd != NULL)
  {
    fclose(fd);
    return true;
  }
  return false;
}

void CUtil::GetThumbnail(const CStdString& strFileName, CStdString& strThumb)
{
  char szThumbNail[1024];
	Crc32 crc;
	crc.Reset();
  crc.Compute(strFileName.c_str(),strlen(strFileName.c_str()));
  sprintf(szThumbNail,"%s\\%x.tbn",g_stSettings.szThumbnailsDirectory,crc);
  strThumb= szThumbNail;
}

void CUtil::GetFileSize(DWORD dwFileSize, CStdString& strFileSize)
{
  char szTemp[128];
  if (dwFileSize < 1024)
  {
    sprintf(szTemp,"%i", dwFileSize);
    strFileSize=szTemp;
    return;
  }
  if (dwFileSize < 1024*1024)
  {
    sprintf(szTemp,"%02.1f KB", ((float)dwFileSize)/1024.0f);
    strFileSize=szTemp;
    return;
  }
  sprintf(szTemp,"%02.1f MB", ((float)dwFileSize)/(1024.0f*1024.0f));
  strFileSize=szTemp;
  return;
  
}

void CUtil::GetDate(SYSTEMTIME stTime, CStdString& strDateTime)
{
  char szTmp[128];
  sprintf(szTmp,"%i-%i-%i %02.2i:%02.2i",
          stTime.wDay,stTime.wMonth,stTime.wYear,
          stTime.wHour,stTime.wMinute);
  strDateTime=szTmp;
}

void CUtil::GetHomePath(CStdString& strPath)
{
	char szXBEFileName[1024];
  CIoSupport helper;
  helper.GetXbePath(szXBEFileName);
  char *szFileName = strrchr(szXBEFileName,'\\');
  *szFileName=0;
  strPath=szXBEFileName;
}

bool CUtil::IsEthernetConnected()
{
	if (!(XNetGetEthernetLinkStatus() & XNET_ETHERNET_LINK_ACTIVE))
		return false;

	return true;
}

bool CUtil::InitializeNetwork(const char* szLocalAddress, const char* szLocalSubnet, const char* szLocalGateway)
{
	if (!IsEthernetConnected())
		return false;

	// if local address is specified
	if ( (szLocalAddress[0]!=0) &&
		 (szLocalSubnet[0]!=0)  &&
		 (szLocalGateway[0]!=0)  )

	{
		// Thanks and credits to Team Evox for the description of the 
		// XNetConfigParams structure.

		TXNetConfigParams configParams;   

		OutputDebugString("Loading network configuration...\n");
		XNetLoadConfigParams( (LPBYTE) &configParams );
		OutputDebugString("Ready.\n");

		BOOL bXboxVersion2 = (configParams.V2_Tag == 0x58425632 );	// "XBV2"
		BOOL bDirty = false;

		OutputDebugString("User local address: ");
		OutputDebugString(szLocalAddress);
		OutputDebugString("\n");
	
		if (bXboxVersion2)
		{
			if (configParams.V2_IP != inet_addr(szLocalAddress))
			{
				configParams.V2_IP = inet_addr(szLocalAddress);
				bDirty = true;
			}
		}
		else
		{
			if (configParams.V1_IP != inet_addr(szLocalAddress))
			{
				configParams.V1_IP = inet_addr(szLocalAddress);
				bDirty = true;
			}
		}

		OutputDebugString("User subnet mask: ");
		OutputDebugString(szLocalSubnet);
		OutputDebugString("\n");

		if (bXboxVersion2)
		{
			if (configParams.V2_Subnetmask != inet_addr(szLocalSubnet))
			{
				configParams.V2_Subnetmask = inet_addr(szLocalSubnet);
				bDirty = true;
			}
		}
		else
		{
			if (configParams.V1_Subnetmask != inet_addr(szLocalSubnet))
			{
				configParams.V1_Subnetmask = inet_addr(szLocalSubnet);
				bDirty = true;
			}
		}

		OutputDebugString("User gateway address: ");
		OutputDebugString(szLocalGateway);
		OutputDebugString("\n");

		if (bXboxVersion2)
		{
			if (configParams.V2_Defaultgateway != inet_addr(szLocalGateway))
			{
				configParams.V2_Defaultgateway = inet_addr(szLocalGateway);
				bDirty = true;
			}
		}
		else
		{
			if (configParams.V1_Defaultgateway != inet_addr(szLocalGateway))
			{
				configParams.V1_Defaultgateway = inet_addr(szLocalGateway);
				bDirty = true;
			}
		}

		if (configParams.Flag != (0x04|0x08) )
		{
			configParams.Flag = 0x04 | 0x08;
			bDirty = true;
		}

		if (bDirty)
		{
			OutputDebugString("Updating network configuration...\n");
			XNetSaveConfigParams( (LPBYTE) &configParams );
			OutputDebugString("Ready.\n");
		}
	}

	XNetStartupParams xnsp;
	memset(&xnsp, 0, sizeof(xnsp));
	xnsp.cfgSizeOfStruct = sizeof(XNetStartupParams);

	// Bypass security so that we may connect to 'untrusted' hosts
	xnsp.cfgFlags = XNET_STARTUP_BYPASS_SECURITY;
  // create more memory for networking
  xnsp.cfgPrivatePoolSizeInPages = 64; // == 256kb, default = 12 (48kb)
  xnsp.cfgEnetReceiveQueueLength = 16; // == 32kb, default = 8 (16kb)
  xnsp.cfgIpFragMaxSimultaneous = 16; // default = 4
  xnsp.cfgIpFragMaxPacketDiv256 = 32; // == 8kb, default = 8 (2kb)
  xnsp.cfgSockMaxSockets = 64; // default = 64
  xnsp.cfgSockDefaultRecvBufsizeInK = 128; // default = 16
  xnsp.cfgSockDefaultSendBufsizeInK = 128; // default = 16
	INT err = XNetStartup(&xnsp);

	XNADDR xna;
	DWORD dwState;
	do
	{
		dwState = XNetGetTitleXnAddr(&xna);
		Sleep(500);
	} while (dwState==XNET_GET_XNADDR_PENDING);

	XNetInAddrToString(xna.ina,g_szTitleIP,32);

	WSADATA WsaData;
	err = WSAStartup( MAKEWORD(2,2), &WsaData );
	return ( err == NO_ERROR );
}

static const __int64 SECS_BETWEEN_EPOCHS = 11644473600;
static const __int64 SECS_TO_100NS       = 10000000;

void CUtil::ConvertTimeTToFileTime(__int64 sec, long nsec, FILETIME &ftTime)
{
	__int64 l64Result =((__int64)sec + SECS_BETWEEN_EPOCHS) + SECS_TO_100NS + (nsec / 100);
	ftTime.dwLowDateTime = (DWORD)l64Result;
	ftTime.dwHighDateTime = (DWORD)(l64Result>>32);
}


void CUtil::ReplaceExtension(const CStdString& strFile, const CStdString& strNewExtension, CStdString& strChangedFile)
{
	CStdString strExtension;
	GetExtension(strFile,strExtension);
	if ( strExtension.size() )
	{
		
		strChangedFile=strFile.substr(0, strFile.size()-4) ;
		strChangedFile+=strNewExtension;
	}
	else
	{
		strChangedFile=strFile;
		strChangedFile+=strNewExtension;
	}
}

void CUtil::GetExtension(const CStdString& strFile, CStdString& strExtension)
{
	strExtension=strFile.substr( strFile.size()-4,4);
	if (strExtension.c_str()[0] != '.') strExtension="";
}

void CUtil::Lower(CStdString& strText)
{
  char szText[1024];
  strcpy(szText, strText.c_str());
  for (int i=0; i < (int)strText.size();++i)
    szText[i]=tolower(szText[i]);
  strText=szText;
};

void CUtil::Unicode2Ansi(const wstring& wstrText,CStdString& strName)
{
  strName="";
  char *pstr=(char*)wstrText.c_str();
  for (int i=0; i < (int)wstrText.size();++i )
  {
    strName += pstr[i*2];
  }
}
bool CUtil::HasSlashAtEnd(const CStdString& strFile)
{
  if (strFile.size()==0) return false;
  char kar=strFile.c_str()[strFile.size()-1];
  if (kar=='/' || kar=='\\') return true;
  return false;
}

 bool CUtil::IsRemote(const CStdString& strFile)
{
	CURL url(strFile);
	if ( url.GetProtocol().size() ) return true;
	return false;
}

 bool CUtil::IsDVD(const CStdString& strFile)
{
	if (strFile.Left(2)=="D:" || strFile.Left(2)=="d:")
		return true;
	return false;
}
void CUtil::GetFileAndProtocol(const CStdString& strURL, CStdString& strDir)
{
	strDir=strURL;
	if (!IsRemote(strURL)) return;
	if (IsDVD(strURL)) return;

	CURL url(strURL);
	strDir.Format("%s://%s", url.GetProtocol().c_str(), url.GetFileName().c_str());
}

void CUtil::RemoveCRLF(CStdString& strLine)
{
	while ( strLine.size() && (strLine.Right(1)=="\n" || strLine.Right(1)=="\r") )
	{
		strLine=strLine.Left((int)strLine.size()-1);
	}

}
bool CUtil::IsPicture(const CStdString& strFile) 
{
	CStdString strExtension;
	CUtil::GetExtension(strFile,strExtension);
  CUtil::Lower(strExtension);
	if ( strstr( g_stSettings.m_szMyPicturesExtensions, strExtension.c_str() ) )
	{
		return true;
	}
	return false;

}

bool CUtil::IsAudio(const CStdString& strFile) 
{
	CStdString strExtension;
	CUtil::GetExtension(strFile,strExtension);
  CUtil::Lower(strExtension);
	if ( strstr( g_stSettings.m_szMyMusicExtensions, strExtension.c_str() ) )
	{
		return true;
	}
	return false;
}
bool CUtil::IsVideo(const CStdString& strFile) 
{
	CStdString strExtension;
	CUtil::GetExtension(strFile,strExtension);
  CUtil::Lower(strExtension);
	if ( strstr( g_stSettings.m_szMyVideoExtensions, strExtension.c_str() ) )
	{
		return true;
	}
	return false;
}