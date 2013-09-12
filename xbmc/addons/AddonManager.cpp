/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
#include "AddonManager.h"
#include "Addon.h"
#include "DllLibCPluff.h"
#include "utils/StringUtils.h"
#include "utils/JobManager.h"
#include "threads/SingleLock.h"
#include "FileItem.h"
#include "LangInfo.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "utils/log.h"
#include "utils/XBMCTinyXML.h"
#ifdef HAS_VISUALISATION
#include "Visualisation.h"
#endif
#ifdef HAS_SCREENSAVER
#include "ScreenSaver.h"
#endif
#ifdef HAS_PVRCLIENTS
#include "DllPVRClient.h"
#include "pvr/addons/PVRClient.h"
#endif
#include "games/GameClient.h"
#include "games/GameManager.h"
//#ifdef HAS_SCRAPERS
#include "Scraper.h"
//#endif
#include "PluginSource.h"
#include "Repository.h"
#include "Skin.h"
#include "Service.h"
#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClients.h"
#include "Util.h"

using namespace std;
using namespace GAMES;
using namespace PVR;

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
  if (!PlatformSupportsAddon(props->plugin))
    return AddonPtr();

  /* Check if user directories need to be created */
  const cp_cfg_element_t *settings = GetExtElement(props->configuration, "settings");
  if (settings)
    CheckUserDirs(settings);

  const TYPE type = TranslateType(props->ext_point_id);
  switch (type)
  {
    case ADDON_PLUGIN:
    case ADDON_SCRIPT:
      return AddonPtr(new CPluginSource(props));
    case ADDON_SCRIPT_LIBRARY:
    case ADDON_SCRIPT_LYRICS:
    case ADDON_SCRIPT_SUBTITLES:
    case ADDON_SCRIPT_MODULE:
    case ADDON_WEB_INTERFACE:
      return AddonPtr(new CAddon(props));
    case ADDON_SCRIPT_WEATHER:
      {
        // Eden (API v2.0) broke old weather add-ons
        AddonPtr result(new CAddon(props));
        AddonVersion ver1 = AddonVersion(GetXbmcApiVersionDependency(result));
        AddonVersion ver2 = AddonVersion("2.0");
        if (ver1 < ver2)
        {
          CLog::Log(LOGINFO,"%s: Weather add-ons for api < 2.0 unsupported (%s)",__FUNCTION__,result->ID().c_str());
          return AddonPtr();
        }
        return result;
      }
    case ADDON_SERVICE:
      return AddonPtr(new CService(props));
    case ADDON_SCRAPER_ALBUMS:
    case ADDON_SCRAPER_ARTISTS:
    case ADDON_SCRAPER_MOVIES:
    case ADDON_SCRAPER_MUSICVIDEOS:
    case ADDON_SCRAPER_TVSHOWS:
    case ADDON_SCRAPER_LIBRARY:
      return AddonPtr(new CScraper(props));
    case ADDON_VIZ:
    case ADDON_SCREENSAVER:
    case ADDON_PVRDLL:
    case ADDON_GAMEDLL:
      { // begin temporary platform handling for Dlls
        // ideally platforms issues will be handled by C-Pluff
        // this is not an attempt at a solution
        CStdString value;
        if (type == ADDON_SCREENSAVER && 0 == strnicmp(props->plugin->identifier, "screensaver.xbmc.builtin.", 25))
        { // built in screensaver
          return AddonPtr(new CAddon(props));
        }
        if (type == ADDON_SCREENSAVER)
        { // Python screensaver
          CStdString library = CAddonMgr::Get().GetExtValue(props->configuration, "@library");
          if (URIUtils::HasExtension(library, ".py"))
            return AddonPtr(new CScreenSaver(props));
        }
#if defined(TARGET_ANDROID)                                                                                                                                                      
          if ((value = GetExtValue(props->plugin->extensions->configuration, "@library_android")) && value.empty())                                                                
            break;                                                                                                                                                                 
#elif defined(TARGET_LINUX) || defined(TARGET_FREEBSD)
        if ((value = GetExtValue(props->plugin->extensions->configuration, "@library_linux")) && value.empty())
          break;
#elif defined(TARGET_WINDOWS) && defined(HAS_SDL_OPENGL)
        if ((value = GetExtValue(props->plugin->extensions->configuration, "@library_wingl")) && value.empty())
          break;
#elif defined(TARGET_WINDOWS) && defined(HAS_DX)
        if ((value = GetExtValue(props->plugin->extensions->configuration, "@library_windx")) && value.empty())
          break;
#elif defined(TARGET_DARWIN)
        if ((value = GetExtValue(props->plugin->extensions->configuration, "@library_osx")) && value.empty())
          break;
#endif
        if (type == ADDON_VIZ)
        {
#if defined(HAS_VISUALISATION)
          return AddonPtr(new CVisualisation(props));
#endif
        }
        else if (type == ADDON_PVRDLL)
        {
#ifdef HAS_PVRCLIENTS
          return AddonPtr(new CPVRClient(props));
#endif
        }
        else if (type == ADDON_GAMEDLL)
        {
          return AddonPtr(new CGameClient(props));
        }
        else
          return AddonPtr(new CScreenSaver(props));
      }
    case ADDON_SKIN:
      return AddonPtr(new CSkinInfo(props));
    case ADDON_VIZ_LIBRARY:
      return AddonPtr(new CAddonLibrary(props));
    case ADDON_REPOSITORY:
      return AddonPtr(new CRepository(props));
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

  ELEMENTS elements;
  if (!GetExtElements((cp_cfg_element_t *)userdirs, "userdir", elements))
    return false;

  ELEMENTS::iterator itr = elements.begin();
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
  m_cpluff = NULL;

  // Create the service manager
  CServiceManager::Get();
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

  m_database.Open();

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
  status = m_cpluff->register_pcollection(m_cp_context, CSpecialProtocol::TranslatePath("special://home/addons"));
  status = m_cpluff->register_pcollection(m_cp_context, CSpecialProtocol::TranslatePath("special://xbmc/addons"));
  status = m_cpluff->register_pcollection(m_cp_context, CSpecialProtocol::TranslatePath("special://xbmcbin/addons"));
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
  // TODO: This involves loading a couple DLLs, so call it delayed or outside the main thread
  RegisterGameClientAddons();
  return true;
}

