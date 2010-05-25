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

cp_log_severity_t clog_to_cp(int lvl);
void cp_fatalErrorHandler(const char *msg);
void cp_logger(cp_log_severity_t level, const char *msg, const char *apid, void *user_data);

/**********************************************************
 * CAddonMgr
 *
 */

map<TYPE, IAddonMgrCallback*> CAddonMgr::m_managers;

AddonPtr CAddonMgr::Factory(const cp_extension_t *props)
{
  /* Check if user directories need to be created */
  const cp_cfg_element_t *settings = GetExtElement(props->plugin->extensions->configuration, "settings");
  if (settings)
    CheckUserDirs(settings);

  const TYPE type = TranslateType(props->ext_point_id);
  switch (type)
  {
    case ADDON_PLUGIN:
    case ADDON_SCRIPT:
    case ADDON_SCRIPT_LIBRARY:
    case ADDON_SCRIPT_LYRICS:
    case ADDON_SCRIPT_WEATHER:
    case ADDON_SCRIPT_SUBTITLES:
      return AddonPtr(new CAddon(props->plugin));
    case ADDON_SCRAPER:
      return AddonPtr(new CScraper(props->plugin));
    case ADDON_VIZ:
    case ADDON_SCREENSAVER:
      { // begin temporary platform handling for Dlls
        // ideally platforms issues will be handled by C-Pluff
        // this is not an attempt at a solution
        CStdString value;
#if defined(_LINUX) && !defined(__APPLE__)
        if ((value = GetExtValue(props->plugin->extensions->configuration, "@library_linux")) && value.empty())
          break;
#elif defined(_WIN32) && defined(HAS_SDL_OPENGL)
        if ((value = GetExtValue(props->plugin->extensions->configuration, "@library_wingl")) && value.empty())
          break;
#elif defined(_WIN32) && defined(HAS_DX)
        if ((value = GetExtValue(props->plugin->extensions->configuration, "@library_windx")) && value.empty())
          break;
#elif defined(__APPLE__)
        if ((value = GetExtValue(props->plugin->extensions->configuration, "@library_osx")) && value.empty())
          break;
#elif defined(_XBOX)
        if ((value = GetExtValue(props->plugin->extensions->configuration, "@library_xbox")) && value.empty())
          break;
#endif
        if (type == ADDON_VIZ)
        {
#if defined(HAS_VISUALISATION)
          return AddonPtr(new CVisualisation(props->plugin));
#endif
        }
        else
          return AddonPtr(new CScreenSaver(props->plugin));
      }
    case ADDON_SKIN:
      return AddonPtr(new CSkinInfo(props->plugin));
    case ADDON_SCRAPER_LIBRARY:
    case ADDON_VIZ_LIBRARY:
      return AddonPtr(new CAddonLibrary(props->plugin));
    case ADDON_REPOSITORY:
      return AddonPtr(new CRepository(props->plugin));
    default:
      break;
  }
  return AddonPtr();
}

bool CAddonMgr::CheckUserDirs(const cp_cfg_element_t *settings)
{
  if (!settings)
    return false;

  const cp_cfg_element_t *userdirs = GetExtElement((cp_cfg_element_t *)settings, "userdirs");
  if (!userdirs)
    return false;

  DEQUEELEMENTS elements;
  bool status = GetExtElementDeque(elements, (cp_cfg_element_t *)userdirs, "userdir");
  if (!status)
    return false;

  IDEQUEELEMENTS itr = elements.begin();
  while (itr != elements.end())
  {
    CStdString path = GetExtValue(*itr++, "@path");
    if (!CFile::Exists(path))
    {
      if (!CUtil::CreateDirectoryEx(path))
      {
        CLog::Log(LOGERROR, "CAddonMgr::CheckUserDirs: Unable to create directory %s.", path.c_str());
        return false;
      }
    }
  }

  return true;
}

CAddonMgr::CAddonMgr()
{
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
  status = m_cpluff->register_pcollection(m_cp_context, _P("special://home/addons"));
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

  FindAddons();
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
  // TODO: This isn't particularly efficient as we create an addon type for each addon using the Factory, just so
  //       we can check addon dependencies in the addon constructor.
  VECADDONS addons;
  return GetAddons(type, addons, content, enabledOnly);
}

