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
#include "settings/AddonSettings.h"
#include "Application.h"
#include "Util.h"
#include "FileSystem/File.h"
#include "FileSystem/SpecialProtocol.h"
#include "FileSystem/Directory.h"
#include "utils/log.h"
#include "utils/RegExp.h"
#include "XMLUtils.h"
#include "GUIDialogYesNo.h"
#include "GUIDialogOK.h"
#include "GUIDialogAddonSettings.h"
#include "GUIWindowManager.h"

#ifdef HAS_VISUALISATION
#include "../visualizations/DllVisualisation.h"
#include "../visualizations/Visualisation.h"
#endif
#ifdef HAS_PVRCLIENTS
#include "../pvrclients/DllPVRClient.h"
#include "../pvrclients/PVRClient.h"
#endif
#ifdef HAS_SCREENSAVERS
#include "../screensavers/DllScreenSaver.h"
#include "../screensavers/ScreenSaver.h"
#endif

using namespace XFILE;
using namespace DIRECTORY;

template<class T, typename U, class V>
static ADDON::CAddon* loadDll(const ADDON::CAddon& addon)
{
  // add the library name readed from info.xml to the addon's path
  CStdString strFileName;
  if (addon.m_guid_parent.IsEmpty())
  {
    strFileName = addon.m_strPath + addon.m_strLibName;
  }
  else
  {
    CStdString extension = CUtil::GetExtension(addon.m_strLibName);
    strFileName = "special://temp/" + addon.m_strLibName;
    CUtil::RemoveExtension(strFileName);
    strFileName += "-" + addon.m_guid + extension;

    if (!CFile::Exists(strFileName))
      CFile::Cache(addon.m_strPath + addon.m_strLibName, strFileName);

    CLog::Log(LOGNOTICE, "Loaded virtual child addon %s", strFileName.c_str());
  }

  // load dll
  T* pDll = new T;
  pDll->SetFile(strFileName);
  pDll->EnableDelayedUnload(false);
  if (!pDll->Load())
  {
    delete pDll;
    return NULL;
  }

  U* pStruct = (U*)malloc(sizeof(U));
  ZeroMemory(pStruct, sizeof(U));
  pDll->GetAddon(pStruct);

  // and pass it to a new instance of typename 'V' which will handle the dll
  return new V(pStruct, pDll, addon);
}

