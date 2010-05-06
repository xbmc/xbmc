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
#include "AddonDatabase.h"
#include "DllLibCPluff.h"
#include "StringUtils.h"
#include "RegExp.h"
#include "XMLUtils.h"
#include "utils/JobManager.h"
#include "utils/SingleLock.h"
#include "FileItem.h"
#include "LangInfo.h"
#include "Settings.h"
#include "GUISettings.h"
#include "DownloadQueueManager.h"
#include "AdvancedSettings.h"
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
#include "Skin.h"

using namespace std;

namespace ADDON
{

int cp_to_clog(cp_log_severity_t);
cp_log_severity_t clog_to_cp(int);

/**********************************************************
 * CAddonMgr
 *
 */

map<TYPE, IAddonMgrCallback*> CAddonMgr::m_managers;

AddonPtr AddonFactory(const cp_extension_t *props)
{
  const TYPE type = TranslateType(props->ext_point_id);
  switch (type)
  {
    case ADDON_PLUGIN:
    case ADDON_SCRIPT:
      return AddonPtr(new CAddon(props->plugin));
    case ADDON_SCRAPER:
      return AddonPtr(new CScraper(props->plugin));
    case ADDON_VIZ:
      return AddonPtr(new CVisualisation(props->plugin));
    case ADDON_SCREENSAVER:
      return AddonPtr(new CScreenSaver(props->plugin));
    case ADDON_SKIN:
      return AddonPtr(new CSkinInfo(props->plugin));
    case ADDON_SCRAPER_LIBRARY:
    case ADDON_VIZ_LIBRARY:
      return AddonPtr(new CAddonLibrary(props->plugin));
    default:
      return AddonPtr();
  }
}

CAddonMgr::CAddonMgr()
{
  FindAddons();
  m_watch.StartZero();
}

CAddonMgr::~CAddonMgr()
{
  DeInit();
}

CAddonMgr &CAddonMgr::Get()
{
  static CAddonMgr sAddonMgr;
  return sAddonMgr;
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

bool CAddonMgr::Init()
{
  m_cpluff = new DllLibCPluff;
  m_cpluff->Load();

  if (!m_cpluff->IsLoaded())
  {
    CLog::Log(LOGERROR, "ADDONS: Fatal Error, could not load libcpluff");
    return false;
  }

  m_cpluff->set_fatal_error_handler(cp_fatalErrorHandler);

  cp_status_t status;
  status = m_cpluff->init();
  if (status != CP_OK)
  {
    CLog::Log(LOGERROR, "ADDONS: Fatal Error, cp_init() returned status: %i", status);
    return false;
  }

  //TODO could separate addons into different contexts
  // would allow partial unloading of addon framework
  m_cp_context = m_cpluff->create_context(&status);
  assert(m_cp_context);
  if (!CSpecialProtocol::XBMCIsHome())
  {
    status = m_cpluff->register_pcollection(m_cp_context, _P("special://home/addons"));
  }
  status = m_cpluff->register_pcollection(m_cp_context, _P("special://xbmc/addons"));
  if (status != CP_OK)
  {
    CLog::Log(LOGERROR, "ADDONS: Fatal Error, cp_register_pcollection() returned status: %i", status);
    return false;
  }

  status = m_cpluff->register_logger(m_cp_context, cp_logger,
      &CAddonMgr::Get(), clog_to_cp(g_advancedSettings.m_logLevel));
  if (status != CP_OK)
  {
    CLog::Log(LOGERROR, "ADDONS: Fatal Error, cp_register_logger() returned status: %i", status);
    return false;
  }

  status = m_cpluff->scan_plugins(m_cp_context, 0);
  return true;
}

void CAddonMgr::DeInit()
{
  if (m_cpluff)
    m_cpluff->destroy();
  m_cpluff = NULL;
}

bool CAddonMgr::HasAddons(const TYPE &type, const CONTENT_TYPE &content/*= CONTENT_NONE*/, bool enabledOnly/*= true*/)
{
  if (type == ADDON_SCREENSAVER || type == ADDON_SKIN)
  {
    cp_status_t status;
    int num;
    CStdString ext_point(TranslateType(type));
    cp_extension_t **exts = m_cpluff->get_extensions_info(m_cp_context, ext_point.c_str(), &status, &num);
    if (status == CP_OK)
      return (num > 0);
  }

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
  if (CAddonMgr::Get().GetAddons(ADDON_PLUGIN, temp, CONTENT_NONE, enabledOnly))
    addons.insert(addons.end(), temp.begin(), temp.end());
  if (CAddonMgr::Get().GetAddons(ADDON_SCRAPER, temp, CONTENT_NONE, enabledOnly))
    addons.insert(addons.end(), temp.begin(), temp.end());
  if (CAddonMgr::Get().GetAddons(ADDON_SCREENSAVER, temp, CONTENT_NONE, enabledOnly))
    addons.insert(addons.end(), temp.begin(), temp.end());
  if (CAddonMgr::Get().GetAddons(ADDON_SCRIPT, temp, CONTENT_NONE, enabledOnly))
    addons.insert(addons.end(), temp.begin(), temp.end());
  if (CAddonMgr::Get().GetAddons(ADDON_SKIN, temp, CONTENT_NONE, enabledOnly))
    addons.insert(addons.end(), temp.begin(), temp.end());
  if (CAddonMgr::Get().GetAddons(ADDON_VIZ, temp, CONTENT_NONE, enabledOnly))
    addons.insert(addons.end(), temp.begin(), temp.end());
  return !addons.empty();
}

bool CAddonMgr::GetAddons(const TYPE &type, VECADDONS &addons, const CONTENT_TYPE &content/*= CONTENT_NONE*/, bool enabledOnly/*= true*/)
{
  CSingleLock lock(m_critSection);
  addons.clear();
  if (type == ADDON_SCREENSAVER || type == ADDON_SKIN)
  {
    cp_status_t status;
    int num;
    CStdString ext_point(TranslateType(type));
    cp_extension_t **exts = m_cpluff->get_extensions_info(m_cp_context, ext_point.c_str(), &status, &num);
    for(int i=0; i <num; i++)
    {
      AddonPtr addon(AddonFactory(exts[i]));
      if (addon)
        addons.push_back(addon);
    }
    m_cpluff->release_info(m_cp_context, exts);
    return addons.size() > 0;
  }

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
  if (type != ADDON_UNKNOWN
      && type != ADDON_SCREENSAVER
      && type != ADDON_SKIN
      && m_addons.find(type) == m_addons.end())
    return false;

  if (type == ADDON_SCREENSAVER || type == ADDON_SKIN)
  {
    cp_status_t status;
    cp_plugin_info_t *cpaddon = NULL;
    cpaddon = m_cpluff->get_plugin_info(m_cp_context, str.c_str(), &status);
    if (status == CP_OK && cpaddon->extensions)
      return (addon = AddonFactory(cpaddon->extensions));
    else
      return false;
  }

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
  m_addons.clear();
  m_idMap.clear();

  // store any addons with unresolved deps, then recheck at the end
  map<CStdString, AddonPtr> unresolved;

  if (!CSpecialProtocol::XBMCIsHome())
    LoadAddons("special://home/addons",unresolved);
  LoadAddons("special://xbmc/addons",unresolved);

  for (map<CStdString,AddonPtr>::iterator it = unresolved.begin(); 
                                          it != unresolved.end(); ++it)
  {
    if (DependenciesMet(it->second))
    {
      if (!UpdateIfKnown(it->second))
      {
        m_addons[it->second->Type()].push_back(it->second);
        m_idMap.insert(make_pair(it->first,it->second));
      }
    }
  }
}

void CAddonMgr::LoadAddons(const CStdString &path, 
                           map<CStdString, AddonPtr>& unresolved)
{
  // parse the user & system dirs for addons of the requested type
  CFileItemList items;
  CDirectory::GetDirectory(path, items);

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

    // only load if addon with same id isn't already loaded
    if(m_idMap.find(addon->ID()) != m_idMap.end() ||
       unresolved.find(addon->ID()) != unresolved.end())
    {
      CLog::Log(LOGDEBUG, "ADDON: already loaded id %s, bypassing package", addon->ID().c_str());
      continue;
    }

    // refuse to store addons with missing library
    CStdString library(CUtil::AddFileToFolder(addon->Path(), addon->LibName()));
    if (!CFile::Exists(library))
    {
      CLog::Log(LOGDEBUG, "ADDON: Missing library file %s, bypassing package", library.c_str());
      continue;
    }

    if (!DependenciesMet(addon))
    {
      unresolved.insert(make_pair(addon->ID(),addon));
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
      {
        if (dep->Version() < min || dep->Version() > max)
          return false;
      }
      else if (!min.str.IsEmpty())
      {
        if (dep->Version() < min)
          return false;
      }
      else
      {
        if (dep->Version() > max)
          return false;
      }
    }
    itr++;
  }
  return true;
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
    CLog::Log(LOGERROR, "ADDON: %s missing <id> element, ignoring", rootElement->GetDocument()->Value());
    return false;
  }
  id = element->GetText();
  //FIXME since we no longer required uuids, should we bother validating anything?
  if (id.IsEmpty())
  {
    CLog::Log(LOGERROR, "ADDON: %s has invalid <id> element, ignoring", rootElement->GetDocument()->Value());
    return false;
  }