bool CAddonMgr::GetAllAddons(VECADDONS &addons, bool enabledOnly/*= true*/)
{
  for (int i = ADDON_UNKNOWN+1; i < ADDON_VIZ_LIBRARY; ++i)
  {
    if (ADDON_REPOSITORY == (TYPE)i)
      continue;
    VECADDONS temp;
    if (CAddonMgr::Get().GetAddons((TYPE)i, temp, CONTENT_NONE, enabledOnly))
      addons.insert(addons.end(), temp.begin(), temp.end());
  }
  return !addons.empty();
}

bool CAddonMgr::GetAddons(const TYPE &type, VECADDONS &addons, const CONTENT_TYPE &content/*= CONTENT_NONE*/, bool enabledOnly/*= true*/)
{
  CSingleLock lock(m_critSection);
  addons.clear();
  cp_status_t status;
  int num;
  CStdString ext_point(TranslateType(type));
  cp_extension_t **exts = m_cpluff->get_extensions_info(m_cp_context, ext_point.c_str(), &status, &num);
  for(int i=0; i <num; i++)
  {
    AddonPtr addon(Factory(exts[i]));
    if (addon && (content == CONTENT_NONE || addon->Supports(content)))
      addons.push_back(addon);
  }
  m_cpluff->release_info(m_cp_context, exts);
  return addons.size() > 0;
}

bool CAddonMgr::GetAddon(const CStdString &str, AddonPtr &addon, const TYPE &type/*=ADDON_UNKNOWN*/, bool enabledOnly/*= true*/)
{
  CSingleLock lock(m_critSection);

  cp_status_t status;
  cp_plugin_info_t *cpaddon = m_cpluff->get_plugin_info(m_cp_context, str.c_str(), &status);
  if (status == CP_OK && cpaddon->extensions)
  {
    addon = Factory(cpaddon->extensions);
    m_cpluff->release_info(m_cp_context, cpaddon);
    return NULL != addon.get();
  }
  if (cpaddon)
    m_cpluff->release_info(m_cp_context, cpaddon);

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
  AddonPtr addon;
  if (GetAddon(id, addon))
    return addon->GetString(number);

  return "";
}

void CAddonMgr::FindAddons()
{
  CSingleLock lock(m_critSection);
  if (m_cpluff && m_cp_context)
    m_cpluff->scan_plugins(m_cp_context, 0);
}

