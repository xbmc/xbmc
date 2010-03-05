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
#include "Settings.h"
#include "GUISettings.h"
#include "SingleLock.h"
#include "DownloadQueueManager.h"

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

  CLog::Log(LOGINFO, "Called Add-on status handler for '%u' of clientName:%s, clientID:%s (same Thread=%s)", status, m_addon->Name().c_str(), m_addon->ID().c_str(), sameThread ? "yes" : "no");

  m_status  = status;
  m_message = message;

  if (sameThread)
  {
    Process();
  }
  else
  {
    CStdString ThreadName;
    ThreadName.Format("Addon Status: %s", m_addon->Name().c_str());

    Create(true, THREAD_MINSTACKSIZE);
    SetName(ThreadName.c_str());
    SetPriority(-15);
  }
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
}

void CAddonStatusHandler::Process()
{
  CSingleLock lock(m_critSection);

  CStdString heading;
  heading.Format("%s: %s", TranslateType(m_addon->Type(), true).c_str(), m_addon->Name().c_str());

  /* AddOn lost connection to his backend (for ones that use Network) */
  if (m_status == STATUS_LOST_CONNECTION)
  {
    CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)g_windowManager.GetWindow(WINDOW_DIALOG_YES_NO);
    if (!pDialog) return;

    pDialog->SetHeading(heading);
    pDialog->SetLine(1, 24070);
    pDialog->SetLine(2, 24073);

    //send message and wait for user input
    ThreadMessage tMsg = {TMSG_DIALOG_DOMODAL, WINDOW_DIALOG_YES_NO, g_windowManager.GetActiveWindow()};
    g_application.getApplicationMessenger().SendMessage(tMsg, true);

    if (pDialog->IsConfirmed())
      CAddonMgr::Get()->GetCallbackForType(m_addon->Type())->RequestRestart(m_addon, false);
  }
  /* Request to restart the AddOn and data structures need updated */
  else if (m_status == STATUS_NEED_RESTART)
  {
    CGUIDialogOK* pDialog = (CGUIDialogOK*)g_windowManager.GetWindow(WINDOW_DIALOG_OK);
    if (!pDialog) return;

    pDialog->SetHeading(heading);
    pDialog->SetLine(1, 24074);

    //send message and wait for user input
    ThreadMessage tMsg = {TMSG_DIALOG_DOMODAL, WINDOW_DIALOG_OK, g_windowManager.GetActiveWindow()};
    g_application.getApplicationMessenger().SendMessage(tMsg, true);

    CAddonMgr::Get()->GetCallbackForType(m_addon->Type())->RequestRestart(m_addon, true);
  }
  /* Some required settings are missing/invalid */
  else if (m_status == STATUS_NEED_SETTINGS)
  {
    CGUIDialogYesNo* pDialogYesNo = (CGUIDialogYesNo*)g_windowManager.GetWindow(WINDOW_DIALOG_YES_NO);
    if (!pDialogYesNo) return;

    pDialogYesNo->SetHeading(heading);
    pDialogYesNo->SetLine(1, 24070);
    pDialogYesNo->SetLine(2, 24072);
    pDialogYesNo->SetLine(3, m_message);

    //send message and wait for user input
    ThreadMessage tMsg = {TMSG_DIALOG_DOMODAL, WINDOW_DIALOG_YES_NO, g_windowManager.GetActiveWindow()};
    g_application.getApplicationMessenger().SendMessage(tMsg, true);

    if (!pDialogYesNo->IsConfirmed()) return;

    if (!m_addon->HasSettings())
      return;

    const AddonPtr addon(m_addon);
    if (CGUIDialogAddonSettings::ShowAndGetInput(addon))
    {
      //todo doesn't dialogaddonsettings save these automatically? should do
      m_addon->SaveSettings();
      CAddonMgr::Get()->GetCallbackForType(m_addon->Type())->RequestRestart(m_addon, true);
    }
    else
      m_addon->LoadSettings();
  }
  /* A unknown event has occurred */
  else if (m_status == STATUS_UNKNOWN)
  {
    CAddonMgr::Get()->DisableAddon(m_addon->ID());
    CGUIDialogOK* pDialog = (CGUIDialogOK*)g_windowManager.GetWindow(WINDOW_DIALOG_OK);
    if (!pDialog) return;

    pDialog->SetHeading(heading);
    pDialog->SetLine(1, 24070);
    pDialog->SetLine(2, 24071);
    pDialog->SetLine(3, m_message);

    //send message and wait for user input
    ThreadMessage tMsg = {TMSG_DIALOG_DOMODAL, WINDOW_DIALOG_OK, g_windowManager.GetActiveWindow()};
    g_application.getApplicationMessenger().SendMessage(tMsg, true);
  }
}


