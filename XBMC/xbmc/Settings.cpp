#include "settings.h"
#include "util.h"
#include "localizestrings.h"
#include "stdstring.h"
using namespace std;

class CSettings g_settings;
struct CSettings::stSettings g_stSettings;

CSettings::CSettings(void)
{
  m_iSlideShowTransistionFrames=25;
  m_iSlideShowStayTime=3000;
	g_stSettings.dwFileVersion =CONFIG_VERSION;
	g_stSettings.m_bMyProgramsViewAsIcons=false;
	g_stSettings.m_bMyProgramsSortAscending=true;
	g_stSettings.m_bMyProgramsSortMethod=0;
	g_stSettings.m_bMyProgramsFlatten=false;
  strcpy(g_stSettings.szDashboard,"C:\\xboxdash.xbe");
  g_stSettings.m_iStartupWindow=0;
  g_stSettings.m_bFTPServer=false;
  strcpy(g_stSettings.m_strLocalIPAdres,"");
  strcpy(g_stSettings.m_strLocalNetmask,"");
  strcpy(g_stSettings.m_strGateway,"");
	strcpy(g_stSettings.m_strNameServer,"");
	strcpy(g_stSettings.m_strTimeServer,"");
  strcpy(g_stSettings.szDefaultSkin,"MediaCenter");
	g_stSettings.m_bMyPicturesViewAsIcons=false;
	g_stSettings.m_bMyPicturesSortAscending=true;
	g_stSettings.m_bMyPicturesSortMethod=0;

	g_stSettings.m_iMoveDelayIR=220;
	g_stSettings.m_iRepeatDelayIR=220;
	g_stSettings.m_iMoveDelayController=220;
	g_stSettings.m_iRepeatDelayController=220;
	strcpy(g_stSettings.m_szMyPicturesExtensions,".bmp|.jpg|.png|.gif|.pcx|.tif|.jpeg");
}

CSettings::~CSettings(void)
{
}


void CSettings::Save() const
{
	FILE* systemSettings = fopen("T:\\system.bin","wb+");
	if (systemSettings!=NULL)
	{
		fwrite(&g_stSettings,sizeof(g_stSettings),1,systemSettings);
		fclose(systemSettings);
	}
}

void CSettings::Load()
{
  struct CSettings::stSettings settings;
	FILE* systemSettings = fopen("T:\\system.bin","rb");
	if (systemSettings!=NULL)
	{
		fread(&settings,sizeof(settings),1,systemSettings);
		fclose(systemSettings);
		if (settings.dwFileVersion==CONFIG_VERSION) 
    {

		  FILE* systemSettings = fopen("T:\\system.bin","rb");
		  fread(&g_stSettings,sizeof(g_stSettings),1,systemSettings);
		  fclose(systemSettings);
    }
	}

	// load xml file...
	CStdString strXMLFile;
	CUtil::GetHomePath(strXMLFile);
	strXMLFile+="\\XboxMediaCenter.xml";

	TiXmlDocument xmlDoc;
  if ( !xmlDoc.LoadFile( strXMLFile.c_str() ) ) return;

	TiXmlElement* pRootElement =xmlDoc.RootElement();
  CStdString strValue=pRootElement->Value();
	if ( strValue != "xboxmediacenter") return ;

	TiXmlElement* pDelaysElement =pRootElement->FirstChildElement("delays");
	if (pDelaysElement)
	{
		TiXmlElement* pRemoteDelays			=pDelaysElement->FirstChildElement("remote");
		TiXmlElement* pControllerDelays =pDelaysElement->FirstChildElement("controller");
		if (pRemoteDelays)
		{
			g_stSettings.m_iMoveDelayIR=GetInteger(pRemoteDelays, "move");
			g_stSettings.m_iRepeatDelayIR=GetInteger(pRemoteDelays, "repeat");
		}

		if (pControllerDelays)
		{
			g_stSettings.m_iMoveDelayController=GetInteger(pControllerDelays, "move");
			g_stSettings.m_iRepeatDelayController=GetInteger(pControllerDelays, "repeat");
		}
	}

  GetString(pRootElement, "skin", g_stSettings.szDefaultSkin,"MediaCenter");
	GetString(pRootElement, "dashboard", g_stSettings.szDashboard,"C:\\xboxdash.xbe");
	
	GetString(pRootElement, "ipadres", g_stSettings.m_strLocalIPAdres,"192.168.0.174");
	GetString(pRootElement, "netmask", g_stSettings.m_strLocalNetmask,"255.255.255.0");
	GetString(pRootElement, "defaultgateway", g_stSettings.m_strGateway,"192.168.0.1");
	GetString(pRootElement, "nameserver", g_stSettings.m_strNameServer,"192.168.0.1");
	GetString(pRootElement, "timeserver", g_stSettings.m_strTimeServer,"207.46.248.43");
  GetString(pRootElement, "thumbnails",g_stSettings.szThumbnailsDirectory,"");
  GetString(pRootElement, "shortcuts", g_stSettings.m_szShortcutDirectory,"");
	GetString(pRootElement, "pictureextensions", g_stSettings.m_szMyPicturesExtensions,".bmp|.jpg|.png|.gif|.pcx|.tif|.jpeg");
  
	g_stSettings.m_iStartupWindow=GetInteger(pRootElement, "startwindow");
	g_stSettings.m_bFTPServer=GetBoolean(pRootElement,"enabled");

  CStdString strDir;
  strDir=g_stSettings.m_szShortcutDirectory;
  ConvertHomeVar(strDir);
  strcpy( g_stSettings.m_szShortcutDirectory, strDir.c_str() );

  strDir=g_stSettings.szThumbnailsDirectory;
  ConvertHomeVar(strDir);
  strcpy( g_stSettings.szThumbnailsDirectory, strDir.c_str() );



  if (g_stSettings.m_szShortcutDirectory[0])
  {
    
		const WCHAR* szText;
    szText=g_localizeStrings.Get(111).c_str();
    CShare share;
    share.strIcon="";
    share.strPath=g_stSettings.m_szShortcutDirectory;
    CUtil::Unicode2Ansi(szText,share.strName);
     m_vecMyProgramsBookmarks.push_back(share);
  }

  // parse my programs bookmarks...
	GetShares(pRootElement,"myprograms",m_vecMyProgramsBookmarks);
	GetShares(pRootElement,"pictures",m_vecMyPictureShares);
  GetShares(pRootElement,"files",m_vecMyFilesShares);
	

}

