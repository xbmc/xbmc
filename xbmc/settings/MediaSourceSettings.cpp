/*
 *  Copyright (C) 2013-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "MediaSourceSettings.h"

#include "ServiceBroker.h"
#include "URL.h"
#include "Util.h"
#include "media/MediaLockState.h"
#include "network/WakeOnAccess.h"
#include "profiles/ProfileManager.h"
#include "settings/SettingsComponent.h"
#include "utils/FileUtils.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/XBMCTinyXML2.h"
#include "utils/XMLUtils.h"
#include "utils/log.h"

#include <cstdlib>
#include <cstring>

namespace
{
constexpr const char* SOURCES_FILE = "sources.xml";
constexpr const char* XML_SOURCES = "sources";
constexpr const char* XML_SOURCE = "source";
} // unnamed namespace

CMediaSourceSettings::CMediaSourceSettings()
{
  Clear();
}

CMediaSourceSettings::~CMediaSourceSettings() = default;

CMediaSourceSettings& CMediaSourceSettings::GetInstance()
{
  static CMediaSourceSettings sMediaSourceSettings;
  return sMediaSourceSettings;
}

std::string CMediaSourceSettings::GetSourcesFile()
{
  const std::shared_ptr<CProfileManager> profileManager =
      CServiceBroker::GetSettingsComponent()->GetProfileManager();

  std::string file;
  if (profileManager->GetCurrentProfile().hasSources())
    file = profileManager->GetProfileUserDataFolder();
  else
    file = profileManager->GetUserDataFolder();

  return URIUtils::AddFileToFolder(file, SOURCES_FILE);
}

void CMediaSourceSettings::OnSettingsLoaded()
{
  Load();
}

void CMediaSourceSettings::OnSettingsUnloaded()
{
  Clear();
}

bool CMediaSourceSettings::Load()
{
  return Load(GetSourcesFile());
}

bool CMediaSourceSettings::Load(const std::string& file)
{
  Clear();

  if (!CFileUtils::Exists(file))
    return false;

  CLog::Log(LOGINFO, "CMediaSourceSettings: loading media sources from {}", file);

  // load xml file
  CXBMCTinyXML2 xmlDoc;
  if (!xmlDoc.LoadFile(file))
  {
    CLog::Log(LOGERROR, "CMediaSourceSettings: error loading {}: Line {}, {}", file,
              xmlDoc.ErrorLineNum(), xmlDoc.ErrorStr());
    return false;
  }

  const tinyxml2::XMLElement* rootElement = xmlDoc.RootElement();
  if (!rootElement || !StringUtils::EqualsNoCase(rootElement->Value(), XML_SOURCES))
    CLog::Log(LOGERROR, "CMediaSourceSettings: sources.xml file does not contain <sources>");

  // parse sources
  std::string dummy;
  GetSources(rootElement, "video", m_videoSources, dummy);
  GetSources(rootElement, "programs", m_programSources, m_defaultProgramSource);
  GetSources(rootElement, "pictures", m_pictureSources, m_defaultPictureSource);
  GetSources(rootElement, "files", m_fileSources, m_defaultFileSource);
  GetSources(rootElement, "music", m_musicSources, m_defaultMusicSource);
  GetSources(rootElement, "games", m_gameSources, dummy);

  return true;
}

bool CMediaSourceSettings::Save() const
{
  return Save(GetSourcesFile());
}

bool CMediaSourceSettings::Save(const std::string& file) const
{
  CXBMCTinyXML2 doc;
  tinyxml2::XMLElement* element = doc.NewElement(XML_SOURCES);
  tinyxml2::XMLNode* rootNode = doc.InsertFirstChild(element);

  if (!rootNode)
    return false;

  // ok, now run through and save each sources section
  SetSources(rootNode, "programs", m_programSources, m_defaultProgramSource);
  SetSources(rootNode, "video", m_videoSources, "");
  SetSources(rootNode, "music", m_musicSources, m_defaultMusicSource);
  SetSources(rootNode, "pictures", m_pictureSources, m_defaultPictureSource);
  SetSources(rootNode, "files", m_fileSources, m_defaultFileSource);
  SetSources(rootNode, "games", m_gameSources, "");

  CWakeOnAccess::GetInstance().QueueMACDiscoveryForAllRemotes();

  return doc.SaveFile(file);
}

void CMediaSourceSettings::Clear()
{
  m_programSources.clear();
  m_pictureSources.clear();
  m_fileSources.clear();
  m_musicSources.clear();
  m_videoSources.clear();
  m_gameSources.clear();
}

std::vector<CMediaSource>* CMediaSourceSettings::GetSources(std::string_view type)
{
  if (type == "programs" || type == "myprograms")
    return &m_programSources;
  else if (type == "files")
    return &m_fileSources;
  else if (type == "music")
    return &m_musicSources;
  else if (type == "video" || type == "videos")
    return &m_videoSources;
  else if (type == "pictures")
    return &m_pictureSources;
  else if (type == "games")
    return &m_gameSources;

  return nullptr;
}

const std::string& CMediaSourceSettings::GetDefaultSource(std::string_view type) const
{
  if (type == "programs" || type == "myprograms")
    return m_defaultProgramSource;
  else if (type == "files")
    return m_defaultFileSource;
  else if (type == "music")
    return m_defaultMusicSource;
  else if (type == "pictures")
    return m_defaultPictureSource;

  return StringUtils::Empty;
}

void CMediaSourceSettings::SetDefaultSource(std::string_view type, std::string_view source)
{
  if (type == "programs" || type == "myprograms")
    m_defaultProgramSource = source;
  else if (type == "files")
    m_defaultFileSource = source;
  else if (type == "music")
    m_defaultMusicSource = source;
  else if (type == "pictures")
    m_defaultPictureSource = source;
}

// NOTE: This function does NOT save the sources.xml file - you need to call SaveSources() separately.
bool CMediaSourceSettings::UpdateSource(std::string_view strType,
                                        std::string_view strOldName,
                                        std::string_view strUpdateChild,
                                        const std::string& strUpdateValue)
{
  std::vector<CMediaSource>* pShares = GetSources(strType);
  if (!pShares)
    return false;

  for (auto& share : *pShares)
  {
    if (share.strName == strOldName)
    {
      if (strUpdateChild == "name")
        share.strName = strUpdateValue;
      else if (strUpdateChild == "lockmode")
        share.GetLockInfo().SetMode(
            static_cast<LockMode>(std::strtol(strUpdateValue.c_str(), nullptr, 10)));
      else if (strUpdateChild == "lockcode")
        share.GetLockInfo().SetCode(strUpdateValue);
      else if (strUpdateChild == "badpwdcount")
        share.GetLockInfo().SetBadPasswordCount(
            static_cast<int>(std::strtol(strUpdateValue.c_str(), nullptr, 10)));
      else if (strUpdateChild == "thumbnail")
        share.m_strThumbnailImage = strUpdateValue;
      else if (strUpdateChild == "path")
      {
        share.vecPaths.clear();
        share.strPath = strUpdateValue;
        share.vecPaths.emplace_back(strUpdateValue);
      }
      else
        return false;

      return true;
    }
  }

  return false;
}

bool CMediaSourceSettings::DeleteSource(std::string_view strType,
                                        std::string_view strName,
                                        std::string_view strPath,
                                        bool virtualSource /* = false */)
{
  std::vector<CMediaSource>* pShares = GetSources(strType);
  if (!pShares)
    return false;

  bool found = false;

  for (auto it = pShares->begin(); it != pShares->end(); ++it)
  {
    if (it->strName == strName && it->strPath == strPath)
    {
      CLog::Log(LOGDEBUG, "CMediaSourceSettings: found share, removing!");
      pShares->erase(it);
      found = true;
      break;
    }
  }

  if (virtualSource)
    return found;

  return Save();
}

