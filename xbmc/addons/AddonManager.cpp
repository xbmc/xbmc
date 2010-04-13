/*
 *      Copyright (C) 2005-2009 Team XBMC
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
#include "AddonManager.h"
#include "Addon.h"
#include "StringUtils.h"
#include "RegExp.h"
#include "XMLUtils.h"
#include "FileItem.h"
#include "Settings.h"
#include "GUISettings.h"
#include "DownloadQueueManager.h"
#include "log.h"

#ifdef HAS_VISUALISATION
#include "DllVisualisation.h"
#include "Visualisation.h"
#endif
#ifdef HAS_SCREENSAVER
#include "DllScreenSaver.h"
#include "ScreenSaver.h"
#endif
//#ifdef HAS_SCRAPERS
#include "Scraper.h"
//#endif
#include "Repository.h"

using namespace std;

namespace ADDON
{


/**********************************************************
 * CAddonMgr
 *
 */

CAddonMgr* CAddonMgr::m_pInstance = NULL;
map<TYPE, IAddonMgrCallback*> CAddonMgr::m_managers;

CAddonMgr::CAddonMgr()
{
}

CAddonMgr::~CAddonMgr()
{
}

CAddonMgr* CAddonMgr::Get()
{
  if (!m_pInstance)
  {
    m_pInstance = new CAddonMgr();
  }
  return m_pInstance;
}

IAddonMgrCallback* CAddonMgr::GetCallbackForType(TYPE type)
{
  if (m_managers.find(type) == m_managers.end())
    return NULL;
  else
    return m_managers[type];
}

bool CAddonMgr::RegisterAddonMgrCallback(const TYPE type, IAddonMgrCallback* cb)
{
  if (cb == NULL)
    return false;

  m_managers.erase(type);
  m_managers[type] = cb;

  return true;
}

void CAddonMgr::UnregisterAddonMgrCallback(TYPE type)
{
  m_managers.erase(type);
}

bool CAddonMgr::HasAddons(const TYPE &type, const CONTENT_TYPE &content/*= CONTENT_NONE*/, bool enabledOnly/*= true*/)
{
  if (m_addons.empty())
  {
    VECADDONS add;
    GetAllAddons(add,false);
  }

  if (content == CONTENT_NONE)
    return (m_addons.find(type) != m_addons.end());

  VECADDONS addons;
  return GetAddons(type, addons, content, enabledOnly);
}

bool CAddonMgr::GetAllAddons(VECADDONS &addons, bool enabledOnly/*= true*/)
{
  VECADDONS temp;
  if (CAddonMgr::Get()->GetAddons(ADDON_PLUGIN, temp, CONTENT_NONE, enabledOnly))
    addons.insert(addons.end(), temp.begin(), temp.end());
  if (CAddonMgr::Get()->GetAddons(ADDON_SCRAPER, temp, CONTENT_NONE, enabledOnly))
    addons.insert(addons.end(), temp.begin(), temp.end());
  if (CAddonMgr::Get()->GetAddons(ADDON_SCREENSAVER, temp, CONTENT_NONE, enabledOnly))
    addons.insert(addons.end(), temp.begin(), temp.end());
  if (CAddonMgr::Get()->GetAddons(ADDON_SCRIPT, temp, CONTENT_NONE, enabledOnly))
    addons.insert(addons.end(), temp.begin(), temp.end());
  if (CAddonMgr::Get()->GetAddons(ADDON_VIZ, temp, CONTENT_NONE, enabledOnly))
    addons.insert(addons.end(), temp.begin(), temp.end());
  return !addons.empty();
}

bool CAddonMgr::GetAddons(const TYPE &type, VECADDONS &addons, const CONTENT_TYPE &content/*= CONTENT_NONE*/, bool enabledOnly/*= true*/)
{
  CSingleLock lock(m_critSection);
  addons.clear();
  if (m_addons.find(type) != m_addons.end())
  {
    IVECADDONS itr = m_addons[type].begin();
    while (itr != m_addons[type].end())
    { // filter out what we're not looking for
      if ((enabledOnly && !(*itr)->Enabled())
        || (content != CONTENT_NONE && !(*itr)->Supports(content)))
      {
        ++itr;
        continue;
      }
      addons.push_back(*itr);
      ++itr;
    }
  }
  return !addons.empty();
}

