
#include "../stdafx.h"
#include "DreamboxDirectory.h"
#include "directorycache.h"
#include "../util.h"
#include "FileCurl.h"
#include "../utils/HttpHeader.h"
#include "../utils/Http.h"
#include "../utils/TuxBoxUtil.h"

CDreamboxDirectory::CDreamboxDirectory(void)
{
}

CDreamboxDirectory::~CDreamboxDirectory(void)
{
}
bool CDreamboxDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  CStdString strBQRequest;
  CStdString strXMLRootString;
  CStdString strXMLChildString;
  
  //Advanced Settings: RootMode! Movies: 
  if(g_advancedSettings.m_iTuxBoxDefaultRootMenu == 3) //Movies! Fixed-> mode=3&submode=4
  {
    CLog::Log(LOGERROR, __FUNCTION__" - Default defined RootMenu : (3) Movies");
    strBQRequest = "xml/services?mode=3&submode=4"; 
    strXMLRootString.Format("movies");
    strXMLChildString.Format("service");
  }
  else if(g_advancedSettings.m_iTuxBoxDefaultRootMenu <= 0 || g_advancedSettings.m_iTuxBoxDefaultRootMenu == 1 || g_advancedSettings.m_iTuxBoxDefaultRootMenu > 4 )
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
        CLog::Log(LOGERROR, __FUNCTION__" - Default defined SubMenu : (1) Services");
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

  
  
  
  
  // Detect and delete slash at end
  CStdString strRoot = strPath;
  if (CUtil::HasSlashAtEnd(strRoot))
  {
    strRoot.Delete(strRoot.size() - 1);
  }

  // init Directory
  if (g_directoryCache.GetDirectory(strRoot, items))
  {
    return true;
  }

  // display progress dialog after 1 seconds
  DWORD dwTimeStamp = GetTickCount();//+ 1000;
  CGUIDialogProgress* dlgProgress = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
  bool dialogopen = false;
  int iProgressPercent = 0;
  if (dlgProgress)
  {
    dlgProgress->SetHeading(20330); //Localize, no need!
    dlgProgress->SetLine(0, 14004);
    dlgProgress->SetLine(1, "");
    dlgProgress->SetLine(2, "");
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
  CStdString strDlgTmp;
  strDlgTmp.Format(g_localizeStrings.Get(20335).c_str(), g_localizeStrings.Get(20336).c_str());
  if(dlgProgress)
  {
    iProgressPercent=iProgressPercent+5;
    dlgProgress->SetLine(1, strDlgTmp);
    dlgProgress->SetPercentage(iProgressPercent);
    dlgProgress->Progress();
  }
  //

  while (iTryConnect <= 1 && !dlgProgress->IsCanceled())
  {
    //DLG: Update Progress
    iProgressPercent=iProgressPercent+5;
    strDlgTmp.Format("Opening %s",url.GetHostName().c_str());
    dlgProgress->SetLine(2, strDlgTmp); //Connecting to host
    dlgProgress->SetPercentage(iProgressPercent);
    dlgProgress->Progress();
    //

    if(http.Open(url, false, iWaitTimer)) 
    {
      //DLG: Update Progress
      iProgressPercent=iProgressPercent+5;
      dlgProgress->SetLine(2, 14004);
      dlgProgress->SetPercentage(iProgressPercent);
      dlgProgress->Progress();
      //
      iTryConnect = 4; //We are connected!

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
        if(dialogopen)
        {
          //DLG: Update Progress
          iProgressPercent=iProgressPercent+1;
          dlgProgress->SetPercentage(iProgressPercent);
          dlgProgress->Progress();
        }
        else if(GetTickCount() > dwTimeStamp)
        {
          //DLG: Update Progress
          iProgressPercent=iProgressPercent+1;
          dlgProgress->SetPercentage(iProgressPercent);
          dlgProgress->Progress();
          dialogopen = true;
        }
      }
      if (dlgProgress->IsCanceled())
      {
        dlgProgress->Close();
        return false;
      }

      //DLG: Update Progress
      dlgProgress->SetLine(2, 14005);
      iProgressPercent=iProgressPercent+1;
      dlgProgress->SetPercentage(iProgressPercent);
      dlgProgress->Progress();
      //

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
      if( (strcmp(root->Value(), strXMLRootString.c_str()) == 0) && bIsBouquet)
      {
        //DLG: Update Progress
        iProgressPercent=iProgressPercent+5;
        dlgProgress->SetPercentage(iProgressPercent);
        dlgProgress->Progress();
        //
        data.Empty();
        result = g_tuxbox.ParseBouquets(root, items, url, strFilter, strXMLChildString);
      }
      else if( (strcmp(root->Value(), strXMLRootString.c_str()) == 0) && !strFilter.IsEmpty())
      {
        //DLG: Update Progress
        iProgressPercent=iProgressPercent+5;
        dlgProgress->SetPercentage(iProgressPercent);
        dlgProgress->Progress();
        //
        result = g_tuxbox.ParseChannels(root, items, url, strFilter, strXMLChildString);
      }
      else
      {
        dlgProgress->Progress();
        CLog::Log(LOGERROR, __FUNCTION__" - Invalid root xml element for TuxBox");
        CLog::Log(LOGDEBUG, __FUNCTION__" - Sample follows...\n%s", data.c_str);
        data.Empty();
        result = false;
      }

      if (url.GetPort()!=0 && url.GetPort()!=80)
      {
        strRoot.Format("tuxbox://%s:%s@%s:%i",url.GetUserName(),url.GetPassWord(),url.GetHostName(),url.GetPort());
      }
      else 
      {
        strRoot.Format("tuxbox://%s:%s@%s",url.GetUserName(),url.GetPassWord(),url.GetHostName());
      }

      dlgProgress->Progress();
      //Build Directory
      CFileItemList vecCacheItems;
      g_directoryCache.ClearDirectory(strRoot);
      for( int i = 0; i <items.Size(); i++ )
      {
        CFileItem* pItem=items[i];
        if (!pItem->IsParentFolder())
          vecCacheItems.Add(new CFileItem( *pItem ));
        dlgProgress->Progress();
      }
      g_directoryCache.SetDirectory(strRoot, vecCacheItems);
      if (dlgProgress) dlgProgress->Close();
    }
    else
    {
       // Update Progress
      if(dlgProgress)
      {
        strDlgTmp.Format(g_localizeStrings.Get(13329).c_str(), url.GetHostName().c_str()); 
        dlgProgress->SetLine(2, strDlgTmp);
        dlgProgress->SetPercentage(iProgressPercent+10);
        dlgProgress->Progress();
      }
      //
      CLog::Log(LOGERROR, __FUNCTION__" - Unable to get XML structure! Try count:%i, Wait Timer:%is",iTryConnect, iWaitTimer);
      iTryConnect++;
      iWaitTimer = iWaitTimer+10;
      result = false;
    }
  }
  if (dlgProgress)
  {
    dlgProgress->SetLine(2, "Closing connection");
    dlgProgress->SetPercentage(100);
    dlgProgress->Close();
  }
  return result;

}