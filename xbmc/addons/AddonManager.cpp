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
#include <memory>
#include "AddonManager.h"
#include "Addon.h"
#include "AudioEncoder.h"
#include "AudioDecoder.h"
#include "ContextMenuManager.h"
#include "DllLibCPluff.h"
#include "LanguageResource.h"
#include "UISoundsResource.h"
#include "utils/StringUtils.h"
#include "utils/JobManager.h"
#include "threads/SingleLock.h"
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
#include "pvr/addons/PVRClient.h"
#endif
//#ifdef HAS_SCRAPERS
#include "Scraper.h"
//#endif
#include "PluginSource.h"
#include "Repository.h"
#include "Skin.h"
#include "Service.h"
#include "ContextItemAddon.h"
#include "Util.h"
#include "addons/Webinterface.h"

using namespace std;
using namespace XFILE;

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
    case ADDON_SCRIPT_MODULE:
    case ADDON_SUBTITLE_MODULE:
      return AddonPtr(new CAddon(props));
    case ADDON_WEB_INTERFACE:
      return AddonPtr(new CWebinterface(props));
    case ADDON_SCRIPT_WEATHER:
      {
        // Eden (API v2.0) broke old weather add-ons
        AddonPtr result(new CAddon(props));
        AddonVersion ver1 = result->GetDependencyVersion("xbmc.python");
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
    case ADDON_AUDIOENCODER:
    case ADDON_AUDIODECODER:
      { // begin temporary platform handling for Dlls
        // ideally platforms issues will be handled by C-Pluff
        // this is not an attempt at a solution
        std::string value;
        if (type == ADDON_SCREENSAVER && 0 == strnicmp(props->plugin->identifier, "screensaver.xbmc.builtin.", 25))
        { // built in screensaver
          return AddonPtr(new CAddon(props));
        }
        if (type == ADDON_SCREENSAVER)
        { // Python screensaver
          std::string library = CAddonMgr::Get().GetExtValue(props->configuration, "@library");
          if (URIUtils::HasExtension(library, ".py"))
            return AddonPtr(new CScreenSaver(props));
        }
        if (type == ADDON_AUDIOENCODER && 0 == strncmp(props->plugin->identifier,
                                                       "audioencoder.xbmc.builtin.", 26))
        { // built in audio encoder
          return AddonPtr(new CAudioEncoder(props));
        }
        std::string tograb;
#if defined(TARGET_ANDROID)
          tograb = "@library_android";
#elif defined(TARGET_LINUX) || defined(TARGET_FREEBSD)
          tograb = "@library_linux";
#elif defined(TARGET_WINDOWS) && defined(HAS_DX)
          tograb = "@library_windx";
#elif defined(TARGET_DARWIN)
          tograb = "@library_osx";
#endif
        value = GetExtValue(props->plugin->extensions->configuration, tograb.c_str());
        if (value.empty())
          break;
        if (type == ADDON_VIZ)
        {
#if defined(HAS_VISUALISATION)
          return AddonPtr(new CVisualisation(props));
#endif
        }
        else if (type == ADDON_PVRDLL)
        {
#ifdef HAS_PVRCLIENTS
          return AddonPtr(new PVR::CPVRClient(props));
#endif
        }
        else if (type == ADDON_AUDIOENCODER)
          return AddonPtr(new CAudioEncoder(props));
        else if (type == ADDON_AUDIODECODER)
          return AddonPtr(new CAudioDecoder(props));
        else
          return AddonPtr(new CScreenSaver(props));
      }
    case ADDON_SKIN:
      return AddonPtr(new CSkinInfo(props));
    case ADDON_RESOURCE_LANGUAGE:
      return AddonPtr(new CLanguageResource(props));
    case ADDON_RESOURCE_UISOUNDS:
      return AddonPtr(new CUISoundsResource(props));
    case ADDON_VIZ_LIBRARY:
      return AddonPtr(new CAddonLibrary(props));
    case ADDON_REPOSITORY:
      return AddonPtr(new CRepository(props));
    case ADDON_CONTEXT_ITEM:
      return AddonPtr(new CContextItemAddon(props));
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
    std::string path = GetExtValue(*itr++, "@path");
    if (!CDirectory::Exists(path))
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
  : m_cp_context(nullptr),
  m_cpluff(nullptr)
{ }

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
  status = m_cpluff->register_pcollection(m_cp_context, CSpecialProtocol::TranslatePath("special://home/addons").c_str());
  if (status != CP_OK)
  {
    CLog::Log(LOGERROR, "ADDONS: Fatal Error, cp_register_pcollection() returned status: %i", status);
    return false;
  }

  status = m_cpluff->register_pcollection(m_cp_context, CSpecialProtocol::TranslatePath("special://xbmc/addons").c_str());
  if (status != CP_OK)
  {
    CLog::Log(LOGERROR, "ADDONS: Fatal Error, cp_register_pcollection() returned status: %i", status);
    return false;
  }

  status = m_cpluff->register_pcollection(m_cp_context, CSpecialProtocol::TranslatePath("special://xbmcbin/addons").c_str());
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

  // disable some system addons by default because they are optional
  VECADDONS addons;
  GetAddons(ADDON_PVRDLL, addons);
  GetAddons(ADDON_AUDIODECODER, addons);
  std::string systemAddonsPath = CSpecialProtocol::TranslatePath("special://xbmc/addons");
  for (auto &addon : addons)
  {
    if (StringUtils::StartsWith(addon->Path(), systemAddonsPath))
    {
      if (!m_database.IsSystemAddonRegistered(addon->ID()))
      {
        m_database.DisableAddon(addon->ID());
        m_database.AddSystemAddon(addon->ID());
      }
    }
  }

  std::vector<std::string> disabled;
  m_database.GetDisabled(disabled);
  m_disabled.insert(disabled.begin(), disabled.end());

  VECADDONS repos;
  if (GetAddons(ADDON_REPOSITORY, repos))
  {
    VECADDONS::iterator it = repos.begin();
    for (;it != repos.end(); ++it)
      CLog::Log(LOGNOTICE, "ADDONS: Using repository %s", (*it)->ID().c_str());
  }

  return true;
}