void CAddonMgr::RegisterGameClientAddons()
{
  VECADDONS gameClients;
  GetAddons(ADDON_GAMEDLL, gameClients, true);
  CGameManager::Get().RegisterAddons(gameClients);
}

void CAddonMgr::DeInit()
{
  if (m_cpluff)
    m_cpluff->destroy();
  delete m_cpluff;
  m_cpluff = NULL;
  m_database.Close();
}

bool CAddonMgr::HasAddons(const TYPE &type, bool enabled /*= true*/)
{
  // TODO: This isn't particularly efficient as we create an addon type for each addon using the Factory, just so
  //       we can check addon dependencies in the addon constructor.
  VECADDONS addons;
  return GetAddons(type, addons, enabled);
}

bool CAddonMgr::GetAllAddons(VECADDONS &addons, bool enabled /*= true*/, bool allowRepos /* = false */)
{
  for (int i = ADDON_UNKNOWN+1; i < ADDON_VIZ_LIBRARY; ++i)
  {
    if (!allowRepos && ADDON_REPOSITORY == (TYPE)i)
      continue;
    VECADDONS temp;
    if (CAddonMgr::Get().GetAddons((TYPE)i, temp, enabled))
      addons.insert(addons.end(), temp.begin(), temp.end());
  }
  return !addons.empty();
}

void CAddonMgr::AddToUpdateableAddons(AddonPtr &pAddon)
{
  CSingleLock lock(m_critSection);
  m_updateableAddons.push_back(pAddon);
}

void CAddonMgr::RemoveFromUpdateableAddons(AddonPtr &pAddon)
{
  CSingleLock lock(m_critSection);
  VECADDONS::iterator it = std::find(m_updateableAddons.begin(), m_updateableAddons.end(), pAddon);
  
  if(it != m_updateableAddons.end())
  {
    m_updateableAddons.erase(it);
  }
}

