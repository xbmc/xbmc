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
#include "Application.h"
#include "utils/log.h"
#include "StringUtils.h"
#include "RegExp.h"
#include "XMLUtils.h"
#include "GUIDialogYesNo.h"
#include "GUIDialogOK.h"
#include "GUIDialogAddonSettings.h"
#include "GUIWindowManager.h"
#include "FileItem.h"
#include "GUISettings.h"
#include "SingleLock.h"

#ifdef HAS_VISUALISATION
#include "../visualizations/DllVisualisation.h"
#include "../visualizations/Visualisation.h"
#endif
#ifdef HAS_PVRCLIENTS
#include "../pvrclients/DllPVRClient.h"
#include "../pvrclients/PVRClient.h"
#endif
#ifdef HAS_SCREENSAVER
#include "../screensavers/DllScreenSaver.h"
#include "../screensavers/ScreenSaver.h"
#endif
//#ifdef HAS_SCRAPERS
#include "../Scraper.h"
//#endif


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

CAddonStatusHandler::CAddonStatusHandler(IAddon* addon, ADDON_STATUS status, CStdString message, bool sameThread)
  : m_addon(addon)
{
  if (m_addon == NULL)
    return;

  CLog::Log(LOGINFO, "Called Add-on status handler for '%u' of clientName:%s, clientGUID:%s (same Thread=%s)", status, m_addon->Name().c_str(), m_addon->UUID().c_str(), sameThread ? "yes" : "no");

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
    /*heading.Format("%s: %s", g_localizeStrings.Get(23012 + m_addon->Type()).c_str(), m_addon->Name().c_str());*/

    pDialog->SetHeading(heading);
    pDialog->SetLine(1, 23047);
    pDialog->SetLine(2, 23048);

    //send message and wait for user input
    ThreadMessage tMsg = {TMSG_DIALOG_DOMODAL, WINDOW_DIALOG_YES_NO, g_windowManager.GetActiveWindow()};
    g_application.getApplicationMessenger().SendMessage(tMsg, true);

    if (pDialog->IsConfirmed())
    {
      CAddonMgr::Get()->GetCallbackForType(m_addon->Type())->RequestRestart(m_addon, false);
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

    CAddonMgr::Get()->GetCallbackForType(m_addon->Type())->RequestRestart(m_addon, true);
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
  /* Some required settings are missing/invalid */
  else if (m_status == STATUS_NEED_SETTINGS)
  {
    CGUIDialogYesNo* pDialogYesNo = (CGUIDialogYesNo*)g_windowManager.GetWindow(WINDOW_DIALOG_YES_NO);
    if (!pDialogYesNo) return;

    CStdString heading;
    heading.Format("%s: %s", g_localizeStrings.Get(23012 + m_addon->Type()).c_str(), m_addon->Name().c_str());

    pDialogYesNo->SetHeading(heading);
    pDialogYesNo->SetLine(1, 23053);
    pDialogYesNo->SetLine(2, 23043);
    pDialogYesNo->SetLine(3, m_message);

    //send message and wait for user input
    ThreadMessage tMsg = {TMSG_DIALOG_DOMODAL, WINDOW_DIALOG_YES_NO, g_windowManager.GetActiveWindow()};
    g_application.getApplicationMessenger().SendMessage(tMsg, true);

    if (!pDialogYesNo->IsConfirmed()) return;

    if (m_addon->Type() == ADDON_SKIN)
    {
      CLog::Log(LOGERROR, "ADDONS: Incompatible type (%s) encountered", ADDON::TranslateType(m_addon->Type()).c_str());
      return;
    }

    if (!m_addon->HasSettings())
    {
      CLog::Log(LOGERROR, "No settings.xml file could be found to AddOn '%s' Settings!", m_addon->Name().c_str());
      return;
    }

    // Create the dialog
    CGUIDialogAddonSettings* pDialog = (CGUIDialogAddonSettings*) g_windowManager.GetWindow(WINDOW_DIALOG_ADDON_SETTINGS);

    heading.Format("$LOCALIZE[23053]: %s %s", g_localizeStrings.Get(23012 + m_addon->Type()).c_str(), m_addon->Name().c_str());
    pDialog->SetHeading(heading);
    const AddonPtr addon(m_addon);
    pDialog->SetAddon(addon);

    pDialog->DoModal();

    if (pDialog->IsConfirmed())
    {
      m_addon->SaveSettings();
      CAddonMgr::Get()->GetCallbackForType(m_addon->Type())->RequestRestart(m_addon, true);
    }
    else
    {
      m_addon->LoadSettings();
    }
  }
  // One or more AddOn file(s) missing (check log's for missing data)
  //TODO if installer has file manifest per addon (for incremental updates), we can check this ourselves
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

    CStdString heading, name;
    name = m_addon->Name();
    int type = m_addon->Type();
    heading.Format("%s: %s", g_localizeStrings.Get(23012 + type).c_str(), name.c_str());

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
 * CAddonMgr
 *
 */

CAddonMgr* CAddonMgr::m_pInstance = NULL;
std::map<ADDON::TYPE, ADDON::IAddonMgrCallback*> CAddonMgr::m_managers;

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

IAddonMgrCallback* CAddonMgr::GetCallbackForType(ADDON::TYPE type)
{
  if (m_managers.find(type) == m_managers.end())
    return NULL;
  else
    return m_managers[type];
}

bool CAddonMgr::RegisterAddonMgrCallback(const ADDON::TYPE type, IAddonMgrCallback* cb)
{
  if (cb == NULL)
    return false;

  m_managers.erase(type);
  m_managers[type] = cb;
  
  return true;
}

void CAddonMgr::UnregisterAddonMgrCallback(ADDON::TYPE type)
{
  m_managers.erase(type);
}

bool CAddonMgr::HasAddons(const ADDON::TYPE &type, const CONTENT_TYPE &content/*= CONTENT_NONE*/)
{
  if (content == CONTENT_NONE)
    return (m_addons.find(type) != m_addons.end());

  VECADDONS addons;
  return GetAddons(type, addons, content, true);
}

bool CAddonMgr::GetAddons(const ADDON::TYPE &type, VECADDONS &addons, const CONTENT_TYPE &content/*= CONTENT_NONE*/, bool enabled/*= true*/)
{
  // recheck addon folders if necessary
  LoadAddonsXML(type);

  addons.clear();
  if (m_addons.find(type) != m_addons.end())
  {
    IVECADDONS itr = m_addons[type].begin();
    while (itr != m_addons[type].end())
    { // filter out what we're not looking for      
      if ((enabled && (*itr)->Disabled())
        || (!(*itr)->Disabled() && !enabled)
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

bool CAddonMgr::GetAddons(const TYPE &type, VECADDONPROPS &props, const CONTENT_TYPE &content, bool enabled)
{
  props.clear();
  VECADDONS addons;
  bool found = GetAddons(type, addons, content, enabled);
  if (found)
  {  // copy each addon's properties    
    IVECADDONS itr = addons.begin();
    while (itr != addons.end())
    {
      const AddonProps addon(*itr);
      props.push_back(addon);
      ++itr;
    }
  }
  return found; 
}

bool CAddonMgr::GetAddon(const ADDON::TYPE &type, const CStdString &str, AddonPtr &addon)
{
  if (m_addons.find(type) == m_addons.end())
    return false;

  bool isUUID = StringUtils::ValidateUUID(str);

  VECADDONS &addons = m_addons[type];
  IVECADDONS adnItr = addons.begin();
  while (adnItr != addons.end())
  {
    //FIXME scrapers were previously registered by filename
    if (isUUID && (*adnItr)->UUID() == str 
      || !isUUID && (*adnItr)->Name() == str
      || type == ADDON_SCRAPER && (*adnItr)->LibName() == str)
    {
      addon = (*adnItr);
      return true;
    }
    adnItr++;
  }

  return false;
}

//TODO handle all 'default' cases here, not just scrapers?
bool CAddonMgr::GetDefaultScraper(CScraperPtr &scraper, const CONTENT_TYPE & content)
{ 
  AddonPtr addon;
  if (GetDefaultScraper(addon, content))
    scraper = boost::dynamic_pointer_cast<CScraper>(addon->Clone());
  else
    return false;

  return true;
}
bool CAddonMgr::GetDefaultScraper(AddonPtr &scraper, const CONTENT_TYPE &content)
{
  CStdString defaultScraper;
  switch (content)
  {
  case CONTENT_MOVIES:
    {
      defaultScraper = g_guiSettings.GetString("scrapers.moviedefault");
      break;
    }
  case CONTENT_TVSHOWS:
    {
      defaultScraper = g_guiSettings.GetString("scrapers.tvshowdefault");
      break;
    }
  case CONTENT_MUSICVIDEOS:
    {
      defaultScraper = g_guiSettings.GetString("scrapers.musicvideodefault");
      break;
    }
  case CONTENT_ALBUMS:
  case CONTENT_ARTISTS:
    {
      defaultScraper = g_guiSettings.GetString("musiclibrary.defaultscraper");
      break;
    }
  default:
    return false;
  }
  return GetAddon(ADDON_SCRAPER, defaultScraper, scraper);
}

CStdString CAddonMgr::GetString(const CStdString &uuid, const int number)
{
  AddonPtr addon = m_uuidMap[uuid];
  if (addon)
    return addon->GetString(number);

  return "";
}

bool CAddonMgr::EnableAddon(const CStdString &uuid)
{
  AddonPtr addon = m_uuidMap[uuid];
  if (!addon)
    return false;
  
  return EnableAddon(addon);
}
bool CAddonMgr::EnableAddon(AddonPtr &addon)
{
  const ADDON::TYPE type = addon->Type();

  if (m_addons.find(type) == m_addons.end())
    return false;

  for (IVECADDONS itr = m_addons[type].begin(); itr != m_addons[type].end(); itr++)
  {
    if (addon->UUID() == (*itr)->UUID())
    {
      addon->Enable();
      CUtil::CreateDirectoryEx(addon->Profile());
      CLog::Log(LOGINFO,"ADDON: Enabled %s: %s", TranslateType(addon->Type()).c_str(), addon->Name().c_str());
      LoadAddonsXML(type);
      return true;
    }
  }
  CLog::Log(LOGINFO,"ADDON: Couldn't find Add-on to Enable: %s", addon->Name().c_str());
  return false;
}

bool CAddonMgr::DisableAddon(const CStdString &uuid)
{
  AddonPtr addon = m_uuidMap[uuid];
  if (!addon)
    return false;
  
  return DisableAddon(addon);
}
bool CAddonMgr::DisableAddon(AddonPtr &addon)
{
  const ADDON::TYPE type = addon->Type();

  if (m_addons.find(type) == m_addons.end())
    return false;

  for (IVECADDONS itr = m_addons[type].begin(); itr != m_addons[type].end(); itr++)
  {
    if (addon == (*itr))
    {
      addon->Disable();

      if (!addon->Parent().empty())
      { // we can delete this duplicate addon
        m_addons[type].erase(itr);
      }

      CLog::Log(LOGINFO,"ADDON: Disabled Add-on: %s", addon->Name().c_str());
      SaveAddonsXML(type);
      return true;
    }
  }
  CLog::Log(LOGINFO,"ADDON: Couldn't find Add-on to Disable: %s", addon->Name().c_str());
  return false;
}

bool CAddonMgr::LoadAddonsXML(const ADDON::TYPE &type, const bool refreshDirs/*=false*/)
{
  VECADDONPROPS props;
  if (!LoadAddonsXML(type, props))
    return false;

  // refresh addon dirs if neccesary/forced
  FindAddons(type, refreshDirs);

  // now enable accordingly
  VECADDONPROPS::const_iterator itr = props.begin();
  while (itr != props.end())
  {
    AddonPtr addon;
    if (itr->parent.empty() && GetAddon(type, itr->uuid, addon))
    {
      EnableAddon(addon);
    }
    else if (GetAddon(type, itr->parent, addon))
    { // multiple addon configurations
      AddonPtr clone = addon->Clone();
      if (clone)
      {
        m_addons[type].push_back(clone);
      }
    }
    else
    { // addon not found
      CLog::Log(LOGERROR, "ADDON: Couldn't find %s requested. Name: %s", TranslateType(type).c_str(), itr->name.c_str());
      //TODO we should really add but mark unavailable, to prompt user
    }
    ++itr;
  }
  return true;
}

void CAddonMgr::FindAddons(const ADDON::TYPE &type, const bool force)
{
  if (!force)
  {
    // recheck each addontype's directories no more than once every ADDON_DIRSCAN_FREQ seconds
    CDateTimeSpan span;
    span.SetDateTimeSpan(0, 0, 0, ADDON_DIRSCAN_FREQ);
    if(m_lastScan[type].IsValid() && m_lastScan[type] > (CDateTime::GetCurrentDateTime() - span))
      return;
  }

  // parse the user & system dirs for addons of the requested type
  CFileItemList items;
  bool isHome = CSpecialProtocol::XBMCIsHome();
  switch (type)
  {
  case ADDON_PVRDLL:
    {
      if (!isHome)
        CDirectory::GetDirectory("special://home/addons/pvr", items, ADDON_PVRDLL_EXT, false);
      CDirectory::GetDirectory("special://xbmc/addons/pvr", items, ADDON_PVRDLL_EXT, false);
      break;
    }
  case ADDON_VIZ:
    { //TODO fix mvis handling
      if (!isHome)
        CDirectory::GetDirectory("special://home/addons/visualizations", items, ADDON_VIZ_EXT, false);
      CDirectory::GetDirectory("special://xbmc/addons/visualizations", items, ADDON_VIZ_EXT, false);
      break;
    }
  case ADDON_SCREENSAVER:
    {
      if (!isHome)
        CDirectory::GetDirectory("special://home/addons/screensavers", items, ADDON_SCREENSAVER_EXT, false);
      CDirectory::GetDirectory("special://xbmc/addons/screensavers", items, ADDON_SCREENSAVER_EXT, false);
      break;
    }
  case ADDON_DSP_AUDIO:
    {
      if (!isHome)
        CDirectory::GetDirectory("special://home/addons/dsp-audio", items, ADDON_DSP_AUDIO_EXT, false);
      CDirectory::GetDirectory("special://xbmc/addons/dsp-audio", items, ADDON_DSP_AUDIO_EXT, false);
      break;
    }
  case ADDON_SCRAPER:
    {
      if (!isHome)
        CDirectory::GetDirectory("special://home/addons/scrapers", items, ADDON_SCRAPER_EXT, false);
      CDirectory::GetDirectory("special://xbmc/addons/scrapers", items, ADDON_SCRAPER_EXT, false);
      break;
    }
  case ADDON_SCRIPT:
    {
      if (!isHome)
        CDirectory::GetDirectory("special://home/addons/scripts", items, ADDON_PYTHON_EXT, false);
      CDirectory::GetDirectory("special://xbmc/addons/scripts", items, ADDON_PYTHON_EXT, false);
      break;
    }
  case ADDON_PLUGIN:
    {
      if (!isHome)
        CDirectory::GetDirectory("special://home/addons/plugins", items, ADDON_PYTHON_EXT, false);
      CDirectory::GetDirectory("special://xbmc/addons/plugins", items, ADDON_PYTHON_EXT, false);
      break;
    }
  default:
    return;
  }

  // for all folders found
  for (int i = 0; i < items.Size(); ++i)
  {
    bool known = false;
    CFileItemPtr item = items[i];

    // read description.xml and populate the addon
    AddonPtr addon;
    if (!AddonFromInfoXML(type, item->m_strPath, addon))
    {
      CLog::Log(LOGDEBUG, "ADDON: Error reading %sdescription.xml, bypassing package", item->m_strPath.c_str());
      continue;
    }

    // update any existing match
    if (m_addons.find(type) != m_addons.end())
    {
      for (unsigned int i = 0; i < m_addons[type].size(); i++)
      {
        if (m_addons[type][i]->UUID() == addon->UUID())
        {
          //TODO inform any manager first, and request removal
          //TODO choose most recent version if varying
          m_addons[type][i] = addon;
          m_uuidMap[addon->UUID()] = addon;
          known = true;
          break;
        }
      }
    }

    // check for/cache icon thumbnail 
    //TODO cache one thumb per addon uuid instead
    CFileItem item2(addon->Path());
    CUtil::AddFileToFolder(addon->Path(), addon->LibName(), item2.m_strPath);
    item2.m_bIsFolder = false;
    item2.SetCachedProgramThumb();
    if (!item2.HasThumbnail())
      item2.SetUserProgramThumb();
    if (!item2.HasThumbnail())
      item2.SetThumbnailImage(addon->Icon());
    if (item2.HasThumbnail())
    {
      XFILE::CFile::Cache(item2.GetThumbnailImage(),item->GetCachedProgramThumb());
    }

    // sanity check
    assert(addon->Type() == type);

    // everything ok, add to available addons if new
    if (!known)
    {
      m_addons[type].push_back(addon);
      m_uuidMap.insert(std::make_pair(addon->UUID(), addon));
    }
  }

  m_lastScan[type] = CDateTime::GetCurrentDateTime();
  CLog::Log(LOGINFO, "ADDON: Found %zu addons of type %s", m_addons[type].size(), TranslateType(type).c_str());
}

bool CAddonMgr::AddonFromInfoXML(const ADDON::TYPE &reqType, const CStdString &path, AddonPtr &addon)
{
  // First check that we can load description.xml
  CStdString strPath(path);
  CUtil::AddFileToFolder(strPath, ADDON_METAFILE, strPath);

  TiXmlDocument xmlDoc;
  if (!xmlDoc.LoadFile(strPath))
  {
    CLog::Log(LOGERROR, "Unable to load: %s, Line %d\n%s", strPath.c_str(), xmlDoc.ErrorRow(), xmlDoc.ErrorDesc());
    return false;
  }

  TiXmlElement *element = xmlDoc.RootElement();
  if (!element || strcmpi(element->Value(), "addoninfo") != 0)
  {
    CLog::Log(LOGERROR, "ADDON: Error loading %s: cannot find <addon> root element", strPath.c_str());
    return false;
  }

  /* Steps required to meet package requirements
  * 1. uuid exists and is valid
  * 2. type exists and is valid
  * 3. version exists
  * 4. operating system matches ours
  * 5. summary exists
  * 6. for scrapers & plugins, support at least one type of content
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
  if (type != reqType)
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
  // will be replaced with default library name if unspecified
  element = NULL;
  element = xmlDoc.RootElement()->FirstChildElement("library");
  if (element)
    addonProps.libname = element->GetText();

  //TODO mvis extension handling
  //TODO move this to addon specific class, if it's needed at all..
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

  /*** end of optional fields ***/

  /* Create an addon object and store in a shared_ptr */
  addon.reset();
  switch (type)
  {
    case ADDON_PLUGIN:
    {
      AddonPtr temp(new CAddon(addonProps));
      addon = temp;
      break;
    }
    case ADDON_SCRIPT:
    {
      AddonPtr temp(new CAddon(addonProps));
      addon = temp;
      break;
    }
    case ADDON_SCRAPER:
    {
      AddonPtr temp(new CScraper(addonProps));
      addon = temp;
      break;
    }
    case ADDON_VIZ:
    {
      AddonPtr temp(new CVisualisation(addonProps));
      addon = temp;
      break;
    }
    case ADDON_SCREENSAVER:
    {
      AddonPtr temp(new CScreenSaver(addonProps));
      addon = temp;
      break;
    }
    default:
      return false;
  }

  /* Everything's valid */
  //CLog::Log(LOGDEBUG, "ADDON: Discovered: Name: %s, UUID: %s, Version: %s, Path: %s", addon->Name().c_str(), addon->UUID().c_str(), addon->Version().c_str(), addon->Path().c_str());

  return true;
}

CStdString CAddonMgr::GetAddonsXMLFile() const
{
  CStdString folder;
  if (g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].hasAddons())
    CUtil::AddFileToFolder(g_settings.GetProfileUserDataFolder(),"addons.xml",folder);
  else
    CUtil::AddFileToFolder(g_settings.GetUserDataFolder(),"addons.xml",folder);

  return folder;
}

bool CAddonMgr::SaveAddonsXML(const ADDON::TYPE &type)
{
  VECADDONPROPS props;
  GetAddons(type, props);

  // TODO: Should we be specifying utf8 here??
  TiXmlDocument doc;

  if (!doc.LoadFile(GetAddonsXMLFile()))
    doc.ClearError();

  // either point to existing addons node, or create one
  TiXmlNode *pRoot = NULL;
  pRoot = doc.FirstChildElement("addons");
  if (!pRoot)
  {
    TiXmlElement xmlRootElement("addons");
    pRoot = doc.InsertEndChild(xmlRootElement);
    if (!pRoot)
      return false;
  }

  // ok, now run through and save the modified addons section
  SetAddons(pRoot, type, props);

  return doc.SaveFile(GetAddonsXMLFile());
}

bool CAddonMgr::SetAddons(TiXmlNode *root, const ADDON::TYPE &type, const VECADDONPROPS &addons)
{
  CStdString strType;
  strType = ADDON::TranslateType(type);

  if (strType.IsEmpty())
    return false;

  TiXmlElement sectionElement(strType);
  TiXmlNode *sectionNode = root->FirstChild(strType);

  if (sectionNode)
  { // must delete the original section before regenerating
    root->RemoveChild(sectionNode);
  }

  if (!addons.empty())
  { // only recreate the sectionNode if there's addons of this type enabled
    sectionNode = root->InsertEndChild(sectionElement);

    VECADDONPROPS::const_iterator itr = addons.begin();
    while (itr != addons.end())
    {
      TiXmlElement element("addon");

      XMLUtils::SetString(&element, "name", itr->name);
      XMLUtils::SetString(&element, "uuid", itr->uuid);
      if (!itr->parent.IsEmpty())
        XMLUtils::SetString(&element, "parentuuid", itr->parent);

      if (!itr->icon.IsEmpty())
        XMLUtils::SetPath(&element, "thumbnail", itr->icon);

      sectionNode->InsertEndChild(element);
      ++itr;
    }
  }

  return true;
}

bool CAddonMgr::LoadAddonsXML(const ADDON::TYPE &type, VECADDONPROPS &addons)
{
  CStdString strXMLFile;
  TiXmlDocument xmlDoc;
  TiXmlElement *pRootElement = NULL;
  strXMLFile = GetAddonsXMLFile();
  CLog::Log(LOGNOTICE, "ADDONS: Attempting to parse %s", strXMLFile.c_str());
  if ( xmlDoc.LoadFile( strXMLFile ) )
  {
    pRootElement = xmlDoc.RootElement();
    CStdString strValue;
    if (pRootElement)
      strValue = pRootElement->Value();
    if ( strValue != "addons")
    {
      CLog::Log(LOGERROR, "ADDONS: %s does not contain <addons> element", strXMLFile.c_str());
      return false;
    }
  }
  else if (CFile::Exists(strXMLFile))
  {
    CLog::Log(LOGERROR, "ADDONS: Error loading %s: Line %d, %s", strXMLFile.c_str(), xmlDoc.ErrorRow(), xmlDoc.ErrorDesc());
    return false;
  }
  else 
  {
    CLog::Log(LOGINFO, "ADDONS: No addons.xml found");
    return true; // no addons enabled for this profile yet
  }

  if (pRootElement)
  { // parse addons...
    GetAddons(pRootElement, type, addons);
    return true;
  }

  return false;
}

void CAddonMgr::GetAddons(const TiXmlElement* pRootElement, const ADDON::TYPE &type, VECADDONPROPS &addons)
{
  CStdString strTagName;
  strTagName = ADDON::TranslateType(type);

  const TiXmlNode *pChild = pRootElement->FirstChild(strTagName.c_str());
  if (pChild)
  {
    pChild = pChild->FirstChild();
    while (pChild > 0)
    {
      CStdString strValue = pChild->Value();
      if (strValue == "addon")
      {
        GetAddon(type, pChild, addons);
      }
      pChild = pChild->NextSibling();
    }
  }
  else
  {
    CLog::Log(LOGDEBUG, "ADDONS: <%s> tag is missing or addons.xml is malformed", strTagName.c_str());
  }
}

bool CAddonMgr::GetAddon(const ADDON::TYPE &type, const TiXmlNode *node, VECADDONPROPS &addons)
{
  // uuid
  const TiXmlNode *pNodePath = node->FirstChild("uuid");
  CStdString uuid;
  if (pNodePath && pNodePath->FirstChild())
  {
    uuid = pNodePath->FirstChild()->Value();
  }
  else
    return false;

  AddonProps props(uuid, type);

  // name
  const TiXmlNode *pNodeName = node->FirstChild("name");
  CStdString strName;
  if (pNodeName && pNodeName->FirstChild())
  {
    props.name = pNodeName->FirstChild()->Value();
  }
  else
    return false;

  // parent uuid if present
  const TiXmlNode *pNodeChildGUID = node->FirstChild("parentuuid");
  if (pNodeChildGUID && pNodeChildGUID->FirstChild())
  {
    props.parent = pNodeChildGUID->FirstChild()->Value();
  }

  // get custom thumbnail
  const TiXmlNode *pThumbnailNode = node->FirstChild("thumbnail");
  if (pThumbnailNode && pThumbnailNode->FirstChild())
  {
    props.icon = pThumbnailNode->FirstChild()->Value();
  }

  // finished
  if (/*props.Valid()*/true)
  {
    addons.insert(addons.end(), props);
    return true;
  }

  return false;
}

} /* namespace ADDON */

