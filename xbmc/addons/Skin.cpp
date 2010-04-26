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

#include "Skin.h"
#include "GUIWindowManager.h"
#include "GUISettings.h"
#include "FileSystem/File.h"
#include "FileSystem/SpecialProtocol.h"
#include "Key.h"
#include "Util.h"
#include "Settings.h"
#include "utils/log.h"
#include "XMLUtils.h"

using namespace std;
using namespace XFILE;

#define SKIN_MIN_VERSION 2.1f

namespace ADDON
{
CSkinInfo g_SkinInfo;

CSkinInfo::CSkinInfo()
{
  SetDefaults();
}

CSkinInfo::~CSkinInfo()
{}

void CSkinInfo::SetDefaults()
{
  m_strBaseDir = "";
  m_DefaultResolution = RES_PAL_4x3;
  m_DefaultResolutionWide = RES_INVALID;
  m_effectsSlowDown = 1.0f;
  m_Version = 1.0;
  m_debugging = false;
  m_onlyAnimateToHome = true;
}

void CSkinInfo::Load(const CStdString& strSkinDir, bool loadIncludes)
{
  SetDefaults();
  m_strBaseDir = strSkinDir;

  // Load from skin.xml
  TiXmlDocument xmlDoc;
  CStdString strFile = m_strBaseDir + "\\skin.xml";
  if (xmlDoc.LoadFile(strFile))
  { // ok - get the default skin folder out of it...
    const TiXmlNode* root = xmlDoc.RootElement();
    if (root && root->ValueStr() == "skin")
    {
      GetResolution(root, "defaultresolution", m_DefaultResolution);
      if (!GetResolution(root, "defaultwideresolution", m_DefaultResolutionWide))
        m_DefaultResolutionWide = m_DefaultResolution;

      CLog::Log(LOGINFO, "Default 4:3 resolution directory is %s", CUtil::AddFileToFolder(m_strBaseDir, GetDirFromRes(m_DefaultResolution)).c_str());
      CLog::Log(LOGINFO, "Default 16:9 resolution directory is %s", CUtil::AddFileToFolder(m_strBaseDir, GetDirFromRes(m_DefaultResolutionWide)).c_str());

      XMLUtils::GetDouble(root, "version", m_Version);
      XMLUtils::GetFloat(root, "effectslowdown", m_effectsSlowDown);
      XMLUtils::GetBoolean(root, "debugging", m_debugging);

      // now load the startupwindow information
      LoadStartupWindows(root->FirstChildElement("startupwindows"));
    }
    else
      CLog::Log(LOGERROR, "%s - %s doesnt contain <skin>", __FUNCTION__, strFile.c_str());
  }
  // Load the skin includes
  if (loadIncludes)
    LoadIncludes();
}

bool CSkinInfo::Check(const CStdString& strSkinDir)
{
  CSkinInfo info;
  info.Load(strSkinDir, false);
  if (info.GetVersion() < GetMinVersion())
  {
    CLog::Log(LOGERROR, "%s(%s) version is to old (%f versus %f)", __FUNCTION__, strSkinDir.c_str(), info.GetVersion(), GetMinVersion());
    return false;
  }
  if (!info.HasSkinFile("Home.xml") || !info.HasSkinFile("Font.xml"))
  {
    CLog::Log(LOGERROR, "%s(%s) does not contain Home.xml or Font.xml", __FUNCTION__, strSkinDir.c_str());
    return false;
  }
  return true;
}

CStdString CSkinInfo::GetSkinPath(const CStdString& strFile, RESOLUTION *res, const CStdString& strBaseDir /* = "" */) const
{
  CStdString strPathToUse = m_strBaseDir;
  if (!strBaseDir.IsEmpty())
    strPathToUse = strBaseDir;

  // if the caller doesn't care about the resolution just use a temporary
  RESOLUTION tempRes = RES_INVALID;
  if (!res)
    res = &tempRes;

  // first try and load from the current resolution's directory
  *res = g_graphicsContext.GetVideoResolution();
  if (*res >= RES_WINDOW)
  {
    unsigned int pixels = g_settings.m_ResInfo[*res].iHeight * g_settings.m_ResInfo[*res].iWidth;
    if (pixels >= 1600 * 900)
    {
      *res = RES_HDTV_1080i;
    }
    else if (pixels >= 900 * 600)
    {
      *res = RES_HDTV_720p;
    }
    else if (((float)g_settings.m_ResInfo[*res].iWidth) / ((float)g_settings.m_ResInfo[*res].iHeight) > 8.0f / (3.0f * sqrt(3.0f)))
    {
      *res = RES_PAL_16x9;
    }
    else
    {
      *res = RES_PAL_4x3;
    }
  }
  CStdString strPath = CUtil::AddFileToFolder(strPathToUse, GetDirFromRes(*res));
  strPath = CUtil::AddFileToFolder(strPath, strFile);
  if (CFile::Exists(strPath))
    return strPath;
  // if we're in 1080i mode, try 720p next
  if (*res == RES_HDTV_1080i)
  {
    *res = RES_HDTV_720p;
    strPath = CUtil::AddFileToFolder(strPathToUse, GetDirFromRes(*res));
    strPath = CUtil::AddFileToFolder(strPath, strFile);
    if (CFile::Exists(strPath))
      return strPath;
  }
  // that failed - drop to the default widescreen resolution if where in a widemode
  if (*res == RES_PAL_16x9 || *res == RES_NTSC_16x9 || *res == RES_HDTV_480p_16x9 || *res == RES_HDTV_720p)
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
  if (*res == RES_INVALID) *res = RES_PAL_4x3;
  return strPath;
}

bool CSkinInfo::HasSkinFile(const CStdString &strFile) const
{
  return CFile::Exists(GetSkinPath(strFile));
}

CStdString CSkinInfo::GetDirFromRes(RESOLUTION res) const
{
  CStdString strRes;
  switch (res)
  {
  case RES_PAL_4x3:
    strRes = "PAL";
    break;
  case RES_PAL_16x9:
    strRes = "PAL16x9";
    break;
  case RES_NTSC_4x3:
  case RES_HDTV_480p_4x3:
    strRes = "NTSC";
    break;
  case RES_NTSC_16x9:
  case RES_HDTV_480p_16x9:
    strRes = "ntsc16x9";
    break;
  case RES_HDTV_720p:
    strRes = "720p";
    break;
  case RES_HDTV_1080i:
    strRes = "1080i";
    break;
  case RES_INVALID:
  default:
    strRes = "";
    break;
  }
  return strRes;
}

CStdString CSkinInfo::GetBaseDir() const
{
  return m_strBaseDir;
}

double CSkinInfo::GetMinVersion()
{
  return SKIN_MIN_VERSION;
}

void CSkinInfo::LoadIncludes()
{
  CStdString includesPath = PTH_IC(GetSkinPath("includes.xml"));
  CLog::Log(LOGINFO, "Loading skin includes from %s", includesPath.c_str());
  m_includes.ClearIncludes();
  m_includes.LoadIncludes(includesPath);
}

void CSkinInfo::ResolveIncludes(TiXmlElement *node, const CStdString &type)
{
  m_includes.ResolveIncludes(node, type);
}

bool CSkinInfo::ResolveConstant(const CStdString &constant, float &value) const
{
  return m_includes.ResolveConstant(constant, value);
}

bool CSkinInfo::ResolveConstant(const CStdString &constant, unsigned int &value) const
{
  float fValue;
  if (m_includes.ResolveConstant(constant, fValue))
  {
    value = (unsigned int)fValue;
    return true;
  }
  return false;
}

int CSkinInfo::GetStartWindow() const
{
  int windowID = g_guiSettings.GetInt("lookandfeel.startupwindow");
  assert(m_startupWindows.size());
  for (vector<CStartupWindow>::const_iterator it = m_startupWindows.begin(); it != m_startupWindows.end(); it++)
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

bool CSkinInfo::IsWide(RESOLUTION res) const
{
  return (res == RES_PAL_16x9 || res == RES_NTSC_16x9 || res == RES_HDTV_480p_16x9 || res == RES_HDTV_720p || res == RES_HDTV_1080i);
}

void CSkinInfo::GetSkinPaths(std::vector<CStdString> &paths) const
{
  RESOLUTION resToUse = RES_INVALID;
  GetSkinPath("Home.xml", &resToUse);
  if (resToUse == RES_HDTV_1080i)
    paths.push_back(CUtil::AddFileToFolder(m_strBaseDir, GetDirFromRes(RES_HDTV_1080i)));
  if (resToUse == RES_HDTV_720p)
    paths.push_back(CUtil::AddFileToFolder(m_strBaseDir, GetDirFromRes(RES_HDTV_720p)));
  if (resToUse != m_DefaultResolutionWide && IsWide(resToUse))
    paths.push_back(CUtil::AddFileToFolder(m_strBaseDir, GetDirFromRes(m_DefaultResolutionWide)));
  if (resToUse != m_DefaultResolution && (!IsWide(resToUse) || m_DefaultResolutionWide != m_DefaultResolution))
    paths.push_back(CUtil::AddFileToFolder(m_strBaseDir, GetDirFromRes(m_DefaultResolution)));
}

bool CSkinInfo::GetResolution(const TiXmlNode *root, const char *tag, RESOLUTION &res) const
{
  CStdString strRes;
  if (XMLUtils::GetString(root, tag, strRes))
  {
    strRes.ToLower();
    if (strRes == "pal")
      res = RES_PAL_4x3;
    else if (strRes == "pal16x9")
      res = RES_PAL_16x9;
    else if (strRes == "ntsc")
      res = RES_NTSC_4x3;
    else if (strRes == "ntsc16x9")
      res = RES_NTSC_16x9;
    else if (strRes == "720p")
      res = RES_HDTV_720p;
    else if (strRes == "1080i")
      res = RES_HDTV_1080i;
    else
    {
      CLog::Log(LOGERROR, "%s invalid resolution specified for <%s>, %s", __FUNCTION__, tag, strRes.c_str());
      return false;
    }
    return true;
  }
  return false;
}

int CSkinInfo::GetFirstWindow() const
{
  int startWindow = GetStartWindow();
  if (HasSkinFile("Startup.xml") && (!m_onlyAnimateToHome || startWindow == WINDOW_HOME))
    startWindow = WINDOW_STARTUP_ANIM;
  return startWindow;
}

} /*namespace ADDON*/
