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

  CLog::Log(LOGINFO, "Called Add-on status handler for '%u' of clientName:%s, clientGUID:%s (same Thread=%s)", status, m_addon->Name().c_str(), m_addon->UUID().c_str(), sameThread ? "yes" : "no");

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
    CAddonMgr::Get()->DisableAddon(m_addon->UUID());
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
  if(!m_lastDirScan[type].IsValid() || (m_lastDirScan[type] + span) < CDateTime::GetCurrentDateTime())
  {
    m_lastDirScan[type] = CDateTime::GetCurrentDateTime();
    LoadAddonsXML(type);
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

bool CAddonMgr::GetAddon(const TYPE &type, const CStdString &str, AddonPtr &addon)
{
  CDateTimeSpan span;
  span.SetDateTimeSpan(0, 0, 0, ADDON_DIRSCAN_FREQ);
  if(!m_lastDirScan[type].IsValid() || (m_lastDirScan[type] + span) < CDateTime::GetCurrentDateTime())
  {
    m_lastDirScan[type] = CDateTime::GetCurrentDateTime();
    LoadAddonsXML(type);
  }

  if (m_addons.find(type) == m_addons.end())
    return false;

  bool isUUID = StringUtils::ValidateUUID(str);

  VECADDONS &addons = m_addons[type];
  IVECADDONS adnItr = addons.begin();
  while (adnItr != addons.end())
  {
    //FIXME scrapers were previously registered by filename
    if ( (isUUID && (*adnItr)->UUID() == str)
      || (!isUUID && (*adnItr)->Name() == str)
      || (type == ADDON_SCRAPER && (*adnItr)->LibName() == str))
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
  return GetAddon(type, setting, addon);
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
  {
    CLog::Log(LOGINFO,"ADDON: Couldn't find Add-on to Enable: %s", uuid.c_str());
    return false;
  }

  return EnableAddon(addon);
}

bool CAddonMgr::EnableAddon(AddonPtr &addon)
{
  CUtil::CreateDirectoryEx(addon->Profile());
  addon->Enable();
  CLog::Log(LOGINFO,"ADDON: Enabled %s: %s : %s", TranslateType(addon->Type()).c_str(), addon->Name().c_str(), addon->Version().Print().c_str());
  SaveAddonsXML(addon->Type());
  return true;
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
      SaveAddonsXML(type);
      return true;
    }
  }
  CLog::Log(LOGINFO,"ADDON: Couldn't find Add-on to Disable: %s", addon->Name().c_str());
  return false;
}