bool CMediaSourceSettings::AddShare(std::string_view type, const CMediaSource& share)
{
  std::vector<CMediaSource>* pShares = GetSources(type);
  if (!pShares)
    return false;

  // translate dir and add to our current shares
  std::string strPath1 = share.strPath;
  if (strPath1.empty())
  {
    CLog::Log(LOGERROR, "CMediaSourceSettings: unable to add empty path");
    return false;
  }
  StringUtils::ToUpper(strPath1);

  CMediaSource shareToAdd = share;
  if (strPath1.at(0) == '$')
  {
    shareToAdd.strPath = CUtil::TranslateSpecialSource(strPath1);
    if (!share.strPath.empty())
      CLog::Log(LOGDEBUG, "CMediaSourceSettings: translated ({}) to path ({})", strPath1,
                shareToAdd.strPath);
    else
    {
      CLog::Log(LOGDEBUG, "CMediaSourceSettings: skipping invalid special directory token ({})",
                strPath1);
      return false;
    }
  }
  pShares->push_back(shareToAdd);

  if (!share.m_ignore)
    return Save();

  return true;
}

bool CMediaSourceSettings::UpdateShare(std::string_view type,
                                       std::string_view oldName,
                                       const CMediaSource& share)
{
  std::vector<CMediaSource>* pShares = GetSources(type);
  if (!pShares)
    return false;

  // update our current share list
  const CMediaSource* pShare = nullptr;
  for (auto& currshare : *pShares)
  {
    if (currshare.strName == oldName)
    {
      currshare.strName = share.strName;
      currshare.strPath = share.strPath;
      currshare.vecPaths = share.vecPaths;
      pShare = &currshare;
      break;
    }
  }

  if (!pShare)
    return false;

  // Update our XML file as well
  return Save();
}