void CAddonMgr::DeInit()
{
  if (m_cpluff)
    m_cpluff->destroy();
  delete m_cpluff;
  m_cpluff = NULL;
  m_database.Close();
  m_disabled.clear();
}

bool CAddonMgr::HasAddons(const TYPE &type, bool enabled /*= true*/)
{
  // TODO: This isn't particularly efficient as we create an addon type for each addon using the Factory, just so
  //       we can check addon dependencies in the addon constructor.
  VECADDONS addons;
  return GetAddons(type, addons, enabled);
}

bool CAddonMgr::GetAllAddons(VECADDONS &addons, bool enabled /*= true*/)
{
  CSingleLock lock(m_critSection);
  if (!m_cp_context)
    return false;

  cp_status_t status;
  int num;
  cp_plugin_info_t **cpaddons = m_cpluff->get_plugins_info(m_cp_context, &status, &num);

  for (int i = 0; i < num; ++i)
  {
    const cp_plugin_info_t *cpaddon = cpaddons[i];
    if (cpaddon->extensions && IsAddonDisabled(cpaddon->identifier) != enabled)
    {
      //Get the first extension only
      AddonPtr addon = Factory(&cpaddon->extensions[0]);
      if (addon)
      {
        if (enabled)
        {
          // if the addon has a running instance, grab that
          AddonPtr runningAddon = addon->GetRunningInstance();
          if (runningAddon)
            addon = runningAddon;
        }
        addons.push_back(addon);
      }
    }
  }
  m_cpluff->release_info(m_cp_context, cpaddons);
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
    AddonIdFinder(const std::string& id)
      : m_id(id)
    {}
    
    bool operator()(const AddonPtr& addon) 
    { 
      return m_id == addon->ID();
    }
    private:
    std::string m_id;
};

bool CAddonMgr::ReloadSettings(const std::string &id)
{
  CSingleLock lock(m_critSection);
  VECADDONS::iterator it = std::find_if(m_updateableAddons.begin(), m_updateableAddons.end(), AddonIdFinder(id));
  
  if( it != m_updateableAddons.end())
  {
    return (*it)->ReloadSettings();
  }
  return false;
}

