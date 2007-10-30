
#include "stdafx.h"
#include "DirectoryTuxBox.h"
#include "DirectoryCache.h"
#include "../Util.h"
#include "FileCurl.h"
#include "../utils/HttpHeader.h"
#include "../utils/HTTP.h"
#include "../utils/TuxBoxUtil.h"

using namespace XFILE;
using namespace DIRECTORY;

CDirectoryTuxBox::CDirectoryTuxBox(void)
{
}

CDirectoryTuxBox::~CDirectoryTuxBox(void)
{
}
bool CDirectoryTuxBox::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  // Detect and delete slash at end
  CStdString strRoot = strPath;
  if (CUtil::HasSlashAtEnd(strRoot))
  strRoot.Delete(strRoot.size() - 1);

  // Is our Directory Cached? 
  if (g_directoryCache.GetDirectory(strRoot, items))
    return true;

  //Get the request strings
  CStdString strBQRequest;
  CStdString strXMLRootString;
  CStdString strXMLChildString;
  if(!GetRootAndChildString(strRoot, strBQRequest, strXMLRootString, strXMLChildString))
    return false;
  
  // display progress dialog after 1 seconds
  DWORD dwTimeStamp = GetTickCount();//+ 1000;
  CGUIDialogProgress* dlgProgress = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
  bool dialogopen = false;
  int iProgressPercent = 0;
  if (dlgProgress)
  {
    dlgProgress->SetHeading(21331);
    dlgProgress->SetLine(0, 14004);
    dlgProgress->StartModal();
    dlgProgress->ShowProgressBar(true);
    dlgProgress->SetPercentage(iProgressPercent);
    dlgProgress->Progress();
  }

  //Set url Protocol
  CURL url(strRoot);
  CStdString strFilter;
  CStdString protocol = url.GetProtocol();
  CStdString strOptions = url.GetOptions();
  url.SetProtocol("http");
  bool bIsBouquet=false;
  
  int ipoint = strOptions.Find("?path=");
  if (ipoint >=0)
  {
    // send Zap!
    return g_tuxbox.ZapToUrl(url, strOptions, ipoint);
  }
  else
  {
    ipoint = strOptions.Find("&reference="); 
    if (ipoint >=0)
    {
      //List reference
      strFilter = strOptions.Right((strOptions.size()-(ipoint+11)));
      bIsBouquet = false; //On Empty is Bouquet
    }
  }
  if(strFilter.IsEmpty())
  {
    url.SetOptions(strBQRequest);
    bIsBouquet = true;
  }
  //Open 
  CFileCurl http;
  int iTryConnect =0;
  int iWaitTimer = 20;
  bool result;
  
  // Update Progress
  CStdString strLine1, strLine2;
  strLine1.Format(g_localizeStrings.Get(21336).c_str(), g_localizeStrings.Get(21337).c_str());
  iProgressPercent=iProgressPercent+5;
  UpdateProgress(dlgProgress, strLine1, "", iProgressPercent, false);
  
  while (iTryConnect <= 1 && !dlgProgress->IsCanceled())
  {
    //Update Progressbar
    iProgressPercent=iProgressPercent+5;
    strLine2.Format("Opening %s",url.GetHostName().c_str()); //Connecting to host
    UpdateProgress(dlgProgress, strLine1, strLine2, iProgressPercent, false);
    
    http.SetTimeout(iWaitTimer);
    if(http.Open(url, false)) 
    {
      //We are connected!
      iTryConnect = 4;

      //Update Progressbar
      iProgressPercent=iProgressPercent+5;
      UpdateProgress(dlgProgress, strLine1, g_localizeStrings.Get(21337).c_str(), iProgressPercent, false);
      
      // restore protocol
      url.SetProtocol(protocol);
      
      int size_read = 0;  
      int size_total = (int)http.GetLength();
      int data_size = 0;
      CStdString data;
      data.reserve(size_total);
      
      // read response from server into string buffer
      char buffer[16384];
      while( ((size_read = http.Read(buffer, sizeof(buffer)-1)) > 0) && !dlgProgress->IsCanceled() )
      {
        buffer[size_read] = 0;
        data += buffer;
        data_size += size_read;
        
        //Update Progressbar
        iProgressPercent=iProgressPercent+1;
        UpdateProgress(dlgProgress, strLine1, g_localizeStrings.Get(21337).c_str(), iProgressPercent, false);

      }
      //Update Progressbar
      if (dlgProgress->IsCanceled())
      {
        dlgProgress->Close();
        return false;
      }
      iProgressPercent=iProgressPercent+5;
      UpdateProgress(dlgProgress, strLine1, g_localizeStrings.Get(14005).c_str(), iProgressPercent, false);
      
      // parse returned xml
      TiXmlDocument doc;
      data.Replace("></",">-</"); //FILL EMPTY ELEMENTS WITH "-"!
      doc.Parse(data.c_str());
      TiXmlElement *root = doc.RootElement();
      if(root == NULL)
      {
        CLog::Log(LOGERROR, __FUNCTION__" - Unable to parse xml");
        CLog::Log(LOGDEBUG, __FUNCTION__" - Sample follows...\n%s", data.c_str());
        dlgProgress->Close();
        return false;
      }
      if( strXMLRootString.Equals(root->Value()) && bIsBouquet)
      {
        //Update Progressbar
        iProgressPercent=iProgressPercent+5;
        UpdateProgress(dlgProgress, strLine1, g_localizeStrings.Get(14005).c_str(), iProgressPercent, false);
      
        data.Empty();
        result = g_tuxbox.ParseBouquets(root, items, url, strFilter, strXMLChildString);
      }
      else if( strXMLRootString.Equals(root->Value()) && !strFilter.IsEmpty() )
      {
        //Update Progressbar
        iProgressPercent=iProgressPercent+5;
        UpdateProgress(dlgProgress, strLine1, g_localizeStrings.Get(14005).c_str(), iProgressPercent, false);
        
        result = g_tuxbox.ParseChannels(root, items, url, strFilter, strXMLChildString);
      }
      else
      {
        CLog::Log(LOGERROR, __FUNCTION__" - Invalid root xml element for TuxBox");
        CLog::Log(LOGDEBUG, __FUNCTION__" - Sample follows...\n%s", data.c_str);
        data.Empty();
        result = false;
      }

      if (url.GetPort()!=0 && url.GetPort()!=80)
      {
        //strRoot.Format("tuxbox://%s:%s@%s:%i",url.GetUserName(),url.GetPassWord(),url.GetHostName(),url.GetPort());
      }
      else 
      {
        //strRoot.Format("tuxbox://%s:%s@%s",url.GetUserName(),url.GetPassWord(),url.GetHostName());
      }

      //Build Directory
      CFileItemList vecCacheItems;
      g_directoryCache.ClearDirectory(strRoot);
      for( int i = 0; i <items.Size(); i++ )
      {
        CFileItem* pItem=items[i];
        if (!pItem->IsParentFolder())
          vecCacheItems.Add(new CFileItem( *pItem ));
        //Update Progressbar
        iProgressPercent=iProgressPercent+2;
        UpdateProgress(dlgProgress, strLine1, g_localizeStrings.Get(14005).c_str(), iProgressPercent, false);
      
      }
      g_directoryCache.SetDirectory(strRoot, vecCacheItems);
      //Close Progressbar
      UpdateProgress(dlgProgress, strLine1, g_localizeStrings.Get(14005).c_str(), 100, true);
    }
    else
    {
      //Update Progressbar
      strLine2.Format(g_localizeStrings.Get(13329).c_str(), url.GetHostName().c_str()); 
      iProgressPercent=iProgressPercent+5;
      UpdateProgress(dlgProgress, strLine1, strLine2, iProgressPercent, false);
      
      CLog::Log(LOGERROR, __FUNCTION__" - Unable to get XML structure! Try count:%i, Wait Timer:%is",iTryConnect, iWaitTimer);
      iTryConnect++;
      iWaitTimer = iWaitTimer+10;
      result = false;
    }
    if (dlgProgress->IsCanceled())
    {
      dlgProgress->Close();
      return false;
    }
  }
  //Close Progressbar
  UpdateProgress(dlgProgress, strLine1, "Closing connection", 100, true);
  
  return result;
}
bool CDirectoryTuxBox::GetRootAndChildString(const CStdString strPath, CStdString& strBQRequest, CStdString& strXMLRootString, CStdString& strXMLChildString )
{
  //Advanced Settings: RootMode! Movies: 
  if(g_advancedSettings.m_iTuxBoxDefaultRootMenu == 3) //Movies! Fixed-> mode=3&submode=4
  {
    CLog::Log(LOGERROR, __FUNCTION__" - Default defined RootMenu : (3) Movies");
    strBQRequest = "xml/services?mode=3&submode=4"; 
    strXMLRootString.Format("movies");
    strXMLChildString.Format("service");
  }
  else if(g_advancedSettings.m_iTuxBoxDefaultRootMenu <= 0 || g_advancedSettings.m_iTuxBoxDefaultRootMenu == 1 ||
    g_advancedSettings.m_iTuxBoxDefaultRootMenu > 4 )
  {
    //Falling Back to the Default RootMenu => 0 Bouquets
    if(g_advancedSettings.m_iTuxBoxDefaultRootMenu < 0 || g_advancedSettings.m_iTuxBoxDefaultRootMenu > 4)
    {
      g_advancedSettings.m_iTuxBoxDefaultRootMenu = 0;
    }

    //Advanced Settings: SubMenu!
    if(g_advancedSettings.m_bTuxBoxSubMenuSelection)
    {
      CLog::Log(LOGDEBUG, __FUNCTION__" SubMenu Channel Selection is Enabled! Requesting Submenu!");
      // DeActivated: Timing Problems, bug in TuxBox.. etc.!
      bool bReqMoRe = true;
      // Detect the RootMode !
      if (strPath.Find("?mode=")>=0)
      {
        CStdString strMode;
        bReqMoRe=false;
        strMode = g_tuxbox.DetectSubMode(strPath, strXMLRootString, strXMLChildString);
      }
      if(bReqMoRe)
      {
        //PopUp Context and Request SubMode with root and child string
        strBQRequest = g_tuxbox.GetSubMode(g_advancedSettings.m_iTuxBoxDefaultRootMenu, strXMLRootString, strXMLChildString);
        if(strBQRequest.IsEmpty())
        {
          strBQRequest = "xml/services?mode=0&submode=4"; //Bouquets
          strXMLRootString.Format("bouquets");
          strXMLChildString.Format("bouquet");
        }
      }
    }
    else
    {
      //Advanced Settings: Set Default Subemnu
      if(g_advancedSettings.m_iTuxBoxDefaultSubMenu == 1)
      {
        CLog::Log(LOGDEBUG, __FUNCTION__" - Default defined SubMenu : (1) Services");
        strBQRequest = "xml/services?mode=0&submode=1"; //Services
        strXMLRootString.Format("services");
        strXMLChildString.Format("service");
      }
      else if(g_advancedSettings.m_iTuxBoxDefaultSubMenu == 2)
      {
        CLog::Log(LOGERROR, __FUNCTION__" - Default defined SubMenu : (2) Satellites");
        strBQRequest = "xml/services?mode=0&submode=2"; //Satellites
        strXMLRootString.Format("satellites");
        strXMLChildString.Format("satellite");
      }
      else if(g_advancedSettings.m_iTuxBoxDefaultSubMenu == 3)
      {
        CLog::Log(LOGERROR, __FUNCTION__" - Default defined SubMenu : (3) Providers");
        strBQRequest = "xml/services?mode=0&submode=3"; //Providers
        strXMLRootString.Format("providers");
        strXMLChildString.Format("provider");
      }
      else
      {
        CLog::Log(LOGERROR, __FUNCTION__" - Default defined SubMenu : (4) Bouquets");
        strBQRequest = "xml/services?mode=0&submode=4"; //Bouquets
        strXMLRootString.Format("bouquets");
        strXMLChildString.Format("bouquet");
      }
    }
  }
  if(strBQRequest.IsEmpty() || strXMLRootString.IsEmpty() || strXMLChildString.IsEmpty())
    return false;
  else
    return true;
}
bool CDirectoryTuxBox::UpdateProgress(CGUIDialogProgress* dlgProgress, CStdString strLn1, CStdString strLn2, int iPercent, bool bCLose)
{
  if (strLn1.IsEmpty()) strLn1 = "";
  if (strLn2.IsEmpty()) strLn2 = "";

  if(dlgProgress)
  {
    if (bCLose)
    {
      dlgProgress->Close();
      return true;
    }

    dlgProgress->SetLine(1, strLn1);
    dlgProgress->SetLine(2, strLn2);
    dlgProgress->SetPercentage(iPercent);
    dlgProgress->Progress();
    return true;
  }
  return false;
}