bool CMediaSourceSettings::GetSource(const std::string& category,
                                     const tinyxml2::XMLNode* source,
                                     CMediaSource& share) const
{
  const tinyxml2::XMLElement* nodeName = source->FirstChildElement("name");
  std::string name;
  if (nodeName && nodeName->FirstChild())
    name = nodeName->FirstChild()->Value();

  if (name.empty())
    return false;

  // get multiple paths
  std::vector<std::string> vecPaths;
  const tinyxml2::XMLElement* pathName = source->FirstChildElement("path");
  while (pathName)
  {
    if (pathName->FirstChild())
    {
      std::string path = pathName->FirstChild()->Value();

      // make sure there are no virtualpaths or stack paths defined in sources.xml
      if (!URIUtils::IsStack(path))
      {
        // translate special tags
        if (!path.empty() && path.at(0) == '$')
          path = CUtil::TranslateSpecialSource(path);

        // need to check path validity again as CUtil::TranslateSpecialSource() may have failed
        if (!path.empty())
        {
          URIUtils::AddSlashAtEnd(path);
          vecPaths.push_back(path);
        }
      }
      else
        CLog::Log(LOGERROR, "CMediaSourceSettings:    invalid path type ({}) in source", path);
    }

    pathName = pathName->NextSiblingElement("path");
  }

  if (vecPaths.empty())
    return false;

  const tinyxml2::XMLElement* lockModeElement = source->FirstChildElement("lockmode");
  const tinyxml2::XMLElement* lockCodeElement = source->FirstChildElement("lockcode");
  const tinyxml2::XMLElement* badPwdCountElement = source->FirstChildElement("badpwdcount");
  const tinyxml2::XMLElement* thumbnailNodeElement = source->FirstChildElement("thumbnail");

  std::vector<std::string> verifiedPaths;
  // disallowed for files, or there's only a single path in the vector
  if (StringUtils::EqualsNoCase(category, "files") || vecPaths.size() == 1)
  {
    verifiedPaths.push_back(vecPaths[0]);
  }
  else // multiple paths?
  {
    // validate the paths
    for (const auto& path : vecPaths)
    {
      const CURL url(path);
      bool isInvalid = false;

      // for my programs
      if (StringUtils::EqualsNoCase(category, "programs") ||
          StringUtils::EqualsNoCase(category, "myprograms"))
      {
        // only allow HD and plugins
        if (url.IsLocal() || url.IsProtocol("plugin"))
          verifiedPaths.emplace_back(path);
        else
          isInvalid = true;
      }
      // for others allow everything (if the user does something silly, we can't stop them)
      else
        verifiedPaths.emplace_back(path);

      // error message
      if (isInvalid)
        CLog::Log(LOGERROR, "CMediaSourceSettings:    invalid path type ({}) for multipath source",
                  path);
    }

    // no valid paths? skip to next source
    if (verifiedPaths.empty())
    {
      CLog::Log(LOGERROR,
                "CMediaSourceSettings:    missing or invalid <name> and/or <path> in source");
      return false;
    }
  }

  share.FromNameAndPaths(name, verifiedPaths);

  KODI::UTILS::CLockInfo& lockInfo{share.GetLockInfo()};
  lockInfo.ResetBadPasswordCount();
  if (lockModeElement)
  {
    lockInfo.SetMode(
        static_cast<LockMode>(std::strtol(lockModeElement->FirstChild()->Value(), nullptr, 10)));
    lockInfo.SetState(LOCK_STATE_LOCKED);
  }

  if (lockCodeElement && lockCodeElement->FirstChild())
    lockInfo.SetCode(lockCodeElement->FirstChild()->Value());

  if (badPwdCountElement && badPwdCountElement->FirstChild())
  {
    lockInfo.SetBadPasswordCount(
        static_cast<int>(std::strtol(badPwdCountElement->FirstChild()->Value(), nullptr, 10)));
  }

  if (thumbnailNodeElement && thumbnailNodeElement->FirstChild())
    share.m_strThumbnailImage = thumbnailNodeElement->FirstChild()->Value();

  XMLUtils::GetBoolean(source, "allowsharing", share.m_allowSharing);

  return true;
}