namespace ADDON
{

/**********************************************************
 * Callback for unknown Add-on types as fallback
 */
CAddonDummyCallback *AddonDummyCallback = new CAddonDummyCallback();


/**********************************************************
 * CAddonStatusHandler - AddOn Status Report Class
 *
 * Used to informate the user about occurred errors and
 * changes inside Add-on's, and ask him what to do.
 *
 */

CCriticalSection CAddonStatusHandler::m_critSection;

CAddonStatusHandler::CAddonStatusHandler(const CAddon* addon, ADDON_STATUS status, CStdString message, bool sameThread)
{
  if (addon == NULL)
    return;

  CLog::Log(LOGINFO, "Called Add-on status handler for '%u' of clientName:%s, clientGUID:%s (same Thread=%s)", status, addon->m_strName.c_str(), addon->m_guid.c_str(), sameThread ? "yes" : "no");

  m_addon   = addon;
  m_status  = status;
  m_message = message;

  if (sameThread)
  {
    Process();
    delete this;
  }
  else
  {
    CStdString ThreadName;
    ThreadName.Format("Addon Status: %s", m_addon->m_strName.c_str());

    Create(false, THREAD_MINSTACKSIZE);
    SetName(ThreadName.c_str());
    SetPriority(-15);
  }
  return;
}

CAddonStatusHandler::~CAddonStatusHandler()
{
  StopThread();
}

void CAddonStatusHandler::OnStartup()
{
}

void CAddonStatusHandler::OnExit()
{
  delete this;
}

void CAddonStatusHandler::Process()
{
  CSingleLock lock(m_critSection);

  /* AddOn lost connection to his backend (for ones that use Network) */
  if (m_status == STATUS_LOST_CONNECTION)
  {
    CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)m_gWindowManager.GetWindow(WINDOW_DIALOG_YES_NO);
    if (!pDialog) return;

    CStdString heading;
    heading.Format("%s: %s", g_localizeStrings.Get(23012 + m_addon->m_addonType).c_str(), m_addon->m_strName.c_str());

    pDialog->SetHeading(heading);
    pDialog->SetLine(1, 23047);
    pDialog->SetLine(2, 23048);

    //send message and wait for user input
    ThreadMessage tMsg = {TMSG_DIALOG_DOMODAL, WINDOW_DIALOG_YES_NO, m_gWindowManager.GetActiveWindow()};
    g_application.getApplicationMessenger().SendMessage(tMsg, true);

    if (pDialog->IsConfirmed())
    {
      CAddonManager::Get()->GetCallbackForType(m_addon->m_addonType)->RequestRestart(m_addon, false);
    }
  }
  /* Request to restart the AddOn and data structures need updated */
  else if (m_status == STATUS_NEED_RESTART)
  {
    CGUIDialogOK* pDialog = (CGUIDialogOK*)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
    if (!pDialog) return;

    CStdString heading;
    heading.Format("%s: %s", g_localizeStrings.Get(23012 + m_addon->m_addonType).c_str(), m_addon->m_strName.c_str());

    pDialog->SetHeading(heading);
    pDialog->SetLine(1, 23049);

    //send message and wait for user input
    ThreadMessage tMsg = {TMSG_DIALOG_DOMODAL, WINDOW_DIALOG_OK, m_gWindowManager.GetActiveWindow()};
    g_application.getApplicationMessenger().SendMessage(tMsg, true);

    CAddonManager::Get()->GetCallbackForType(m_addon->m_addonType)->RequestRestart(m_addon, true);
  }
  /* Request to restart XBMC (hope no AddOn need or do this) */
  else if (m_status == STATUS_NEED_EMER_RESTART)
  {
    /* okey we really don't need to restart, only deinit Add-on, but that could be damn hard if something is playing*/
    //TODO - General way of handling setting changes that require restart

    CGUIDialogYesNo *pDialog = (CGUIDialogYesNo *)m_gWindowManager.GetWindow(WINDOW_DIALOG_YES_NO);
    if (!pDialog) return ;

    CStdString heading;
    heading.Format("%s: %s", g_localizeStrings.Get(23012 + m_addon->m_addonType).c_str(), m_addon->m_strName.c_str());

    pDialog->SetHeading(heading);
    pDialog->SetLine( 0, 23050);
    pDialog->SetLine( 1, 23051);
    pDialog->SetLine( 2, 23052);

    //send message and wait for user input
    ThreadMessage tMsg = {TMSG_DIALOG_DOMODAL, WINDOW_DIALOG_YES_NO, m_gWindowManager.GetActiveWindow()};
    g_application.getApplicationMessenger().SendMessage(tMsg, true);

    if (pDialog->IsConfirmed())
    {
      g_application.getApplicationMessenger().RestartApp();
    }
  }
  /* Some required settings are missing/invalid */
  else if (m_status == STATUS_NEED_SETTINGS)
  {
    CGUIDialogYesNo* pDialogYesNo = (CGUIDialogYesNo*)m_gWindowManager.GetWindow(WINDOW_DIALOG_YES_NO);
    if (!pDialogYesNo) return;

    CStdString heading;
    heading.Format("%s: %s", g_localizeStrings.Get(23012 + m_addon->m_addonType).c_str(), m_addon->m_strName.c_str());

    pDialogYesNo->SetHeading(heading);
    pDialogYesNo->SetLine(1, 23053);
    pDialogYesNo->SetLine(2, 23043);
    pDialogYesNo->SetLine(3, m_message);

    //send message and wait for user input
    ThreadMessage tMsg = {TMSG_DIALOG_DOMODAL, WINDOW_DIALOG_YES_NO, m_gWindowManager.GetActiveWindow()};
    g_application.getApplicationMessenger().SendMessage(tMsg, true);

    if (!pDialogYesNo->IsConfirmed()) return;

    if (!CAddonSettings::SettingsExist(*m_addon))
    {
      CLog::Log(LOGERROR, "No settings.xml file could be found to AddOn '%s' Settings!", m_addon->m_strName.c_str());
      return;
    }

    CURL cUrl(m_addon->m_strPath);

    // Load language strings temporarily
    CAddon::LoadAddonStrings(cUrl);

    // Create the dialog
    CGUIDialogAddonSettings* pDialog = (CGUIDialogAddonSettings*) m_gWindowManager.GetWindow(WINDOW_DIALOG_ADDON_SETTINGS);

    heading.Format("$LOCALIZE[23053]: %s %s", g_localizeStrings.Get(23012 + m_addon->m_addonType).c_str(), m_addon->m_strName.c_str());
    pDialog->SetHeading(heading);

    CAddonSettings settings;
    settings.Load(cUrl);
    pDialog->SetSettings(settings);

    pDialog->DoModal();

    settings = pDialog->GetSettings();
    settings.Save();

    // Unload temporary language strings
    CAddon::ClearAddonStrings();

    if (pDialog->IsConfirmed())
    {
      CAddonManager::Get()->GetCallbackForType(m_addon->m_addonType)->RequestRestart(m_addon, true);
    }
  }
  /* One or more AddOn file(s) missing (check log's for missing data) */
  else if (m_status == STATUS_MISSING_FILE)
  {
    CGUIDialogOK* pDialog = (CGUIDialogOK*)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
    if (!pDialog) return;

    CStdString heading;
    heading.Format("%s: %s", g_localizeStrings.Get(23012 + m_addon->m_addonType).c_str(), m_addon->m_strName.c_str());

    pDialog->SetHeading(heading);
    pDialog->SetLine(1, 23055);
    pDialog->SetLine(2, 23056);
    pDialog->SetLine(3, m_message);

    //send message and wait for user input
    ThreadMessage tMsg = {TMSG_DIALOG_DOMODAL, WINDOW_DIALOG_OK, m_gWindowManager.GetActiveWindow()};
    g_application.getApplicationMessenger().SendMessage(tMsg, true);
  }
  /* A unknown event is occurred */
  else if (m_status == STATUS_UNKNOWN)
  {
    CGUIDialogOK* pDialog = (CGUIDialogOK*)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
    if (!pDialog) return;

    CStdString heading;
    heading.Format("%s: %s", g_localizeStrings.Get(23012 + m_addon->m_addonType).c_str(), m_addon->m_strName.c_str());

    pDialog->SetHeading(heading);
    pDialog->SetLine(1, 23057);
    pDialog->SetLine(2, 23056);
    pDialog->SetLine(3, m_message);

    //send message and wait for user input
    ThreadMessage tMsg = {TMSG_DIALOG_DOMODAL, WINDOW_DIALOG_OK, m_gWindowManager.GetActiveWindow()};
    g_application.getApplicationMessenger().SendMessage(tMsg, true);
  }