struct AddonIdFinder 
{ 
    AddonIdFinder(const CStdString& id)
      : m_id(id)
    {}
    
    bool operator()(const AddonPtr& addon) 
    { 
      return m_id.Equals(addon->ID()); 
    }
    private:
    CStdString m_id;
};

bool CAddonMgr::ReloadSettings(const CStdString &id)
{
  CSingleLock lock(m_critSection);
  VECADDONS::iterator it = std::find_if(m_updateableAddons.begin(), m_updateableAddons.end(), AddonIdFinder(id));
  
  if( it != m_updateableAddons.end())
  {
    return (*it)->ReloadSettings();
  }
  return false;
}

bool CAddonMgr::GetAllOutdatedAddons(VECADDONS &addons, bool enabled /*= true*/)
{
  CSingleLock lock(m_critSection);
  for (int i = ADDON_UNKNOWN+1; i < ADDON_VIZ_LIBRARY; ++i)
  {
    VECADDONS temp;
    if (CAddonMgr::Get().GetAddons((TYPE)i, temp, enabled))
    {
      AddonPtr repoAddon;
      for (unsigned int j = 0; j < temp.size(); j++)
      {
        // Ignore duplicates due to add-ons with multiple extension points
        bool found = false;
        for (VECADDONS::const_iterator addonIt = addons.begin(); addonIt != addons.end(); addonIt++)
        {
          if ((*addonIt)->ID() == temp[j]->ID())
            found = true;
        }

        if (found || !m_database.GetAddon(temp[j]->ID(), repoAddon))
          continue;

        if (temp[j]->Version() < repoAddon->Version() &&
            !m_database.IsAddonBlacklisted(temp[j]->ID(),
                                           repoAddon->Version().c_str()))
          addons.push_back(repoAddon);
      }
    }
  }
  return !addons.empty();
}

bool CAddonMgr::HasOutdatedAddons(bool enabled /*= true*/)
{
  VECADDONS dummy;
  return GetAllOutdatedAddons(dummy,enabled);
}

bool CAddonMgr::GetAddons(const TYPE &type, VECADDONS &addons, bool enabled /* = true */)
{
  CSingleLock lock(m_critSection);
  addons.clear();
  cp_status_t status;
  int num;
  CStdString ext_point(TranslateType(type));
  cp_extension_t **exts = m_cpluff->get_extensions_info(m_cp_context, ext_point.c_str(), &status, &num);
  for(int i=0; i <num; i++)
  {
    const cp_extension_t *props = exts[i];
    if (m_database.IsAddonDisabled(props->plugin->identifier) != enabled)
    {
      // get a pointer to a running pvrclient if it's already started, or we won't be able to change settings
      if (TranslateType(props->ext_point_id) == ADDON_PVRDLL &&
          enabled &&
          g_PVRManager.IsStarted())
      {
        AddonPtr pvrAddon;
        if (g_PVRClients->GetClient(props->plugin->identifier, pvrAddon))
        {
          addons.push_back(pvrAddon);
          continue;
        }
      }

      if (enabled && TranslateType(props->ext_point_id) == ADDON_GAMEDLL)
      {
        GameClientPtr gameClient;
        if (CGameManager::Get().GetClient(props->plugin->identifier, gameClient))
        {
          addons.push_back(gameClient);
          continue;
        }
      }

      AddonPtr addon(Factory(props));
      if (addon)
        addons.push_back(addon);
    }
  }
  m_cpluff->release_info(m_cp_context, exts);
  return addons.size() > 0;
}

bool CAddonMgr::GetAddon(const CStdString &str, AddonPtr &addon, const TYPE &type/*=ADDON_UNKNOWN*/, bool enabledOnly /*= true*/)
{
  CSingleLock lock(m_critSection);

  cp_status_t status;
  cp_plugin_info_t *cpaddon = m_cpluff->get_plugin_info(m_cp_context, str.c_str(), &status);
  if (status == CP_OK && cpaddon)
  {
    addon = GetAddonFromDescriptor(cpaddon, type==ADDON_UNKNOWN?"":TranslateType(type));
    m_cpluff->release_info(m_cp_context, cpaddon);

    if (addon && addon.get())
    {
      if (enabledOnly && m_database.IsAddonDisabled(addon->ID()))
        return false;

      if (addon->Type() == ADDON_PVRDLL && g_PVRManager.IsStarted())
      {
        AddonPtr pvrAddon;
        if (g_PVRClients->GetClient(addon->ID(), pvrAddon))
          addon = pvrAddon;
      }
    }
    return NULL != addon.get();
  }
  if (cpaddon)
    m_cpluff->release_info(m_cp_context, cpaddon);

  return false;
}