  /* Validate type */
  TYPE type;
  element = rootElement->FirstChildElement("type");
  if (!element)
  {
    CLog::Log(LOGERROR, "ADDON: %s missing <type> element, ignoring", rootElement->GetDocument()->Value());
    return false;
  }
  type = TranslateType(element->GetText());
  if (type == ADDON_UNKNOWN)
  {
    CLog::Log(LOGERROR, "ADDON: %s has invalid type identifier: '%d'", rootElement->GetDocument()->Value(), type);
    return false;
  }

  /* Retrieve Name */
  CStdString name;
  if (!GetTranslatedString(rootElement,"title",name))
  {
    CLog::Log(LOGERROR, "ADDON: %s missing <title> element, ignoring", rootElement->GetDocument()->Value());
    return false;
  }

  /* Retrieve version */
  CStdString version;
  element = rootElement->FirstChildElement("version");
  if (!element)
  {
    CLog::Log(LOGERROR, "ADDON: %s missing <version> element, ignoring", rootElement->GetDocument()->Value());
    return false;
  }
  /* Validate version */
  version = element->GetText();
  CRegExp versionRE;
  versionRE.RegComp(ADDON_VERSION_RE.c_str());
  if (versionRE.RegFind(version.c_str()) != 0)
  {
    CLog::Log(LOGERROR, "ADDON: %s has invalid <version> element, ignoring", rootElement->GetDocument()->Value());
    return false;
  }