  return;
}


/**********************************************************
 * CAddonManager
 *
 */

IAddonCallback *CAddonManager::m_cbMultitye        = NULL;
IAddonCallback *CAddonManager::m_cbViz             = NULL;
IAddonCallback *CAddonManager::m_cbSkin            = NULL;
IAddonCallback *CAddonManager::m_cbPVR             = NULL;
IAddonCallback *CAddonManager::m_cbScript          = NULL;
IAddonCallback *CAddonManager::m_cbScraperPVR      = NULL;
IAddonCallback *CAddonManager::m_cbScraperVideo    = NULL;
IAddonCallback *CAddonManager::m_cbScraperMusic    = NULL;
IAddonCallback *CAddonManager::m_cbScraperProgram  = NULL;
IAddonCallback *CAddonManager::m_cbScreensaver     = NULL;
IAddonCallback *CAddonManager::m_cbPluginPVR       = NULL;
IAddonCallback *CAddonManager::m_cbPluginVideo     = NULL;
IAddonCallback *CAddonManager::m_cbPluginMusic     = NULL;
IAddonCallback *CAddonManager::m_cbPluginProgram   = NULL;
IAddonCallback *CAddonManager::m_cbPluginPictures  = NULL;
IAddonCallback *CAddonManager::m_cbPluginWeather   = NULL;
IAddonCallback *CAddonManager::m_cbDSPAudio        = NULL;

CAddonManager* CAddonManager::m_pInstance = NULL;

CAddonManager::CAddonManager()
{
  m_pInstance = NULL;
}

CAddonManager::~CAddonManager()
{
}

CAddonManager* CAddonManager::Get()
{
  if (!m_pInstance)
  {
    m_pInstance = new CAddonManager();
  }
  return m_pInstance;
}

IAddonCallback* CAddonManager::GetCallbackForType(AddonType type)
{
  IAddonCallback *cb_tmp;

  switch (type)
  {
    case ADDON_MULTITYPE:
      cb_tmp = m_cbMultitye;
      break;
    case ADDON_VIZ:
      cb_tmp = m_cbViz;
      break;
    case ADDON_SKIN:
      cb_tmp = m_cbSkin;
      break;
    case ADDON_PVRDLL:
      cb_tmp = m_cbPVR;
      break;
    case ADDON_SCRIPT:
      cb_tmp = m_cbScript;
      break;
    case ADDON_SCRAPER_PVR:
      cb_tmp = m_cbScraperPVR;
      break;
    case ADDON_SCRAPER_VIDEO:
      cb_tmp = m_cbScraperVideo;
      break;
    case ADDON_SCRAPER_MUSIC:
      cb_tmp = m_cbScraperMusic;
      break;
    case ADDON_SCRAPER_PROGRAM:
      cb_tmp = m_cbScraperProgram;
      break;
    case ADDON_SCREENSAVER:
      cb_tmp = m_cbScreensaver;
      break;
    case ADDON_PLUGIN_PVR:
      cb_tmp = m_cbPluginPVR;
      break;
    case ADDON_PLUGIN_MUSIC:
      cb_tmp = m_cbPluginMusic;
      break;
    case ADDON_PLUGIN_VIDEO:
      cb_tmp = m_cbPluginVideo;
      break;
    case ADDON_PLUGIN_PROGRAM:
      cb_tmp = m_cbPluginProgram;
      break;
    case ADDON_PLUGIN_PICTURES:
      cb_tmp = m_cbPluginPictures;
      break;
    case ADDON_PLUGIN_WEATHER:
      cb_tmp = m_cbPluginWeather;
      break;
    case ADDON_DSP_AUDIO:
      cb_tmp = m_cbDSPAudio;
      break;
    case ADDON_UNKNOWN:
    default:
      cb_tmp = NULL;
      break;
  }

  if (cb_tmp == NULL)
    cb_tmp = AddonDummyCallback;

  return cb_tmp;
}