//TODO handle all 'default' cases here, not just scrapers & vizs
bool CAddonMgr::GetDefault(const TYPE &type, AddonPtr &addon)
{
  CStdString setting;
  switch (type)
  {
  case ADDON_VIZ:
    setting = CSettings::Get().GetString("musicplayer.visualisation");
    break;
  case ADDON_SCREENSAVER:
    setting = CSettings::Get().GetString("screensaver.mode");
    break;
  case ADDON_SCRAPER_ALBUMS:
    setting = CSettings::Get().GetString("musiclibrary.albumsscraper");
    break;
  case ADDON_SCRAPER_ARTISTS:
    setting = CSettings::Get().GetString("musiclibrary.artistsscraper");
    break;
  case ADDON_SCRAPER_MOVIES:
    setting = CSettings::Get().GetString("scrapers.moviesdefault");
    break;
  case ADDON_SCRAPER_MUSICVIDEOS:
    setting = CSettings::Get().GetString("scrapers.musicvideosdefault");
    break;
  case ADDON_SCRAPER_TVSHOWS:
    setting = CSettings::Get().GetString("scrapers.tvshowsdefault");
    break;
  case ADDON_WEB_INTERFACE:
    setting = CSettings::Get().GetString("services.webskin");
    break;
  default:
    return false;
  }
  return GetAddon(setting, addon, type);
}

bool CAddonMgr::SetDefault(const TYPE &type, const CStdString &addonID)
{
  switch (type)
  {
  case ADDON_VIZ:
    CSettings::Get().SetString("musicplayer.visualisation",addonID);
    break;
  case ADDON_SCREENSAVER:
    CSettings::Get().SetString("screensaver.mode",addonID);
    break;
  case ADDON_SCRAPER_ALBUMS:
    CSettings::Get().SetString("musiclibrary.albumsscraper",addonID);
    break;
  case ADDON_SCRAPER_ARTISTS:
    CSettings::Get().SetString("musiclibrary.artistsscraper",addonID);
    break;
  case ADDON_SCRAPER_MOVIES:
    CSettings::Get().SetString("scrapers.moviesdefault",addonID);
    break;
  case ADDON_SCRAPER_MUSICVIDEOS:
    CSettings::Get().SetString("scrapers.musicvideosdefault",addonID);
    break;
  case ADDON_SCRAPER_TVSHOWS:
    CSettings::Get().SetString("scrapers.tvshowsdefault",addonID);
    break;
  default:
    return false;
  }

  return true;
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
  {
    CSingleLock lock(m_critSection);
    if (m_cpluff && m_cp_context)
    {
      m_cpluff->scan_plugins(m_cp_context, CP_SP_UPGRADE);
      SetChanged();
    }
  }
  NotifyObservers(ObservableMessageAddons);
}

