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

#include "RssManager.h"

#include <utility>

#include "addons/AddonInstaller.h"
#include "addons/AddonManager.h"
#include "filesystem/File.h"
#include "interfaces/builtins/Builtins.h"
#include "messaging/ApplicationMessenger.h"
#include "messaging/helpers/DialogHelper.h"
#include "profiles/ProfilesManager.h"
#include "settings/lib/Setting.h"
#include "settings/Settings.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "utils/RssReader.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"

using namespace XFILE;
using namespace KODI::MESSAGING;

using KODI::MESSAGING::HELPERS::DialogResponse;

CRssManager::CRssManager()
{
  m_bActive = false;
}

CRssManager::~CRssManager()
{
  Stop();
}

CRssManager& CRssManager::GetInstance()
{
  static CRssManager sRssManager;
  return sRssManager;
}

void CRssManager::OnSettingsLoaded()
{
  Load();
}

void CRssManager::OnSettingsUnloaded()
{
  Clear();
}

void CRssManager::OnSettingAction(const CSetting *setting)
{
  if (setting == NULL)
    return;

  const std::string &settingId = setting->GetId();
  if (settingId == CSettings::SETTING_LOOKANDFEEL_RSSEDIT)
  {
    ADDON::AddonPtr addon;
    if (!ADDON::CAddonMgr::GetInstance().GetAddon("script.rss.editor", addon))
    {
      if (!CAddonInstaller::GetInstance().InstallModal("script.rss.editor", addon))
        return;
    }
    CBuiltins::GetInstance().Execute("RunScript(script.rss.editor)");
  }
}

void CRssManager::Start()
 {
   m_bActive = true;
}

void CRssManager::Stop()
{
  CSingleLock lock(m_critical);
  m_bActive = false;
  for (unsigned int i = 0; i < m_readers.size(); i++)
  {
    if (m_readers[i].reader)
      delete m_readers[i].reader;
  }
  m_readers.clear();
}

bool CRssManager::Load()
{
  CSingleLock lock(m_critical);
  std::string rssXML = CProfilesManager::GetInstance().GetUserDataItem("RssFeeds.xml");
  if (!CFile::Exists(rssXML))
    return false;

  CXBMCTinyXML rssDoc;
  if (!rssDoc.LoadFile(rssXML))
  {
    CLog::Log(LOGERROR, "CRssManager: error loading %s, Line %d\n%s", rssXML.c_str(), rssDoc.ErrorRow(), rssDoc.ErrorDesc());
    return false;
  }

  const TiXmlElement *pRootElement = rssDoc.RootElement();
  if (pRootElement == NULL || !StringUtils::EqualsNoCase(pRootElement->ValueStr(), "rssfeeds"))
  {
    CLog::Log(LOGERROR, "CRssManager: error loading %s, no <rssfeeds> node", rssXML.c_str());
    return false;
  }

  m_mapRssUrls.clear();
  const TiXmlElement* pSet = pRootElement->FirstChildElement("set");
  while (pSet != NULL)
  {
    int iId;
    if (pSet->QueryIntAttribute("id", &iId) == TIXML_SUCCESS)
    {
      RssSet set;
      set.rtl = pSet->Attribute("rtl") != NULL && strcasecmp(pSet->Attribute("rtl"), "true") == 0;
      const TiXmlElement* pFeed = pSet->FirstChildElement("feed");
      while (pFeed != NULL)
      {
        int iInterval;
        if (pFeed->QueryIntAttribute("updateinterval", &iInterval) != TIXML_SUCCESS)
        {
          iInterval = 30; // default to 30 min
          CLog::Log(LOGDEBUG, "CRssManager: no interval set, default to 30!");
        }

        if (pFeed->FirstChild() != NULL)
        {
          //! @todo UTF-8: Do these URLs need to be converted to UTF-8?
          //!              What about the xml encoding?
          std::string strUrl = pFeed->FirstChild()->ValueStr();
          set.url.push_back(strUrl);
          set.interval.push_back(iInterval);
        }
        pFeed = pFeed->NextSiblingElement("feed");
      }

      m_mapRssUrls.insert(std::make_pair(iId,set));
    }
    else
      CLog::Log(LOGERROR, "CRssManager: found rss url set with no id in RssFeeds.xml, ignored");

    pSet = pSet->NextSiblingElement("set");
  }

  return true;
}

bool CRssManager::Reload()
{
  Stop();
  if (!Load())
    return false;
  Start();

  return true;
}

void CRssManager::Clear()
{
  CSingleLock lock(m_critical);
  m_mapRssUrls.clear();
}

// returns true if the reader doesn't need creating, false otherwise
bool CRssManager::GetReader(int controlID, int windowID, IRssObserver* observer, CRssReader *&reader)
{
  CSingleLock lock(m_critical);
  // check to see if we've already created this reader
  for (unsigned int i = 0; i < m_readers.size(); i++)
  {
    if (m_readers[i].controlID == controlID && m_readers[i].windowID == windowID)
    {
      reader = m_readers[i].reader;
      reader->SetObserver(observer);
      reader->UpdateObserver();
      return true;
    }
  }
  // need to create a new one
  READERCONTROL readerControl;
  readerControl.controlID = controlID;
  readerControl.windowID = windowID;
  reader = readerControl.reader = new CRssReader;
  m_readers.push_back(readerControl);
  return false;
}
