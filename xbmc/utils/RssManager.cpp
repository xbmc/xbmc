/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RssManager.h"

#include "ServiceBroker.h"
#include "addons/AddonInstaller.h"
#include "addons/AddonManager.h"
#include "addons/addoninfo/AddonType.h"
#include "interfaces/builtins/Builtins.h"
#include "profiles/ProfileManager.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "utils/FileUtils.h"
#include "utils/RssReader.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML2.h"
#include "utils/log.h"

#include <mutex>
#include <utility>

using namespace KODI::MESSAGING;


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

void CRssManager::OnSettingAction(const std::shared_ptr<const CSetting>& setting)
{
  if (setting == NULL)
    return;

  const std::string &settingId = setting->GetId();
  if (settingId == CSettings::SETTING_LOOKANDFEEL_RSSEDIT)
  {
    ADDON::AddonPtr addon;
    if (!CServiceBroker::GetAddonMgr().GetAddon("script.rss.editor", addon,
                                                ADDON::OnlyEnabled::CHOICE_YES))
    {
      if (!ADDON::CAddonInstaller::GetInstance().InstallModal(
              "script.rss.editor", addon, ADDON::InstallModalPrompt::CHOICE_YES))
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
  std::unique_lock<CCriticalSection> lock(m_critical);
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
  const std::shared_ptr<CProfileManager> profileManager = CServiceBroker::GetSettingsComponent()->GetProfileManager();

  std::unique_lock<CCriticalSection> lock(m_critical);

  std::string rssXML = profileManager->GetUserDataItem("RssFeeds.xml");
  if (!CFileUtils::Exists(rssXML))
    return false;

  CXBMCTinyXML2 rssDoc;
  if (!rssDoc.LoadFile(rssXML))
  {
    CLog::Log(LOGERROR, "CRssManager: error loading {}, Line {}\n{}", rssXML, rssDoc.ErrorLineNum(),
              rssDoc.ErrorStr());
    return false;
  }

  auto* rootElement = rssDoc.RootElement();
  if (!rootElement || !StringUtils::EqualsNoCase(rootElement->Value(), "rssfeeds"))
  {
    CLog::Log(LOGERROR, "CRssManager: error loading {}, no <rssfeeds> node", rssXML);
    return false;
  }

  m_mapRssUrls.clear();
  auto* setElement = rootElement->FirstChildElement("set");
  while (setElement)
  {
    int iId;
    if (setElement->QueryIntAttribute("id", &iId) == tinyxml2::XML_SUCCESS)
    {
      RssSet set;
      set.rtl = setElement->Attribute("rtl") != nullptr &&
                StringUtils::CompareNoCase(setElement->Attribute("rtl"), "true") == 0;
      auto* feedElement = setElement->FirstChildElement("feed");
      while (feedElement)
      {
        int iInterval = 30; // default to 30 min
        if (feedElement->QueryIntAttribute("updateinterval", &iInterval) != tinyxml2::XML_SUCCESS)
        {
          CLog::Log(LOGDEBUG, "CRssManager: no interval set, default to 30!");
        }

        if (feedElement->FirstChild())
        {
          std::string strUrl = feedElement->FirstChild()->Value();
          set.url.push_back(strUrl);
          set.interval.push_back(iInterval);
        }
        feedElement = feedElement->NextSiblingElement("feed");
      }

      m_mapRssUrls.insert(std::make_pair(iId,set));
    }
    else
    {
      CLog::Log(LOGERROR, "CRssManager: found rss url set with no id in RssFeeds.xml, ignored");
    }

    setElement = setElement->NextSiblingElement("set");
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
  std::unique_lock<CCriticalSection> lock(m_critical);
  m_mapRssUrls.clear();
}

// returns true if the reader doesn't need creating, false otherwise
bool CRssManager::GetReader(int controlID, int windowID, IRssObserver* observer, CRssReader *&reader)
{
  std::unique_lock<CCriticalSection> lock(m_critical);
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