void CMediaSourceSettings::GetSources(const tinyxml2::XMLNode* rootElement,
                                      const std::string& tagName,
                                      std::vector<CMediaSource>& items,
                                      std::string& defaultString) const
{

  defaultString = "";
  items.clear();

  const tinyxml2::XMLElement* childElement = rootElement->FirstChildElement(tagName.c_str());
  if (!childElement)
  {
    CLog::Log(LOGDEBUG, "CMediaSourceSettings: <{}> tag is missing or sources.xml is malformed",
              tagName);
    return;
  }

  const tinyxml2::XMLNode* child = childElement->FirstChild();
  while (child)
  {
    std::string value = child->Value();
    if (value == XML_SOURCE ||
        value == "bookmark") // "bookmark" left in for backwards compatibility
    {
      CMediaSource share;
      if (GetSource(tagName, child, share))
        items.push_back(share);
      else
        CLog::Log(LOGERROR,
                  "CMediaSourceSettings:    Missing or invalid <name> and/or <path> in source");
    }
    else if (value == "default")
    {
      const tinyxml2::XMLNode* valueNode = child->FirstChild();
      if (valueNode)
      {
        const char* text = child->FirstChild()->Value();
        if (strcmp(text, "\0") != 0)
          defaultString = text;
        CLog::Log(LOGDEBUG, "CMediaSourceSettings:    Setting <default> source to : {}",
                  defaultString);
      }
    }

    child = child->NextSibling();
  }
}

bool CMediaSourceSettings::SetSources(tinyxml2::XMLNode* root,
                                      const char* section,
                                      const std::vector<CMediaSource>& shares,
                                      const std::string& defaultPath) const
{
  tinyxml2::XMLDocument* doc = root->GetDocument();
  tinyxml2::XMLElement* newElement = doc->NewElement(section);
  tinyxml2::XMLNode* sectionNode = root->InsertEndChild(newElement);

  if (!sectionNode)
    return false;

  XMLUtils::SetPath(sectionNode, "default", defaultPath);
  for (const auto& share : shares)
  {
    if (share.m_ignore)
      continue;

    tinyxml2::XMLElement* sourceElement = doc->NewElement(XML_SOURCE);

    XMLUtils::SetString(sourceElement, "name", share.strName);

    for (const auto& path : share.vecPaths)
      XMLUtils::SetPath(sourceElement, "path", path);

    const KODI::UTILS::CLockInfo& lockInfo{share.GetLockInfo()};
    if (lockInfo.GetState() > LOCK_STATE_NO_LOCK)
    {
      XMLUtils::SetInt(sourceElement, "lockmode", static_cast<int>(lockInfo.GetMode()));
      XMLUtils::SetString(sourceElement, "lockcode", lockInfo.GetCode());
      XMLUtils::SetInt(sourceElement, "badpwdcount", lockInfo.GetBadPasswordCount());
    }

    if (!share.m_strThumbnailImage.empty())
      XMLUtils::SetPath(sourceElement, "thumbnail", share.m_strThumbnailImage);

    XMLUtils::SetBoolean(sourceElement, "allowsharing", share.m_allowSharing);

    sectionNode->InsertEndChild(sourceElement);
  }

  return true;
}
