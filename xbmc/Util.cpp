#include "util.h"
#include "xbox/iosupport.h"
#include "crc32.h"
#include "settings.h"
#include "utils/log.h"
#include "xbox/undocumented.h"
#include "lib/common/xbnet.h"
#include "url.h"
#include "shortcut.h"
#include "common/xbresource.h"
#include "graphiccontext.h"
#include "sectionloader.h"
#include "lib/cximage/ximage.h"
#include "filesystem/file.h"
#include "DetectDVDType.h"
#include "autoptrhandle.h"
#include "playlistfactory.h"
#include "ThumbnailCache.h"
#include "application.h"
#include "picture.h"

using namespace AUTOPTR;
using namespace MEDIA_DETECT;
using namespace XFILE;
using namespace PLAYLIST;
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

bool CUtil::IsDefaultXBE(const CStdString& strFileName)
{
	char* pFileName=GetFileName(strFileName);
	if (!pFileName) return false;
	if (CUtil::cmpnocase(pFileName, "default.xbe")==0) return true;
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
	if (strFile.size()==0)
	{
		if (url.GetProtocol() == "smb" && (url.GetHostName().size() > 0))
		{
			// we have an smb share with only server or workgroup name
			// set hostname to "" and return true.
			url.SetHostName("");
			url.GetURL(strParent);
			return true;
		}
		return false;
	}

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

//Make sure you have a full path in the filename, otherwise adds the base path before.
void CUtil::GetQualifiedFilename(const CStdString &strBasePath, CStdString &strFilename)
{
	CURL plItemUrl(strFilename);
	CURL plBaseUrl(strBasePath);

	if(plBaseUrl.GetProtocol().length()==0) //Base in local directory
	{
		if(plItemUrl.GetProtocol().length()==0 ) //Filename is local or not qualified
		{
			if (!( isalpha(strFilename.c_str()[0]) && strFilename.c_str()[1] == ':')) //Filename not fully qualified
			{
				if (strFilename.c_str()[0] == '\\') 
        {
          strFilename.Replace('/','\\');
					strFilename = strBasePath + strFilename;
        }
				else
        {
          strFilename.Replace('/','\\');
					strFilename = strBasePath + '\\' + strFilename;
        }
			}
		}
	}
	else //Base is remote
	{
		if(plItemUrl.GetProtocol().length()==0 ) //Filename is local
		{
			if (strFilename.c_str()[0] == '/') //Begins with a slash.. not good.. but we try to make the best of it..
      {
        strFilename.Replace('\\','/');
				strFilename = strBasePath + strFilename;
      }
			else
				strFilename = strBasePath + '/' + strFilename;
		}
	}
}


//*********************************************************************************************
void CUtil::LaunchXbe(char* szPath, char* szXbe, char* szParameters)
{
  CLog::Log("launch xbe:%s %s", szPath,szXbe);
  CLog::Log(" mount %s as D:", szPath);

	CIoSupport helper;
	helper.Unmount("D:");
	helper.Mount("D:",szPath);

  CLog::Log("launch xbe:%s", szXbe);

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
	CFile file;
	if (!file.Open(strFileName))
		return false;

	file.Close();
	return true;
}

void CUtil::GetThumbnail(const CStdString& strFileName, CStdString& strThumb)
{
  CStdString strFile;
  CUtil::ReplaceExtension(strFileName,".tbn",strFile);
  if (CUtil::FileExists(strFile))
  {
    strThumb=strFile;
    return;
  }

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

void CUtil::GetFileSize(__int64 dwFileSize, CStdString& strFileSize)
{
  char szTemp[128];
  // file < 1 kbyte?
  if (dwFileSize < 1024)
  {
		//	substract the integer part of the float value
		float fRemainder=(((float)dwFileSize)/1024.0f)-floor(((float)dwFileSize)/1024.0f);
		float fToAdd=0.0f;
		if (fRemainder < 0.01f)
			fToAdd=0.1f;
    sprintf(szTemp,"%2.1f KB", (((float)dwFileSize)/1024.0f)+fToAdd);
    strFileSize=szTemp;
    return;
  }
  __int64 iOneMeg=1024*1024;

  // file < 1 megabyte?
  if (dwFileSize < iOneMeg)
  {
    sprintf(szTemp,"%02.1f KB", ((float)dwFileSize)/1024.0f);
    strFileSize=szTemp;
    return;
  }

  // file < 1 GByte?
  __int64 iOneGigabyte=iOneMeg;
  iOneGigabyte *= (__int64)1000;
  if (dwFileSize < iOneGigabyte)
  {
    sprintf(szTemp,"%02.1f MB", ((float)dwFileSize)/((float)iOneMeg));
    strFileSize=szTemp;
    return;
  }
  //file > 1 GByte
  int iGigs=0;
  while (dwFileSize >= iOneGigabyte)
  {
    dwFileSize -=iOneGigabyte;
    iGigs++;
  }
  float fMegs=((float)dwFileSize)/((float)iOneMeg);
  fMegs /=1000.0f;
  fMegs+=iGigs;
  sprintf(szTemp,"%02.1f GB", fMegs);
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

void CUtil::GetTitleIP(CStdString& ip)
{
	ip = g_szTitleIP;
}

bool CUtil::InitializeNetwork(const char* szLocalAddress, const char* szLocalSubnet, const char* szLocalGateway, const char* szNameServer)
{

  if (!IsEthernetConnected())
  {
    CLog::Log("network cable unplugged");
    return false;
  }

  struct network_info networkinfo ;
  memset(&networkinfo ,0,sizeof(networkinfo ));
  bool bSetup(false);
  if (CUtil::cmpnocase(szLocalAddress,"dhcp")==0 )
  {
    bSetup=true;
    CLog::Log("use DHCP");
    networkinfo.DHCP=true;
  }
  else if (szLocalAddress[0] && szLocalSubnet[0] && szLocalGateway[0] && szNameServer[0])
  {
    bSetup=true;
    CLog::Log("use static ip");
    networkinfo.DHCP=false;
    strcpy(networkinfo.ip,szLocalAddress);
    strcpy(networkinfo.subnet,szLocalSubnet);
    strcpy(networkinfo.gateway,szLocalGateway);
    strcpy(networkinfo.DNS1,szNameServer);
  }
  else
  {
    CLog::Log("Not initializing network, using settings as they are setup by dashboard");
    if ( szLocalAddress[0] || szLocalSubnet[0] || szLocalGateway[0] ||szNameServer[0])
    {
      CLog::Log("Error initializing network. To setup a network you must choose:");
      CLog::Log("Static ip:"); 
      CLog::Log("  fill in the <ip>, <netmask>, <defaultgateway>, <nameserver> tags in xboxmediacenter.xml");
      CLog::Log("DHCP:");
      CLog::Log("  just set <ip>DHCP</ip> in xboxmediacenter.xml");
      CLog::Log("use existing settings from dashboard:");
      CLog::Log("  leave the <ip>, <netmask>, <defaultgateway>, <nameserver> tags in xboxmediacenter.xml empty");
      CLog::Log("your current setup doesnt do any of those options. You filled in some tags, but not all!!");
    }
  }

  if (bSetup)
  {
    CLog::Log("setting up network...");
    int iCount=0;
    while (CUtil::SetUpNetwork( false, networkinfo )!=0 && iCount < 100)
    {
      Sleep(50);
      iCount++;
    }
  }
  else
  {
    CLog::Log("init network");
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
  }

  CLog::Log("get local ip adres:");
	XNADDR xna;
	DWORD dwState;
	do
	{
		dwState = XNetGetTitleXnAddr(&xna);
		Sleep(50);
	} while (dwState==XNET_GET_XNADDR_PENDING);

	XNetInAddrToString(xna.ina,g_szTitleIP,32);

  CLog::Log("ip adres:%s",g_szTitleIP);
	WSADATA WsaData;
	int err = WSAStartup( MAKEWORD(2,2), &WsaData );
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
		
		strChangedFile=strFile.substr(0, strFile.size()-strExtension.size()) ;
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
	int iPos=strFile.ReverseFind(".");
	if (iPos <0)
	{
		strExtension="";
		return;
	}
	strExtension=strFile.Right( strFile.size()-iPos);
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
	CStdString strProtocol=url.GetProtocol();
	strProtocol.ToLower();
	if (strProtocol=="cdda" || strProtocol=="iso9660") return false;
	if ( url.GetProtocol().size() ) return true;
	return false;
}

 bool CUtil::IsDVD(const CStdString& strFile)
{
	if (strFile.Left(2)=="D:" || strFile.Left(2)=="d:")
		return true;

	return false;
}
bool CUtil::IsCDDA(const CStdString& strFile)
{
CURL url(strFile);
if (url.GetProtocol()=="cdda") 
	return true;
return false;
}
bool CUtil::IsISO9660(const CStdString& strFile)
{
	CStdString strLeft=strFile.Left(8);
	strLeft.ToLower();
	if (strLeft=="iso9660:")
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
bool CUtil::IsShoutCast(const CStdString& strFileName)
{
	if (strstr(strFileName.c_str(), "shout:") ) return true;
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
	if (strstr(strFile.c_str(), ".cdda") ) return true;
	if (IsShoutCast(strFile) ) return true;

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
bool CUtil::IsPlayList(const CStdString& strFile) 
{
	CStdString strExtension;
	CUtil::GetExtension(strFile,strExtension);
	strExtension.ToLower();
	if (strExtension==".m3u") return true;
	if (strExtension==".b4s") return true;
	if (strExtension==".pls") return true;
	if (strExtension==".strm") return true;
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
int  CUtil::LoadString(CStdString &strTxt, byte* pBuffer)
{
	strTxt="";
	int iSize;
	int iCount=sizeof(int);
	memcpy(&iSize,pBuffer,sizeof(int));
	if (iSize==0) return iCount;
	char *szTmp = new char [iSize+2];
	memcpy(szTmp ,&pBuffer[iCount], iSize);
	szTmp[iSize]=0;
	iCount+=iSize;
	strTxt=szTmp;
	delete [] szTmp;
	return iCount;
}
bool CUtil::LoadString(string &strTxt, FILE *fd)
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


void CUtil::GetSongInfo(const CStdString& strFileName, CStdString& strSongCacheName)
{
	Crc32 crc;
	crc.Reset();
  crc.Compute(strFileName.c_str(),strlen(strFileName.c_str()));
	strSongCacheName.Format("%s\\songinfo\\%x.si",g_stSettings.m_szAlbumDirectory,crc);
}

void CUtil::GetAlbumThumb(const CStdString& strFileName, CStdString& strThumb, bool bTempDir /*=false*/)
{
	Crc32 crc;
	crc.Reset();
  crc.Compute(strFileName.c_str(),strlen(strFileName.c_str()));
	if (bTempDir)
		strThumb.Format("%s\\thumbs\\temp\\%x.tbn",g_stSettings.m_szAlbumDirectory,crc);
	else
		strThumb.Format("%s\\thumbs\\%x.tbn",g_stSettings.m_szAlbumDirectory,crc);
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


bool CUtil::GetDirectoryName(const CStdString& strFileName, CStdString& strDescription)
{
	CStdString strFName=CUtil::GetFileName(strFileName);
	strDescription=strFileName.Left(strFileName.size()-strFName.size());
	if (CUtil::HasSlashAtEnd(strDescription) )
	{
		strDescription=strDescription.Left(strDescription.size()-1);
	}
	int iPos=strDescription.ReverseFind("\\");
	if (iPos < 0)
		iPos=strDescription.ReverseFind("/");
	if (iPos >=0)
	{
		strDescription=strDescription.Right(strDescription.size()-iPos);
	}
	else strDescription=strFName;
	return true;
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
	
	DWORD dwCertificateLocation;
	DWORD dwLoadAddress;
	DWORD dwRead;
//	WCHAR wcTitle[41];
	
  CAutoPtrHandle  hFile( CreateFile( strFilePath.c_str(), 
						GENERIC_READ, 
						FILE_SHARE_READ, 
						NULL,
						OPEN_EXISTING,
						FILE_ATTRIBUTE_NORMAL,
						NULL ));
	if ( hFile.isValid() )
	{
		if ( SetFilePointer(	(HANDLE)hFile,  0x104, NULL, FILE_BEGIN ) == 0x104 )
		{
			if ( ReadFile( (HANDLE)hFile, &dwLoadAddress, 4, &dwRead, NULL ) )
			{
				if ( SetFilePointer(	(HANDLE)hFile,  0x118, NULL, FILE_BEGIN ) == 0x118 )
				{
					if ( ReadFile( (HANDLE)hFile, &dwCertificateLocation, 4, &dwRead, NULL ) )
					{
						dwCertificateLocation -= dwLoadAddress;
						// Add offset into file
						dwCertificateLocation += 8;
						if ( SetFilePointer(	(HANDLE)hFile,  dwCertificateLocation, NULL, FILE_BEGIN ) == dwCertificateLocation )
						{
							dwReturn = 0;
							ReadFile( (HANDLE)hFile, &dwReturn, sizeof(DWORD), &dwRead, NULL );
							if ( dwRead != sizeof(DWORD) )
							{
								dwReturn = 0;
							}
						}

					}
				}
			}
		}
	}
	return dwReturn;
}

void CUtil::FillInDefaultIcons(VECFILEITEMS &items)
{
	for (int i=0; i < (int)items.size(); ++i)
	{
		CFileItem* pItem=items[i];
		FillInDefaultIcon(pItem);

	}
}

void CUtil::FillInDefaultIcon(CFileItem* pItem)
{
  // find the default icon for a file or folder item
  // for files this can be the (depending on the file type)
  //   default picture for photo's 
  //   default picture for songs
  //   default picture for videos
  //   default picture for shortcuts
  //   default picture for playlists
  //   or the icon embedded in an .xbe
  //
  // for folders 
  //   for .. folders the default picture for parent folder
  //   for other folders the defaultFolder.png

  CStdString strThumb;
  bool bOnlyDefaultXBE=g_stSettings.m_bMyProgramsDefaultXBE;
	if (!pItem->m_bIsFolder)
	{
		if (CUtil::IsPicture(pItem->m_strPath) )
		{
		  // picture
			pItem->SetIconImage("defaultPicture.png");
		}
		else if ( CUtil::IsAudio(pItem->m_strPath) )
		{
		  // album database
      pItem->SetIconImage("defaultAudio.png");
		}
		else if ( bOnlyDefaultXBE ? CUtil::IsDefaultXBE(pItem->m_strPath) : CUtil::IsXBE(pItem->m_strPath) )
		{
		  // xbe
			pItem->SetIconImage("defaultProgram.png");
			if ( !CUtil::IsDVD(pItem->m_strPath) )
			{
				CStdString strDescription;
				if (! CUtil::GetXBEDescription(pItem->m_strPath,strDescription))
				{
					CStdString strFName=CUtil::GetFileName(pItem->m_strPath);
					strDescription=pItem->m_strPath.Left(pItem->m_strPath.size()-strFName.size());
					if (CUtil::HasSlashAtEnd(strDescription) )
					{
						strDescription=strDescription.Left(strDescription.size()-1);
					}
					int iPos=strDescription.ReverseFind("\\");
					if (iPos < 0)
						iPos=strDescription.ReverseFind("/");
					if (iPos >=0)
					{
						strDescription=strDescription.Right(strDescription.size()-iPos);
					}
					else strDescription=strFName;
				}
				if (strDescription.size())
				{
					CStdString strFname;
					strFname=CUtil::GetFileName(pItem->m_strPath);
					strFname.ToLower();
					if (strFname!="dashupdate.xbe" && strFname!="downloader.xbe" && strFname != "update.xbe")
					{
						CShortcut cut;
						cut.m_strPath=pItem->m_strPath;
						cut.Save(strDescription);
					}
				}
			}
		}
		else if (CUtil::IsVideo(pItem->m_strPath) )
		{
		  // video
			pItem->SetIconImage("defaultVideo.png");
		}
		else if (CUtil::IsPlayList(pItem->m_strPath) )
		{
		  // playlist
			CStdString strDir;
			CStdString strFileName;
			pItem->SetIconImage("defaultPlaylist.png");
      CPlayListFactory factory;
      auto_ptr<CPlayList> pPlayList (factory.Create(pItem->m_strPath) );
      if (NULL != pPlayList.get() )
      {
        if (pPlayList->Load(pItem->m_strPath))
        {
			    strFileName=CUtil::GetFileName(pItem->m_strPath);
			    strDir.Format("%s\\playlists\\%s",g_stSettings.m_szAlbumDirectory,strFileName.c_str());
			    if ( strDir != pItem->m_strPath )
			    {
            pPlayList->Save(strDir) ;
			    }
        }
      }
		}
		else if (CUtil::IsShortCut(pItem->m_strPath) )
		{
      // shortcut
			CStdString strDescription;
			CStdString strFName;
			strFName=CUtil::GetFileName(pItem->m_strPath);
			int iPos=strFName.ReverseFind(".");
			strDescription=strFName.Left(iPos);
			pItem->SetLabel(strDescription);
			pItem->SetIconImage("defaultShortcut.png");
		}
	}
	else
	{
		if (pItem->GetLabel()=="..")
		{
			pItem->SetIconImage("defaultFolderBack.png");
		}
	}

	if (pItem->GetIconImage()=="")
	{
		if (pItem->m_bIsFolder)
		{
			pItem->SetIconImage("defaultFolder.png");
		}
		if (!pItem->m_bIsFolder)
		{
			CStdString strExtension;
			CUtil::GetExtension(pItem->m_strPath,strExtension);
			
			for (int i=0; i < (int)g_settings.m_vecIcons.size(); ++i)
			{
				CFileTypeIcon& icon=g_settings.m_vecIcons[i];

				if (CUtil::cmpnocase(strExtension.c_str(), icon.m_strName)==0)
				{
					pItem->SetIconImage(icon.m_strIcon);
					break;
				}
			}
		}
	}

	if (pItem->GetThumbnailImage()=="")
	{
		if (pItem->GetIconImage()!="")
		{
			CStdString strBig;
			int iPos=pItem->GetIconImage().Find(".");
			strBig=pItem->GetIconImage().Left(iPos);
			strBig+="Big";
			strBig+=pItem->GetIconImage().Right(pItem->GetIconImage().size()-(iPos));
			pItem->SetThumbnailImage(strBig);
		}
	}
}

void CUtil::SetThumbs(VECFILEITEMS &items)
{
  for (int i=0; i < (int)items.size(); ++i)
  {
    CFileItem* pItem=items[i];
		SetThumb(pItem);
  }
}

bool CUtil::GetFolderThumb(const CStdString& strFolder, CStdString& strThumb)
{
  // get the thumbnail for the folder contained in strFolder
  // and return the filename of the thumbnail in strThumb
  //
  // if folder contains folder.jpg and is local on xbox HD then use it as the thumbnail
  // if folder contains folder.jpg but is located on a share then cache the folder.jpg
  // to q:\thumbs and return the cached image as a thumbnail
  CStdString strFolderImage;
  strThumb="";
	AddFileToFolder(strFolder, "folder.jpg", strFolderImage);

  // remote or local file?
	if (CUtil::IsRemote(strFolder) )
	{
    CURL url(strFolder);
    // dont try to locate a folder.jpg for streams &  shoutcast
    if (url.GetProtocol() =="http" || url.GetProtocol()=="HTTP") return false;
    if (url.GetProtocol() =="shout" || url.GetProtocol()=="SHOUT") return false;
    if (url.GetProtocol() =="mms" || url.GetProtocol()=="MMS") return false;

    // check if folder.jpg exists 
		CUtil::GetThumbnail( strFolderImage,strThumb);
		CFile file;
    // yes, then cache it to xbox HD
		if ( file.Cache(strFolderImage.c_str(), strThumb.c_str(),NULL,NULL))
		{
			return true;
		}
		else
		{
			if (CUtil::FileExists(strThumb))
			{
				return true;
			}
		}
	}
	else if (CUtil::FileExists(strFolderImage) )
	{
    // is local, and folder.jpg exists. Use it
		strThumb=strFolderImage;
    return true;
	}

  // no thumb found
  strThumb="";
  return false;
}

void CUtil::SetThumb(CFileItem* pItem)
{
  // set the thumbnail for an file item
  
  // if it already has a thumbnail, then return
	if ( pItem->HasThumbnail() ) return;
	
  // get the path to the thumbnail (q:\thumb\agdhffh.tbn)
	CStdString strThumb;
	CUtil::GetThumbnail( pItem->m_strPath,strThumb);

  // does thumb exists?
	if (!CUtil::FileExists(strThumb) )
	{
    // no thumb does NOT exits
		bool bGotIcon(false);
		if (CUtil::IsRemote(pItem->m_strPath) )
		{
			CFile file;
			CStdString strThumbnailFileName;
			CUtil::ReplaceExtension(pItem->m_strPath,".tbn", strThumbnailFileName);
			if ( file.Cache(strThumbnailFileName.c_str(), strThumb.c_str(),NULL,NULL))
			{
				pItem->SetThumbnailImage(strThumb);
				bGotIcon=true;
			}
		}

    // fill in the folder thumbs
		if (!bGotIcon && pItem->GetLabel() != "..")
		{
      // this is a folder ?
			if (pItem->m_bIsFolder)
			{
        // yes, then get the folder thumbnail
        if ( CUtil::GetFolderThumb(pItem->m_strPath, strThumb))
        {
					pItem->SetThumbnailImage(strThumb);
        }
			}
		}
	}
	else
	{
    // yes thumb exists, 
		pItem->SetThumbnailImage(strThumb);
	}
}

void CUtil::ShortenFileName(CStdString& strFileNameAndPath)
{
	CStdString strFile=CUtil::GetFileName(strFileNameAndPath);
	if (strFile.size() > 42)
	{
		CStdString strExtension;
		CUtil::GetExtension(strFileNameAndPath, strExtension);
		CStdString strPath=strFileNameAndPath.Left( strFileNameAndPath.size() - strFile.size() );
		
		strFile=strFile.Left(42-strExtension.size());
		strFile+=strExtension;

		CStdString strNewFile=strPath;
		if (!CUtil::HasSlashAtEnd(strPath)) 
			strNewFile+="\\";

		strNewFile+=strFile;
		strFileNameAndPath=strNewFile;
	}
}

void CUtil::ConvertPathToUrl( const CStdString& strPath, const CStdString& strProtocol, CStdString& strOutUrl )
{
	strOutUrl = strProtocol;
	CStdString temp = strPath;
	temp.Replace( '\\', '/' );
	temp.Delete( 0, 3 );
	strOutUrl += temp;
}

void CUtil::GetDVDDriveIcon( const CStdString& strPath, CStdString& strIcon )
{
	if ( !CDetectDVDMedia::IsDiscInDrive() ) {
		strIcon="defaultDVDEmpty.png";
		return;
	}

	if ( IsDVD(strPath) ) {
		CCdInfo* pInfo = CDetectDVDMedia::GetCdInfo();
		//	xbox DVD
		if ( pInfo != NULL && pInfo->IsUDFX( 1 ) ) {
			strIcon="defaultXBOXDVD.png";
			return;
		}
		strIcon="defaultDVDRom.png";
		return;
	}

	if ( IsISO9660(strPath) ) {
		CCdInfo* pInfo = CDetectDVDMedia::GetCdInfo();
		if ( pInfo != NULL && pInfo->IsVideoCd( 1 ) ) {
			strIcon="defaultVCD.png";
			return;
		}
		strIcon="defaultDVDRom.png";
		return;
	}
	
	if ( IsCDDA(strPath) ) {
		strIcon="defaultCDDA.png";
		return;
	}
}

void CUtil::RemoveTempFiles()
{
	WIN32_FIND_DATA wfd;
	
	CStdString strAlbumDir;
	strAlbumDir.Format("%s\\*.tmp",g_stSettings.m_szAlbumDirectory);
	memset(&wfd,0,sizeof(wfd));

	CAutoPtrFind hFind( FindFirstFile(strAlbumDir.c_str(),&wfd));
	if (!hFind.isValid())
		return ;
	do
	{
		if ( !(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
		{
			string strFile="Q:\\albums\\";
			strFile += wfd.cFileName;
			DeleteFile(strFile.c_str());
		}
	} while (FindNextFile(hFind, &wfd));

	//CStdString strTempThumbDir;
	//strTempThumbDir.Format("%s\\thumbs\\temp\\*.tbn",g_stSettings.m_szAlbumDirectory);
	//memset(&wfd,0,sizeof(wfd));

	//CAutoPtrFind hFind1( FindFirstFile(strTempThumbDir.c_str(),&wfd));
	//if (!hFind1.isValid())
	//	return ;
	//do
	//{
	//	if ( !(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
	//	{
	//		CStdString strFile;
	//		strFile.Format("%s\\thumbs\\temp\\",g_stSettings.m_szAlbumDirectory);
	//		strFile += wfd.cFileName;
	//		DeleteFile(strFile.c_str());
	//	}
	//} while (FindNextFile(hFind1, &wfd));
}

bool CUtil::IsHD(const CStdString& strFileName)
{
	CURL url(strFileName);
	CStdString strProtocol=url.GetProtocol();
	strProtocol.ToLower();
	if (strProtocol=="cdda" || strProtocol=="iso9660") return false;
	if ( url.GetProtocol().size() ) return false;
	return true;
}

// Following 6 routines added by JM to determine (possible) source type based
// on frame size
bool CUtil::IsNTSC_VCD(int iWidth, int iHeight)
{
  return (iWidth==352 && iHeight==240);
}

bool CUtil::IsNTSC_SVCD(int iWidth, int iHeight)
{
  return (iWidth==480 && iHeight==480);
}

bool CUtil::IsNTSC_DVD(int iWidth, int iHeight)
{
  return (iWidth==720 && iHeight==480);
}

bool CUtil::IsPAL_VCD(int iWidth, int iHeight)
{
  return (iWidth==352 && iHeight==488);
}

bool CUtil::IsPAL_SVCD(int iWidth, int iHeight)
{
  return (iWidth==480 && iHeight==576);
}

bool CUtil::IsPAL_DVD(int iWidth, int iHeight)
{
  return (iWidth==720 && iHeight==576);
}


void CUtil::RemoveIllegalChars( CStdString& strText)
{
	char szRemoveIllegal [1024];
	strcpy(szRemoveIllegal ,strText.c_str());
	static char legalChars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890!#$%&'()-@[]^_`{}~ ";
	char *cursor;
	for (cursor = szRemoveIllegal; *(cursor += strspn(cursor, legalChars)); /**/ )
		*cursor = '_';
	strText=szRemoveIllegal;
}

void CUtil::CacheSubtitles(const CStdString& strMovie)
{
	char * sub_exts[] = {  ".utf", ".utf8", ".utf-8", ".sub", ".srt", ".smi", ".rt", ".txt", ".ssa", ".aqt", ".jss", ".ass", ".idx",".ifo", NULL};
	int iPos=0;
  bool bFoundSubs=false;
  CURL url(strMovie);
  if (url.GetProtocol() == "http") return;
  if (url.GetProtocol() == "shout") return;
  // check alternate subtitle directory
  if (strlen(g_stSettings.m_szAlternateSubtitleDirectory)==0) return;
  WIN32_FIND_DATA wfd;
  CAutoPtrFind hFind ( FindFirstFile("T:\\*.*",&wfd));
	if (hFind.isValid())
  {
	  do
	  {
		  if (wfd.cFileName[0]!=0)
		  {
			  if ( (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)==0 )
        {
          CStdString strFile;
          strFile.Format("T:\\%s", wfd.cFileName);
          if (strFile.Find("subtitle")>=0 )
          {
            ::DeleteFile(strFile.c_str());
          }
        }
      }
    } while (FindNextFile((HANDLE)hFind, &wfd));
  }

  iPos=0;
	while (sub_exts[iPos])
	{
		CStdString strSource,strDest;
		strDest.Format("T:\\subtitle%s", sub_exts[iPos]);

		if (CUtil::IsVideo(strMovie))
		{
      CStdString strFname;
      strFname=CUtil::GetFileName(strMovie);
			strSource.Format("%s\\%s", g_stSettings.m_szAlternateSubtitleDirectory,strFname.c_str());
			CUtil::ReplaceExtension(strSource,sub_exts[iPos],strSource);
			CFile file;
			if ( file.Cache(strSource.c_str(), strDest.c_str(),NULL,NULL))
      {
        CLog::Log(" cached subtitle %s->%s\n", strSource.c_str(), strDest.c_str());
        bFoundSubs=true;
      }
		}
		iPos++;
	}

  if (bFoundSubs) return;

  // check original movie directory
  iPos=0;
	while (sub_exts[iPos])
	{
		CStdString strSource,strDest;
		strDest.Format("T:\\subtitle%s", sub_exts[iPos]);

		::DeleteFile(strDest);
		if (CUtil::IsVideo(strMovie))
		{
			strSource=strMovie;
			CUtil::ReplaceExtension(strMovie,sub_exts[iPos],strSource);
			CFile file;
			if ( file.Cache(strSource.c_str(), strDest.c_str(),NULL,NULL))
      {
        CLog::Log(" cached subtitle %s->%s\n", strSource.c_str(), strDest.c_str());
        bFoundSubs=true;
      }
		}
		iPos++;
	}


}

void CUtil::SecondsToHMSString(long lSeconds, CStdString& strHMS)
{
	int hh = lSeconds / 3600;
	lSeconds = lSeconds%3600;
	int mm = lSeconds / 60;
	int ss = lSeconds % 60;

	if (hh>=1)
		strHMS.Format("%2.2i:%02.2i:%02.2i",hh,mm,ss);
	else
		strHMS.Format("%i:%02.2i",mm,ss);

}
void CUtil::PrepareSubtitleFonts()
{				
  if (strlen(g_stSettings.m_szSubtitleFont)==0) return;
  if (g_stSettings.m_iSubtitleHeight==0) return;

  CStdString strPath,strHomePath,strSearchMask;
	strHomePath = "Q:";
  strPath.Format("%s\\mplayer\\font\\%s\\%i\\",
          strHomePath.c_str(),
	        g_stSettings.m_szSubtitleFont,g_stSettings.m_iSubtitleHeight);

  strSearchMask=strPath+"*.*";
	WIN32_FIND_DATA wfd;
  CAutoPtrFind hFind ( FindFirstFile(strSearchMask.c_str(),&wfd));
	if (hFind.isValid())
  {
	  do
	  {
		  if (wfd.cFileName[0]!=0)
		  {
			  if ( (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)==0 )
        {
          CStdString strSource,strDest;
          strSource.Format("%s%s",strPath.c_str(),wfd.cFileName);
          strDest.Format("%s\\mplayer\\font\\%s",strHomePath.c_str(),  wfd.cFileName);
          ::CopyFile(strSource.c_str(),strDest.c_str(),FALSE);
        }
      }
    } while (FindNextFile((HANDLE)hFind, &wfd));
  }
}

__int64 CUtil::ToInt64(DWORD dwHigh, DWORD dwLow)
{
  __int64 n;
  n=dwHigh;
  n <<=32;
  n += dwLow;
  return n;
}

void CUtil::AddFileToFolder(const CStdString& strFolder, const CStdString& strFile, CStdString& strResult)
{
  strResult=strFolder;
  if (!CUtil::HasSlashAtEnd(strResult)) 
  {
    if (strResult.Find("//")>=0 )
      strResult+="/";
    else
      strResult+="\\";
  }
  strResult += strFile;
}

bool CUtil::IsNFO(const CStdString& strFile)
{
  char *pExtension=CUtil::GetExtension(strFile);
  if (!pExtension) return false;
  if (CUtil::cmpnocase(pExtension,".nfo")==0) return true;
  return false;
}

void CUtil::GetPath(const CStdString& strFileName, CStdString& strPath)
{
  int iPos1=strFileName.Find("/");
  int iPos2=strFileName.Find("\\");
  int iPos3=strFileName.Find(":");
  if (iPos2>iPos1) iPos1=iPos2;
  if (iPos3>iPos1) iPos1=iPos3;

  strPath=strFileName.Left(iPos1-1);
}

void CUtil::Split(const CStdString& strFileNameAndPath, CStdString& strPath, CStdString& strFileName)
{
	strFileName="";
	strPath="";
	int i=strFileNameAndPath.size()-1;
	while (i > 0)
	{
		char ch=strFileNameAndPath[i];
		if (ch==':' || ch=='/' || ch=='\\') break;
		else i--;
	}
	strPath     = strFileNameAndPath.Left(i);
	strFileName = strFileNameAndPath.Right(strFileNameAndPath.size() - i);
}

int CUtil::GetFolderCount(VECFILEITEMS &items)
{
	int nFolderCount=0;
	for (int i = 0; i < (int)items.size(); i++) 
	{
		CFileItem* pItem = items[i];
		if (pItem->m_bIsFolder) 
			nFolderCount++;
	}

	return nFolderCount;
}

bool CUtil::ThumbExists(const CStdString& strFileName, bool bAddCache)
{
	return CThumbnailCache::GetThumbnailCache()->ThumbExists(strFileName, bAddCache);
}

void CUtil::ThumbCacheAdd(const CStdString& strFileName, bool bFileExists)
{
	CThumbnailCache::GetThumbnailCache()->Add(strFileName, bFileExists);
}

void CUtil::ThumbCacheClear()
{
	CThumbnailCache::GetThumbnailCache()->Clear();
}

bool CUtil::ThumbCached(const CStdString& strFileName)
{
	return CThumbnailCache::GetThumbnailCache()->IsCached(strFileName);
}

void CUtil::PlayDVD()
{
  if (g_stSettings.m_szExternalDVDPlayer[0])
  {
    char szPath[1024];
    char szDevicePath[1024];
    char szXbePath[1024];
	  char szParameters[1024];
    memset(szParameters,0,sizeof(szParameters));
    strcpy(szPath,g_stSettings.m_szExternalDVDPlayer);
    char* szBackslash = strrchr(szPath,'\\');
		*szBackslash=0x00;
		char* szXbe = &szBackslash[1];
		char* szColon = strrchr(szPath,':');
		*szColon=0x00;
		char* szDrive = szPath;
		char* szDirectory = &szColon[1];
    CIoSupport helper;
		helper.GetPartition( (LPCSTR) szDrive, szDevicePath);
		strcat(szDevicePath,szDirectory);
		wsprintf(szXbePath,"d:\\%s",szXbe);

    g_application.Stop();
    CUtil::LaunchXbe(szDevicePath,szXbePath,NULL);
  }
  else
  {
    CIoSupport helper;
    helper.Remount("D:","Cdrom0");
    g_application.PlayFile("dvd://1");
  }
}

DWORD CUtil::SetUpNetwork( bool resetmode, struct network_info& networkinfo )
{
  static unsigned char params[512]; 
  static DWORD				vReturn;
  static XNADDR				sAddress;
	char temp_str[64];
	static char mode = 0;
	XNADDR xna;
	DWORD dwState;

	if( resetmode )
	{
		resetmode = 0;
		mode = 0;
	}

	if( !XNetGetEthernetLinkStatus() )
	{
		return 1;
	}

	if( mode == 100 )
	{
    CLog::Log("  pending...");
		dwState = XNetGetTitleXnAddr(&xna);
		if(dwState==XNET_GET_XNADDR_PENDING)
			return 1;
		mode = 5;
		char	azIPAdd[256];
		memset( azIPAdd,0,256);
		XNetInAddrToString(xna.ina,azIPAdd,32);
    CLog::Log("ip adres:%s",azIPAdd);
	}

	// if local address is specified 
	if( mode == 0 )
	{
		if ( !networkinfo.DHCP )
		{
			TXNetConfigParams configParams;   

			XNetLoadConfigParams( (LPBYTE) &configParams );
			BOOL bXboxVersion2 = (configParams.V2_Tag == 0x58425632 );	// "XBV2"
			BOOL bDirty = FALSE;

			if (bXboxVersion2)
			{
				if (configParams.V2_IP != inet_addr(networkinfo.ip))
				{
					configParams.V2_IP = inet_addr(networkinfo.ip);
					bDirty = TRUE;
				}
			}
			else
			{
				if (configParams.V1_IP != inet_addr(networkinfo.ip))
				{
					configParams.V1_IP = inet_addr(networkinfo.ip);
					bDirty = TRUE;
				}
			}

			if (bXboxVersion2)
			{
				if (configParams.V2_Subnetmask != inet_addr(networkinfo.subnet))
				{
					configParams.V2_Subnetmask = inet_addr(networkinfo.subnet);
					bDirty = TRUE;
				}
			}
			else
			{
				if (configParams.V1_Subnetmask != inet_addr(networkinfo.subnet))
				{
					configParams.V1_Subnetmask = inet_addr(networkinfo.subnet);
					bDirty = TRUE;
				}
			}

			if (bXboxVersion2)
			{
				if (configParams.V2_Defaultgateway != inet_addr(networkinfo.gateway))
				{
					configParams.V2_Defaultgateway = inet_addr(networkinfo.gateway);
					bDirty = TRUE;
				}
			}
			else
			{
				if (configParams.V1_Defaultgateway != inet_addr(networkinfo.gateway))
				{
					configParams.V1_Defaultgateway = inet_addr(networkinfo.gateway);
					bDirty = TRUE;
				}
			}

			if (bXboxVersion2)
			{
				if (configParams.V2_DNS1 != inet_addr(networkinfo.DNS1))
				{
					configParams.V2_DNS1 = inet_addr(networkinfo.DNS1);
					bDirty = TRUE;
				}
			}
			else
			{
				if (configParams.V1_DNS1 != inet_addr(networkinfo.DNS1))
				{
					configParams.V1_DNS1 = inet_addr(networkinfo.DNS1);
					bDirty = TRUE;
				}
			}

			if (bXboxVersion2)
			{
				if (configParams.V2_DNS2 != inet_addr(networkinfo.DNS2))
				{
					configParams.V2_DNS2 = inet_addr(networkinfo.DNS2);
					bDirty = TRUE;
				}
			}
			else
			{
				if (configParams.V1_DNS2 != inet_addr(networkinfo.DNS2))
				{
					configParams.V1_DNS2 = inet_addr(networkinfo.DNS2);
					bDirty = TRUE;
				}
			}

			if (configParams.Flag != (0x04|0x08) )
			{
				configParams.Flag = 0x04 | 0x08;
				bDirty = TRUE;
			}

			XNetSaveConfigParams( (LPBYTE) &configParams );

			XNetStartupParams xnsp;
			memset(&xnsp, 0, sizeof(xnsp));
			xnsp.cfgSizeOfStruct = sizeof(XNetStartupParams);

			// Bypass security so that we may connect to 'untrusted' hosts
			xnsp.cfgFlags = XNET_STARTUP_BYPASS_SECURITY;
		// create more memory for networking
			xnsp.cfgPrivatePoolSizeInPages = 128; // == 256kb, default = 12 (48kb)
			xnsp.cfgEnetReceiveQueueLength = 64; // == 128kb, default = 8 (16kb)
			xnsp.cfgIpFragMaxSimultaneous = 64; // default = 4
			xnsp.cfgIpFragMaxPacketDiv256 = 64; // == 8kb, default = 8 (2kb)
			xnsp.cfgSockMaxSockets = 64; // default = 64
			xnsp.cfgSockDefaultRecvBufsizeInK = 128; // default = 16
			xnsp.cfgSockDefaultSendBufsizeInK = 128; // default = 16
      CLog::Log("requesting local ip adres");
			int err = XNetStartup(&xnsp);
		  mode = 100;
			return 1;
		}
		else
		{
	    /**     Set DHCP-flags from a known DHCP mode  (maybe some day we will fix this)  **/
			XNetLoadConfigParams(params);
			memset( params, 0, (sizeof(IN_ADDR) * 5) + 20 );
			params[40]=33;	params[41]=223;	params[42]=196;	params[43]=67;	params[44]=6;	
			params[45]=145;	params[46]=157;	params[47]=118;	params[48]=182;	params[49]=239;	
			params[50]=68;	params[51]=197;	params[52]=133;	params[53]=150;	params[54]=118;	
			params[55]=211;	params[56]=38;	params[57]=87;	params[58]=222;	params[59]=119;		
			params[64]=0;	params[72]=0;	params[73]=0;	params[74]=0;	params[75]=0;
			params[340]=160; 		params[341]=93;				params[342]=131;			params[343]=191;			params[344]=46;	

			XNetStartupParams xnsp;
	
			memset(&xnsp, 0, sizeof(xnsp));
			xnsp.cfgSizeOfStruct = sizeof(XNetStartupParams);
			// Bypass security so that we may connect to 'untrusted' hosts
			xnsp.cfgFlags = XNET_STARTUP_BYPASS_SECURITY;
 
			xnsp.cfgPrivatePoolSizeInPages = 128; // == 256kb, default = 12 (48kb)
			xnsp.cfgEnetReceiveQueueLength = 64; // == 32kb, default = 8 (16kb)
			xnsp.cfgIpFragMaxSimultaneous = 64; // default = 4
			xnsp.cfgIpFragMaxPacketDiv256 = 64; // == 8kb, default = 8 (2kb)
			xnsp.cfgSockMaxSockets = 64; // default = 64
			xnsp.cfgSockDefaultRecvBufsizeInK = 128; // default = 16
			xnsp.cfgSockDefaultSendBufsizeInK = 128; // default = 16

			XNetSaveConfigParams(params);
      CLog::Log("requesting DHCP");
			int err = XNetStartup(&xnsp);
			mode = 5;
		}


	}

	char g_szTitleIP[32];

	WSADATA WsaData;
	int err;

	char ftploop;
	if( mode == 5 )
	{
		XNADDR xna;
		DWORD dwState;
		
		dwState = XNetGetTitleXnAddr(&xna);

		if( dwState==XNET_GET_XNADDR_PENDING) 
			return 1;

		XNetInAddrToString(xna.ina,g_szTitleIP,32);
		err = WSAStartup( MAKEWORD(2,2), &WsaData );
		ftploop = 1;
		mode ++;
	}

	if( mode == 6 )
	{
		vReturn = XNetGetTitleXnAddr(&sAddress);


		ftploop = 1;

		if( vReturn != XNET_GET_XNADDR_PENDING ) 
		{
			char	azIPAdd[256];
			//char	azMessage[256];

			memset(azIPAdd,0,sizeof(azIPAdd));

			XNetInAddrToString(sAddress.ina,azIPAdd,sizeof(azIPAdd));
			//strcpy(NetworkStatus,azIPAdd);
			//strcpy( NetworkStatusInternal, NetworkStatus );
			{
				DWORD temp = XNetGetEthernetLinkStatus();
				if(  temp & XNET_ETHERNET_LINK_ACTIVE )
				{

					if ( temp & XNET_ETHERNET_LINK_FULL_DUPLEX )
					{
            // full duplex
            CLog::Log("  full duplex");
					}

					if ( temp & XNET_ETHERNET_LINK_HALF_DUPLEX )
					{
						// half duplex
            CLog::Log("  half duplex");
					}

					if ( temp & XNET_ETHERNET_LINK_100MBPS )
					{
            CLog::Log("  100 mbps");
					}

					if ( temp & XNET_ETHERNET_LINK_10MBPS )
					{
            CLog::Log("  10bmps");
					}

					if ( vReturn &  XNET_GET_XNADDR_STATIC )
					{
            CLog::Log("  static ip");							
					}

					if ( vReturn &  XNET_GET_XNADDR_DHCP )
					{
            CLog::Log("  Dynamic IP");
					}

					if ( vReturn & XNET_GET_XNADDR_DNS )
					{
            CLog::Log("  DNS");
					}

					if ( vReturn & XNET_GET_XNADDR_ETHERNET )
					{
						CLog::Log("  ethernet");
					}

					if ( vReturn & XNET_GET_XNADDR_NONE )
					{
						CLog::Log("  none");
					}

					if ( vReturn & XNET_GET_XNADDR_ONLINE )
					{
						CLog::Log("  online");
					}

					if ( vReturn & XNET_GET_XNADDR_PENDING )
					{
						CLog::Log("  pending");
					}

					if ( vReturn & XNET_GET_XNADDR_TROUBLESHOOT )
					{
						CLog::Log("  error");
					}

					if ( vReturn & XNET_GET_XNADDR_PPPOE )
					{
						CLog::Log("  ppoe");
					}

					sprintf(temp_str,"  IP: %s",azIPAdd);					
          CLog::Log(temp_str);
				}
				if( !strstr(azIPAdd,"0.0.0.0"))
				{
					ftploop = 0;
					mode ++;
					Sleep(1000);
					return 0;
				}
			}
		}

		Sleep(50); 
		return 1;
	}
	return 1;
}

void CUtil::GetVideoThumbnail(const CStdString& strIMDBID, CStdString& strThumb)
{
  strThumb.Format("%s\\imdb\\imdb%s.jpg",g_stSettings.szThumbnailsDirectory,strIMDBID.c_str());
}

void CUtil::SetMusicThumbs(VECFILEITEMS &items)
{
  for (int i=0; i < (int)items.size(); ++i)
  {
    CFileItem* pItem=items[i];
		SetMusicThumb(pItem);
  }
}

void CUtil::SetMusicThumb(CFileItem* pItem)
{
	CStdString strThumb;
	CStdString strAlbum=pItem->m_musicInfoTag.GetAlbum();
	//	Set album thumb for file (or directory in albumwindow)
	if (!strAlbum.IsEmpty())
	{
		CStdString strPath, strFileName;
		if (!pItem->m_bIsFolder)
			CUtil::Split(pItem->m_strPath, strPath, strFileName);
		else
			strPath=pItem->m_strPath;
		// look for a permanent thumb (Q:\albums\thumbs)
		CUtil::GetAlbumThumb(strAlbum+strPath,strThumb);
		if (CUtil::ThumbExists(strThumb,true) )
		{
			pItem->SetIconImage(strThumb);
			pItem->SetThumbnailImage(strThumb);
		}
		else 
		{
			// look for a temporary thumb (Q:\albums\thumbs\temp)
			CUtil::GetAlbumThumb(strAlbum+strPath,strThumb, true);
			if (CUtil::ThumbExists(strThumb, true) )
			{
				pItem->SetIconImage(strThumb);
				pItem->SetThumbnailImage(strThumb);
			}
			else if (pItem->m_bIsFolder) //	fill thumb for directory in albumwindow
			{
				CUtil::AddFileToFolder(pItem->m_strPath, "folder.jpg", strThumb);
				if (CUtil::ThumbExists(strThumb, true))
				{
					//	We found a folder.jpg
					CStdString strFolderThumb;
					CUtil::GetAlbumThumb(strThumb,strFolderThumb, true);
					if (!CUtil::ThumbExists(strFolderThumb))
					{
						// resize it and save it to the temp thumb dir
						CPicture pic;
						if (pic.CreateAlbumThumbnail(strThumb, strThumb))
						{
							strThumb=strFolderThumb;
							CUtil::ThumbCacheAdd(strThumb, true);
						}
					}
					else
					{
						strThumb=strFolderThumb;
					}
					pItem->SetIconImage(strThumb);
					pItem->SetThumbnailImage(strThumb);
				}
				else
				{
					strThumb="music.jpg";
					pItem->SetIconImage("music.jpg");
				}
			}
		}
	}
	//	do we have a normal folder
	if (strAlbum.IsEmpty() && pItem->m_bIsFolder && pItem->GetLabel()!="..")
	{
		// Check for folder.jpg
		CUtil::AddFileToFolder(pItem->m_strPath, "folder.jpg", strThumb);
		if (CUtil::ThumbExists(strThumb, true))
		{
			//	We found a folder.jpg
			CStdString strFolderThumb;
			CUtil::GetAlbumThumb(strThumb,strFolderThumb, true);
			if (!CUtil::ThumbExists(strFolderThumb))
			{
				// resize it and save it to the temp thumb dir
				CPicture pic;
				if (pic.CreateAlbumThumbnail(strThumb, strThumb))
				{
					strThumb=strFolderThumb;
					CUtil::ThumbCacheAdd(strThumb, true);
					pItem->SetThumbnailImage(strThumb);
				}
			}
			else
			{
					pItem->SetThumbnailImage(strThumb);
			}
		}
	}
}
