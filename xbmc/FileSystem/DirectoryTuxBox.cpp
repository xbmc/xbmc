/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */


#include "stdafx.h"
#include "DirectoryTuxBox.h"
#include "DirectoryCache.h"
#include "Util.h"
#include "FileCurl.h"
#include "utils/HttpHeader.h"
#include "utils/TuxBoxUtil.h"
#include "URL.h"
#include "GUIWindowManager.h"
#include "GUIDialogProgress.h"
#include "tinyXML/tinyxml.h"
#include "Settings.h"
#include "FileItem.h"

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
  // so we know that we have enigma2
  static bool enigma2 = false;
  // Detect and delete slash at end
  CStdString strRoot = strPath;
  if (CUtil::HasSlashAtEnd(strRoot))
  strRoot.Delete(strRoot.size() - 1);

  //Get the request strings
  CStdString strBQRequest;
  CStdString strXMLRootString;
  CStdString strXMLChildString;
  if(!GetRootAndChildString(strRoot, strBQRequest, strXMLRootString, strXMLChildString))
    return false;

  // display progress dialog after 1 seconds
  CGUIDialogProgress* dlgProgress = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
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
    if (ipoint >=0 || enigma2)
    {
      //List reference
      strFilter = strOptions.Right((strOptions.size()-(ipoint+11)));
      bIsBouquet = false; //On Empty is Bouquet
      if (enigma2)
      {
        CStdString strPort;
        strPort.Format(":%i",url.GetPort());
        if (strRoot.Right(strPort.GetLength()) != strPort) // If not root dir, enable Channels
          strFilter = "e2"; // Disable Bouquets for Enigma2

        GetRootAndChildStringEnigma2(strBQRequest, strXMLRootString, strXMLChildString);
        url.SetOptions("");
        url.SetFileName(strBQRequest);
      }
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
  bool result = false;

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
      http.Close();
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
        CLog::Log(LOGERROR, "%s - Unable to parse xml", __FUNCTION__);
        CLog::Log(LOGERROR, "%s - Sample follows...\n%s", __FUNCTION__, data.c_str());
        dlgProgress->Close();
        return false;
      }
      if( strXMLRootString.Equals(root->Value()) && bIsBouquet)
      {
        //Update Progressbar
        iProgressPercent=iProgressPercent+5;
        UpdateProgress(dlgProgress, strLine1, g_localizeStrings.Get(14005).c_str(), iProgressPercent, false);

        data.Empty();
        if (enigma2)
          result = g_tuxbox.ParseBouquetsEnigma2(root, items, url, strFilter, strXMLChildString);
        else
          result = g_tuxbox.ParseBouquets(root, items, url, strFilter, strXMLChildString);
      }
      else if( strXMLRootString.Equals(root->Value()) && !strFilter.IsEmpty() )
      {
        //Update Progressbar
        iProgressPercent=iProgressPercent+5;
        UpdateProgress(dlgProgress, strLine1, g_localizeStrings.Get(14005).c_str(), iProgressPercent, false);

        data.Empty();
        if (enigma2)
          result = g_tuxbox.ParseChannelsEnigma2(root, items, url, strFilter, strXMLChildString);
        else
          result = g_tuxbox.ParseChannels(root, items, url, strFilter, strXMLChildString);
      }
      else
      {
        CLog::Log(LOGERROR, "%s - Invalid root xml element for TuxBox", __FUNCTION__);
        CLog::Log(LOGERROR, "%s - Sample follows...\n%s", __FUNCTION__, data.c_str());
        data.Empty();
        result = false;
      }

      //Build Directory
      for( int i = 0; i <items.Size(); i++ )
      {
        CFileItemPtr pItem=items[i];
        if (!pItem->IsParentFolder())
        //Update Progressbar
        iProgressPercent=iProgressPercent+2;
        UpdateProgress(dlgProgress, strLine1, g_localizeStrings.Get(14005).c_str(), iProgressPercent, false);
      }
      //Close Progressbar
      UpdateProgress(dlgProgress, strLine1, g_localizeStrings.Get(14005).c_str(), 100, true);
    }
    else
    {
      //Update Progressbar
      strLine2.Format(g_localizeStrings.Get(13329).c_str(), url.GetHostName().c_str());
      iProgressPercent=iProgressPercent+5;
      UpdateProgress(dlgProgress, strLine1, strLine2, iProgressPercent, false);

      CLog::Log(LOGERROR, "%s - Unable to get XML structure! Try count:%i, Wait Timer:%is",__FUNCTION__, iTryConnect, iWaitTimer);
      iTryConnect++;
      if (iTryConnect == 2) //try enigma2 instead of enigma1, best entrypoint here i thought
      {
        enigma2 = true;
        GetRootAndChildStringEnigma2(strBQRequest, strXMLRootString, strXMLChildString);
        url.SetOptions("");
        url.SetFileName(strBQRequest);
        iTryConnect = 0;
      }
      iWaitTimer = iWaitTimer+10;
      result = false;
      http.Close(); // Close old connections
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

void CDirectoryTuxBox::GetRootAndChildStringEnigma2(CStdString& strBQRequest, CStdString& strXMLRootString, CStdString& strXMLChildString )
{
  // Allways take getallservices for Enigma2
  strBQRequest = "web/getallservices"; //Bouquets and Channels
  strXMLRootString.Format("e2servicelistrecursive");
  strXMLChildString.Format("e2bouquet");
}

bool CDirectoryTuxBox::GetRootAndChildString(const CStdString strPath, CStdString& strBQRequest, CStdString& strXMLRootString, CStdString& strXMLChildString )
{
  //Advanced Settings: RootMode! Movies:
  if(g_advancedSettings.m_iTuxBoxDefaultRootMenu == 3) //Movies! Fixed-> mode=3&submode=4
  {
    CLog::Log(LOGDEBUG, "%s - Default defined RootMenu : (3) Movies", __FUNCTION__);
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
      CLog::Log(LOGDEBUG, "%s SubMenu Channel Selection is Enabled! Requesting Submenu!", __FUNCTION__);
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
        CLog::Log(LOGDEBUG, "%s - Default defined SubMenu : (1) Services", __FUNCTION__);
        strBQRequest = "xml/services?mode=0&submode=1"; //Services
        strXMLRootString.Format("services");
        strXMLChildString.Format("service");
      }
      else if(g_advancedSettings.m_iTuxBoxDefaultSubMenu == 2)
      {
        CLog::Log(LOGDEBUG, "%s - Default defined SubMenu : (2) Satellites", __FUNCTION__);
        strBQRequest = "xml/services?mode=0&submode=2"; //Satellites
        strXMLRootString.Format("satellites");
        strXMLChildString.Format("satellite");
      }
      else if(g_advancedSettings.m_iTuxBoxDefaultSubMenu == 3)
      {
        CLog::Log(LOGDEBUG, "%s - Default defined SubMenu : (3) Providers", __FUNCTION__);
        strBQRequest = "xml/services?mode=0&submode=3"; //Providers
        strXMLRootString.Format("providers");
        strXMLChildString.Format("provider");
      }
      else
      {
        CLog::Log(LOGDEBUG, "%s - Default defined SubMenu : (4) Bouquets", __FUNCTION__);
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