bool CAddonMgr::GetAllOutdatedAddons(VECADDONS &addons, bool getLocalVersion /*= false*/)
{
  CSingleLock lock(m_critSection);
  for (int i = ADDON_UNKNOWN+1; i < ADDON_MAX; ++i)
  {
    VECADDONS temp;
    if (CAddonMgr::Get().GetAddons((TYPE)i, temp, true))
    {
      AddonPtr repoAddon;
      for (unsigned int j = 0; j < temp.size(); j++)
      {
        // Ignore duplicates due to add-ons with multiple extension points
        bool found = false;
        for (VECADDONS::const_iterator addonIt = addons.begin(); addonIt != addons.end(); ++addonIt)
        {
          if ((*addonIt)->ID() == temp[j]->ID())
            found = true;
        }

        if (found || !m_database.GetAddon(temp[j]->ID(), repoAddon))
          continue;

        if (temp[j]->Version() < repoAddon->Version() &&
            !m_database.IsAddonBlacklisted(temp[j]->ID(),
                                           repoAddon->Version().asString().c_str()))
        {
          if (getLocalVersion)
            repoAddon->Props().version = temp[j]->Version();
          addons.push_back(repoAddon);
        }
      }
    }
  }
  return !addons.empty();
}

bool CAddonMgr::HasOutdatedAddons()
{
  VECADDONS dummy;
  return GetAllOutdatedAddons(dummy);
}

bool CAddonMgr::GetAddons(const TYPE &type, VECADDONS &addons, bool enabled /* = true */)
{
  CSingleLock lock(m_critSection);
  if (!m_cp_context)
    return false;
  cp_status_t status;
  int num;
  std::string ext_point(TranslateType(type));
  cp_extension_t **exts = m_cpluff->get_extensions_info(m_cp_context, ext_point.c_str(), &status, &num);
  for(int i=0; i <num; i++)
  {
    const cp_extension_t *props = exts[i];
    if (IsAddonDisabled(props->plugin->identifier) != enabled)
    {
      AddonPtr addon(Factory(props));
      if (addon)
      {
        if (enabled)
        {
          // if the addon has a running instance, grab that
          AddonPtr runningAddon = addon->GetRunningInstance();
          if (runningAddon)
            addon = runningAddon;
        }
        addons.push_back(addon);
      }
    }
  }
  m_cpluff->release_info(m_cp_context, exts);
  return addons.size() > 0;
}

bool CAddonMgr::GetAddon(const std::string &str, AddonPtr &addon, const TYPE &type/*=ADDON_UNKNOWN*/, bool enabledOnly /*= true*/)
{
  CSingleLock lock(m_critSection);

  cp_status_t status;
  cp_plugin_info_t *cpaddon = m_cpluff->get_plugin_info(m_cp_context, str.c_str(), &status);
  if (status == CP_OK && cpaddon)
  {
    addon = GetAddonFromDescriptor(cpaddon, type==ADDON_UNKNOWN?"":TranslateType(type));
    m_cpluff->release_info(m_cp_context, cpaddon);

    if (addon)
    {
      if (enabledOnly && IsAddonDisabled(addon->ID()))
        return false;

      // if the addon has a running instance, grab that
      AddonPtr runningAddon = addon->GetRunningInstance();
      if (runningAddon)
        addon = runningAddon;
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
  std::string setting;
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
  case ADDON_RESOURCE_LANGUAGE:
    setting = CSettings::Get().GetString("locale.language");
    break;
  default:
    return false;
  }
  return GetAddon(setting, addon, type);
}

bool CAddonMgr::SetDefault(const TYPE &type, const std::string &addonID)
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
  case ADDON_RESOURCE_LANGUAGE:
    CSettings::Get().SetString("locale.language", addonID);
    break;
  default:
    return false;
  }

  return true;
}

std::string CAddonMgr::GetString(const std::string &id, const int number)
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

void CAddonMgr::UnregisterAddon(const std::string& ID)
{
  CSingleLock lock(m_critSection);
  m_disabled.erase(ID);
  if (m_cpluff && m_cp_context)
  {
    m_cpluff->uninstall_plugin(m_cp_context, ID.c_str());
    SetChanged();
    lock.Leave();
    NotifyObservers(ObservableMessageAddons);
  }
}

bool CAddonMgr::DisableAddon(const std::string& id)
{
  CSingleLock lock(m_critSection);
  if (m_disabled.find(id) != m_disabled.end())
    return true; //already disabled

  if (!CanAddonBeDisabled(id))
    return false;
  if (!m_database.DisableAddon(id))
    return false;
  if (!m_disabled.insert(id).second)
    return false;

  //success
  ADDON::OnDisabled(id);
  return true;
}