bool CAddonManager::RegisterAddonCallback(AddonType type, IAddonCallback* cb)
{
  if (cb == NULL)
    return false;

  if (type == ADDON_MULTITYPE && m_cbMultitye == NULL)
    m_cbMultitye = cb;
  else if (type == ADDON_VIZ && m_cbViz == NULL)
    m_cbViz = cb;
  else if (type == ADDON_SKIN && m_cbSkin == NULL)
    m_cbSkin = cb;
  else if (type == ADDON_PVRDLL && m_cbPVR == NULL)
    m_cbPVR = cb;
  else if (type == ADDON_SCRIPT && m_cbScript == NULL)
    m_cbScript = cb;
  else if (type == ADDON_SCRAPER_PVR && m_cbScraperPVR == NULL)
    m_cbScraperPVR = cb;
  else if (type == ADDON_SCRAPER_VIDEO && m_cbScraperVideo == NULL)
    m_cbScraperVideo = cb;
  else if (type == ADDON_SCRAPER_MUSIC && m_cbScraperMusic == NULL)
    m_cbScraperMusic = cb;
  else if (type == ADDON_SCRAPER_PROGRAM && m_cbScraperProgram == NULL)
    m_cbScraperProgram = cb;
  else if (type == ADDON_SCREENSAVER && m_cbScreensaver == NULL)
    m_cbScreensaver = cb;
  else if (type == ADDON_PLUGIN_PVR && m_cbPluginPVR == NULL)
    m_cbPluginPVR = cb;
  else if (type == ADDON_PLUGIN_MUSIC && m_cbPluginMusic == NULL)
    m_cbPluginMusic = cb;
  else if (type == ADDON_PLUGIN_VIDEO && m_cbPluginVideo == NULL)
    m_cbPluginVideo = cb;
  else if (type == ADDON_PLUGIN_PROGRAM && m_cbPluginProgram == NULL)
    m_cbPluginProgram = cb;
  else if (type == ADDON_PLUGIN_PICTURES && m_cbPluginPictures == NULL)
    m_cbPluginPictures = cb;
  else if (type == ADDON_PLUGIN_WEATHER && m_cbPluginWeather == NULL)
    m_cbPluginWeather = cb;
  else if (type == ADDON_DSP_AUDIO && m_cbDSPAudio == NULL)
    m_cbDSPAudio = cb;
  else
    return false;

  return true;
}

void CAddonManager::UnregisterAddonCallback(AddonType type)
{
  switch (type)
  {
    case ADDON_MULTITYPE:
      m_cbMultitye = NULL;
      return;
    case ADDON_VIZ:
      m_cbViz = NULL;
      return;
    case ADDON_SKIN:
      m_cbSkin = NULL;
      return;
    case ADDON_PVRDLL:
      m_cbPVR = NULL;
      return;
    case ADDON_SCRIPT:
      m_cbScript = NULL;
      return;
    case ADDON_SCRAPER_PVR:
      m_cbScraperPVR = NULL;
      return;
    case ADDON_SCRAPER_VIDEO:
      m_cbScraperVideo = NULL;
      return;
    case ADDON_SCRAPER_MUSIC:
      m_cbScraperMusic = NULL;
      return;
    case ADDON_SCRAPER_PROGRAM:
      m_cbScraperProgram = NULL;
      return;
    case ADDON_SCREENSAVER:
      m_cbScreensaver = NULL;
      return;
    case ADDON_PLUGIN_PVR:
      m_cbPluginPVR = NULL;
      return;
    case ADDON_PLUGIN_MUSIC:
      m_cbPluginMusic = NULL;
      return;
    case ADDON_PLUGIN_VIDEO:
      m_cbPluginVideo = NULL;
      return;
    case ADDON_PLUGIN_PROGRAM:
      m_cbPluginProgram = NULL;
      return;
    case ADDON_PLUGIN_PICTURES:
      m_cbPluginPictures = NULL;
      return;
    case ADDON_PLUGIN_WEATHER:
      m_cbPluginWeather = NULL;
      return;
    case ADDON_DSP_AUDIO:
      m_cbDSPAudio = NULL;
      return;
    case ADDON_UNKNOWN:
    default:
      return;
  }
}
/*****************************************************************************/
VECADDONS *CAddonManager::GetAllAddons()
{
  return &m_allAddons;
}

VECADDONS *CAddonManager::GetAddonsFromType(const AddonType &type)
{
  switch (type)
  {
    case ADDON_MULTITYPE:
      return &m_multitypeAddons;
    case ADDON_VIZ:
      return &m_visualisationAddons;
    case ADDON_SKIN:
      return &m_skinAddons;
    case ADDON_PVRDLL:
      return &m_pvrAddons;
    case ADDON_SCRIPT:
      return &m_scriptAddons;
    case ADDON_SCRAPER_PVR:
      return &m_scraperPVRAddons;
    case ADDON_SCRAPER_VIDEO:
      return &m_scraperVideoAddons;
    case ADDON_SCRAPER_MUSIC:
      return &m_scraperMusicAddons;
    case ADDON_SCRAPER_PROGRAM:
      return &m_scraperProgramAddons;
    case ADDON_SCREENSAVER:
      return &m_screensaverAddons;
    case ADDON_PLUGIN_PVR:
      return &m_pluginPvrAddons;
    case ADDON_PLUGIN_MUSIC:
      return &m_pluginMusicAddons;
    case ADDON_PLUGIN_VIDEO:
      return &m_pluginVideoAddons;
    case ADDON_PLUGIN_PROGRAM:
      return &m_pluginProgramAddons;
    case ADDON_PLUGIN_PICTURES:
      return &m_pluginPictureAddons;
    case ADDON_PLUGIN_WEATHER:
      return &m_pluginWeatherAddons;
    case ADDON_DSP_AUDIO:
      return &m_DSPAudioAddons;
    default:
      return NULL;
  }
}