/**********************************************************
 * CAddonMgr
 *
 */

CAddonMgr* CAddonMgr::m_pInstance = NULL;
std::map<TYPE, IAddonMgrCallback*> CAddonMgr::m_managers;

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

bool CAddonMgr::HasAddons(const TYPE &type, const CONTENT_TYPE &content/*= CONTENT_NONE*/)
{
  if (m_addons.empty())
  {
    VECADDONS add;
    GetAllAddons(add,false);
  }

  if (content == CONTENT_NONE)
    return (m_addons.find(type) != m_addons.end());

  VECADDONS addons;
  return GetAddons(type, addons, content, true);
}

void CAddonMgr::UpdateRepos()
{
  m_downloads.push_back(g_DownloadManager.RequestFile(ADDON_XBMC_REPO_URL, this));
}

bool CAddonMgr::ParseRepoXML(const CStdString &path)
{
  //TODO
  //check file exists, for each addoninfo, create an AddonProps struct, store in m_remoteAddons
  return false;
}

void CAddonMgr::OnFileComplete(TICKET aTicket, CStdString& aFilePath, INT aByteRxCount, Result aResult)
{
  for (unsigned i=0; i < m_downloads.size(); i++)
  {
    if (m_downloads[i].wQueueId == aTicket.wQueueId
        && m_downloads[i].dwItemId == aTicket.dwItemId)
    {
      CLog::Log(LOGINFO, "ADDONS: Downloaded addons.xml");
      ParseRepoXML(aFilePath);
    }
  }
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
  // recheck addons.xml & each addontype's directories no more than once every ADDON_DIRSCAN_FREQ seconds
  CDateTimeSpan span;
  span.SetDateTimeSpan(0, 0, 0, ADDON_DIRSCAN_FREQ);
  if(!m_lastDirScan.IsValid() || (m_lastDirScan + span) < CDateTime::GetCurrentDateTime())
  {
    m_lastDirScan = CDateTime::GetCurrentDateTime();
    LoadAddonsXML();
  }

  addons.clear();
  if (m_addons.find(type) != m_addons.end())
  {
    IVECADDONS itr = m_addons[type].begin();
    while (itr != m_addons[type].end())
    { // filter out what we're not looking for
      if ((enabledOnly && (*itr)->Disabled())
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

bool CAddonMgr::GetAddon(const CStdString &str, AddonPtr &addon, const TYPE &type/*=ADDON_UNKNOWN*/)
{
  CDateTimeSpan span;
  span.SetDateTimeSpan(0, 0, 0, ADDON_DIRSCAN_FREQ);
  if(!m_lastDirScan.IsValid() || (m_lastDirScan + span) < CDateTime::GetCurrentDateTime())
  {
    m_lastDirScan = CDateTime::GetCurrentDateTime();
    LoadAddonsXML();
  }

  if (type != ADDON_UNKNOWN && m_addons.find(type) == m_addons.end())
    return false;

  if (m_idMap[str])
  {
    addon = m_idMap[str];
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

bool CAddonMgr::EnableAddon(const CStdString &id)
{
  AddonPtr addon = m_idMap[id];
  if (!addon)
  {
    CLog::Log(LOGINFO,"ADDON: Couldn't find Add-on to Enable: %s", id.c_str());
    return false;
  }

  return EnableAddon(addon);
}

bool CAddonMgr::EnableAddon(AddonPtr &addon)
{
  CUtil::CreateDirectoryEx(addon->Profile());
  addon->Enable();
  CLog::Log(LOGINFO,"ADDON: Enabled %s: %s : %s", TranslateType(addon->Type()).c_str(), addon->Name().c_str(), addon->Version().Print().c_str());
  SaveAddonsXML();
  return true;
}

bool CAddonMgr::DisableAddon(const CStdString &id)
{
  AddonPtr addon = m_idMap[id];
  if (!addon)
    return false;
  return DisableAddon(addon);
}

bool CAddonMgr::DisableAddon(AddonPtr &addon)
{
  const TYPE type = addon->Type();

  if (m_addons.find(type) == m_addons.end())
    return false;

  for (IVECADDONS itr = m_addons[type].begin(); itr != m_addons[type].end(); itr++)
  {
    if (addon == (*itr))
    {
      addon->Disable();

      if (addon->Parent())
      { // we can delete this cloned addon
        m_addons[type].erase(itr);
      }

      CLog::Log(LOGINFO,"ADDON: Disabled %s: %s", TranslateType(addon->Type()).c_str(), addon->Name().c_str());
      SaveAddonsXML();
      return true;
    }
  }
  CLog::Log(LOGINFO,"ADDON: Couldn't find Add-on to Disable: %s", addon->Name().c_str());
  return false;
}

bool CAddonMgr::LoadAddonsXML()
{
  VECADDONPROPS props;
  if (!LoadAddonsXML(props))
    return false;

  // refresh addon dirs if neccesary/forced
  FindAddons();

  // now enable accordingly
  VECADDONPROPS::const_iterator itr = props.begin();
  while (itr != props.end())
  {
    AddonPtr addon;
    if (itr->parent.empty() && GetAddon(itr->id, addon, itr->type))
    {
      EnableAddon(addon);
    }
    else if (GetAddon(itr->parent, addon))
    { // multiple addon configurations
      AddonPtr clone = addon->Clone(addon);
      if (clone)
      {
        m_addons[addon->Type()].push_back(clone);
      }
    }
    else
    { // addon not found
      CLog::Log(LOGERROR, "ADDON: Couldn't find addon requested with ID: %s", itr->id.c_str());
      //TODO we should really add but mark unavailable, to prompt user
    }
    ++itr;
  }
  return true;
}

void CAddonMgr::FindAddons()
{
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

    // read description.xml and populate the addon
    AddonPtr addon;
    if (!AddonFromInfoXML(item->m_strPath, addon))
    {
      CLog::Log(LOGDEBUG, "ADDON: Error reading %sdescription.xml, bypassing package", item->m_strPath.c_str());
      continue;
    }

    // refuse to store addons with missing library
    CStdString library(addon->Path());
    CUtil::AddFileToFolder(library, addon->LibName(), library);
    if (!CFile::Exists(library))
    {
      CLog::Log(LOGDEBUG, "ADDON: Missing library file %s, bypassing package", library.c_str());
      continue;
    }

    // check for/cache icon thumbnail
    //TODO cache one thumb per addon id instead
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
        m_idMap.insert(std::make_pair(addon->ID(), addon));
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
        m_idMap.insert(std::make_pair(addon->ID(), addon));
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
        m_idMap.insert(std::make_pair(addon->ID(), addon));
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
    for (unsigned i=0; i < m_remoteAddons.size(); i++)
    {
      if (m_remoteAddons[i].id == id)
      {
        if(m_remoteAddons[i].version >= min && m_remoteAddons[i].version <= max)
        {
          //TODO line up download
          return false;
        }
      }
    }
    itr++;
  }
  return deps.empty();
}

bool CAddonMgr::AddonFromInfoXML(const CStdString &path, AddonPtr &addon)
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
  element = NULL;
  element = xmlDoc.RootElement()->FirstChildElement("id");
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
  element = NULL;
  element = xmlDoc.RootElement()->FirstChildElement("type");
  if (!element)
  {
    CLog::Log(LOGERROR, "ADDON: %s missing <id> element, ignoring", strPath.c_str());
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
  element = NULL;
  element = xmlDoc.RootElement()->FirstChildElement("title");
  if (!element)
  {
    CLog::Log(LOGERROR, "ADDON: %s missing <title> element, ignoring", strPath.c_str());
    return false;
  }
  name = element->GetText();

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

  /* Path, ID & Version are valid */
  AddonProps addonProps(id, type, version);
  addonProps.name = name;
  addonProps.path = path;
  addonProps.icon = CUtil::AddFileToFolder(path, "default.tbn");

  /* Retrieve license */
  element = NULL;
  element = xmlDoc.RootElement()->FirstChildElement("license");
/*  if (!element)
  {
    CLog::Log(LOGERROR, "ADDON: %s missing <license> element, ignoring", strPath.c_str());
    return false;
  }
  addonProps.license = element->GetText();*/

  /* Retrieve platforms which this addon supports */
  CStdString platform;
  element = NULL;
  element = xmlDoc.RootElement()->FirstChildElement("platforms")->FirstChildElement("platform");
  if (!element)
  {
    CLog::Log(LOGERROR, "ADDON: %s missing <platforms> element, ignoring", strPath.c_str());
    return false;
  }

  bool all(false);
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

  /* Retrieve dependencies that this addon requires */
  std::map<CStdString, std::pair<const AddonVersion, const AddonVersion> > deps;
  element = NULL;
  element = xmlDoc.RootElement()->FirstChildElement("dependencies");
  if (element)
  {
    element = element->FirstChildElement("dependency");
    if (!element)
      CLog::Log(LOGDEBUG, "ADDON: %s missing at least one <dependency> element, will ignore this dependency", strPath.c_str());
    else
    {
      do
      {
        CStdString min = element->Attribute("minversion");
        CStdString max = element->Attribute("maxversion");
        CStdString id = element->GetText();
        if (!id || (!min && ! max))
        {
          CLog::Log(LOGDEBUG, "ADDON: %s malformed <dependency> element, will ignore this dependency", strPath.c_str());
          continue;
        }
        deps.insert(std::make_pair(id, std::make_pair(AddonVersion(min), AddonVersion(max))));
        element = element->NextSiblingElement("dependency");
      } while (element != NULL);
    }
  }

  /*** end of optional fields ***/

  /* Create an addon object and store in a shared_ptr */
  addon.reset();
  switch (type)
  {
    case ADDON_PLUGIN:
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
    case ADDON_SCRAPER_LIBRARY:
    case ADDON_VIZ_LIBRARY:
    {
      AddonPtr temp(new CAddonLibrary(addonProps));
      addon = temp;
      break;
    }
    default:
      return false;
  }

  addon->SetDeps(deps);
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

bool CAddonMgr::SaveAddonsXML()
{
  //TODO lock
  if (m_idMap.empty())
    return true;

  TiXmlDocument doc;
  TiXmlNode *pRoot = NULL;
  TiXmlElement xmlRootElement("addons");
  pRoot = doc.InsertEndChild(xmlRootElement);

  std::map<CStdString, AddonPtr>::iterator itr = m_idMap.begin();
  while (itr != m_idMap.end())
  {
    AddonPtr addon = (*itr).second;
    if (addon && !addon->Disabled())
    {
      TYPE type = addon->Type();
      CStdString strType = TranslateType(type);
      TiXmlElement sectionElement(strType);
      TiXmlNode *node = pRoot->FirstChild(strType);
      if (!node)
        node = pRoot->InsertEndChild(sectionElement);

      TiXmlElement element("addon");
      XMLUtils::SetString(&element, "id", addon->ID());
      if (addon->Parent())
        XMLUtils::SetString(&element, "parentid", addon->Parent()->ID());
      //XMLUtils::SetString(&element, "repo", addon->Repo()->ID());
      node->InsertEndChild(element);
    }
    itr++;
  }
  return doc.SaveFile(GetAddonsXMLFile());
}

bool CAddonMgr::LoadAddonsXML(VECADDONPROPS &addons)
{
  CStdString strXMLFile;
  TiXmlDocument xmlDoc;
  TiXmlElement *pRootElement = NULL;
  strXMLFile = GetAddonsXMLFile();
  if ( xmlDoc.LoadFile( strXMLFile ) )
  {
    pRootElement = xmlDoc.RootElement();
    CStdString strValue;
    if (pRootElement)
      strValue = pRootElement->Value();
    if ( strValue != "addons")
    {
      CLog::Log(LOGDEBUG, "ADDONS: %s does not contain <addons> element", strXMLFile.c_str());
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
    GetAddons(pRootElement, addons);
    return true;
  }

  return false;
}

void CAddonMgr::GetAddons(const TiXmlElement* pAddons, VECADDONPROPS &addons)
{

  const TiXmlNode *pType = 0;
  while( pType = pAddons->IterateChildren( pType ) )
  {
    TYPE type = TranslateType(pType->Value());
    const TiXmlNode *pAddon = pType->FirstChild();
    while (pAddon > 0)
    {
      CStdString strValue = pAddon->Value();
      if (strValue == "addon")
      {
        GetAddon(type, pAddon, addons);
      }
      pAddon = pAddon->NextSibling();
    }
  }
}

bool CAddonMgr::GetAddon(const TYPE &type, const TiXmlNode *node, VECADDONPROPS &addons)
{
  // id
  const TiXmlNode *pNodeID = node->FirstChild("id");
  CStdString id;
  if (pNodeID && pNodeID->FirstChild())
  {
    id = pNodeID->FirstChild()->Value();
  }
  else
    return false;

  // will grab the version from description.xml
  CStdString version;
  AddonProps props(id, type, version);

  // parent id if present
  const TiXmlNode *pNodeParent = node->FirstChild("parentid");
  if (pNodeParent && pNodeParent->FirstChild())
  {
    props.parent = pNodeParent->FirstChild()->Value();
  }

  addons.insert(addons.end(), props);
  return true;
}

} /* namespace ADDON */

