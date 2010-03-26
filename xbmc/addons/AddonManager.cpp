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
#include "StringUtils.h"
#include "RegExp.h"
#include "XMLUtils.h"
#include "FileItem.h"
#include "Settings.h"
#include "GUISettings.h"
#include "DownloadQueueManager.h"
//#include "AdvancedSettings.h"
#include "DllLibCPluff.h"
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

namespace ADDON
{


/**********************************************************
 * CAddonMgr
 *
 */
int cp_to_clog(cp_log_severity_t lvl);

CAddonMgr* CAddonMgr::m_pInstance = NULL;
std::map<TYPE, IAddonMgrCallback*> CAddonMgr::m_managers;

CAddonMgr::CAddonMgr()
{
}

CAddonMgr::~CAddonMgr()
{
  if(m_cpluff)
    m_cpluff->destroy();
}

CAddonMgr* CAddonMgr::Get()
{
  if (!m_pInstance)
  {
    m_pInstance = new CAddonMgr();
    m_pInstance->OnInit();
  }
  return m_pInstance;
}

void CAddonMgr::OnInit()
{
  m_cpluff = new DllLibCPluff;
  m_cpluff->Load();

  if (!m_cpluff->IsLoaded())
    assert(false);

  cp_status_t status;
  setlocale(LC_ALL, ""); //FIXME where should this be handled?
  cp_log_severity_t log;
  if (g_advancedSettings.m_logLevel >= LOG_LEVEL_DEBUG_SAMBA)
    log = CP_LOG_DEBUG;
  else if (g_advancedSettings.m_logLevel >= LOG_LEVEL_DEBUG)
    log = CP_LOG_INFO;
  else
    log = CP_LOG_WARNING;

  m_cpluff->set_fatal_error_handler(cp_fatalErrorHandler);
  status = m_cpluff->init();
  if (status != CP_OK)
  {
    CLog::Log(LOGERROR, "ADDONS: Fatal Error, cp_init() returned status: %i", status);
    assert(false);
  }

  //TODO could separate addons into different contexts 
  // would allow partial unloading of addon framework
  m_cp_context = m_cpluff->create_context(&status);
  assert(m_cp_context);

  status = m_cpluff->register_pcollection(m_cp_context, "/home/alasdair/code/git-xbmc/addons");
  assert(status == CP_OK);
  status = m_cpluff->register_logger(m_cp_context, cp_logger, &CAddonMgr::m_pInstance, CP_LOG_INFO);
  assert(status == CP_OK);
  status = m_cpluff->scan_plugins(m_cp_context, 0);
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

bool CAddonMgr::HasAddons(const TYPE &type, const CONTENT_TYPE &content/*= CONTENT_NONE*/, bool enabledOnly/*= true*/)
{
  if (type == ADDON_VFSDLL)
    return true;

  VECADDONS addons;
  return GetAddons(type, addons, content, enabledOnly);
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
  return true;
}

bool CAddonMgr::GetAddons(const TYPE &type, VECADDONS &addons, const CONTENT_TYPE &content/*= CONTENT_NONE*/, bool enabledOnly/*= true*/)
{
  // recheck addons.xml & each addontype's directories no more than once every ADDON_DIRSCAN_FREQ seconds
  CDateTimeSpan span;
  span.SetDateTimeSpan(0, 0, 0, ADDON_DIRSCAN_FREQ);
  if(!m_lastDirScan.IsValid() || (m_lastDirScan + span) < CDateTime::GetCurrentDateTime())
  {
    m_lastDirScan = CDateTime::GetCurrentDateTime();
    cp_status_t status = m_cpluff->scan_plugins(m_cp_context, 0);
    if (status != CP_OK)
      CLog::Log(LOGERROR, "ADDON: CPluff scan_plugins() failed");
  }

  return GetExtensions(type, addons, content);
}

bool CAddonMgr::GetAddon(const CStdString &str, AddonPtr &addon, const TYPE &type/*=ADDON_UNKNOWN*/, bool enabledOnly/*= true*/)
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
    if(enabledOnly)
      return !addon->Disabled();
    else
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
      if(enabledOnly)
        return !addon->Disabled();
      else
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

bool CAddonMgr::LoadAddonsXML()
{
  // NB. as addons are enabled by default, all this now checks for is
  // cloned non-scraper addons
  // i.e pvr clients only
  VECADDONPROPS props;
  if (!LoadAddonsXML(props))
    return false;

  // now enable accordingly
  VECADDONPROPS::const_iterator itr = props.begin();
  while (itr != props.end())
  {
    if (itr->parent.size())
    {
      AddonPtr addon;
      if (GetAddon(itr->parent, addon, itr->type, false))
      { // multiple addon configurations
        AddonPtr clone = addon->Clone(addon);
        if (clone)
        {
          m_addons[addon->Type()].push_back(clone);
        }
      }
      else
      { // addon not found
        CLog::Log(LOGERROR, "ADDON: Couldn't find addon to clone with requested with ID: %s", itr->parent.c_str());
        //TODO we should really add but mark unavailable, to prompt user
      }
    }
    ++itr;
  }
  return true;
}

CStdString CAddonMgr::GetAddonsXMLFile() const
{
  CStdString folder;
  if (g_settings.GetCurrentProfile().hasAddons())
    CUtil::AddFileToFolder(g_settings.GetProfileUserDataFolder(),"addons.xml",folder);
  else
    CUtil::AddFileToFolder(g_settings.GetUserDataFolder(),"addons.xml",folder);

  return folder;
}

bool CAddonMgr::SaveAddonsXML()
{
  // NB only saves cloned non-scraper addons
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
    if (addon && addon->Parent())
    {
      TYPE type = addon->Type();
      CStdString strType = TranslateType(type);
      TiXmlElement sectionElement(strType);
      TiXmlNode *node = pRoot->FirstChild(strType);
      if (!node)
        node = pRoot->InsertEndChild(sectionElement);

      TiXmlElement element("addon");
      XMLUtils::SetString(&element, "id", addon->ID());
      XMLUtils::SetString(&element, "parentid", addon->Parent()->ID());
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
  while( ( pType = pAddons->IterateChildren( pType ) ) )
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

  // parent id
  const TiXmlNode *pNodeParent = node->FirstChild("parentid");
  if (pNodeParent && pNodeParent->FirstChild())
  {
    props.parent = pNodeParent->FirstChild()->Value();
    addons.insert(addons.end(), props);
    return true;
  }
  return false;
}

/*
 * libcpluff interaction
 */

void CAddonMgr::CPluffFatalError(const char *msg)
{
  CLog::Log(LOGERROR, "ADDONS: CPluffFatalError(%s)", msg);
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


void CAddonMgr::CPluffLog(cp_log_severity_t level, const char *msg, const char *apid, void *user_data)
{
  if(!apid)
    CLog::Log(LOGDEBUG, "ADDON: '%s'", msg);
  else
    CLog::Log(LOGDEBUG, "ADDON: '%s' reports '%s'", apid, msg);
}

bool CAddonMgr::GetExtensions(const TYPE &type, VECADDONS &addons, const CONTENT_TYPE &content)
{
  cp_status_t status;
  int num;
  CStdString ext_point(TranslateType(type));
  cp_extension_t **exts = m_cpluff->get_extensions_info(m_cp_context, ext_point.c_str(), &status, &num); 
  for(int i=0; i <num; i++)
  {
    CStdString id(exts[i]->plugin->identifier);
    CStdString version(exts[i]->plugin->version);
    AddonProps props(id, type, version);
    props.name = CStdString(exts[i]->name);
    props.summary = CStdString(exts[i]->plugin->summary);
    props.path = CStdString(exts[i]->plugin->plugin_path);
    props.icon = props.path + "/default.tbn"; //TODO store icons per ID
    addons.push_back(AddonFactory(type, props));
  }
  m_cpluff->release_info(m_cp_context, exts);
  return addons.size();
}

AddonPtr AddonFactory(const AddonProps &props)
{
  return AddonPtr(new T(props));
}

} /* namespace ADDON */

