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

  CLog::Log(LOGINFO, "Called Add-on status handler for '%u' of clientName:%s, clientUUID:%s (same Thread=%s)", status, addon->Name().c_str(), addon->UUID().c_str(), sameThread ? "yes" : "no");

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
    ThreadName.Format("Addon Status: %s", m_addon->Name().c_str());

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
    heading.Format("%s: %s", g_localizeStrings.Get(23012 + m_addon->Type()).c_str(), m_addon->Name().c_str());

    pDialog->SetHeading(heading);
    pDialog->SetLine(1, 23047);
    pDialog->SetLine(2, 23048);

    //send message and wait for user input
    ThreadMessage tMsg = {TMSG_DIALOG_DOMODAL, WINDOW_DIALOG_YES_NO, g_windowManager.GetActiveWindow()};
    g_application.getApplicationMessenger().SendMessage(tMsg, true);

    if (pDialog->IsConfirmed())
    {
      g_addonmanager.GetCallbackForType(m_addon->Type())->RequestRestart(m_addon, false);
    }
  }
  /* Request to restart the AddOn and data structures need updated */
  else if (m_status == STATUS_NEED_RESTART)
  {
    CGUIDialogOK* pDialog = (CGUIDialogOK*)g_windowManager.GetWindow(WINDOW_DIALOG_OK);
    if (!pDialog) return;

    CStdString heading;
    heading.Format("%s: %s", g_localizeStrings.Get(23012 + m_addon->Type()).c_str(), m_addon->Name().c_str());

    pDialog->SetHeading(heading);
    pDialog->SetLine(1, 23049);

    //send message and wait for user input
    ThreadMessage tMsg = {TMSG_DIALOG_DOMODAL, WINDOW_DIALOG_OK, g_windowManager.GetActiveWindow()};
    g_application.getApplicationMessenger().SendMessage(tMsg, true);

    g_addonmanager.GetCallbackForType(m_addon->Type())->RequestRestart(m_addon, true);
  }
  /* Request to restart XBMC (hope no AddOn need or do this) */
  else if (m_status == STATUS_NEED_EMER_RESTART)
  {
    /* okey we really don't need to restart, only deinit Add-on, but that could be damn hard if something is playing*/
    //TODO - General way of handling setting changes that require restart

    CGUIDialogYesNo *pDialog = (CGUIDialogYesNo *)g_windowManager.GetWindow(WINDOW_DIALOG_YES_NO);
    if (!pDialog) return ;

    CStdString heading;
    heading.Format("%s: %s", g_localizeStrings.Get(23012 + m_addon->Type()).c_str(), m_addon->Name().c_str());

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
    heading.Format("%s: %s", g_localizeStrings.Get(23012 + m_addon->Type()).c_str(), m_addon->Name().c_str());

    pDialogYesNo->SetHeading(heading);
    pDialogYesNo->SetLine(1, 23054);
    pDialogYesNo->SetLine(2, 23043);
    pDialogYesNo->SetLine(3, m_message);

    //send message and wait for user input
    ThreadMessage tMsg = {TMSG_DIALOG_DOMODAL, WINDOW_DIALOG_YES_NO, g_windowManager.GetActiveWindow()};
    g_application.getApplicationMessenger().SendMessage(tMsg, true);

    if (!pDialogYesNo->IsConfirmed()) return;

    if (!CAddonSettings::SettingsExist(m_addon->Path()))
    {
      CLog::Log(LOGERROR, "No settings.xml file could be found to AddOn '%s' Settings!", m_addon->Name().c_str());
      return;
    }

    CURL cUrl(m_addon->Path());

    // Load language strings temporarily
    CAddon::LoadAddonStrings(cUrl);

    // Create the dialog
    CGUIDialogAddonSettings* pDialog = (CGUIDialogAddonSettings*) g_windowManager.GetWindow(WINDOW_DIALOG_ADDON_SETTINGS);

    heading.Format("$LOCALIZE[23054]: %s %s", g_localizeStrings.Get(23012 + m_addon->Type()).c_str(), m_addon->Name().c_str());
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
      g_addonmanager.GetCallbackForType(m_addon->Type())->RequestRestart(m_addon, true);
    }
  }
  /* One or more AddOn file(s) missing (check log's for missing data) */
  else if (m_status == STATUS_MISSING_FILE)
  {
    CGUIDialogOK* pDialog = (CGUIDialogOK*)g_windowManager.GetWindow(WINDOW_DIALOG_OK);
    if (!pDialog) return;

    CStdString heading;
    heading.Format("%s: %s", g_localizeStrings.Get(23012 + m_addon->Type()).c_str(), m_addon->Name().c_str());

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
    heading.Format("%s: %s", g_localizeStrings.Get(23012 + m_addon->Type()).c_str(), m_addon->Name().c_str());

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
 * CAddonManager
 *
 */

class CAddonManager g_addonmanager;
std::map<ADDON::TYPE, ADDON::IAddonCallback*> CAddonManager::m_managers;

CAddonManager::CAddonManager()
{

}

CAddonManager::~CAddonManager()
{
}

IAddonCallback* CAddonManager::GetCallbackForType(ADDON::TYPE type)
{
  if (m_managers.find(type) == m_managers.end())
    return NULL;
  else
    return m_managers[type];
}

bool CAddonManager::RegisterAddonCallback(const ADDON::TYPE type, IAddonCallback* cb)
{
  if (cb == NULL)
    return false;

  m_managers.erase(type);
  m_managers[type] = cb;

  return true;
}

void CAddonManager::UnregisterAddonCallback(ADDON::TYPE type)
{
  m_managers.erase(type);
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
    GetAddons(pRootElement, ADDON_SCRAPER);
    GetAddons(pRootElement, ADDON_SCREENSAVER);
    GetAddons(pRootElement, ADDON_PLUGIN);
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
  SetAddons(pRoot, ADDON_SCRAPER, m_scraperAddons);
  SetAddons(pRoot, ADDON_SCREENSAVER, m_screensaverAddons);
  SetAddons(pRoot, ADDON_PLUGIN, m_pluginAddons);
  SetAddons(pRoot, ADDON_DSP_AUDIO, m_DSPAudioAddons);

  return doc.SaveFile(GetAddonsFile());
}

VECADDONS *CAddonManager::GetAllAddons()
{
  return &m_allAddons;//&m_allAddons;
}

VECADDONS *CAddonManager::GetAddonsFromType(const TYPE &type)
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
    case ADDON_SCRAPER:
      return &m_scraperAddons;
    case ADDON_SCREENSAVER:
      return &m_screensaverAddons;
    case ADDON_PLUGIN:
      return &m_pluginAddons;
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
    if (m_allAddons[i].UUID() == guid)
    {
      addon = m_allAddons[i];
      return true;
    }
  }
  return false;
}