bool CAddonMgr::LoadAddonsXML(const TYPE &type)
{
  VECADDONPROPS props;
  if (!LoadAddonsXML(type, props))
    return false;

  // refresh addon dirs if neccesary/forced
  FindAddons(type);

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
      AddonPtr clone = addon->Clone(addon);
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

bool CAddonMgr::GetAddonProps(const TYPE &type, VECADDONPROPS &props)
{
  props.clear();
  VECADDONS addons;
  bool found = GetAddons(type, addons);
  if (found)
  {
    IVECADDONS itr = addons.begin();
    while (itr != addons.end())
    {
      AddonProps addon(*itr);
      props.push_back(addon);
      ++itr;
    }
  }
  return found;
}

void CAddonMgr::FindAddons(const TYPE &type)
{
  // parse the user & system dirs for addons of the requested type
  CFileItemList items;
  bool isHome = CSpecialProtocol::XBMCIsHome();

  // store any addons with unresolved deps, then recheck at the end
  VECADDONS unresolved;

  switch (type)
  {
  case ADDON_VIZ:
    { //TODO fix mvis handling
      if (!isHome)
        CDirectory::GetDirectory("special://home/addons/visualizations", items, ADDON_VIS_EXT, false);
      CDirectory::GetDirectory("special://xbmc/addons/visualizations", items, ADDON_VIS_EXT, false);
      break;
    }
  case ADDON_SCREENSAVER:
    {
      if (!isHome)
        CDirectory::GetDirectory("special://home/addons/screensavers", items, ADDON_SCREENSAVER_EXT, false);
      CDirectory::GetDirectory("special://xbmc/addons/screensavers", items, ADDON_SCREENSAVER_EXT, false);
      break;
    }
  case ADDON_SCRAPER:
    {
      if (!isHome)
        CDirectory::GetDirectory("special://home/addons/scrapers", items, ADDON_SCRAPER_EXT, false);
      CDirectory::GetDirectory("special://xbmc/addons/scrapers", items, ADDON_SCRAPER_EXT, false);
      break;
    }
  case ADDON_SCRAPER_LIBRARY:
    {
      if (!isHome)
        CDirectory::GetDirectory("special://home/addons/libraries/scrapers", items, ADDON_SCRAPER_EXT, false);
      CDirectory::GetDirectory("special://xbmc/addons/libraries/scrapers", items, ADDON_SCRAPER_EXT, false);
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

  //FIXME this only checks library dependencies - multiple addon type deps
  //need a more sophisticated approach
  if (type == ADDON_SCRAPER)
    FindAddons(ADDON_SCRAPER_LIBRARY);
  else if (type == ADDON_VIZ)
    FindAddons(ADDON_VIZ_LIBRARY);

  // for all folders found
  for (int i = 0; i < items.Size(); ++i)
  {
    CFileItemPtr item = items[i];

    // read description.xml and populate the addon
    AddonPtr addon;
    if (!AddonFromInfoXML(type, item->m_strPath, addon))
    {
      CLog::Log(LOGDEBUG, "ADDON: Error reading %sdescription.xml, bypassing package", item->m_strPath.c_str());
      continue;
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
        assert(addon->Type() == type);
        m_addons[type].push_back(addon);
        m_uuidMap.insert(std::make_pair(addon->UUID(), addon));
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
        m_addons[type].push_back(addon);
        m_uuidMap.insert(std::make_pair(addon->UUID(), addon));
      }
    }
  }
  CLog::Log(LOGINFO, "ADDON: Found %"PRIuS" addons of type %s", m_addons.find(type) == m_addons.end() ? 0: m_addons[type].size(), TranslateType(type).c_str());
}

bool CAddonMgr::UpdateIfKnown(AddonPtr &addon)
{
  if (m_addons.find(addon->Type()) != m_addons.end())
  {
    for (unsigned i = 0; i < m_addons[addon->Type()].size(); i++)
    {
      if (m_addons[addon->Type()][i]->UUID() == addon->UUID())
      {
        //TODO inform any manager first, and request removal
        //TODO choose most recent version if varying
        m_addons[addon->Type()][i] = addon;
        CStdString uuid = addon->UUID();
        m_uuidMap.erase(uuid);
        m_uuidMap.insert(std::make_pair(addon->UUID(), addon));
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
    CStdString uuid;
    uuid = (*itr).first;
    AddonVersion min = (*itr).second.first;
    AddonVersion max = (*itr).second.second;
    if (m_uuidMap.count(uuid))
    {
      AddonPtr dep = m_uuidMap[uuid];
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
      if (m_remoteAddons[i].uuid == uuid)
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

bool CAddonMgr::AddonFromInfoXML(const TYPE &reqType, const CStdString &path, AddonPtr &addon)
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
  * 4. a license is specified
  * 5. operating system matches ours
  * 6. summary exists
  * 7. for scrapers & plugins, support at least one type of content
  *
  * NOTE: addon dependencies are handled in ::FindAddons()
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
  TYPE type = TranslateType(element->GetText());
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

  /* Path, UUID & Version are valid */
  AddonProps addonProps(uuid, type, version);
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
        CStdString uuid = element->GetText();
        if (!uuid || (!min && ! max))
        {
          CLog::Log(LOGDEBUG, "ADDON: %s malformed <dependency> element, will ignore this dependency", strPath.c_str());
          continue;
        }
        deps.insert(std::make_pair(uuid, std::make_pair(AddonVersion(min), AddonVersion(max))));
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

bool CAddonMgr::SaveAddonsXML(const TYPE &type)
{
  VECADDONPROPS props;
  GetAddonProps(type, props);

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

bool CAddonMgr::SetAddons(TiXmlNode *root, const TYPE &type, const VECADDONPROPS &addons)
{
  CStdString strType;
  strType = TranslateType(type);

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
      XMLUtils::SetString(&element, "uuid", itr->uuid);
      if (!itr->parent.IsEmpty())
        XMLUtils::SetString(&element, "parentuuid", itr->parent);
      sectionNode->InsertEndChild(element);
      ++itr;
    }
  }
  return true;
}

bool CAddonMgr::LoadAddonsXML(const TYPE &type, VECADDONPROPS &addons)
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

void CAddonMgr::GetAddons(const TiXmlElement* pRootElement, const TYPE &type, VECADDONPROPS &addons)
{
  CStdString strTagName;
  strTagName = TranslateType(type);

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

bool CAddonMgr::GetAddon(const TYPE &type, const TiXmlNode *node, VECADDONPROPS &addons)
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

  // this AddonProps doesn't need a valid version
  CStdString version;
  AddonProps props(uuid, type, version);

  // name if present
  const TiXmlNode *pNodeName = node->FirstChild("name");
  CStdString strName;
  if (pNodeName && pNodeName->FirstChild())
  {
    props.name = pNodeName->FirstChild()->Value();
  }

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

  addons.insert(addons.end(), props);
  return true;
}

} /* namespace ADDON */

