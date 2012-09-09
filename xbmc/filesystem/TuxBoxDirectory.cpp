/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */


#include "TuxBoxDirectory.h"
#include "DirectoryCache.h"
#include "CurlFile.h"
#include "utils/HttpHeader.h"
#include "utils/TuxBoxUtil.h"
#include "URL.h"
#include "utils/XBMCTinyXML.h"
#include "settings/AdvancedSettings.h"
#include "FileItem.h"
#include "utils/log.h"
#include "utils/URIUtils.h"

using namespace XFILE;

CTuxBoxDirectory::CTuxBoxDirectory(void)
{
}

CTuxBoxDirectory::~CTuxBoxDirectory(void)
{
}

bool CTuxBoxDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  // so we know that we have enigma2
  static bool enigma2 = false;
  // Detect and delete slash at end
  CStdString strRoot = strPath;
  URIUtils::RemoveSlashAtEnd(strRoot);

  //Get the request strings
  CStdString strBQRequest;
  CStdString strXMLRootString;
  CStdString strXMLChildString;
  if(!GetRootAndChildString(strRoot, strBQRequest, strXMLRootString, strXMLChildString))
    return false;

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
  CCurlFile http;
  int iTryConnect = 0;
  int iWaitTimer = 20;
  bool result = false;

  while (iTryConnect < 4)
  {
    http.SetTimeout(iWaitTimer);
    if(http.Open(url))
    {
      //We are connected!
      iTryConnect = 4;

      // restore protocol
      url.SetProtocol(protocol);

      int size_read = 0;
      int size_total = (int)http.GetLength();
      int data_size = 0;
      CStdString data;
      data.reserve(size_total);

      // read response from server into string buffer
      char buffer[16384];
      while ((size_read = http.Read(buffer, sizeof(buffer)-1)) > 0)
      {
        buffer[size_read] = 0;
        data += buffer;
        data_size += size_read;
      }
      http.Close();

      // parse returned xml
      CXBMCTinyXML doc;
      data.Replace("></",">-</"); //FILL EMPTY ELEMENTS WITH "-"!
      doc.Parse(data.c_str());
      TiXmlElement *root = doc.RootElement();
      if(root == NULL)
      {
        CLog::Log(LOGERROR, "%s - Unable to parse xml", __FUNCTION__);
        CLog::Log(LOGERROR, "%s - Sample follows...\n%s", __FUNCTION__, data.c_str());
        return false;
      }
      if( strXMLRootString.Equals(root->Value()) && bIsBouquet)
      {
        data.Empty();
        if (enigma2)
          result = g_tuxbox.ParseBouquetsEnigma2(root, items, url, strFilter, strXMLChildString);
        else
          result = g_tuxbox.ParseBouquets(root, items, url, strFilter, strXMLChildString);
      }
      else if( strXMLRootString.Equals(root->Value()) && !strFilter.IsEmpty() )
      {
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
    }
    else
    {
      CLog::Log(LOGERROR, "%s - Unable to get XML structure! Try count:%i, Wait Timer:%is",__FUNCTION__, iTryConnect, iWaitTimer);
      iTryConnect++;
      if (iTryConnect == 2) //try enigma2 instead of enigma1, best entrypoint here i thought
      {	
        enigma2 = true;
        GetRootAndChildStringEnigma2(strBQRequest, strXMLRootString, strXMLChildString);
        url.SetOptions("");
        url.SetFileName(strBQRequest);
//        iTryConnect = 0;
        iWaitTimer = 20;
      }
      else
        iWaitTimer = iWaitTimer+10;

      result = false;
      http.Close(); // Close old connections
    }
  }
  items.SetContent("movies");
  return result;
}

void CTuxBoxDirectory::GetRootAndChildStringEnigma2(CStdString& strBQRequest, CStdString& strXMLRootString, CStdString& strXMLChildString )
{
  // Allways take getallservices for Enigma2
  strBQRequest = "web/getallservices"; //Bouquets and Channels
  strXMLRootString.Format("e2servicelistrecursive");
  strXMLChildString.Format("e2bouquet");
}

bool CTuxBoxDirectory::GetRootAndChildString(const CStdString strPath, CStdString& strBQRequest, CStdString& strXMLRootString, CStdString& strXMLChildString )
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
