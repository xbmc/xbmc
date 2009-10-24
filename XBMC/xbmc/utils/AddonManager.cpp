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

#include "Addon.h"
#include "Application.h"
#include "Util.h"
#include "FileItem.h"
#include "StringUtils.h"
#include "utils/RegExp.h"
#include "FileSystem/File.h"
#include "FileSystem/SpecialProtocol.h"
#include "FileSystem/Directory.h"
#include "GUIWindowManager.h"
#include "GUIDialogAddonSettings.h"
#include "GUIDialogOK.h"
#include "GUIDialogYesNo.h"
#include "Profile.h"
#include "utils/log.h"
#include "LocalizeStrings.h"
#include "GUISettings.h"
#include "AddonManager.h"
#include "utils/SingleLock.h"
#include "XMLUtils.h"

using namespace XFILE;
using namespace DIRECTORY;

namespace ADDON
{

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
    CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)g_windowManager.GetWindow(WINDOW_DIALOG_YES_NO);
    if (!pDialog) return;

    CStdString heading;
    heading.Format("%s: %s", g_localizeStrings.Get(23012 + m_addon->m_addonType).c_str(), m_addon->m_strName.c_str());

    pDialog->SetHeading(heading);
    pDialog->SetLine(1, 23047);
    pDialog->SetLine(2, 23048);

    //send message and wait for user input
    ThreadMessage tMsg = {TMSG_DIALOG_DOMODAL, WINDOW_DIALOG_YES_NO, g_windowManager.GetActiveWindow()};
    g_application.getApplicationMessenger().SendMessage(tMsg, true);

    if (pDialog->IsConfirmed())
    {
      g_addonmanager.GetCallbackForType(m_addon->m_addonType)->RequestRestart(m_addon, false);
    }
  }
  /* Request to restart the AddOn and data structures need updated */
  else if (m_status == STATUS_NEED_RESTART)
  {
    CGUIDialogOK* pDialog = (CGUIDialogOK*)g_windowManager.GetWindow(WINDOW_DIALOG_OK);
    if (!pDialog) return;

    CStdString heading;
    heading.Format("%s: %s", g_localizeStrings.Get(23012 + m_addon->m_addonType).c_str(), m_addon->m_strName.c_str());

    pDialog->SetHeading(heading);
    pDialog->SetLine(1, 23049);

    //send message and wait for user input
    ThreadMessage tMsg = {TMSG_DIALOG_DOMODAL, WINDOW_DIALOG_OK, g_windowManager.GetActiveWindow()};
    g_application.getApplicationMessenger().SendMessage(tMsg, true);

    g_addonmanager.GetCallbackForType(m_addon->m_addonType)->RequestRestart(m_addon, true);
  }
  /* Request to restart XBMC (hope no AddOn need or do this) */
  else if (m_status == STATUS_NEED_EMER_RESTART)
  {
    /* okey we really don't need to restart, only deinit Add-on, but that could be damn hard if something is playing*/
    //TODO - General way of handling setting changes that require restart

    CGUIDialogYesNo *pDialog = (CGUIDialogYesNo *)g_windowManager.GetWindow(WINDOW_DIALOG_YES_NO);
    if (!pDialog) return ;

    CStdString heading;
    heading.Format("%s: %s", g_localizeStrings.Get(23012 + m_addon->m_addonType).c_str(), m_addon->m_strName.c_str());

    pDialog->SetHeading(heading);
    pDialog->SetLine( 0, 23050);
    pDialog->SetLine( 1, 23051);
    pDialog->SetLine( 2, 23052);

    //send message and wait for user input
    ThreadMessage tMsg = {TMSG_DIALOG_DOMODAL, WINDOW_DIALOG_YES_NO, g_windowManager.GetActiveWindow()};
    g_application.getApplicationMessenger().SendMessage(tMsg, true);

    if (pDialog->IsConfirmed())
    {
      g_application.getApplicationMessenger().RestartApp();
    }
  }
  /* A setting value is invalid */
  else if (m_status == STATUS_NEED_SETTINGS)
  {
    CGUIDialogYesNo* pDialogYesNo = (CGUIDialogYesNo*)g_windowManager.GetWindow(WINDOW_DIALOG_YES_NO);
    if (!pDialogYesNo) return;

    CStdString heading;
    heading.Format("%s: %s", g_localizeStrings.Get(23012 + m_addon->m_addonType).c_str(), m_addon->m_strName.c_str());

    pDialogYesNo->SetHeading(heading);
    pDialogYesNo->SetLine(1, 23054);
    pDialogYesNo->SetLine(2, 23043);
    pDialogYesNo->SetLine(3, m_message);

    //send message and wait for user input
    ThreadMessage tMsg = {TMSG_DIALOG_DOMODAL, WINDOW_DIALOG_YES_NO, g_windowManager.GetActiveWindow()};
    g_application.getApplicationMessenger().SendMessage(tMsg, true);

    if (!pDialogYesNo->IsConfirmed()) return;

    if (!CAddonSettings::SettingsExist(m_addon->m_strPath))
    {
      CLog::Log(LOGERROR, "No settings.xml file could be found to AddOn '%s' Settings!", m_addon->m_strName.c_str());
      return;
    }

    CURL cUrl(m_addon->m_strPath);

    // Load language strings temporarily
    CAddon::LoadAddonStrings(cUrl);

    // Create the dialog
    CGUIDialogAddonSettings* pDialog = (CGUIDialogAddonSettings*) g_windowManager.GetWindow(WINDOW_DIALOG_ADDON_SETTINGS);

    heading.Format("$LOCALIZE[23054]: %s %s", g_localizeStrings.Get(23012 + m_addon->m_addonType).c_str(), m_addon->m_strName.c_str());
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
      g_addonmanager.GetCallbackForType(m_addon->m_addonType)->RequestRestart(m_addon, true);
    }
  }
  /* One or more AddOn file(s) missing (check log's for missing data) */
  else if (m_status == STATUS_MISSING_FILE)
  {
    CGUIDialogOK* pDialog = (CGUIDialogOK*)g_windowManager.GetWindow(WINDOW_DIALOG_OK);
    if (!pDialog) return;

    CStdString heading;
    heading.Format("%s: %s", g_localizeStrings.Get(23012 + m_addon->m_addonType).c_str(), m_addon->m_strName.c_str());

    pDialog->SetHeading(heading);
    pDialog->SetLine(1, 23055);
    pDialog->SetLine(2, 23056);
    pDialog->SetLine(3, m_message);

    //send message and wait for user input
    ThreadMessage tMsg = {TMSG_DIALOG_DOMODAL, WINDOW_DIALOG_OK, g_windowManager.GetActiveWindow()};
    g_application.getApplicationMessenger().SendMessage(tMsg, true);
  }
  /* A unknown event is occurred */
  else if (m_status == STATUS_UNKNOWN)
  {
    CGUIDialogOK* pDialog = (CGUIDialogOK*)g_windowManager.GetWindow(WINDOW_DIALOG_OK);
    if (!pDialog) return;

    CStdString heading;
    heading.Format("%s: %s", g_localizeStrings.Get(23012 + m_addon->m_addonType).c_str(), m_addon->m_strName.c_str());

    pDialog->SetHeading(heading);
    pDialog->SetLine(1, 23057);
    pDialog->SetLine(2, 23056);
    pDialog->SetLine(3, m_message);

    //send message and wait for user input
    ThreadMessage tMsg = {TMSG_DIALOG_DOMODAL, WINDOW_DIALOG_OK, g_windowManager.GetActiveWindow()};
    g_application.getApplicationMessenger().SendMessage(tMsg, true);
  }

  return;
}