bool CAddonMgr::GetAddon(const CStdString &str, AddonPtr &addon, const TYPE &type/*=ADDON_UNKNOWN*/, bool enabledOnly/*= true*/)
{
  CSingleLock lock(m_critSection);
  if (type != ADDON_UNKNOWN && m_addons.find(type) == m_addons.end())
    return false;

  if (m_idMap[str])
  {
    addon = m_idMap[str];
    if(enabledOnly)
      return addon->Enabled();
    else
      return true;
  }

  VECADDONS &addons = m_addons[type];
  IVECADDONS adnItr = addons.begin();
  while (adnItr != addons.end())
  {
    //FIXME scrapers were previously registered by filename
    if ((*adnItr)->Name() == str || (type == ADDON_SCRAPER && (*adnItr)->LibName() == str))
    {
      addon = (*adnItr);
      if(enabledOnly)
        return addon->Enabled();
      else
        return true;
    }
    adnItr++;
  }

  return false;
}

//TODO handle all 'default' cases here, not just scrapers & vizs
bool CAddonMgr::GetDefault(const TYPE &type, AddonPtr &addon, const CONTENT_TYPE &content)
{
  if (type != ADDON_SCRAPER && type != ADDON_VIZ)
    return false;

  CStdString setting;
  if (type == ADDON_VIZ)
    setting = g_guiSettings.GetString("musicplayer.visualisation");
  else
  {
    switch (content)
    {
    case CONTENT_MOVIES:
      {
        setting = g_guiSettings.GetString("scrapers.moviedefault");
        break;
      }
    case CONTENT_TVSHOWS:
      {
        setting = g_guiSettings.GetString("scrapers.tvshowdefault");
        break;
      }
    case CONTENT_MUSICVIDEOS:
      {
        setting = g_guiSettings.GetString("scrapers.musicvideodefault");
        break;
      }
    case CONTENT_ALBUMS:
    case CONTENT_ARTISTS:
      {
        setting = g_guiSettings.GetString("musiclibrary.scraper");
        break;
      }
    default:
      return false;
    }
  }
  return GetAddon(setting, addon, type);
}

CStdString CAddonMgr::GetString(const CStdString &id, const int number)
{
  AddonPtr addon = m_idMap[id];
  if (addon)
    return addon->GetString(number);

  return "";
}

void CAddonMgr::FindAddons()
{
  CSingleLock lock(m_critSection);
  // parse the user & system dirs for addons of the requested type
  CFileItemList items;
  if (!CSpecialProtocol::XBMCIsHome())
    CDirectory::GetDirectory("special://home/addons", items);
  CDirectory::GetDirectory("special://xbmc/addons", items);

  // store any addons with unresolved deps, then recheck at the end
  VECADDONS unresolved;

  // for all folders found
  for (int i = 0; i < items.Size(); ++i)
  {
    CFileItemPtr item = items[i];

    if(!item->m_bIsFolder)
      continue;

    // read description.xml and populate the addon
    AddonPtr addon;
    if (!AddonFromInfoXML(item->m_strPath, addon))
      continue;

    // refuse to store addons with missing library
    CStdString library(CUtil::AddFileToFolder(addon->Path(), addon->LibName()));
    if (!CFile::Exists(library))
    {
      CLog::Log(LOGDEBUG, "ADDON: Missing library file %s, bypassing package", library.c_str());
      continue;
    }

    // check for/cache icon thumbnail
    //TODO cache one thumb per addon id instead
    CFileItem item2(CUtil::AddFileToFolder(addon->Path(), addon->LibName()), false);
    item2.SetCachedProgramThumb();
    if (!item2.HasThumbnail())
      item2.SetUserProgramThumb();
    if (!item2.HasThumbnail())
      item2.SetThumbnailImage(addon->Icon());
    if (item2.HasThumbnail())
    {
      XFILE::CFile::Cache(item2.GetThumbnailImage(),item->GetCachedProgramThumb());
    }

    if (!DependenciesMet(addon))
    {
      unresolved.push_back(addon);
      continue;
    }
    else
    { // everything ok, add to available addons if new
      if (UpdateIfKnown(addon))
        continue;
      else
      {
        m_addons[addon->Type()].push_back(addon);
        m_idMap.insert(make_pair(addon->ID(), addon));
      }
    }
  }

  for (unsigned i = 0; i < unresolved.size(); i++)
  {
    AddonPtr& addon = unresolved[i];
    if (DependenciesMet(addon))
    {
      if (!UpdateIfKnown(addon))
      {
        m_addons[addon->Type()].push_back(addon);
        m_idMap.insert(make_pair(addon->ID(), addon));
      }
    }
  }
//  CLog::Log(LOGINFO, "ADDON: Found %"PRIuS" addons", m_addons.find(type) == m_addons.end() ? 0: m_addons[type].size(), TranslateType(type).c_str());
}

