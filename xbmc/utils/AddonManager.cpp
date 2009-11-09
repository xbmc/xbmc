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
std::map<ADDON::TYPE, ADDON::IAddonCallback*> CAddonMgr::m_managers;

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

IAddonCallback* CAddonMgr::GetCallbackForType(ADDON::TYPE type)
{
  if (m_managers.find(type) == m_managers.end())
    return NULL;
  else
    return m_managers[type];
}

bool CAddonMgr::RegisterAddonCallback(const ADDON::TYPE type, IAddonCallback* cb)
{
  if (cb == NULL)
    return false;

  m_managers.erase(type);
  m_managers[type] = cb;
  
  return true;
}

void CAddonMgr::UnregisterAddonCallback(ADDON::TYPE type)
{
  m_managers.erase(type);
}

/*****************************************************************************/
bool CAddonMgr::GetAddons(const ADDON::TYPE &type, VECADDONS &addons, const CONTENT_TYPE &content, bool enabled, bool refresh)
{
  // recheck addon folders if necessary
  FindAddons(type, refresh);

  addons.clear();
  bool found = (m_addons.find(type) != m_addons.end());
  if (found)
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
  return found; 
}

bool CAddonMgr::GetAddons(const TYPE &type, VECADDONPROPS &props, const CONTENT_TYPE &content, bool enabled, bool refresh)
{
  props.clear();
  VECADDONS addons;
  bool found = GetAddons(type, addons, content, enabled, refresh);
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

//TODO next two methods duplicate behaviour
bool CAddonMgr::GetAddon(const ADDON::TYPE &type, const CStdString &str, AddonPtr &addon)
{
  // recheck addon folders if necessary
  FindAddons(type);

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

//TODO remove support
bool CAddonMgr::GetAddonFromPath(const CStdString &path, AddonPtr &addon)
{
  // first remove any filenames from path
  CStdString dir;
  CUtil::GetDirectory(path, dir);
  CUtil::RemoveSlashAtEnd(dir);
  /* iterate through alladdons vec and return matched Addon */
  MAPADDONS::iterator typeItr = m_addons.begin();
  while (typeItr !=  m_addons.end())
  {
    VECADDONS addons = m_addons[typeItr->first];
    IVECADDONS adnItr = addons.begin();
    while (adnItr != addons.end())
    {
      if ((*adnItr).get()->Path() == dir)
      {
        addon = (*adnItr);
        return true;
      }
      adnItr++;
    }
    typeItr++;
  }

  return false;
}

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
      CLog::Log(LOGINFO,"ADDON: Enabled %s: %s", TranslateType(addon->Type()).c_str(), addon->Name().c_str());
      return true; //don't update addons.xml until confirmed
    }
  }
  CLog::Log(LOGINFO,"ADDON: Couldn't find Add-on to Enable: %s", addon->Name().c_str());
  return false;
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
      //TODO BUG, if addon disabled from outwith addonbrowser, addons.xml not updated
      // add locks to LoadAddonsXML
      return true;
    }
  }
  CLog::Log(LOGINFO,"ADDON: Couldn't find Add-on to Disable: %s", addon->Name().c_str());
  return false;
}

bool CAddonMgr::SaveAddonsXML(const ADDON::TYPE &type)
{
  VECADDONPROPS props;
  GetAddons(type, props);
  return g_settings.SaveAddonsXML(type, props);
}