void CAddonMgr::RemoveAddon(const CStdString& ID)
{
  if (m_cpluff && m_cp_context)
  {
    m_cpluff->uninstall_plugin(m_cp_context,ID.c_str());
    SetChanged();
    NotifyObservers(ObservableMessageAddons);
  }
  // Let the game manager update the information associated with this addon
  CGameManager::Get().UnregisterAddonByID(ID);
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
      return AddonPtr(new CPluginSource(addonProps));
    case ADDON_SCRIPT_LIBRARY:
    case ADDON_SCRIPT_LYRICS:
    case ADDON_SCRIPT_WEATHER:
    case ADDON_SCRIPT_SUBTITLES:
    case ADDON_SCRIPT_MODULE:
    case ADDON_WEB_INTERFACE:
      return AddonPtr(new CAddon(addonProps));
    case ADDON_SERVICE:
      return AddonPtr(new CService(addonProps));
    case ADDON_SCRAPER_ALBUMS:
    case ADDON_SCRAPER_ARTISTS:
    case ADDON_SCRAPER_MOVIES:
    case ADDON_SCRAPER_MUSICVIDEOS:
    case ADDON_SCRAPER_TVSHOWS:
    case ADDON_SCRAPER_LIBRARY:
      return AddonPtr(new CScraper(addonProps));
    case ADDON_SKIN:
      return AddonPtr(new CSkinInfo(addonProps));
#if defined(HAS_VISUALISATION)
    case ADDON_VIZ:
      return AddonPtr(new CVisualisation(addonProps));
#endif
    case ADDON_SCREENSAVER:
      return AddonPtr(new CScreenSaver(addonProps));
    case ADDON_VIZ_LIBRARY:
      return AddonPtr(new CAddonLibrary(addonProps));
    case ADDON_PVRDLL:
      return AddonPtr(new CPVRClient(addonProps));
    case ADDON_GAMEDLL:
      return AddonPtr(new CGameClient(addonProps));
    case ADDON_REPOSITORY:
      return AddonPtr(new CRepository(addonProps));
    default:
      break;
  }
  return AddonPtr();
}

/*
 * libcpluff interaction
 */

bool CAddonMgr::PlatformSupportsAddon(const cp_plugin_info_t *plugin) const
{
  if (!plugin || !plugin->num_extensions)
    return false;
  const cp_extension_t *metadata = GetExtension(plugin, "xbmc.addon.metadata");
  if (!metadata)
    return false;

  vector<CStdString> platforms;
  if (CAddonMgr::Get().GetExtList(metadata->configuration, "platform", platforms))
  {
    for (unsigned int i = 0; i < platforms.size(); ++i)
    {
      if (platforms[i] == "all")
        return true;
#if defined(TARGET_ANDROID)
      if (platforms[i] == "android")
#elif defined(TARGET_LINUX) || defined(TARGET_FREEBSD)
      if (platforms[i] == "linux")
#elif defined(TARGET_WINDOWS) && defined(HAS_SDL_OPENGL)
      if (platforms[i] == "wingl")
#elif defined(TARGET_WINDOWS) && defined(HAS_DX)
      if (platforms[i] == "windx")
#elif defined(TARGET_DARWIN_OSX)
// Remove this after Frodo and add an architecture filter
// in addition to platform.
#if defined(__x86_64__)
      if (platforms[i] == "osx64" || platforms[i] == "osx")
#else
      if (platforms[i] == "osx32" || platforms[i] == "osx")
#endif
#elif defined(TARGET_DARWIN_IOS)
      if (platforms[i] == "ios")
#endif
        return true;
    }
    return false; // no <platform> works for us
  }
  return true; // assume no <platform> is equivalent to <platform>all</platform>
}

const cp_cfg_element_t *CAddonMgr::GetExtElement(cp_cfg_element_t *base, const char *path)
{
  const cp_cfg_element_t *element = NULL;
  if (base)
    element = m_cpluff->lookup_cfg_element(base, path);
  return element;
}

bool CAddonMgr::GetExtElements(cp_cfg_element_t *base, const char *path, ELEMENTS &elements)
{
  if (!base || !path)
    return false;

  for (unsigned int i = 0; i < base->num_children; i++)
  {
    CStdString temp = base->children[i].name;
    if (!temp.compare(path))
      elements.push_back(&base->children[i]);
  }

  return !elements.empty();
}

const cp_extension_t *CAddonMgr::GetExtension(const cp_plugin_info_t *props, const char *extension) const
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

CStdString CAddonMgr::GetExtValue(cp_cfg_element_t *base, const char *path)
{
  const char *value = NULL;
  if (base && (value = m_cpluff->lookup_cfg_value(base, path)))
    return CStdString(value);
  else return CStdString();
}

bool CAddonMgr::GetExtList(cp_cfg_element_t *base, const char *path, vector<CStdString> &result) const
{
  if (!base || !path)
    return false;
  CStdString all = m_cpluff->lookup_cfg_value(base, path);
  if (all.IsEmpty())
    return false;
  StringUtils::SplitString(all, " ", result);
  return true;
}