bool CAddonManager::GetAddonFromGUID(const CStdString &guid, CAddon &addon)
{
  /* iterate through alladdons vec and return matched Addon */
  for (unsigned int i = 0; i < m_allAddons.size(); i++)
  {
    if (m_allAddons[i].m_guid == guid)
    {
      addon = m_allAddons[i];
      return true;
    }
  }
  return false;
}

bool CAddonManager::GetAddonFromNameAndType(const CStdString &name, const AddonType &type, CAddon &addon)
{
  VECADDONS *addons = GetAddonsFromType(type);
  if (!addons) return false;

  for (IVECADDONS it = addons->begin(); it != addons->end(); it++)
  {
    if ((*it).m_strName == name)
    {
      addon = (*it);
      return true;
    }
  }
  return false;
}

bool CAddonManager::DisableAddon(const CStdString &guid, const AddonType &type)
{
  VECADDONS *addons = GetAddonsFromType(type);
  if (!addons) return false;

  for (IVECADDONS it = addons->begin(); it != addons->end(); it++)
  {
    if ((*it).m_guid == guid)
    {
      CLog::Log(LOGDEBUG,"found addon, disabling!");
      addons->erase(it);
      break;
    }
  }

  return SaveAddons(type);
}

bool CAddonManager::SaveAddons(const AddonType &type)
{
  // call settings.saveaddons
  return false;
}

void CAddonManager::UpdateAddons()
{
  //TODO: only caches packaged icon thumbnail
  m_allAddons.clear();

  // Go thru all addon directorys in xbmc and if present in user directory
  CFileItemList items;
  // User add-on's have priority over application add-on's
  if (!CSpecialProtocol::XBMCIsHome())
  {
    CDirectory::GetDirectory("special://home/addons/multitype", items, ADDON_MULTITYPE_EXT, false);
    CDirectory::GetDirectory("special://home/addons/visualisations", items, ADDON_VIZ_EXT, false);
    CDirectory::GetDirectory("special://home/addons/pvr", items, ADDON_PVRDLL_EXT, false);
    CDirectory::GetDirectory("special://home/addons/scripts", items, ADDON_SCRIPT_EXT, false);
    CDirectory::GetDirectory("special://home/addons/scrapers/pvr", items, ADDON_SCRAPER_EXT, false);
    CDirectory::GetDirectory("special://home/addons/scrapers/video", items, ADDON_SCRAPER_EXT, false);
    CDirectory::GetDirectory("special://home/addons/scrapers/music", items, ADDON_SCRAPER_EXT, false);
    CDirectory::GetDirectory("special://home/addons/scrapers/programs", items, ADDON_SCRAPER_EXT, false);
    CDirectory::GetDirectory("special://home/addons/screensavers", items, ADDON_SCREENSAVER_EXT, false);
    CDirectory::GetDirectory("special://home/addons/dsp-audio", items, ADDON_DSP_AUDIO_EXT, false);
  }
  // Now load the add-on's located in the application directory
  CDirectory::GetDirectory("special://xbmc/addons/multitype", items, ADDON_MULTITYPE_EXT, false);
  CDirectory::GetDirectory("special://xbmc/addons/visualisations", items, ADDON_VIZ_EXT, false);
  CDirectory::GetDirectory("special://xbmc/addons/pvr", items, ADDON_PVRDLL_EXT, false);
  CDirectory::GetDirectory("special://xbmc/addons/scripts", items, ADDON_SCRIPT_EXT, false);
  CDirectory::GetDirectory("special://xbmc/addons/scrapers/pvr", items, ADDON_SCRAPER_EXT, false);
  CDirectory::GetDirectory("special://xbmc/addons/scrapers/video", items, ADDON_SCRAPER_EXT, false);
  CDirectory::GetDirectory("special://xbmc/addons/scrapers/music", items, ADDON_SCRAPER_EXT, false);
  CDirectory::GetDirectory("special://xbmc/addons/scrapers/programs", items, ADDON_SCRAPER_EXT, false);
  CDirectory::GetDirectory("special://xbmc/addons/screensavers", items, ADDON_SCREENSAVER_EXT, false);
  CDirectory::GetDirectory("special://xbmc/addons/dsp-audio", items, ADDON_DSP_AUDIO_EXT, false);

  // Plugin Directory currently only located in Home
  CDirectory::GetDirectory("special://home/addons/plugins/pvr", items, ADDON_PLUGIN_PVR_EXT, false);
  CDirectory::GetDirectory("special://home/addons/plugins/music", items, ADDON_PLUGIN_MUSIC_EXT, false);
  CDirectory::GetDirectory("special://home/addons/plugins/video", items, ADDON_PLUGIN_VIDEO_EXT, false);
  CDirectory::GetDirectory("special://home/addons/plugins/programs", items, ADDON_PLUGIN_PROGRAM_EXT, false);
  CDirectory::GetDirectory("special://home/addons/plugins/pictures", items, ADDON_PLUGIN_PICTURES_EXT, false);
  CDirectory::GetDirectory("special://home/addons/plugins/weather", items, ADDON_PLUGIN_WEATHER_EXT, false);

  if (items.Size() == 0)
    return;

  // for each folder found
  for (int i = 0; i < items.Size(); ++i)
  {
    CFileItemPtr item = items[i];

    // read info.xml and generate the addon
    CAddon addon;
    if (!AddonFromInfoXML(item->m_strPath, addon))
    {
      CLog::Log(LOGERROR, "Addon: Error reading %s/info.xml, bypassing package", item->m_strPath.c_str());
      continue;
    }

    // iterate through current alladdons vec and skip if guid is already present
    for (unsigned int i = 0; i < m_allAddons.size(); i++)
    {
      if (m_allAddons[i].m_guid == addon.m_guid)
      {
        CLog::Log(LOGNOTICE, "Addon: GUID=%s, Name=%s already present in %s, ignoring package", addon.m_guid.c_str(), addon.m_strName.c_str(), m_allAddons[i].m_strPath.c_str());
        continue;
      }
    }

    // check for/cache icon thumbnail
    item->SetThumbnailImage("");
    item->SetCachedProgramThumb();
    if (!item->HasThumbnail())
      item->SetUserProgramThumb();
    if (!item->HasThumbnail())
    {
      CFileItem item2(addon.m_strPath);
      CUtil::AddFileToFolder(addon.m_strPath, addon.m_strLibName, item2.m_strPath);
      item2.m_bIsFolder = false;
      item2.SetCachedProgramThumb();
      if (!item2.HasThumbnail())
        item2.SetUserProgramThumb();
      if (!item2.HasThumbnail())
        item2.SetThumbnailImage(addon.m_icon);
      if (item2.HasThumbnail())
      {
        XFILE::CFile::Cache(item2.GetThumbnailImage(),item->GetCachedProgramThumb());
        item->SetThumbnailImage(item->GetCachedProgramThumb());
      }
    }
    addon.m_strPath = item->m_strPath;

    // everything ok, add to available addons
    m_allAddons.push_back(addon);
  }

  /* Copy also virtual child add-on's to the list */
  for (unsigned int i = 0; i < m_virtualAddons.size(); i++)
    m_allAddons.push_back(m_virtualAddons[i]);
}