void CAddonMgr::RemoveAddon(const CStdString& ID)
{
  if (m_cpluff && m_cp_context)
    m_cpluff->uninstall_plugin(m_cp_context,ID.c_str());
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

const char *CAddonMgr::GetTranslatedString(const cp_cfg_element_t *root, const char *tag)
{
  if (!root)
    return NULL;

  const cp_cfg_element_t *eng = NULL;
  for (unsigned int i = 0; i < root->num_children; i++)
  {
    const cp_cfg_element_t &child = root->children[i];
    if (strcmp(tag, child.name) == 0)
    { // see if we have a "lang" attribute
      const char *lang = m_cpluff->lookup_cfg_value((cp_cfg_element_t*)&child, "@lang");
      if (lang && 0 == strcmp(lang,g_langInfo.GetDVDAudioLanguage().c_str()))
        return child.value;
      if (!lang || 0 == strcmp(lang, "en"))
        eng = &child;
    }
  }
  return (eng) ? eng->value : NULL;
}

AddonPtr CAddonMgr::AddonFromProps(AddonProps& addonProps)
{
  switch (addonProps.type)
  {
    case ADDON_PLUGIN:
    case ADDON_SCRIPT:
    case ADDON_SCRIPT_LIBRARY:
    case ADDON_SCRIPT_LYRICS:
    case ADDON_SCRIPT_WEATHER:
    case ADDON_SCRIPT_SUBTITLES:
      return AddonPtr(new CAddon(addonProps));
    case ADDON_SCRAPER:
      return AddonPtr(new CScraper(addonProps));
    case ADDON_SKIN:
      return AddonPtr(new CSkinInfo(addonProps));
#if defined(HAS_VISUALISATION)
    case ADDON_VIZ:
      return AddonPtr(new CVisualisation(addonProps));
#endif
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

const cp_cfg_element_t *CAddonMgr::GetExtElement(cp_cfg_element_t *base, const char *path)
{
  const cp_cfg_element_t *element = NULL;
  if (base)
    element = m_cpluff->lookup_cfg_element(base, path);
  return element;
}

/* Returns all duplicate elements from a base element */
bool CAddonMgr::GetExtElementDeque(DEQUEELEMENTS &elements, cp_cfg_element_t *base, const char *path)
{
  if (!base)
    return false;

  unsigned int i = 0;
  while (true)
  {
    if (i >= base->num_children)
      break;
    CStdString temp = (base->children+i)->name;
    if (!temp.compare(path))
      elements.push_back(base->children+i);
    i++;
  }

  if (elements.empty()) return false;
  return true;
}

const cp_extension_t *CAddonMgr::GetExtension(const cp_plugin_info_t *props, const char *extension)
{
  if (!props)
    return NULL;
  for (unsigned int i = 0; i < props->num_extensions; ++i)
  {
    if (0 == strcmp(props->extensions[i].ext_point_id, extension))
      return &props->extensions[i];
  }
  return NULL;
}

ADDONDEPS CAddonMgr::GetDeps(const CStdString &id)
{
  ADDONDEPS result;
  cp_status_t status;

  cp_plugin_info_t *info = m_cpluff->get_plugin_info(m_cp_context,id.c_str(),&status);
  if (info)
  {
    for (unsigned int i=0;i<info->num_imports;++i)
      result.insert(make_pair(CStdString(info->imports[i].plugin_id),
                              make_pair(AddonVersion(info->version),
                                        AddonVersion(info->version))));
    m_cpluff->release_info(m_cp_context, info);
  }

  return result;
}

CStdString CAddonMgr::GetExtValue(cp_cfg_element_t *base, const char *path)
{
  const char *value = NULL;
  if (base && (value = m_cpluff->lookup_cfg_value(base, path)))
    return CStdString(value);
  else return CStdString();
}

vector<CStdString> CAddonMgr::GetExtValues(cp_cfg_element_t *base, const char *path)
{
  vector<CStdString> result;
  cp_cfg_element_t *parent = m_cpluff->lookup_cfg_element(base,path);
  for (unsigned int i=0;parent && i<parent->num_children;++i)
    result.push_back(parent->children[i].value); 

  return result;
}

bool CAddonMgr::LoadAddonDescription(const CStdString &path, AddonPtr &addon)
{
  cp_status_t status;
  cp_plugin_info_t *info = m_cpluff->load_plugin_descriptor(m_cp_context, _P(path).c_str(), &status);
  if (info)
  {
    addon = Factory(info->extensions);
    m_cpluff->release_info(m_cp_context, info);
    return NULL != addon.get();
  }
  return false;
}

bool CAddonMgr::AddonsFromRepoXML(const TiXmlElement *root, VECADDONS &addons)
{
  // create a context for these addons
  cp_status_t status;
  cp_context_t *context = m_cpluff->create_context(&status);
  if (!root || !context)
    return false;

  // each addon XML should have a UTF-8 declaration
  TiXmlDeclaration decl("1.0", "UTF-8", "");
  const TiXmlElement *element = root->FirstChildElement("addon");
  while (element)
  {
    // dump the XML back to text
    std::string xml;
    xml << decl;
    xml << *element;
    cp_status_t status;
    cp_plugin_info_t *info = m_cpluff->load_plugin_descriptor_from_memory(context, xml.c_str(), xml.size(), &status);
    if (info)
    {
      AddonPtr addon = Factory(info->extensions);
      m_cpluff->release_info(context, info);
      // FIXME: sanity check here that the addon satisfies our requirements?
      if (addon.get())
        addons.push_back(addon);
    }
    element = element->NextSiblingElement("addon");
  }
  m_cpluff->destroy_context(context);
  return true;
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

void cp_fatalErrorHandler(const char *msg)
{
  CLog::Log(LOGERROR, "ADDONS: CPluffFatalError(%s)", msg);
}

void cp_logger(cp_log_severity_t level, const char *msg, const char *apid, void *user_data)
{
  if(!apid)
    CLog::Log(cp_to_clog(level), "ADDON: cpluff: '%s'", msg);
  else
    CLog::Log(cp_to_clog(level), "ADDON: cpluff: '%s' reports '%s'", apid, msg);
}

} /* namespace ADDON */