AddonPtr CAddonMgr::GetAddonFromDescriptor(const cp_plugin_info_t *info,
                                           const CStdString& type)
{
  if (!info)
    return AddonPtr();

  if (!info->extensions)
  { // no extensions, so we need only the dep information
    return AddonPtr(new CAddon(info));
  }

  // FIXME: If we want to support multiple extension points per addon, we'll need to extend this to not just take
  //        the first extension point (eg use the TYPE information we pass in)

  // grab a relevant extension point, ignoring our xbmc.addon.metadata extension point
  for (unsigned int i = 0; i < info->num_extensions; ++i)
  {
    if (0 != strcmp("xbmc.addon.metadata", info->extensions[i].ext_point_id) &&
        (type.empty() || 0 == strcmp(type.c_str(), info->extensions[i].ext_point_id)))
    { // note that Factory takes care of whether or not we have platform support
      return Factory(&info->extensions[i]);
    }
  }
  return AddonPtr();
}

// FIXME: This function may not be required
bool CAddonMgr::LoadAddonDescription(const CStdString &path, AddonPtr &addon)
{
  cp_status_t status;
  cp_plugin_info_t *info = m_cpluff->load_plugin_descriptor(m_cp_context, CSpecialProtocol::TranslatePath(path).c_str(), &status);
  if (info)
  {
    addon = GetAddonFromDescriptor(info);
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
      AddonPtr addon = GetAddonFromDescriptor(info);
      if (addon.get())
        addons.push_back(addon);
      m_cpluff->release_info(context, info);
    }
    element = element->NextSiblingElement("addon");
  }
  m_cpluff->destroy_context(context);
  return true;
}

bool CAddonMgr::LoadAddonDescriptionFromMemory(const TiXmlElement *root, AddonPtr &addon)
{
  // create a context for these addons
  cp_status_t status;
  cp_context_t *context = m_cpluff->create_context(&status);
  if (!root || !context)
    return false;

  // dump the XML back to text
  std::string xml;
  xml << TiXmlDeclaration("1.0", "UTF-8", "");
  xml << *root;
  cp_plugin_info_t *info = m_cpluff->load_plugin_descriptor_from_memory(context, xml.c_str(), xml.size(), &status);
  if (info)
  {
    addon = GetAddonFromDescriptor(info);
    m_cpluff->release_info(context, info);
  }
  m_cpluff->destroy_context(context);
  return addon != NULL;
}

bool CAddonMgr::StartServices(const bool beforelogin)
{
  CLog::Log(LOGDEBUG, "ADDON: Starting service addons.");

  VECADDONS services;
  if (!GetAddons(ADDON_SERVICE, services))
    return false;

  bool ret = true;
  for (IVECADDONS it = services.begin(); it != services.end(); ++it)
  {
    boost::shared_ptr<CService> service = boost::dynamic_pointer_cast<CService>(*it);
    if (service)
    {
      if ( (beforelogin && service->GetStartOption() == CService::STARTUP)
        || (!beforelogin && service->GetStartOption() == CService::LOGIN) )
        ret &= service->Start();
    }
  }

  return ret;
}

void CAddonMgr::StopServices(const bool onlylogin)
{
  CLog::Log(LOGDEBUG, "ADDON: Stopping service addons.");

  VECADDONS services;
  if (!GetAddons(ADDON_SERVICE, services))
    return;

  for (IVECADDONS it = services.begin(); it != services.end(); ++it)
  {
    boost::shared_ptr<CService> service = boost::dynamic_pointer_cast<CService>(*it);
    if (service)
    {
      if ( (onlylogin && service->GetStartOption() == CService::LOGIN)
        || (!onlylogin) )
        service->Stop();
    }
  }
}

int cp_to_clog(cp_log_severity_t lvl)
{
  if (lvl >= CP_LOG_ERROR)
    return LOGINFO;
  return LOGDEBUG;
}

cp_log_severity_t clog_to_cp(int lvl)
{
  if (lvl >= LOG_LEVEL_DEBUG)
    return CP_LOG_INFO;
  return CP_LOG_ERROR;
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