bool CAddonMgr::EnableAddon(const std::string& id)
{
  CSingleLock lock(m_critSection);
  if (m_disabled.find(id) == m_disabled.end())
    return true; //already enabled

  if (!m_database.DisableAddon(id, false))
    return false;
  if (m_disabled.erase(id) == 0)
    return false;

  //success
  ADDON::OnEnabled(id);
  return true;
}

bool CAddonMgr::IsAddonDisabled(const std::string& ID)
{
  CSingleLock lock(m_critSection);
  return m_disabled.find(ID) != m_disabled.end();
}

bool CAddonMgr::CanAddonBeDisabled(const std::string& ID)
{
  if (ID.empty())
    return false;

  CSingleLock lock(m_critSection);
  AddonPtr localAddon;
  // can't disable an addon that isn't installed
  if (!GetAddon(ID, localAddon, ADDON_UNKNOWN, false))
    return false;

  // can't disable an addon that is in use
  if (localAddon->IsInUse())
    return false;

  // installed PVR addons can always be disabled
  if (localAddon->Type() == ADDON_PVRDLL)
    return true;

  // installed audio decoder addons can always be disabled
  if (localAddon->Type() == ADDON_AUDIODECODER)
    return true;

  // installed audio encoder addons can always be disabled
  if (localAddon->Type() == ADDON_AUDIOENCODER)
    return true;

  std::string systemAddonsPath = CSpecialProtocol::TranslatePath("special://xbmc/addons");
  // can't disable system addons
  if (StringUtils::StartsWith(localAddon->Path(), systemAddonsPath))
    return false;

  return true;
}

bool CAddonMgr::IsAddonInstalled(const std::string& ID)
{
  AddonPtr tmp;
  return GetAddon(ID, tmp, ADDON_UNKNOWN, false);
}

bool CAddonMgr::CanAddonBeInstalled(const std::string& ID)
{
  if (ID.empty())
    return false;

  CSingleLock lock(m_critSection);
  // can't install already installed addon
  if (IsAddonInstalled(ID))
    return false;

  // can't install broken addons
  if (!m_database.IsAddonBroken(ID).empty())
    return false;

  return true;
}

bool CAddonMgr::CanAddonBeInstalled(const AddonPtr& addon)
{
  if (addon == NULL)
    return false;

  CSingleLock lock(m_critSection);
  // can't install already installed addon
  if (IsAddonInstalled(addon->ID()))
    return false;

  // can't install broken addons
  if (!addon->Props().broken.empty())
    return false;

  return true;
}