bool CAddonManager::GetAddonFromNameAndType(const CStdString &name, const TYPE &type, CAddon &addon)
{
  VECADDONS *addons = GetAddonsFromType(type);
  if (!addons) return false;

  for (IVECADDONS it = addons->begin(); it != addons->end(); it++)
  {
    if ((*it).Name() == name)
    {
      addon = (*it);
      return true;
    }
  }
  return false;
}

bool CAddonManager::DisableAddon(const CStdString &guid, const TYPE &type)
{
  VECADDONS *addons = GetAddonsFromType(type);
  if (!addons) return false;

  for (IVECADDONS it = addons->begin(); it != addons->end(); it++)
  {
    if ((*it).UUID() == guid)
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
      if (m_allAddons[i].UUID() == addon.UUID())
      {
        CLog::Log(LOGNOTICE, "Addon: UUID=%s, Name=%s already present in %s, ignoring package", addon.UUID().c_str(), addon.Name().c_str(), m_allAddons[i].Path().c_str());
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
      CFileItem item2(addon.Path());
      CUtil::AddFileToFolder(addon.Path(), addon.LibName(), item2.m_strPath);
      item2.m_bIsFolder = false;
      item2.SetCachedProgramThumb();
      if (!item2.HasThumbnail())
        item2.SetUserProgramThumb();
      if (!item2.HasThumbnail())
        item2.SetThumbnailImage(addon.Icon());
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
   * 1. uuid exists and is valid
   * 2. type exists and is valid
   * 3. version exists
   * 4. operating system matches ours
   * 5. summary exists
   */

  /* Read uuid */
  CStdString uuid;
  element = xmlDoc.RootElement()->FirstChildElement("uuid");
  if (!element)
  {
    CLog::Log(LOGERROR, "ADDON: %s does not contain the <uuid> element, ignoring", strPath.c_str());
    return false;
  }
  uuid = element->GetText();

  /* Validate type */
  element = xmlDoc.RootElement()->FirstChildElement("type");
  ADDON::TYPE type = TranslateType(element->GetText());
  if (type < ADDON_MULTITYPE || type > ADDON_DSP_AUDIO)
  {
    CLog::Log(LOGERROR, "ADDON: %s has invalid type identifier: '%d'", strPath.c_str(), type);
    return false;
  }

  /* Validate uuid */
  if (!StringUtils::ValidateUUID(uuid))
  {
    CLog::Log(LOGERROR, "ADDON: %s has invalid <uuid> element, ignoring", strPath.c_str());
    return false;
  }

  /* Path & UUID are valid */
  AddonProps addonProps(uuid, type);
  addonProps.path = path;

  /* Retrieve Name */
  CStdString name;
  element = NULL;
  element = xmlDoc.RootElement()->FirstChildElement("title");
  if (!element)
  {
    CLog::Log(LOGERROR, "ADDON: %s missing <title> element, ignoring", strPath.c_str());
    return false;
  }
  addonProps.name = element->GetText();

  /* Retrieve version */
  CStdString version;
  element = NULL;
  element = xmlDoc.RootElement()->FirstChildElement("version");
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
  addonProps.version = version;

  /* Retrieve platforms which this addon supports */
  CStdString platform;
  element = NULL;
  element = xmlDoc.RootElement()->FirstChildElement("platforms")->FirstChildElement("platform");
  if (!element)
  {
    CLog::Log(LOGERROR, "ADDON: %s missing <platforms> element, ignoring", strPath.c_str());
    return false;
  }

  bool all;
  std::set<CStdString> platforms;
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
#if defined(_LINUX)
    if (!platforms.count("linux"))
    {
      CLog::Log(LOGERROR, "ADDON: %s is not supported under Linux, ignoring", strPath.c_str());
      return false;
    }
#elif defined(_WIN32)
    if (!platforms.count("windows"))
    {
      CLog::Log(LOGERROR, "ADDON: %s is not supported under Windows, ignoring", strPath.c_str());
      return false;
    }
#elif defined(__APPLE__)
    if (!platforms.count("osx"))
    {
      CLog::Log(LOGERROR, "ADDON: %s is not supported under OSX, ignoring", strPath.c_str());
      return false;
    }
#elif defined(_XBOX)
    if (!platforms.count("xbox"))
    {
      CLog::Log(LOGERROR, "ADDON: %s is not supported under XBOX, ignoring", strPath.c_str());
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
    CLog::Log(LOGERROR, "ADDON: %s missing <summary> element, ignoring", strPath.c_str());
    return false;
  }
  addonProps.summary = element->GetText();

  if (addonProps.type == ADDON_SCRAPER || addonProps.type == ADDON_PLUGIN)
  {
    /* Retrieve content types that this addon supports */
    CStdString platform;
    element = NULL;
    if (xmlDoc.RootElement()->FirstChildElement("supportedcontent"))
    {
      element = xmlDoc.RootElement()->FirstChildElement("supportedcontent")->FirstChildElement("content");
    }
    if (!element)
    {
      CLog::Log(LOGERROR, "ADDON: %s missing <supportedcontent> element, ignoring", strPath.c_str());
      return false;
    }

    std::set<CONTENT_TYPE> contents;
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
      CLog::Log(LOGERROR, "ADDON: %s %s supports no available content-types, ignoring",  TranslateType(addonProps.type).c_str(), addonProps.name.c_str());
      return false;
    }
    else
    {
      addonProps.contents = contents;
    }
  }

  /*** Beginning of optional fields ***/
  /* Retrieve description */
  element = NULL;
  element = xmlDoc.RootElement()->FirstChildElement("description");
  if (element)
    addonProps.description = element->GetText();

  /* Retrieve author */
  element = NULL;
  element = xmlDoc.RootElement()->FirstChildElement("author");
  if (element)
    addonProps.author = element->GetText();

  /* Retrieve disclaimer */
  element = NULL;
  element = xmlDoc.RootElement()->FirstChildElement("disclaimer");
  if (element)
    addonProps.disclaimer = element->GetText();

  /* Retrieve library file name */
  //TODO why are these 2 in optional fields??
  element = NULL;
  element = xmlDoc.RootElement()->FirstChildElement("library");
  if (element)
    addonProps.libname = element->GetText();

#ifdef _WIN32
  /* Retrieve WIN32 library file name in case it is present
   * This is required for no overwrite to the fixed WIN32 add-on's
   * during compile time
   */
  element = NULL;
  element = xmlDoc.RootElement()->FirstChildElement("librarywin32");
  if (element) // If it is found overwrite standard library name
    addonProps.libname = element->GetText();
#endif

  /* Retrieve thumbnail file name */
  CStdString icon;
  element = NULL;
  element = xmlDoc.RootElement()->FirstChildElement("icon");
  if (element)
  {
    CStdString iconPath = element->GetText();
    addonProps.icon = path + iconPath;
  }

  /*** end of optional fields ***/

  addon.Set(addonProps);
  /* Everything's valid */

  CLog::Log(LOGINFO, "Addon: %s retrieved. Name: %s, UUID: %s, Version: %s",
            strPath.c_str(), addon.Name().c_str(), addon.UUID().c_str(), addon.Version().c_str());

  return true;
}

bool CAddonManager::SetAddons(TiXmlNode *root, const TYPE &type, const VECADDONS &addons)
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
    case ADDON_SCRAPER:
      strType = "scraper";
      break;
    case ADDON_SCREENSAVER:
      strType = "screensaver";
      break;
    case ADDON_PLUGIN:
      strType = "plugin";
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

      XMLUtils::SetString(&element, "name", addon.Name());
      XMLUtils::SetPath(&element, "path", addon.Path());
      if (!addon.Parent().IsEmpty())
        XMLUtils::SetString(&element, "childuuid", addon.UUID());

      if (!addon.Icon().IsEmpty())
        XMLUtils::SetPath(&element, "thumbnail", addon.Icon());

      sectionNode->InsertEndChild(element);
    }
  }
  return true;
}

void CAddonManager::GetAddons(const TiXmlElement* pRootElement, const TYPE &type)
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
  case ADDON_SCRAPER:
      strTagName = "scraper";
      break;
  case ADDON_PLUGIN:
      strTagName = "plugin";
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
          if (!addon.Parent().IsEmpty())
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

bool CAddonManager::GetAddon(const TYPE &type, const TiXmlNode *node, CAddon &addon)
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
  CStdString strPath;
  if (pNodePath && pNodePath->FirstChild())
  {
    strPath = pNodePath->FirstChild()->Value();
  }
  else
    return false;

  // validate path and addon package
  if (!AddonFromInfoXML(strPath, addon))
    return false;

  if (addon.Type() != type)
  {
    // something really weird happened
    return false;
  }

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