bool CAddonManager::AddonFromInfoXML(const CStdString &path, CAddon &addon)
{
  // First check that we can load info.xml
  CStdString strPath(path);
  CUtil::AddFileToFolder(strPath, "info.xml", strPath);

  TiXmlDocument xmlDoc;
  if (!xmlDoc.LoadFile(strPath))
  {
    CLog::Log(LOGERROR, "Unable to load: %s, Line %d\n%s", strPath.c_str(), xmlDoc.ErrorRow(), xmlDoc.ErrorDesc());
    return false;
  }

  TiXmlElement *element = xmlDoc.RootElement();
  if (!element || strcmpi(element->Value(), "addoninfo") != 0)
  {
    CLog::Log(LOGERROR, "Addon: Error loading %s: cannot find root element 'addon'", strPath.c_str());
    return false;
  }

  /* Steps required to meet package requirements
  * 1. guid exists and is valid
  * 2. type exists and is valid
  * 3. version exists
  * 4. operating system matches ours
  * 5. summary exists
  */

  /* Read guid */
  CStdString guid;
  element = xmlDoc.RootElement()->FirstChildElement("guid");
  if (!element)
  {
    CLog::Log(LOGERROR, "Addon: %s does not contain the <guid> element, ignoring", strPath.c_str());
    return false;
  }
  guid = element->GetText(); // grab guid

  /* Validate type */
  element = xmlDoc.RootElement()->FirstChildElement("type");
  int type = atoi(element->GetText());
  if (type < ADDON_MULTITYPE || type > ADDON_DSP_AUDIO)
  {
    CLog::Log(LOGERROR, "Addon: %s has invalid type identifier: %d", strPath.c_str(), type);
    return false;
  }
  addon.m_addonType = (AddonType) type; // type was validated //TODO this cast to AddonType

  /* Validate guid*/
  CRegExp guidRE;
  guidRE.RegComp(ADDON_GUID_RE.c_str());
  if (guidRE.RegFind(guid.c_str()) != 0)
  {
    CLog::Log(LOGERROR, "Addon: %s has invalid <guid> element, ignoring", strPath.c_str());
    return false;
  }
  if (addon.m_guid.IsEmpty())
    addon.m_guid = guid; // guid was validated
  else
  {
    addon.m_guid_parent = guid; // guid was validated and is part of a child addon

    VECADDONS *addons = GetAddonsFromType(addon.m_addonType);
    if (!addons) return false;

    for (IVECADDONS it = addons->begin(); it != addons->end(); it++)
    {
      if ((*it).m_guid == addon.m_guid_parent)
      {
        (*it).m_childs++;
        break;
      }
    }
  }

  /* Retrieve Name */
  CStdString name;
  element = NULL;
  element = xmlDoc.RootElement()->FirstChildElement("name");
  if (!element)
  {
    CLog::Log(LOGERROR, "Addon: %s missing <name> element, ignoring", strPath.c_str());
    return false;
  }
  addon.m_strName = element->GetText();

  /* Retrieve version */
  CStdString version;
  element = NULL;
  element = xmlDoc.RootElement()->FirstChildElement("version");
  if (!element)
  {
    CLog::Log(LOGERROR, "Addon: %s missing <version> element, ignoring", strPath.c_str());
    return false;
  }
  /* Validate version */
  version = element->GetText();
  CRegExp versionRE;
  versionRE.RegComp(ADDON_VERSION_RE.c_str());
  if (versionRE.RegFind(version.c_str()) != 0)
  {
    CLog::Log(LOGERROR, "Addon: %s has invalid <version> element, ignoring", strPath.c_str());
    return false;
  }
  addon.m_strVersion = version; // guid was validated

  /* Retrieve platform which is supported */
  CStdString platform;
  element = NULL;
  element = xmlDoc.RootElement()->FirstChildElement("platform");
  if (!element)
  {
    CLog::Log(LOGERROR, "Addon: %s missing <platform> element, ignoring", strPath.c_str());
    return false;
  }

  /* Validate platform */
  platform = element->GetText();
  if (platform.Find("all") < 0)
  {
#if defined(_LINUX)
    if (platform.Find("linux") < 0)
    {
      CLog::Log(LOGERROR, "Addon: %s is not supported under Linux, ignoring", strPath.c_str());
      return false;
    }
#elif defined(_WIN32PC)
    if (platform.Find("windows") < 0)
    {
      CLog::Log(LOGERROR, "Addon: %s is not supported under Windows, ignoring", strPath.c_str());
      return false;
    }
#elif defined(__APPLE__)
    if (platform.Find("osx") < 0)
    {
      CLog::Log(LOGERROR, "Addon: %s is not supported under OSX, ignoring", strPath.c_str());
      return false;
    }
#elif defined(_XBOX)
    if (platform.Find("xbox") < 0)
    {
      CLog::Log(LOGERROR, "Addon: %s is not supported under XBOX, ignoring", strPath.c_str());
      return false;
    }
#endif
  }

  /* Retrieve summary */
  CStdString summary;
  element = NULL;
  element = xmlDoc.RootElement()->FirstChildElement("summary");
  if (!element)
  {
    CLog::Log(LOGERROR, "Addon: %s missing <summary> element, ignoring", strPath.c_str());
    return false;
  }
  addon.m_summary = element->GetText(); // summary was present

  /*** Beginning of optional fields ***/
  /* Retrieve description */
  CStdString description;
  element = NULL;
  element = xmlDoc.RootElement()->FirstChildElement("description");
  if (element)
    addon.m_strDesc = element->GetText();

  /* Retrieve creator */
  CStdString creator;
  element = NULL;
  element = xmlDoc.RootElement()->FirstChildElement("creator");
  if (element)
    addon.m_strCreator = element->GetText();

  /* Retrieve disclaimer */
  CStdString disclaimer;
  element = NULL;
  element = xmlDoc.RootElement()->FirstChildElement("disclaimer");
  if (element)
    addon.m_disclaimer = element->GetText();

  /* Retrieve library file name */
  CStdString library;
  element = NULL;
  element = xmlDoc.RootElement()->FirstChildElement("library");
  if (element)
    addon.m_strLibName = element->GetText();

#ifdef _WIN32PC
  /* Retrieve WIN32 library file name in case it is present
  * This is required for no overwrite to the fixed WIN32 add-on's
  * during compile time
  */
  CStdString library_win32;
  element = NULL;
  element = xmlDoc.RootElement()->FirstChildElement("librarywin32");
  if (element) // If it is found overwrite standart library name
    addon.m_strLibName = element->GetText();
#endif

  /* Retrieve thumbnail file name */
  CStdString icon;
  element = NULL;
  element = xmlDoc.RootElement()->FirstChildElement("icon");
  if (element)
  {
    CStdString iconPath = element->GetText();
    addon.m_icon = path + iconPath;
  }

  /*** end of optional fields ***/

  /* Everything's valid */

  CLog::Log(LOGINFO, "Addon: %s retrieved. Name: %s, GUID: %s, Version: %s",
            strPath.c_str(), addon.m_strName.c_str(), addon.m_guid.c_str(), addon.m_strVersion.c_str());

  return true;
}

