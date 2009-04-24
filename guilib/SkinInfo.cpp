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

#include "include.h"
#include "SkinInfo.h"
#include "GUIWindowManager.h"
#include "GUISettings.h"
#include "FileSystem/File.h"
#include "FileSystem/SpecialProtocol.h"
#include "Util.h"
#include "Settings.h"

using namespace std;
using namespace XFILE;

#define SKIN_MIN_VERSION 2.0

CSkinInfo g_SkinInfo; // global

CSkinInfo::CSkinInfo()
{
  m_DefaultResolution = INVALID;
  m_DefaultResolutionWide = INVALID;
  m_strBaseDir = "";
  m_iNumCreditLines = 0;
  m_effectsSlowDown = 1.0;
  m_onlyAnimateToHome = true;
  m_Version = 1.0;
  m_skinzoom = 1.0f;
}

CSkinInfo::~CSkinInfo()
{}

void CSkinInfo::Load(const CStdString& strSkinDir)
{
  m_strBaseDir = strSkinDir;
  m_DefaultResolution = INVALID;  // set to INVALID to denote that there is no default res here
  m_DefaultResolutionWide = INVALID;
  m_effectsSlowDown = 1.0;
  // Load from skin.xml
  TiXmlDocument xmlDoc;
  CStdString strFile = m_strBaseDir + "\\skin.xml";
  if (xmlDoc.LoadFile(strFile))
  { // ok - get the default skin folder out of it...
    TiXmlElement* pRootElement = xmlDoc.RootElement();
    CStdString strValue = pRootElement->Value();
    if (strValue != "skin")
    {
      CLog::Log(LOGERROR, "file :%s doesnt contain <skin>", strFile.c_str());
    }
    else
    { // get the default resolution
      TiXmlNode *pChild = pRootElement->FirstChild("defaultresolution");
      if (pChild)
      { // found the defaultresolution tag
        CStdString strDefaultDir = pChild->FirstChild()->Value();
        strDefaultDir = strDefaultDir.ToLower();
        if (strDefaultDir == "pal") m_DefaultResolution = PAL_4x3;
        else if (strDefaultDir == "pal16x9") m_DefaultResolution = PAL_16x9;
        else if (strDefaultDir == "ntsc") m_DefaultResolution = NTSC_4x3;
        else if (strDefaultDir == "ntsc16x9") m_DefaultResolution = NTSC_16x9;
        else if (strDefaultDir == "720p") m_DefaultResolution = HDTV_720p;
        else if (strDefaultDir == "1080i") m_DefaultResolution = HDTV_1080i;
      }
      CLog::Log(LOGINFO, "Default 4:3 resolution directory is %s", CUtil::AddFileToFolder(m_strBaseDir, GetDirFromRes(m_DefaultResolution)).c_str());

      pChild = pRootElement->FirstChild("defaultresolutionwide");
      if (pChild && pChild->FirstChild())
      { // found the defaultresolution tag
        CStdString strDefaultDir = pChild->FirstChild()->Value();
        strDefaultDir = strDefaultDir.ToLower();
        if (strDefaultDir == "pal") m_DefaultResolutionWide = PAL_4x3;
        else if (strDefaultDir == "pal16x9") m_DefaultResolutionWide = PAL_16x9;
        else if (strDefaultDir == "ntsc") m_DefaultResolutionWide = NTSC_4x3;
        else if (strDefaultDir == "ntsc16x9") m_DefaultResolutionWide = NTSC_16x9;
        else if (strDefaultDir == "720p") m_DefaultResolutionWide = HDTV_720p;
        else if (strDefaultDir == "1080i") m_DefaultResolutionWide = HDTV_1080i;
      }
      else
        m_DefaultResolutionWide = m_DefaultResolution; // default to same as 4:3
      CLog::Log(LOGINFO, "Default 16:9 resolution directory is %s", CUtil::AddFileToFolder(m_strBaseDir, GetDirFromRes(m_DefaultResolutionWide)).c_str());

      // get the version
      pChild = pRootElement->FirstChild("version");
      if (pChild && pChild->FirstChild())
      {
        m_Version = atof(pChild->FirstChild()->Value());
        CLog::Log(LOGINFO, "Skin version is: %s", pChild->FirstChild()->Value());
      }

      // get the effects slowdown parameter
      pChild = pRootElement->FirstChild("effectslowdown");
      if (pChild && pChild->FirstChild())
      {
        m_effectsSlowDown = atof(pChild->FirstChild()->Value());
      }
      // now load the credits information
      pChild = pRootElement->FirstChild("credits");
      if (pChild)
      { // ok, run through the credits
        TiXmlNode *pGrandChild = pChild->FirstChild("skinname");
        if (pGrandChild && pGrandChild->FirstChild())
        {
          CStdString strName = pGrandChild->FirstChild()->Value();
#ifndef _LINUX
          swprintf(credits[0], L"%S Skin", strName.Left(44).c_str());
#else
          swprintf(credits[0], CREDIT_LINE_LENGTH - 1, L"%s Skin", strName.Left(44).c_str());
#endif
        }
        pGrandChild = pChild->FirstChild("name");
        m_iNumCreditLines = 1;
        while (pGrandChild && pGrandChild->FirstChild() && m_iNumCreditLines < 6)
        {
          CStdString strName = pGrandChild->FirstChild()->Value();
#ifndef _LINUX
          swprintf(credits[m_iNumCreditLines], L"%S", strName.Left(49).c_str());
#else
          swprintf(credits[m_iNumCreditLines], CREDIT_LINE_LENGTH - 1, L"%s", strName.Left(49).c_str());
#endif
          m_iNumCreditLines++;
          pGrandChild = pGrandChild->NextSibling("name");
        }
      }

      // get the skin zoom parameter. it's how much skin should be enlarged to get rid of overscan
      pChild = pRootElement->FirstChild("zoom");
      if (pChild && pChild->FirstChild())
        m_skinzoom = (float)atof(pChild->FirstChild()->Value());
      else
        m_skinzoom = 1.0f;

      // now load the startupwindow information
      LoadStartupWindows(pRootElement->FirstChildElement("startupwindows"));
    }
  }
  // Load the skin includes
  LoadIncludes();
}