bool CAddonMgr::LoadAddonsXML(const ADDON::TYPE &type)
{
  VECADDONPROPS props;
  if (!g_settings.LoadAddonsXML(type, props))
    return false;

  // recheck addon folders if necessary
  FindAddons(type);

  // now enable accordingly
  VECADDONPROPS::const_iterator itr = props.begin();
  while (itr != props.end())
  {
    AddonPtr addon;
    if (itr->parent.empty() && GetAddon(type, itr->uuid, addon))
    {
      addon->Enable();
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

void CAddonMgr::FindAddons(const ADDON::TYPE &type, const bool refresh)
{
  if (!refresh)
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
        CDirectory::GetDirectory("special://home/addons/visualisations", items, ADDON_VIZ_EXT, false);
      CDirectory::GetDirectory("special://xbmc/addons/visualisations", items, ADDON_VIZ_EXT, false);
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
  /* Plugin directories only located in Home */ //TODO why?
  case ADDON_PLUGIN:
    {
      CDirectory::GetDirectory("special://home/addons/plugins", items, ADDON_PLUGIN_MUSIC_EXT, false);
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
      CLog::Log(LOGDEBUG, "ADDON: Error reading %sdescription.xml, bypassing package", item->m_strPath.c_str()); //TODO why slash at end of path?
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
    item->SetThumbnailImage("");
    item->SetCachedProgramThumb();
    if (!item->HasThumbnail())
      item->SetUserProgramThumb();
    if (!item->HasThumbnail())
    {
      CFileItem item2(addon->Path());
      CStdString defaulticon;
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
        item->SetThumbnailImage(item->GetCachedProgramThumb());
      }
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
  CLog::Log(LOGINFO, "ADDON: Found %u addons of type %s", m_addons[type].size(), TranslateType(type).c_str());
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
#elif defined(_WIN32PC)
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
  //TODO why are these 2 in optional fields??
  element = NULL;
  element = xmlDoc.RootElement()->FirstChildElement("library");
  if (element)
    addonProps.libname = element->GetText();

#ifdef _WIN32PC
  /* Retrieve WIN32 library file name in case it is present
  * This is required for no overwrite to the fixed WIN32 add-on's
  * during compile time
  */
  element = NULL;
  element = xmlDoc.RootElement()->FirstChildElement("librarywin32");
  if (element) // If it is found overwrite standard library name
    addonProps.libname = element->GetText();
#endif

  //TODO mvis extension handling
  /*
    
  addonProps.icon = path + "default.tbn";

  /*** end of optional fields ***/

  /* Create an addon object and store in a shared_ptr */
  addon.reset();
  switch (type)
  {
    //case ADDON_PLUGIN:
    //{
    //  AddonPtr temp(new CAddon(addonProps));
    //  addon = temp;
    //  break;
    //}
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

/*****************************************************************************/
ADDON_STATUS CAddonMgr::SetSetting(const IAddon* addon, const char *settingName, const void *settingValue)
{
  if (!addon)
    return STATUS_UNKNOWN;

  CLog::Log(LOGINFO, "ADDONS: set setting of clientName: %s, settingName: %s", addon->Name().c_str(), settingName);
  /*CLIENTMAPITR itr = m_clients.begin();
  while (itr != m_clients.end())
  {
    if (m_clients[(*itr).first]->UUID() == addon->UUID())
    {
      if (m_clients[(*itr).first]->m_strName == addon->Name())
      {
        return m_clients[(*itr).first]->SetSetting(settingName, settingValue);
      }
    }
    itr++;
  } */
  return STATUS_UNKNOWN;
}

bool CAddonMgr::RequestRestart(const IAddon* addon, bool datachanged)
{
  if (!addon)
    return false;

  CLog::Log(LOGINFO, "ADDONS: requested restart of clientName:%s, clientGUID:%s", addon->Name().c_str(), addon->UUID().c_str());
  /*CLIENTMAPITR itr = m_clients.begin();
  while (itr != m_clients.end())
  {
    if (m_clients[(*itr).first]->UUID() == addon->UUID())
    {
      if (m_clients[(*itr).first]->m_strName == addon->Name())
      {
        CLog::Log(LOGINFO, "ADDONS: restarting clientName:%s, clientGUID:%s", addon->Name().c_str(), addon->UUID().c_str());
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

bool CAddonMgr::RequestRemoval(const IAddon* addon)
{
  if (!addon)
    return false;

  CLog::Log(LOGINFO, "ADDONS: requested removal of clientName:%s, clientGUID:%s", addon->Name().c_str(), addon->UUID().c_str());
  /*CLIENTMAPITR itr = m_clients.begin();
  while (itr != m_clients.end())
  {
    if (m_clients[(*itr).first]->UUID() == addon->UUID())
    {
      if (m_clients[(*itr).first]->m_strName == addon->Name())
      {
        CLog::Log(LOGINFO, "ADDONS: removing clientName:%s, clientGUID:%s", addon->Name().c_str(), addon->UUID().c_str());
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