bool CAddonManager::SetAddons(TiXmlNode *root, const AddonType &type, const VECADDONS &addons)
{
  CStdString strType;
  switch (type)
  {
    case ADDON_MULTITYPE:
      strType = "multitype";
      break;
    case ADDON_VIZ:
      strType = "visualistation";
      break;
    case ADDON_SKIN:
      strType = "skin";
      break;
    case ADDON_PVRDLL:
      strType = "pvr";
      break;
    case ADDON_SCRIPT:
      strType = "script";
      break;
    case ADDON_SCRAPER_PVR:
      strType = "scraperpvr";
      break;
    case ADDON_SCRAPER_VIDEO:
      strType = "scrapervideo";
      break;
    case ADDON_SCRAPER_MUSIC:
      strType = "scrapermusic";
      break;
    case ADDON_SCRAPER_PROGRAM:
      strType = "scraperprogram";
      break;
    case ADDON_SCREENSAVER:
      strType = "screensaver";
      break;
    case ADDON_PLUGIN_PVR:
      strType = "pluginpvr";
      break;
    case ADDON_PLUGIN_MUSIC:
      strType = "pluginmusic";
      break;
    case ADDON_PLUGIN_VIDEO:
      strType = "pluginvideo";
      break;
    case ADDON_PLUGIN_PROGRAM:
      strType = "pluginprogram";
      break;
    case ADDON_PLUGIN_PICTURES:
      strType = "pluginpictures";
      break;
    case ADDON_PLUGIN_WEATHER:
      strType = "pluginweather";
      break;
    case ADDON_DSP_AUDIO:
      strType = "dspaudio";
      break;
    default:
      return false;
  }

  TiXmlElement sectionElement(strType);
  TiXmlNode *sectionNode = root->InsertEndChild(sectionElement);
  if (sectionNode)
  {
    for (unsigned int i = 0; i < addons.size(); i++)
    {
      const CAddon &addon = addons[i];
      TiXmlElement element("addon");

      XMLUtils::SetString(&element, "name", addon.m_strName);
      XMLUtils::SetPath(&element, "path", addon.m_strPath);
      if (!addon.m_guid_parent.IsEmpty())
        XMLUtils::SetString(&element, "childguid", addon.m_guid);

      if (!addon.m_icon.IsEmpty())
        XMLUtils::SetPath(&element, "thumbnail", addon.m_icon);

      sectionNode->InsertEndChild(element);
    }
  }
  return true;
}

