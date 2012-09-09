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

#include "Skin.h"
#include "AddonManager.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "guilib/Key.h"
#include "utils/URIUtils.h"
#include "settings/Settings.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "settings/GUISettings.h"

// fallback for new skin resolution code
#include "filesystem/Directory.h"

using namespace std;
using namespace XFILE;

#define SKIN_MIN_VERSION 2.1f

boost::shared_ptr<ADDON::CSkinInfo> g_SkinInfo;

namespace ADDON
{

CSkinInfo::CSkinInfo(const AddonProps &props, const RESOLUTION_INFO &resolution)
  : CAddon(props), m_defaultRes(resolution)
{
}

CSkinInfo::CSkinInfo(const cp_extension_t *ext)
  : CAddon(ext)
{
  ELEMENTS elements;
  if (CAddonMgr::Get().GetExtElements(ext->configuration, "res", elements))
  {
    for (ELEMENTS::iterator i = elements.begin(); i != elements.end(); ++i)
    {
      int width = atoi(CAddonMgr::Get().GetExtValue(*i, "@width"));
      int height = atoi(CAddonMgr::Get().GetExtValue(*i, "@height"));
      bool defRes = CAddonMgr::Get().GetExtValue(*i, "@default").Equals("true");
      CStdString folder = CAddonMgr::Get().GetExtValue(*i, "@folder");
      float aspect = 0;
      CStdStringArray fracs;
      CStdString strAspect = CAddonMgr::Get().GetExtValue(*i, "@aspect");
      StringUtils::SplitString(strAspect, ":", fracs);
      if (fracs.size() == 2)
        aspect = (float)(atof(fracs[0].c_str())/atof(fracs[1].c_str()));
      if (width > 0 && height > 0)
      {
        RESOLUTION_INFO res(width, height, aspect, folder);
        res.strId = strAspect; // for skin usage, store aspect string in strId
        if (defRes)
          m_defaultRes = res;
        m_resolutions.push_back(res);
      }
    }
  }
  else
  { // no resolutions specified -> backward compatibility
    CStdString defaultWide = CAddonMgr::Get().GetExtValue(ext->configuration, "@defaultwideresolution");
    if (defaultWide.IsEmpty())
      defaultWide = CAddonMgr::Get().GetExtValue(ext->configuration, "@defaultresolution");
    TranslateResolution(defaultWide, m_defaultRes);
  }

  CStdString str = CAddonMgr::Get().GetExtValue(ext->configuration, "@effectslowdown");
  if (!str.IsEmpty())
    m_effectsSlowDown = (float)atof(str.c_str());
  else
    m_effectsSlowDown = 1.f;

  str = CAddonMgr::Get().GetExtValue(ext->configuration, "@debugging");
  m_debugging = !strcmp(str.c_str(), "true");

  m_onlyAnimateToHome = true;
  LoadStartupWindows(ext);
  m_Version = 2.11;
}

CSkinInfo::~CSkinInfo()
{
}

struct closestRes
{
  closestRes(const RESOLUTION_INFO &target) : m_target(target) { };
  bool operator()(const RESOLUTION_INFO &i, const RESOLUTION_INFO &j)
  {
    float diff = fabs(i.DisplayRatio() - m_target.DisplayRatio()) - fabs(j.DisplayRatio() - m_target.DisplayRatio());
    if (diff < 0) return true;
    if (diff > 0) return false;
    diff = fabs((float)i.iHeight - m_target.iHeight) - fabs((float)j.iHeight - m_target.iHeight);
    if (diff < 0) return true;
    if (diff > 0) return false;
    return fabs((float)i.iWidth - m_target.iWidth) < fabs((float)j.iWidth - m_target.iWidth);
  }
  RESOLUTION_INFO m_target;
};

void CSkinInfo::Start()
{
  if (!m_resolutions.size())
  { // try falling back to whatever resolutions exist in the directory
    CFileItemList items;
    CDirectory::GetDirectory(Path(), items, "", DIR_FLAG_NO_FILE_DIRS);
    for (int i = 0; i < items.Size(); i++)
    {
      RESOLUTION_INFO res;
      if (items[i]->m_bIsFolder && TranslateResolution(items[i]->GetLabel(), res))
        m_resolutions.push_back(res);
    }
  }

  if (!m_resolutions.empty())
  {
    // find the closest resolution
    const RESOLUTION_INFO &target = g_graphicsContext.GetResInfo();
    RESOLUTION_INFO& res = *std::min_element(m_resolutions.begin(), m_resolutions.end(), closestRes(target));
    m_currentAspect = res.strId;
  }
}

CStdString CSkinInfo::GetSkinPath(const CStdString& strFile, RESOLUTION_INFO *res, const CStdString& strBaseDir /* = "" */) const
{
  if (m_resolutions.empty())
    return ""; // invalid skin

  CStdString strPathToUse = Path();
  if (!strBaseDir.IsEmpty())
    strPathToUse = strBaseDir;

  // if the caller doesn't care about the resolution just use a temporary
  RESOLUTION_INFO tempRes;
  if (!res)
    res = &tempRes;

  // find the closest resolution
  const RESOLUTION_INFO &target = g_graphicsContext.GetResInfo();
  *res = *std::min_element(m_resolutions.begin(), m_resolutions.end(), closestRes(target));

  CStdString strPath = URIUtils::AddFileToFolder(strPathToUse, res->strMode);
  strPath = URIUtils::AddFileToFolder(strPath, strFile);
  if (CFile::Exists(strPath))
    return strPath;

  // use the default resolution
  *res = m_defaultRes;

  strPath = URIUtils::AddFileToFolder(strPathToUse, res->strMode);
  strPath = URIUtils::AddFileToFolder(strPath, strFile);
  return strPath;
}

bool CSkinInfo::HasSkinFile(const CStdString &strFile) const
{
  return CFile::Exists(GetSkinPath(strFile));
}

double CSkinInfo::GetMinVersion()
{
  return SKIN_MIN_VERSION;
}

void CSkinInfo::LoadIncludes()
{
  CStdString includesPath = CSpecialProtocol::TranslatePathConvertCase(GetSkinPath("includes.xml"));
  CLog::Log(LOGINFO, "Loading skin includes from %s", includesPath.c_str());
  m_includes.ClearIncludes();
  m_includes.LoadIncludes(includesPath);
}

void CSkinInfo::ResolveIncludes(TiXmlElement *node, std::map<int, bool>* xmlIncludeConditions /* = NULL */)
{
  if(xmlIncludeConditions)
    xmlIncludeConditions->clear();

  m_includes.ResolveIncludes(node, xmlIncludeConditions);
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

bool CSkinInfo::LoadStartupWindows(const cp_extension_t *ext)
{
  m_startupWindows.clear();
  m_startupWindows.push_back(CStartupWindow(WINDOW_HOME, "513"));
  m_startupWindows.push_back(CStartupWindow(WINDOW_PVR, "19180"));
  m_startupWindows.push_back(CStartupWindow(WINDOW_PROGRAMS, "0"));
  m_startupWindows.push_back(CStartupWindow(WINDOW_PICTURES, "1"));
  m_startupWindows.push_back(CStartupWindow(WINDOW_MUSIC, "2"));
  m_startupWindows.push_back(CStartupWindow(WINDOW_VIDEOS, "3"));
  m_startupWindows.push_back(CStartupWindow(WINDOW_FILES, "7"));
  m_startupWindows.push_back(CStartupWindow(WINDOW_SETTINGS_MENU, "5"));
  m_startupWindows.push_back(CStartupWindow(WINDOW_WEATHER, "8"));
  m_onlyAnimateToHome = true;
  return true;
}

void CSkinInfo::GetSkinPaths(std::vector<CStdString> &paths) const
{
  RESOLUTION_INFO res;
  GetSkinPath("Home.xml", &res);
  if (!res.strMode.empty())
    paths.push_back(URIUtils::AddFileToFolder(Path(), res.strMode));
  if (res.strMode != m_defaultRes.strMode)
    paths.push_back(URIUtils::AddFileToFolder(Path(), m_defaultRes.strMode));
}

bool CSkinInfo::TranslateResolution(const CStdString &name, RESOLUTION_INFO &res)
{
  if (name.Equals("pal"))
    res = RESOLUTION_INFO(720, 576, 4.0f/3, "pal");
  else if (name.Equals("pal16x9"))
    res = RESOLUTION_INFO(720, 576, 16.0f/9, "pal16x9");
  else if (name.Equals("ntsc"))
    res = RESOLUTION_INFO(720, 480, 4.0f/3, "ntsc");
  else if (name.Equals("ntsc16x9"))
    res = RESOLUTION_INFO(720, 480, 16.0f/9, "ntsc16x9");
  else if (name.Equals("720p"))
    res = RESOLUTION_INFO(1280, 720, 0, "720p");
  else if (name.Equals("1080i"))
    res = RESOLUTION_INFO(1920, 1080, 0, "1080i");
  else
    return false;
  return true;
}

int CSkinInfo::GetFirstWindow() const
{
  int startWindow = GetStartWindow();
  if (HasSkinFile("Startup.xml") && (!m_onlyAnimateToHome || startWindow == WINDOW_HOME))
    startWindow = WINDOW_STARTUP_ANIM;
  return startWindow;
}

bool CSkinInfo::IsInUse() const
{
  // Could extend this to prompt for reverting to the standard skin perhaps
  return g_guiSettings.GetString("lookandfeel.skin") == ID();
}

const INFO::CSkinVariableString* CSkinInfo::CreateSkinVariable(const CStdString& name, int context)
{
  return m_includes.CreateSkinVariable(name, context);
}

} /*namespace ADDON*/