bool CSkinInfo::Check(const CStdString& strSkinDir)
{
  bool bVersionOK = false;
  // Load from skin.xml
  TiXmlDocument xmlDoc;
  CStdString strFile = CUtil::AddFileToFolder(strSkinDir, "skin.xml");
  CStdString strGoodPath = strSkinDir;
  if (xmlDoc.LoadFile(strFile))
  { // ok - get the default res folder out of it...
    TiXmlElement* pRootElement = xmlDoc.RootElement();
    CStdString strValue = pRootElement->Value();
    if (strValue == "skin")
    { // get the default resolution
      TiXmlNode *pChild = pRootElement->FirstChild("defaultresolution");
      if (pChild)
      { // found the defaultresolution tag
#ifndef _LINUX
        strGoodPath += "\\";
#else
        strGoodPath += "/";
#endif
        CStdString resolution = pChild->FirstChild()->Value();
        if (resolution == "pal") resolution = "PAL";
        else if (resolution == "pal16x9") resolution = "PAL16x9";
        else if (resolution == "ntsc") resolution = "NTSC";
        else if (resolution == "ntsc16x9") resolution = "NTSC16x9";
        strGoodPath = CUtil::AddFileToFolder(strGoodPath, resolution);
      }
      // get the version
      pChild = pRootElement->FirstChild("version");
      if (pChild)
      {
        bVersionOK = atof(pChild->FirstChild()->Value()) >= SKIN_MIN_VERSION;
        CLog::Log(LOGINFO, "Skin version is: %s", pChild->FirstChild()->Value());
      }
    }
  }
  // Check to see if we have a good path
  CStdString strFontXML = CUtil::AddFileToFolder(strGoodPath, "Font.xml");
  CStdString strHomeXML = CUtil::AddFileToFolder(strGoodPath, "Home.xml");
  if ( CFile::Exists(strFontXML) &&
       CFile::Exists(strHomeXML) && bVersionOK )
  {
    return true;
  }
  return false;
}

CStdString CSkinInfo::GetSkinPath(const CStdString& strFile, RESOLUTION *res, const CStdString& strBaseDir /* = "" */)
{
  CStdString strPathToUse = m_strBaseDir;
  if (!strBaseDir.IsEmpty())
    strPathToUse = strBaseDir;
  // first try and load from the current resolution's directory
  int height=0;
  *res = g_graphicsContext.GetVideoResolution();
  if (*res >= WINDOW)
  {
    height = g_settings.m_ResInfo[*res].iHeight;
    if (height>=1080 && (g_settings.m_ResInfo[*res].dwFlags & D3DPRESENTFLAG_WIDESCREEN))
    {
      *res = HDTV_1080i;
    }
    else if (height>=720 && (g_settings.m_ResInfo[*res].dwFlags & D3DPRESENTFLAG_WIDESCREEN))
    {
      *res = HDTV_720p;
    }
    else if (g_settings.m_ResInfo[*res].dwFlags & D3DPRESENTFLAG_WIDESCREEN)
    {
      *res = PAL_16x9;
    }
    else
    {
      *res = PAL_4x3;
    }
  }
  CStdString strPath = CUtil::AddFileToFolder(strPathToUse, GetDirFromRes(*res));
  strPath = CUtil::AddFileToFolder(strPath, strFile);
  if (CFile::Exists(strPath))
    return strPath;
  // if we're in 1080i mode, try 720p next
  if (*res == HDTV_1080i)
  {
    *res = HDTV_720p;
    strPath = CUtil::AddFileToFolder(strPathToUse, GetDirFromRes(*res));
    strPath = CUtil::AddFileToFolder(strPath, strFile);
    if (CFile::Exists(strPath))
      return strPath;
  }
  // that failed - drop to the default widescreen resolution if where in a widemode
  if (*res == PAL_16x9 || *res == NTSC_16x9 || *res == HDTV_480p_16x9 || *res == HDTV_720p)
  {
    *res = m_DefaultResolutionWide;
    strPath = CUtil::AddFileToFolder(strPathToUse, GetDirFromRes(*res));
    strPath = CUtil::AddFileToFolder(strPath, strFile);
    if (CFile::Exists(strPath))
      return strPath;
  }
  // that failed - drop to the default resolution
  *res = m_DefaultResolution;
  strPath = CUtil::AddFileToFolder(strPathToUse, GetDirFromRes(*res));
  strPath = CUtil::AddFileToFolder(strPath, strFile);
  // check if we don't have any subdirectories
  if (*res == INVALID) *res = PAL_4x3;
  return strPath;
}