void CSettings::ConvertHomeVar(CStdString& strText)
{
	// Replaces first occurence of $HOME with the home directory.
	// "$HOME\bookmarks" becomes for instance "e:\apps\xbmp\bookmarks"

	char szText[1024];
  char szTemp[1024];
  char *pReplace,*pReplace2;

	CStdString strHomePath;
	CUtil::GetHomePath(strHomePath);
	strcpy(szText,strText.c_str());

  pReplace = strstr(szText, "$HOME");

  if (pReplace!=NULL) 
	{
	  pReplace2 = pReplace + sizeof("$HOME")-1;
	  strncpy(szTemp, pReplace2, sizeof("$HOME") - (szText-pReplace2) );
	  strcpy(pReplace, strHomePath.c_str() );
	  strcat(szText, szTemp);
  }
	strText=szText;
}

void CSettings::GetShares(const TiXmlElement* pRootElement, const CStdString& strTagName, VECSHARES& items)
{
	const TiXmlNode *pChild = pRootElement->FirstChild(strTagName.c_str());
	if (pChild)
	{
		pChild = pChild->FirstChild();
		while (pChild>0)
		{
			CStdString strValue=pChild->Value();
			if (strValue=="bookmark")
			{
				const TiXmlNode *pNodeName=pChild->FirstChild("name");
				const TiXmlNode *pPathName=pChild->FirstChild("path");
				const TiXmlNode *pIconNode=pChild->FirstChild("icon");
        const TiXmlNode *pCacheNode=pChild->FirstChild("cache");
				if (pNodeName && pPathName)
				{
          const char* szName=pNodeName->FirstChild()->Value();
          const char* szPath=pPathName->FirstChild()->Value();

					CShare share;
					share.strName=szName;
					share.strPath=szPath;
          share.strIcon="";
					share.m_iBufferSize=0;

          if (pIconNode)
          {
            share.strIcon=pIconNode->FirstChild()->Value();
          }
          
          if (pCacheNode)
          {
            share.m_iBufferSize=atoi( pCacheNode->FirstChild()->Value() );
          }

          
          OutputDebugString("\n");
					ConvertHomeVar(share.strPath);

					items.push_back(share);
				}
			}
		  pChild=pChild->NextSibling();
		}
	}
}

void CSettings::GetString(const TiXmlElement* pRootElement, const CStdString& strTagName, char* szValue, const CStdString& strDefaultValue)
{
	strcpy(szValue,"");
	const TiXmlNode *pChild = pRootElement->FirstChild(strTagName.c_str());
	if (pChild)
	{
		strcpy(szValue,pChild->FirstChild()->Value());
	}
	if (strlen(szValue)==0)
	{
		strcpy(szValue,strDefaultValue.c_str());
	}
}

int CSettings::GetInteger(const TiXmlElement* pRootElement, const CStdString& strTagName)
{
	const TiXmlNode *pChild = pRootElement->FirstChild(strTagName.c_str());
	if (pChild)
	{
		return atoi( pChild->FirstChild()->Value() );
	}
	return 0;
}

bool CSettings::GetBoolean(const TiXmlElement* pRootElement, const CStdString& strTagName)
{
	char szString[128];
	GetString(pRootElement,strTagName,szString,"");
	if ( CUtil::cmpnocase(szString,"enabled")==0 ||
			 CUtil::cmpnocase(szString,"yes")==0 ||
			 CUtil::cmpnocase(szString,"on")==0 ||
			 CUtil::cmpnocase(szString,"true") ) return true;

	return false;
}