/**********************************************************
 * Callback for unknown Add-on types as fallback
 */
CAddonDummyCallback *AddonDummyCallback = new CAddonDummyCallback();


/**********************************************************
 * CAddonManager
 *
 */
 
class CAddonManager g_addonmanager;

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

CAddonManager::CAddonManager()
{

}

CAddonManager::~CAddonManager()
{
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

void CAddonManager::LoadAddons()
{
  CStdString strXMLFile;
  TiXmlDocument xmlDoc;
  TiXmlElement *pRootElement = NULL;
  strXMLFile = GetAddonsFile();
  CLog::Log(LOGNOTICE, "%s", strXMLFile.c_str());
  if ( xmlDoc.LoadFile( strXMLFile ) )
  {
    pRootElement = xmlDoc.RootElement();
    CStdString strValue;
    if (pRootElement)
      strValue = pRootElement->Value();
    if ( strValue != "addons")
      CLog::Log(LOGERROR, "%s addons.xml file does not contain <addons>", __FUNCTION__);
  }
  else if (CFile::Exists(strXMLFile))
    CLog::Log(LOGERROR, "%s Error loading %s: Line %d, %s", __FUNCTION__, strXMLFile.c_str(), xmlDoc.ErrorRow(), xmlDoc.ErrorDesc());

  if (pRootElement)
  { // parse addons...
    m_virtualAddons.clear();
    GetAddons(pRootElement, ADDON_MULTITYPE);
    GetAddons(pRootElement, ADDON_VIZ);
    GetAddons(pRootElement, ADDON_SKIN);
    GetAddons(pRootElement, ADDON_PVRDLL);
    GetAddons(pRootElement, ADDON_SCRIPT);
    GetAddons(pRootElement, ADDON_SCRAPER_PVR);
    GetAddons(pRootElement, ADDON_SCRAPER_VIDEO);
    GetAddons(pRootElement, ADDON_SCRAPER_MUSIC);
    GetAddons(pRootElement, ADDON_SCRAPER_PROGRAM);
    GetAddons(pRootElement, ADDON_SCREENSAVER);
    GetAddons(pRootElement, ADDON_PLUGIN_PVR);
    GetAddons(pRootElement, ADDON_PLUGIN_MUSIC);
    GetAddons(pRootElement, ADDON_PLUGIN_VIDEO);
    GetAddons(pRootElement, ADDON_PLUGIN_PROGRAM);
    GetAddons(pRootElement, ADDON_PLUGIN_PICTURES);
    GetAddons(pRootElement, ADDON_PLUGIN_WEATHER);
    GetAddons(pRootElement, ADDON_DSP_AUDIO);
    // and so on
  }
}

bool CAddonManager::SaveAddons()
{
  // TODO: Should we be specifying utf8 here??
  TiXmlDocument doc;
  TiXmlElement xmlRootElement("addons");
  TiXmlNode *pRoot = doc.InsertEndChild(xmlRootElement);
  if (!pRoot) return false;

  // ok, now run through and save each addons section
  SetAddons(pRoot, ADDON_MULTITYPE, m_multitypeAddons);
  SetAddons(pRoot, ADDON_VIZ, m_visualisationAddons);
  SetAddons(pRoot, ADDON_SKIN, m_skinAddons);
  SetAddons(pRoot, ADDON_PVRDLL, m_pvrAddons);
  SetAddons(pRoot, ADDON_SCRIPT, m_scriptAddons);
  SetAddons(pRoot, ADDON_SCRAPER_PVR, m_scraperPVRAddons);
  SetAddons(pRoot, ADDON_SCRAPER_VIDEO, m_scraperVideoAddons);
  SetAddons(pRoot, ADDON_SCRAPER_MUSIC, m_scraperMusicAddons);
  SetAddons(pRoot, ADDON_SCRAPER_PROGRAM, m_scraperProgramAddons);
  SetAddons(pRoot, ADDON_SCREENSAVER, m_screensaverAddons);
  SetAddons(pRoot, ADDON_PLUGIN_PVR, m_pluginPvrAddons);
  SetAddons(pRoot, ADDON_PLUGIN_MUSIC, m_pluginMusicAddons);
  SetAddons(pRoot, ADDON_PLUGIN_VIDEO, m_pluginVideoAddons);
  SetAddons(pRoot, ADDON_PLUGIN_PROGRAM, m_pluginProgramAddons);
  SetAddons(pRoot, ADDON_PLUGIN_PICTURES, m_pluginPictureAddons);
  SetAddons(pRoot, ADDON_PLUGIN_WEATHER, m_pluginWeatherAddons);
  SetAddons(pRoot, ADDON_DSP_AUDIO, m_DSPAudioAddons);

  return doc.SaveFile(GetAddonsFile());
}

VECADDONS *CAddonManager::GetAllAddons()
{
  return &m_allAddons;//&m_allAddons;
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

  return SaveAddons();
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
  size_t found = platform.Find("all");
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

void CAddonManager::GetAddons(const TiXmlElement* pRootElement, const AddonType &type)
{
  CStdString strTagName;
  switch (type)
  {
  case ADDON_MULTITYPE:
      strTagName = "multitype";
      break;
  case ADDON_VIZ:
      strTagName = "visualistation";
      break;
  case ADDON_SKIN:
      strTagName = "skin";
      break;
  case ADDON_PVRDLL:
      strTagName = "pvr";
      break;
  case ADDON_SCRIPT:
      strTagName = "script";
      break;
  case ADDON_SCRAPER_PVR:
      strTagName = "scraperpvr";
      break;
  case ADDON_SCRAPER_VIDEO:
      strTagName = "scrapervideo";
      break;
  case ADDON_SCRAPER_MUSIC:
      strTagName = "scrapermusic";
      break;
  case ADDON_SCRAPER_PROGRAM:
      strTagName = "scraperprogram";
      break;
  case ADDON_SCREENSAVER:
      strTagName = "screensaver";
      break;
  case ADDON_PLUGIN_PVR:
      strTagName = "pluginpvr";
      break;
  case ADDON_PLUGIN_MUSIC:
      strTagName = "pluginmusic";
      break;
  case ADDON_PLUGIN_VIDEO:
      strTagName = "pluginvideo";
      break;
  case ADDON_PLUGIN_PROGRAM:
      strTagName = "pluginprogram";
      break;
  case ADDON_PLUGIN_PICTURES:
      strTagName = "pluginpictures";
      break;
  case ADDON_PLUGIN_WEATHER:
      strTagName = "pluginweather";
      break;
  case ADDON_DSP_AUDIO:
      strTagName = "dspaudio";
      break;
  default:
    return;
  }

  VECADDONS *addons = GetAddonsFromType(type);
  addons->clear();

  const TiXmlNode *pChild = pRootElement->FirstChild(strTagName.c_str());
  if (pChild)
  {
    pChild = pChild->FirstChild();
    while (pChild > 0)
    {
      CStdString strValue = pChild->Value();
      if (strValue == "addon")
      {
        CAddon addon;
        if (GetAddon(type, pChild, addon))
        {
          addons->push_back(addon);
          /* If it is a virtual child add-on push it also to the m_virtualAddons list */
          if (!addon.m_guid_parent.IsEmpty())
          {
            m_virtualAddons.push_back(addon);
          }
        }
      }
      pChild = pChild->NextSibling();
    }
  }
  else
  {
    CLog::Log(LOGDEBUG, "  <%s> tag is missing or addons.xml is malformed", strTagName.c_str());
  }
}

bool CAddonManager::GetAddon(const AddonType &type, const TiXmlNode *node, CAddon &addon)
{
  // name
  const TiXmlNode *pNodeName = node->FirstChild("name");
  CStdString strName;
  if (pNodeName && pNodeName->FirstChild())
  {
    strName = pNodeName->FirstChild()->Value();
  }
  else
    return false;

  // path
  const TiXmlNode *pNodePath = node->FirstChild("path");
  if (pNodePath && pNodePath->FirstChild())
  {
    addon.m_strPath = pNodePath->FirstChild()->Value();
  }
  else
    return false;

  // childguid if present
  const TiXmlNode *pNodeChildGUID = node->FirstChild("childguid");
  if (pNodeChildGUID && pNodeChildGUID->FirstChild())
  {
    addon.m_guid = pNodeChildGUID->FirstChild()->Value();
  }

  // validate path and addon package
  if (!AddonFromInfoXML(addon.m_strPath, addon))
    return false;

  if (addon.m_addonType != type)
  {
    // something really weird happened
    return false;
  }

  // get custom thumbnail
  const TiXmlNode *pThumbnailNode = node->FirstChild("thumbnail");
  if (pThumbnailNode && pThumbnailNode->FirstChild())
  {
    addon.m_icon = pThumbnailNode->FirstChild()->Value();
  }

  // get custom name
  if (strName != addon.m_strName)
    addon.m_strName = strName;

  return true;
}

void CAddonManager::SaveVirtualAddon(CAddon &addon)
{
  m_virtualAddons.push_back(addon);
  return;
}

CStdString CAddonManager::GetAddonsFile() const
{
  CStdString folder;
  if (g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].hasAddons())
    CUtil::AddFileToFolder(g_settings.GetProfileUserDataFolder(),"addons.xml",folder);
  else
    CUtil::AddFileToFolder(g_settings.GetUserDataFolder(),"addons.xml",folder);

  return folder;
}

CStdString CAddonManager::GetAddonsFolder() const
{
  CStdString folder = "special://home/addons";

  if ( CDirectory::Exists(folder) )
    return folder;

  folder = "special://xbmc/addons";
  return folder;
}

}; /* namespace ADDON */