bool CAddonMgr::UpdateIfKnown(AddonPtr &addon)
{
  if (m_addons.find(addon->Type()) != m_addons.end())
  {
    for (unsigned i = 0; i < m_addons[addon->Type()].size(); i++)
    {
      if (m_addons[addon->Type()][i]->ID() == addon->ID())
      {
        //TODO inform any manager first, and request removal
        //TODO choose most recent version if varying
        m_addons[addon->Type()][i] = addon;
        CStdString id = addon->ID();
        m_idMap.erase(id);
        m_idMap.insert(make_pair(addon->ID(), addon));
        return true;
      }
    }
  }
  return false;
}

bool CAddonMgr::DependenciesMet(AddonPtr &addon)
{
  // As remote repos are not functioning,
  // this will fail if a dependency is not found locally
  if (!addon)
    return false;

  CSingleLock lock(m_critSection);
  ADDONDEPS deps = addon->GetDeps();
  ADDONDEPS::iterator itr = deps.begin();
  while (itr != deps.end())
  {
    CStdString id;
    id = (*itr).first;
    AddonVersion min = (*itr).second.first;
    AddonVersion max = (*itr).second.second;
    if (m_idMap.count(id))
    {
      AddonPtr dep = m_idMap[id];
      // we're guaranteed to have at least max OR min here
      if (!min.str.IsEmpty() && !max.str.IsEmpty())
        return (dep->Version() >= min && dep->Version() <= max);
      else if (!min.str.IsEmpty())
        return (dep->Version() >= min);
      else
        return (dep->Version() <= max);
    }
    itr++;
  }
  return deps.empty();
}

bool CAddonMgr::AddonFromInfoXML(const CStdString &path, AddonPtr &addon)
{
  // First check that we can load description.xml
  CStdString strPath(CUtil::AddFileToFolder(path, ADDON_METAFILE));
  if(!CFile::Exists(strPath))
    return false;

  TiXmlDocument xmlDoc;
  if (!xmlDoc.LoadFile(strPath))
  {
    CLog::Log(LOGERROR, "Unable to load: %s, Line %d\n%s", strPath.c_str(), xmlDoc.ErrorRow(), xmlDoc.ErrorDesc());
    return false;
  }

  const TiXmlElement *element = xmlDoc.RootElement();
  if (!element || strcmpi(element->Value(), "addoninfo") != 0)
  {
    CLog::Log(LOGERROR, "ADDON: Error loading %s: cannot find <addon> root element", xmlDoc.Value());
    return false;
  }

  return AddonFromInfoXML(element, addon, strPath);
}