CStdString CSkinInfo::GetDirFromRes(RESOLUTION res)
{
  CStdString strRes;
  switch (res)
  {
  case PAL_4x3:
    strRes = "PAL";
    break;
  case PAL_16x9:
    strRes = "PAL16x9";
    break;
  case NTSC_4x3:
  case HDTV_480p_4x3:
    strRes = "NTSC";
    break;
  case NTSC_16x9:
  case HDTV_480p_16x9:
    strRes = "ntsc16x9";
    break;
  case HDTV_720p:
    strRes = "720p";
    break;
  case HDTV_1080i:
    strRes = "1080i";
    break;
  case INVALID:
  default:
    strRes = "";
    break;
  }
  return strRes;
}

CStdString CSkinInfo::GetBaseDir()
{
  return m_strBaseDir;
}

wchar_t* CSkinInfo::GetCreditsLine(int i)
{
  if (i < m_iNumCreditLines)
    return credits[i];
  else
    return NULL;
}

double CSkinInfo::GetMinVersion()
{
  return SKIN_MIN_VERSION;
}

void CSkinInfo::LoadIncludes()
{
  RESOLUTION res;
  CStdString includesPath = PTH_IC(GetSkinPath("includes.xml", &res));
  CLog::Log(LOGINFO, "Loading skin includes from %s", includesPath.c_str());
  m_includes.ClearIncludes();
  m_includes.LoadIncludes(includesPath);
}

void CSkinInfo::ResolveIncludes(TiXmlElement *node, const CStdString &type)
{
  m_includes.ResolveIncludes(node, type);
}

bool CSkinInfo::ResolveConstant(const CStdString &constant, float &value)
{
  return m_includes.ResolveConstant(constant, value);
}

bool CSkinInfo::ResolveConstant(const CStdString &constant, DWORD &value)
{
  float fValue;
  if (m_includes.ResolveConstant(constant, fValue))
  {
    value = (DWORD)fValue;
    return true;
  }
  return false;
}

int CSkinInfo::GetStartWindow()
{
  int windowID = g_guiSettings.GetInt("lookandfeel.startupwindow");
  assert(m_startupWindows.size());
  for (vector<CStartupWindow>::iterator it = m_startupWindows.begin(); it != m_startupWindows.end(); it++)
  {
    if (windowID == (*it).m_id)
      return windowID;
  }
  // return our first one
  return m_startupWindows[0].m_id;
}

bool CSkinInfo::LoadStartupWindows(const TiXmlElement *startup)
{
  m_startupWindows.clear();
  if (startup)
  { // yay, run through and grab the startup windows
    const TiXmlElement *window = startup->FirstChildElement("window");
    while (window && window->FirstChild())
    {
      int id;
      window->Attribute("id", &id);
      CStdString name = window->FirstChild()->Value();
      m_startupWindows.push_back(CStartupWindow(id + WINDOW_HOME, name));
      window = window->NextSiblingElement("window");
    }
  }

  // ok, now see if we have any startup windows
  if (!m_startupWindows.size())
  { // nope - add the default ones
    m_startupWindows.push_back(CStartupWindow(WINDOW_HOME, "513"));
    m_startupWindows.push_back(CStartupWindow(WINDOW_PROGRAMS, "0"));
    m_startupWindows.push_back(CStartupWindow(WINDOW_PICTURES, "1"));
    m_startupWindows.push_back(CStartupWindow(WINDOW_MUSIC, "2"));
    m_startupWindows.push_back(CStartupWindow(WINDOW_VIDEOS, "3"));
    m_startupWindows.push_back(CStartupWindow(WINDOW_FILES, "7"));
    m_startupWindows.push_back(CStartupWindow(WINDOW_SETTINGS_MENU, "5"));
    m_startupWindows.push_back(CStartupWindow(WINDOW_SCRIPTS, "247"));
    m_onlyAnimateToHome = true;
  }
  else
    m_onlyAnimateToHome = false;
  return true;
}
void CSkinInfo::SetDefaults()
{
  m_DefaultResolution = PAL_4x3;
  m_DefaultResolutionWide = PAL_16x9;
}