  /* Path, ID & Version are valid */
  AddonProps addonProps(id, type, version);
  addonProps.name = name;
  CUtil::GetDirectory(strPath,addonProps.path);
  /* Set Icon */
  addonProps.icon = "icon.png";
  /* Set Changelog */
  addonProps.changelog = CUtil::AddFileToFolder(addonProps.path,"changelog.txt");
  /* Set Fanart */
  addonProps.fanart = CUtil::AddFileToFolder(addonProps.path,"fanart.jpg");

  /* Retrieve license */
  element = rootElement->FirstChildElement("license");
/*  if (!element)
  {
    CLog::Log(LOGERROR, "ADDON: %s missing <license> element, ignoring", rootElement->GetDocument()->Value());
    return false;
  }
  addonProps.license = element->GetText();*/

  /* Retrieve platforms which this addon supports */
  CStdString platform;
  element = rootElement->FirstChildElement("platforms")->FirstChildElement("platform");
  if (!element)
  {
    CLog::Log(LOGERROR, "ADDON: %s missing <platforms> element, ignoring", rootElement->GetDocument()->Value());
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
      CLog::Log(LOGNOTICE, "ADDON: %s is not supported under Linux, ignoring", rootElement->GetDocument()->Value());
      return false;
    }
#elif defined(_WIN32) && defined(HAS_SDL_OPENGL)
    if (!platforms.count("windows-gl") && !platforms.count("windows"))
    {
      CLog::Log(LOGNOTICE, "ADDON: %s is not supported under Windows/OpenGL, ignoring", rootElement->GetDocument()->Value());
      return false;
    }
#elif defined(_WIN32) && defined(HAS_DX)
    if (!platforms.count("windows-dx") && !platforms.count("windows"))
    {
      CLog::Log(LOGNOTICE, "ADDON: %s is not supported under Windows/DirectX, ignoring", rootElement->GetDocument()->Value());
      return false;
    }
#elif defined(__APPLE__)
    if (!platforms.count("osx"))
    {
      CLog::Log(LOGNOTICE, "ADDON: %s is not supported under OSX, ignoring", rootElement->GetDocument()->Value());
      return false;
    }
#elif defined(_XBOX)
    if (!platforms.count("xbox"))
    {
      CLog::Log(LOGNOTICE, "ADDON: %s is not supported under XBOX, ignoring", rootElement->GetDocument()->Value());
      return false;
    }
#endif
  }

  /* Retrieve summary */
  if (!GetTranslatedString(rootElement,"summary",addonProps.summary))
  {
    CLog::Log(LOGERROR, "ADDON: %s missing <summary> element, ignoring", rootElement->GetDocument()->Value());
    return false;
  }

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
      CLog::Log(LOGERROR, "ADDON: %s missing <supportedcontent> element, ignoring", rootElement->GetDocument()->Value());
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
  /* Retrieve icon */
  element = rootElement->FirstChildElement("icon");
  if (element)
    addonProps.icon = element->GetText();

  /* Retrieve description */
  element = rootElement->FirstChildElement("description");
  GetTranslatedString(rootElement,"description",addonProps.description);

  /* Retrieve author */
  element = rootElement->FirstChildElement("author");
  if (element)
    addonProps.author = element->GetText();

  /* Retrieve disclaimer */
  element = rootElement->FirstChildElement("disclaimer");
  GetTranslatedString(rootElement,"disclaimer",addonProps.disclaimer);

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
      CLog::Log(LOGDEBUG, "ADDON: %s missing at least one <dependency> element, will ignore this dependency", rootElement->GetDocument()->Value());
    else
    {
      do
      {
        const char* min = element->Attribute("minversion");
        const char* max = element->Attribute("maxversion");
        const char* id = element->GetText();
        if (!id || (!min && !max))
        {
          CLog::Log(LOGDEBUG, "ADDON: %s malformed <dependency> element, will ignore this dependency", rootElement->GetDocument()->Value());
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

bool CAddonMgr::GetTranslatedString(const TiXmlElement *xmldoc, const char *tag, CStdString& data)
{
  const TiXmlElement *element = xmldoc->FirstChildElement(tag);
  const TiXmlElement *enelement = NULL;
  while (element)
  {
    const char* lang = element->Attribute("lang");
    if (lang && strcmp(lang,g_langInfo.GetDVDAudioLanguage().c_str()) == 0)
      break;
    if (!lang || strcmp(lang,"en") == 0)
      enelement = element;
    element = element->NextSiblingElement(tag);
  }
  if (!element)
    element = enelement;
  if (element)
    data = element->GetText();

  return element != NULL;
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
    case ADDON_SKIN:
      return AddonPtr(new CSkinInfo(addonProps));
    case ADDON_VIZ:
      return AddonPtr(new CVisualisation(addonProps));
    case ADDON_SCREENSAVER:
      return AddonPtr(new CScreenSaver(addonProps));
    case ADDON_SCRAPER_LIBRARY:
    case ADDON_VIZ_LIBRARY:
    case ADDON_SCRIPT_LIBRARY:
      return AddonPtr(new CAddonLibrary(addonProps));
    case ADDON_REPOSITORY:
      return AddonPtr(new CRepository(addonProps));
    default:
      break;
  }
  return AddonPtr();
}

void CAddonMgr::UpdateRepos()
{
  CSingleLock lock(m_critSection);
  if (m_watch.GetElapsedSeconds() < 600)
    return;
  m_watch.StartZero();
  VECADDONS addons;
  GetAddons(ADDON_REPOSITORY,addons);
  for (unsigned int i=0;i<addons.size();++i)
  {
    RepositoryPtr repo = boost::dynamic_pointer_cast<CRepository>(addons[i]);
    if (repo->LastUpdate()+CDateTimeSpan(0,6,0,0) < CDateTime::GetCurrentDateTime())
    {
      CLog::Log(LOGDEBUG,"Checking repository %s for updates",repo->Name().c_str());
      CJobManager::GetInstance().AddJob(new CRepositoryUpdateJob(repo),this);
    }
  }
}

void CAddonMgr::OnJobComplete(unsigned int jobID, bool success, CJob* job)
{
  if (!success)
    return;

  ((CRepositoryUpdateJob*)job)->m_repo->SetUpdated(CDateTime::GetCurrentDateTime());
}

/*
 * libcpluff interaction
 */

CStdString CAddonMgr::GetExtValue(cp_cfg_element_t *base, const char *path)
{
  const char *value = NULL;
  if (base && (value = m_cpluff->lookup_cfg_value(base, path)))
    return CStdString(value);
  else return CStdString();
}

void CAddonMgr::CPluffFatalError(const char *msg)
{
  CLog::Log(LOGERROR, "ADDONS: CPluffFatalError(%s)", msg);
}

int cp_to_clog(cp_log_severity_t lvl)
{
  if( lvl == CP_LOG_DEBUG )
    return 0;
  else if (lvl == CP_LOG_INFO)
    return 1;
  else if (lvl == CP_LOG_WARNING)
    return 3;
  else
    return 4;
}

cp_log_severity_t clog_to_cp(int lvl)
{
  if (lvl >= 4)
    return CP_LOG_ERROR;
  else if (lvl == 3)
    return CP_LOG_WARNING;
  else if (lvl >= 1)
    return CP_LOG_INFO;
  else
    return CP_LOG_DEBUG;
}

void CAddonMgr::CPluffLog(cp_log_severity_t level, const char *msg, const char *apid, void *user_data)
{
  if(!apid)
    CLog::Log(cp_to_clog(level), "ADDON: cpluff: '%s'", msg);
  else
    CLog::Log(cp_to_clog(level), "ADDON: cpluff: '%s' reports '%s'", apid, msg);
}

} /* namespace ADDON */