bool CAddonMgr::AddonFromInfoXML(const TiXmlElement *rootElement,
                                 AddonPtr &addon, const CStdString &strPath)
{
  /* Steps required to meet package requirements
  * 1. id exists and is valid
  * 2. type exists and is valid
  * 3. version exists
  * 4. a license is specified
  * 5. operating system matches ours
  * 6. summary exists
  * 7. for scrapers & plugins, support at least one type of content
  *
  * NOTE: addon dependencies are handled in ::FindAddons()
  */

  /* Validate id */
  CStdString id;
  const TiXmlElement *element = rootElement->FirstChildElement("id");
  if (!element)
  {
    CLog::Log(LOGERROR, "ADDON: %s missing <id> element, ignoring", strPath.c_str());
    return false;
  }
  id = element->GetText();
  //FIXME since we no longer required uuids, should we bother validating anything?
  if (id.IsEmpty())
  {
    CLog::Log(LOGERROR, "ADDON: %s has invalid <id> element, ignoring", strPath.c_str());
    return false;
  }

  /* Validate type */
  TYPE type;
  element = rootElement->FirstChildElement("type");
  if (!element)
  {
    CLog::Log(LOGERROR, "ADDON: %s missing <type> element, ignoring", strPath.c_str());
    return false;
  }
  type = TranslateType(element->GetText());
  if (type == ADDON_UNKNOWN)
  {
    CLog::Log(LOGERROR, "ADDON: %s has invalid type identifier: '%d'", strPath.c_str(), type);
    return false;
  }

  /* Retrieve Name */
  CStdString name;
  element = rootElement->FirstChildElement("title");
  if (!element)
  {
    CLog::Log(LOGERROR, "ADDON: %s missing <title> element, ignoring", strPath.c_str());
    return false;
  }
  name = element->GetText();

  /* Retrieve version */
  CStdString version;
  element = rootElement->FirstChildElement("version");
  if (!element)
  {
    CLog::Log(LOGERROR, "ADDON: %s missing <version> element, ignoring", strPath.c_str());
    return false;
  }
  /* Validate version */
  version = element->GetText();
  CRegExp versionRE;
  versionRE.RegComp(ADDON_VERSION_RE.c_str());
  if (versionRE.RegFind(version.c_str()) != 0)
  {
    CLog::Log(LOGERROR, "ADDON: %s has invalid <version> element, ignoring", strPath.c_str());
    return false;
  }

  /* Path, ID & Version are valid */
  AddonProps addonProps(id, type, version);
  addonProps.name = name;
  CUtil::GetDirectory(strPath,addonProps.path);
  addonProps.icon = CUtil::AddFileToFolder(addonProps.path, "default.tbn");

  /* Retrieve license */
  element = rootElement->FirstChildElement("license");
/*  if (!element)
  {
    CLog::Log(LOGERROR, "ADDON: %s missing <license> element, ignoring", strPath.c_str());
    return false;
  }
  addonProps.license = element->GetText();*/

  /* Retrieve platforms which this addon supports */
  CStdString platform;
  element = rootElement->FirstChildElement("platforms")->FirstChildElement("platform");
  if (!element)
  {
    CLog::Log(LOGERROR, "ADDON: %s missing <platforms> element, ignoring", strPath.c_str());
    return false;
  }

  bool all(false);
  set<CStdString> platforms;
  do
  {
    CStdString platform = element->GetText();
    if (platform == "all")
    {
      all = true;
      break;
    }
    platforms.insert(platform);
    element = element->NextSiblingElement("platform");
  } while (element != NULL);

  if (!all)
  {
#if defined(_LINUX) && !defined(__APPLE__)
    if (!platforms.count("linux"))
    {
      CLog::Log(LOGNOTICE, "ADDON: %s is not supported under Linux, ignoring", strPath.c_str());
      return false;
    }
#elif defined(_WIN32)
    if (!platforms.count("windows"))
    {
      CLog::Log(LOGNOTICE, "ADDON: %s is not supported under Windows, ignoring", strPath.c_str());
      return false;
    }
#elif defined(__APPLE__)
    if (!platforms.count("osx"))
    {
      CLog::Log(LOGNOTICE, "ADDON: %s is not supported under OSX, ignoring", strPath.c_str());
      return false;
    }
#elif defined(_XBOX)
    if (!platforms.count("xbox"))
    {
      CLog::Log(LOGNOTICE, "ADDON: %s is not supported under XBOX, ignoring", strPath.c_str());
      return false;
    }
#endif
  }

  /* Retrieve summary */
  CStdString summary;
  element = rootElement->FirstChildElement("summary");
  if (!element)
  {
    CLog::Log(LOGERROR, "ADDON: %s missing <summary> element, ignoring", strPath.c_str());
    return false;
  }
  addonProps.summary = element->GetText();

  if (addonProps.type == ADDON_SCRAPER || addonProps.type == ADDON_PLUGIN)
  {
    /* Retrieve content types that this addon supports */
    CStdString platform;
    if (rootElement->FirstChildElement("supportedcontent"))
    {
      element = rootElement->FirstChildElement("supportedcontent")->FirstChildElement("content");
    }
    if (!element)
    {
      CLog::Log(LOGERROR, "ADDON: %s missing <supportedcontent> element, ignoring", strPath.c_str());
      return false;
    }

    set<CONTENT_TYPE> contents;
    do
    {
      CONTENT_TYPE content = TranslateContent(element->GetText());
      if (content != CONTENT_NONE)
      {
        contents.insert(content);
      }
      element = element->NextSiblingElement("content");
    } while (element != NULL);

    if (contents.empty())
    {
      CLog::Log(LOGERROR, "ADDON: %s %s supports no available content-types, ignoring", TranslateType(addonProps.type).c_str(), addonProps.name.c_str());
      return false;
    }
    else
    {
      addonProps.contents = contents;
    }
  }

  /*** Beginning of optional fields ***/
  /* Retrieve description */
  element = rootElement->FirstChildElement("description");
  if (element)
    addonProps.description = element->GetText();

  /* Retrieve author */
  element = rootElement->FirstChildElement("author");
  if (element)
    addonProps.author = element->GetText();

  /* Retrieve disclaimer */
  element = rootElement->FirstChildElement("disclaimer");
  if (element)
    addonProps.disclaimer = element->GetText();

  /* Retrieve library file name */
  // will be replaced with default library name if unspecified
  element = rootElement->FirstChildElement("library");
  if (element)
    addonProps.libname = element->GetText();

  //TODO move this to addon specific class, if it's needed at all..
#ifdef _WIN32
  /* Retrieve WIN32 library file name in case it is present
  * This is required for no overwrite to the fixed WIN32 add-on's
  * during compile time
  */
  element = rootElement->FirstChildElement("librarywin32");
  if (element) // If it is found overwrite standard library name
    addonProps.libname = element->GetText();
#endif

  /* Retrieve dependencies that this addon requires */
  element = rootElement->FirstChildElement("dependencies");
  if (element)
  {
    element = element->FirstChildElement("dependency");
    if (!element)
      CLog::Log(LOGDEBUG, "ADDON: %s missing at least one <dependency> element, will ignore this dependency", strPath.c_str());
    else
    {
      do
      {
        const char* min = element->Attribute("minversion");
        const char* max = element->Attribute("maxversion");
        const char* id = element->GetText();
        if (!id || (!min && !max))
        {
          CLog::Log(LOGDEBUG, "ADDON: %s malformed <dependency> element, will ignore this dependency", strPath.c_str());
          element = element->NextSiblingElement("dependency");
          continue;
        }
        addonProps.dependencies.insert(make_pair(CStdString(id), make_pair(AddonVersion(min?min:""), AddonVersion(max?max:""))));
        element = element->NextSiblingElement("dependency");
      } while (element != NULL);
    }
  }

  /*** end of optional fields ***/

  /* Create an addon object and store in a shared_ptr */
  addon = AddonFromProps(addonProps);

  return addon.get() != NULL;
}

AddonPtr CAddonMgr::AddonFromProps(AddonProps& addonProps)
{
  switch (addonProps.type)
  {
    case ADDON_PLUGIN:
    case ADDON_SCRIPT:
      return AddonPtr(new CAddon(addonProps));
    case ADDON_SCRAPER:
      return AddonPtr(new CScraper(addonProps));
    case ADDON_VIZ:
      return AddonPtr(new CVisualisation(addonProps));
    case ADDON_SCREENSAVER:
      return AddonPtr(new CScreenSaver(addonProps));
    case ADDON_SCRAPER_LIBRARY:
    case ADDON_VIZ_LIBRARY:
      return AddonPtr(new CAddonLibrary(addonProps));
    case ADDON_REPOSITORY:
      return AddonPtr(new CRepository(addonProps));
    default:
      break;
  }
  return AddonPtr();
}

} /* namespace ADDON */

