#include "util.h"
#include "xbox/iosupport.h"
#include "crc32.h"
#include "settings.h"
#include "xbox/undocumented.h"
#include "lib/common/xbnet.h"
#include "url.h"
#include "shortcut.h"
#include "common/xbresource.h"
#include "graphiccontext.h"
#include "sectionloader.h"
#include "lib/cximage/ximage.h"

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
	if (strFileName.size()==0) return false;
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
	if (CUtil::IsXBE(strFileName))
	{
		if (CUtil::GetXBEIcon(strFileName,strThumb) ) return ;
		strThumb="defaultProgamIcon.png";
		return;
	}
	if (CUtil::IsShortCut(strFileName) )
	{
		CShortcut shortcut;
		if ( shortcut.Create( strFileName ) )
		{
			CStdString strFile=shortcut.m_strPath;
			
			GetThumbnail(strFile,strThumb);
			return;			
		}
	}

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
 void CUtil::URLEncode(CStdString& strURLData)
{
	CStdString strResult;
	for (int i=0; i < (int)strURLData.size(); ++i)
	{
			char kar=strURLData[i];
			if (kar==' ') strResult+='+';
			else if (isalnum(kar) || kar=='&'  || kar=='=' ) strResult+=kar;
			else {
				CStdString strTmp;
				strTmp.Format("%%%02.2x", kar);
				strResult+=strTmp;
			}
	}
	strURLData=strResult;
}

void CUtil::SaveString(const CStdString &strTxt, FILE *fd)
{
	int iSize=strTxt.size();
	fwrite(&iSize,1,sizeof(int),fd);
	if (iSize > 0)
	{
		fwrite(&strTxt.c_str()[0],1,iSize,fd);
	}
}

bool CUtil::LoadString(CStdString &strTxt, FILE *fd)
{
	strTxt="";
	int iSize;
	int iRead=fread(&iSize,1,sizeof(int),fd);
	if (iRead != sizeof(int) ) return false;
	if (feof(fd)) return false;
	if (iSize==0) return true;
	if (iSize > 0 && iSize < 16384)
	{
		char *szTmp = new char [iSize+2];
		iRead=fread(szTmp,1,iSize,fd);
		if (iRead != iSize)
		{
			delete [] szTmp;
			return false;
		}
		szTmp[iSize]=0;
		strTxt=szTmp;
		delete [] szTmp;
		return true;
	}
	return false;
}

void CUtil::SaveInt(int iValue, FILE *fd)
{
	fwrite(&iValue,1,sizeof(int),fd);
}

int CUtil::LoadInt( FILE *fd)
{
	int iValue;
	fread(&iValue,1,sizeof(int),fd);
	return iValue;
}

void CUtil::LoadDateTime(SYSTEMTIME& dateTime, FILE *fd)
{
	fread(&dateTime,1,sizeof(dateTime),fd);
}

void CUtil::SaveDateTime(SYSTEMTIME& dateTime, FILE *fd)
{
	fwrite(&dateTime,1,sizeof(dateTime),fd);
}


void CUtil::GetSongCacheName(const CStdString& strFileName, CStdString& strSongCacheName)
{
	Crc32 crc;
	crc.Reset();
  crc.Compute(strFileName.c_str(),strlen(strFileName.c_str()));
	strSongCacheName.Format("%s\\%x.si",g_stSettings.m_szAlbumDirectory,crc);
}