std::string CAddonMgr::GetTranslatedString(const cp_cfg_element_t *root, const char *tag)
{
  if (!root)
    return "";

  std::map<std::string, std::string> translatedValues;
  for (unsigned int i = 0; i < root->num_children; i++)
  {
    const cp_cfg_element_t &child = root->children[i];
    if (strcmp(tag, child.name) == 0)
    {
      // see if we have a "lang" attribute
      const char *lang = m_cpluff->lookup_cfg_value((cp_cfg_element_t*)&child, "@lang");
      if (lang != NULL && g_langInfo.GetLocale().Matches(lang))
        translatedValues.insert(std::make_pair(lang, child.value != NULL ? child.value : ""));
      else if (lang == NULL || strcmp(lang, "en") == 0 || strcmp(lang, "en_GB") == 0)
        translatedValues.insert(std::make_pair("en_GB", child.value != NULL ? child.value : ""));
    }
  }

  // put together a list of languages
  std::set<std::string> languages;
  for (auto const& translatedValue : translatedValues)
    languages.insert(translatedValue.first);

  // find the language from the list that matches the current locale best
  std::string matchingLanguage = g_langInfo.GetLocale().FindBestMatch(languages);
  if (matchingLanguage.empty())
    matchingLanguage = "en_GB";

  auto const& translatedValue = translatedValues.find(matchingLanguage);
  if (translatedValue != translatedValues.end())
    return translatedValue->second;

  return "";
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
    case ADDON_SCRIPT_MODULE:
    case ADDON_SUBTITLE_MODULE:
      return AddonPtr(new CAddon(addonProps));
    case ADDON_WEB_INTERFACE:
      return AddonPtr(new CWebinterface(addonProps));
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
      return AddonPtr(new PVR::CPVRClient(addonProps));
    case ADDON_AUDIOENCODER:
      return AddonPtr(new CAudioEncoder(addonProps));
    case ADDON_AUDIODECODER:
      return AddonPtr(new CAudioDecoder(addonProps));
    case ADDON_RESOURCE_LANGUAGE:
      return AddonPtr(new CLanguageResource(addonProps));
    case ADDON_RESOURCE_UISOUNDS:
      return AddonPtr(new CUISoundsResource(addonProps));
    case ADDON_REPOSITORY:
      return AddonPtr(new CRepository(addonProps));
    case ADDON_CONTEXT_ITEM:
      return AddonPtr(new CContextItemAddon(addonProps));
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
  const cp_extension_t *metadata = GetExtension(plugin, "xbmc.addon.metadata"); //<! backword compatibilty
  if (!metadata)
    metadata = CAddonMgr::Get().GetExtension(plugin, "kodi.addon.metadata");
  if (!metadata)
    return false;

  vector<std::string> platforms;
  if (CAddonMgr::Get().GetExtList(metadata->configuration, "platform", platforms))
  {
    for (vector<std::string>::const_iterator platform = platforms.begin(); platform != platforms.end(); ++platform)
    {
      if (*platform == "all")
        return true;
#if defined(TARGET_ANDROID)
      if (*platform == "android")
#elif defined(TARGET_LINUX) || defined(TARGET_FREEBSD)
      if (*platform == "linux")
#elif defined(TARGET_WINDOWS) && defined(HAS_DX)
      if (*platform == "windx")
#elif defined(TARGET_DARWIN_OSX)
// Remove this after Frodo and add an architecture filter
// in addition to platform.
#if defined(__x86_64__)
      if (*platform == "osx64" || *platform == "osx")
#else
      if (*platform == "osx32" || *platform == "osx")
#endif
#elif defined(TARGET_DARWIN_IOS)
      if (*platform == "ios")
#endif
        return true;
    }
    return false; // no <platform> works for us
  }
  return true; // assume no <platform> is equivalent to <platform>all</platform>
}

cp_cfg_element_t *CAddonMgr::GetExtElement(cp_cfg_element_t *base, const char *path)
{
  cp_cfg_element_t *element = NULL;
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
    std::string temp = base->children[i].name;
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

std::string CAddonMgr::GetExtValue(cp_cfg_element_t *base, const char *path)
{
  const char *value = "";
  if (base && (value = m_cpluff->lookup_cfg_value(base, path)))
    return value;
  else
    return "";
}

bool CAddonMgr::GetExtList(cp_cfg_element_t *base, const char *path, vector<std::string> &result) const
{
  result.clear();
  if (!base || !path)
    return false;
  const char *all = m_cpluff->lookup_cfg_value(base, path);
  if (!all || *all == 0)
    return false;
  StringUtils::Tokenize(all, result, ' ');
  return true;
}

AddonPtr CAddonMgr::GetAddonFromDescriptor(const cp_plugin_info_t *info,
                                           const std::string& type)
{
  if (!info)
    return AddonPtr();

  if (!info->extensions && type.empty())
  { // no extensions, so we need only the dep information
    return AddonPtr(new CAddon(info));
  }

  // grab a relevant extension point, ignoring our kodi.addon.metadata extension point
  for (unsigned int i = 0; i < info->num_extensions; ++i)
  {
    if (0 != strcmp("xbmc.addon.metadata" , info->extensions[i].ext_point_id) && //<! backword compatibilty
        0 != strcmp("kodi.addon.metadata" , info->extensions[i].ext_point_id) &&
        (type.empty() || 0 == strcmp(type.c_str(), info->extensions[i].ext_point_id)))
    { // note that Factory takes care of whether or not we have platform support
      return Factory(&info->extensions[i]);
    }
  }
  return AddonPtr();
}

// FIXME: This function may not be required
bool CAddonMgr::LoadAddonDescription(const std::string &path, AddonPtr &addon)
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
    std::shared_ptr<CService> service = std::dynamic_pointer_cast<CService>(*it);
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
    std::shared_ptr<CService> service = std::dynamic_pointer_cast<CService>(*it);
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