/*****************************************************************************/
ADDON_STATUS CAddonManager::SetSetting(const CAddon* addon, const char *settingName, const void *settingValue)
{
  if (!addon)
    return STATUS_UNKNOWN;

  CLog::Log(LOGINFO, "ADDONS: set setting of clientName: %s, settingName: %s", addon->m_strName.c_str(), settingName);
  /*CLIENTMAPITR itr = m_clients.begin();
  while (itr != m_clients.end())
  {
    if (m_clients[(*itr).first]->m_guid == addon->m_guid)
    {
      if (m_clients[(*itr).first]->m_strName == addon->m_strName)
      {
        return m_clients[(*itr).first]->SetSetting(settingName, settingValue);
      }
    }
    itr++;
  } */
  return STATUS_UNKNOWN;
}

bool CAddonManager::RequestRestart(const CAddon* addon, bool datachanged)
{
  if (!addon)
    return false;

  CLog::Log(LOGINFO, "ADDONS: requested restart of clientName:%s, clientGUID:%s", addon->m_strName.c_str(), addon->m_guid.c_str());
  /*CLIENTMAPITR itr = m_clients.begin();
  while (itr != m_clients.end())
  {
    if (m_clients[(*itr).first]->m_guid == addon->m_guid)
    {
      if (m_clients[(*itr).first]->m_strName == addon->m_strName)
      {
        CLog::Log(LOGINFO, "ADDONS: restarting clientName:%s, clientGUID:%s", addon->m_strName.c_str(), addon->m_guid.c_str());
        m_clients[(*itr).first]->ReInit();
        if (datachanged)
        {

        }
      }
    }
    itr++;
  }*/
  return true;
}

bool CAddonManager::RequestRemoval(const CAddon* addon)
{
  if (!addon)
    return false;

  CLog::Log(LOGINFO, "ADDONS: requested removal of clientName:%s, clientGUID:%s", addon->m_strName.c_str(), addon->m_guid.c_str());
  /*CLIENTMAPITR itr = m_clients.begin();
  while (itr != m_clients.end())
  {
    if (m_clients[(*itr).first]->m_guid == addon->m_guid)
    {
      if (m_clients[(*itr).first]->m_strName == addon->m_strName)
      {
        CLog::Log(LOGINFO, "ADDONS: removing clientName:%s, clientGUID:%s", addon->m_strName.c_str(), addon->m_guid.c_str());
        m_clients[(*itr).first]->Remove();
        m_clients.erase((*itr).first);
        return true;
      }
    }
    itr++;
  }*/

  return false;
}

} /* namespace ADDON */