void CUtil::GetAlbumThumb(const CStdString& strFileName, CStdString& strThumb)
{
	Crc32 crc;
	crc.Reset();
  crc.Compute(strFileName.c_str(),strlen(strFileName.c_str()));
	strThumb.Format("%s\\%x.tbn",g_stSettings.m_szAlbumDirectory,crc);
}
void CUtil::GetAlbumCacheName(const CStdString& strFileName, CStdString& strAlbumThumb)
{
	CStdString strTmp="";
	for (int i=0; i < (int)strFileName.size(); ++i)
	{
		char kar=strFileName[i];
		if ( isalpha( (byte)kar) ) strTmp +=kar;
	}
	strTmp.ToLower();
	Crc32 crc;
	crc.Reset();
  crc.Compute(strTmp.c_str(),strTmp.size());
	strAlbumThumb.Format("%s\\%x.ai",g_stSettings.m_szAlbumDirectory,crc);
}
bool CUtil::GetXBEIcon(const CStdString& strFilePath, CStdString& strIcon)
{
  // check if thumbnail already exists
  char szThumbNail[1024];
	Crc32 crc;
	crc.Reset();
  crc.Compute(strFilePath.c_str(),strlen(strFilePath.c_str()));
  sprintf(szThumbNail,"%s\\%x.tbn",g_stSettings.szThumbnailsDirectory,crc);
  strIcon= szThumbNail;
  if (CUtil::FileExists(strIcon) )
  {
    //yes, just return
    return true;
  }

  // no, then create a new thumb
  // Locate file ID and get TitleImage.xbx E:\UDATA\<ID>\TitleImage.xbx

  bool bFoundThumbnail=false;
  CStdString szFileName;
	szFileName.Format("E:\\UDATA\\%08x\\TitleImage.xbx", GetXbeID( strFilePath ) );
			
  CXBPackedResource* pPackedResource = new CXBPackedResource();
  if( SUCCEEDED( pPackedResource->Create( szFileName.c_str(), 1, NULL ) ) )
  {
    LPDIRECT3DTEXTURE8 pTexture;
    LPDIRECT3DTEXTURE8 m_pTexture;
		D3DSURFACE_DESC descSurface;

		pTexture = pPackedResource->GetTexture((DWORD)0);

		if ( pTexture )
		{
      if ( SUCCEEDED( pTexture->GetLevelDesc( 0, &descSurface ) ) )
      {
        int iHeight=descSurface.Height;
        int iWidth=descSurface.Width;
        DWORD dwFormat=descSurface.Format;
        g_graphicsContext.Get3DDevice()->CreateTexture( 128,
									                128,
									                1,
									                0,
									                D3DFMT_LIN_A8R8G8B8,
									                0,
									                &m_pTexture);
				LPDIRECT3DSURFACE8 pSrcSurface = NULL;
				LPDIRECT3DSURFACE8 pDestSurface = NULL;

        pTexture->GetSurfaceLevel( 0, &pSrcSurface );
        m_pTexture->GetSurfaceLevel( 0, &pDestSurface );

        D3DXLoadSurfaceFromSurface( pDestSurface, NULL, NULL, 
                                    pSrcSurface, NULL, NULL,
                                    D3DX_DEFAULT, D3DCOLOR( 0 ) );
        D3DLOCKED_RECT rectLocked;
        if ( D3D_OK == m_pTexture->LockRect(0,&rectLocked,NULL,0L  ) )
        {
		        BYTE *pBuff   = (BYTE*)rectLocked.pBits;	
		        if (pBuff)
		        {
			        DWORD strideScreen=rectLocked.Pitch;
              //mp_msg(0,0," strideScreen=%i\n", strideScreen);

              CSectionLoader::Load("CXIMAGE");
              CxImage* pImage = new CxImage(iWidth, iHeight, 24, CXIMAGE_FORMAT_JPG);
				      for (int y=0; y < iHeight; y++)
              {
                byte *pPtr = pBuff+(y*(strideScreen));
                for (int x=0; x < iWidth;x++)
                {
                  byte Alpha=*(pPtr+3);
                  byte b=*(pPtr+0);
                  byte g=*(pPtr+1);
                  byte r=*(pPtr+2);
                  pPtr+=4;
                  
                  pImage->SetPixelColor(x,y,RGB(r,g,b));
                }
              }

              m_pTexture->UnlockRect(0);
		          //mp_msg(0,0,"save as %s\n", szThumbNail);
		          pImage->Resample(64,64,0);
		          pImage->Flip();
              pImage->Save(strIcon.c_str(),CXIMAGE_FORMAT_JPG);
		          delete pImage;
              bFoundThumbnail=true;
              CSectionLoader::Unload("CXIMAGE");
            }
            else m_pTexture->UnlockRect(0);
        }
        pSrcSurface->Release();
        pDestSurface->Release();
        m_pTexture->Release();
      }
      pTexture->Release();
    }
  }
  delete pPackedResource;
  return bFoundThumbnail;
}


bool CUtil::GetXBEDescription(const CStdString& strFileName, CStdString& strDescription)
{

		_XBE_CERTIFICATE HC;
		_XBE_HEADER HS;

		FILE* hFile  = fopen(strFileName.c_str(),"rb");
    if (!hFile)
    {
      strDescription=CUtil::GetFileName(strFileName);
      return false;
    }
		fread(&HS,1,sizeof(HS),hFile);
		fseek(hFile,HS.XbeHeaderSize,SEEK_SET);
		fread(&HC,1,sizeof(HC),hFile);
		fclose(hFile);

		CHAR TitleName[40];
		WideCharToMultiByte(CP_ACP,0,HC.TitleName,-1,TitleName,40,NULL,NULL);
    if (strlen(TitleName) > 0)
    {
		  strDescription=TitleName;
      return true;
    }
    strDescription=CUtil::GetFileName(strFileName);
    return false;
}

DWORD CUtil::GetXbeID( const CStdString& strFilePath)
{
	DWORD dwReturn = 0;
	HANDLE hFile;
	DWORD dwCertificateLocation;
	DWORD dwLoadAddress;
	DWORD dwRead;
//	WCHAR wcTitle[41];
	
  hFile = CreateFile( strFilePath.c_str(), 
						GENERIC_READ, 
						FILE_SHARE_READ, 
						NULL,
						OPEN_EXISTING,
						FILE_ATTRIBUTE_NORMAL,
						NULL );
	if ( hFile != INVALID_HANDLE_VALUE )
	{
		if ( SetFilePointer(	hFile,  0x104, NULL, FILE_BEGIN ) == 0x104 )
		{
			if ( ReadFile( hFile, &dwLoadAddress, 4, &dwRead, NULL ) )
			{
				if ( SetFilePointer(	hFile,  0x118, NULL, FILE_BEGIN ) == 0x118 )
				{
					if ( ReadFile( hFile, &dwCertificateLocation, 4, &dwRead, NULL ) )
					{
						dwCertificateLocation -= dwLoadAddress;
						// Add offset into file
						dwCertificateLocation += 8;
						if ( SetFilePointer(	hFile,  dwCertificateLocation, NULL, FILE_BEGIN ) == dwCertificateLocation )
						{
							dwReturn = 0;
							ReadFile( hFile, &dwReturn, sizeof(DWORD), &dwRead, NULL );
							if ( dwRead != sizeof(DWORD) )
							{
								dwReturn = 0;
							}
						}

					}
				}
			}
		}
		CloseHandle(hFile);
	}
	return dwReturn;
}