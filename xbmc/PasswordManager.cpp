/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PasswordManager.h"

#include "ServiceBroker.h"
#include "URL.h"
#include "profiles/ProfileManager.h"
#include "profiles/dialogs/GUIDialogLockSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/FileUtils.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML2.h"
#include "utils/XMLUtils.h"
#include "utils/log.h"

#include <cstring>
#include <mutex>

CPasswordManager &CPasswordManager::GetInstance()
{
  static CPasswordManager sPasswordManager;
  return sPasswordManager;
}

CPasswordManager::CPasswordManager()
{
  m_loaded = false;
}

bool CPasswordManager::AuthenticateURL(CURL &url)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  if (!m_loaded)
    Load();
  std::string lookup(GetLookupPath(url));
  std::map<std::string, std::string>::const_iterator it = m_temporaryCache.find(lookup);
  if (it == m_temporaryCache.end())
  { // second step, try something that doesn't quite match
    it = m_temporaryCache.find(GetServerLookup(lookup));
  }
  if (it != m_temporaryCache.end())
  {
    CURL auth(it->second);
    url.SetDomain(auth.GetDomain());
    url.SetPassword(auth.GetPassWord());
    url.SetUserName(auth.GetUserName());
    return true;
  }
  return false;
}

bool CPasswordManager::PromptToAuthenticateURL(CURL &url)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  std::string passcode;
  std::string username = url.GetUserName();
  std::string domain = url.GetDomain();
  if (!domain.empty())
    username = domain + '\\' + username;

  bool saveDetails = false;
  if (!CGUIDialogLockSettings::ShowAndGetUserAndPassword(username, passcode, url.GetWithoutUserDetails(), &saveDetails))
    return false;

  // domain/name to domain\name
  std::string name = username;
  std::replace(name.begin(), name.end(), '/', '\\');

  if (url.IsProtocol("smb") && name.find('\\') != std::string::npos)
  {
    auto pair = StringUtils::Split(name, '\\', 2);
    url.SetDomain(pair[0]);
    url.SetUserName(pair[1]);
  }
  else
  {
    url.SetDomain("");
    url.SetUserName(username);
  }

  url.SetPassword(passcode);

  // save the information for later
  SaveAuthenticatedURL(url, saveDetails);
  return true;
}

void CPasswordManager::SaveAuthenticatedURL(const CURL &url, bool saveToProfile)
{
  // don't store/save authenticated url if it doesn't contain username
  if (url.GetUserName().empty())
    return;

  std::unique_lock<CCriticalSection> lock(m_critSection);

  std::string path = GetLookupPath(url);
  std::string authenticatedPath = url.Get();

  if (!m_loaded)
    Load();

  if (saveToProfile)
  { // write to some random XML file...
    m_permanentCache[path] = authenticatedPath;
    Save();
  }

  // save for both this path and more generally the server as a whole.
  m_temporaryCache[path] = authenticatedPath;
  m_temporaryCache[GetServerLookup(path)] = authenticatedPath;
}

bool CPasswordManager::IsURLSupported(const CURL &url)
{
  return url.IsProtocol("smb")
    || url.IsProtocol("nfs")
    || url.IsProtocol("ftp")
    || url.IsProtocol("ftps")
    || url.IsProtocol("sftp")
    || url.IsProtocol("http")
    || url.IsProtocol("https")
    || url.IsProtocol("dav")
    || url.IsProtocol("davs");
}

void CPasswordManager::Clear()
{
  m_temporaryCache.clear();
  m_permanentCache.clear();
  m_loaded = false;
}

void CPasswordManager::Load()
{
  Clear();

  const std::shared_ptr<CProfileManager> profileManager = CServiceBroker::GetSettingsComponent()->GetProfileManager();

  std::string passwordsFile = profileManager->GetUserDataItem("passwords.xml");
  if (CFileUtils::Exists(passwordsFile))
  {
    CXBMCTinyXML2 doc;
    if (!doc.LoadFile(passwordsFile))
    {
      CLog::LogF(LOGERROR, "Unable to load: {}, Line {}\n{}", passwordsFile, doc.ErrorLineNum(),
                 doc.ErrorStr());
      return;
    }
    const auto* root = doc.RootElement();
    if (root == nullptr)
      return;

    if (std::strcmp(root->Value(), "passwords") != 0)
      return;
    // read in our passwords
    const auto* path = root->FirstChildElement("path");
    while (path != nullptr)
    {
      std::string from, to;
      if (XMLUtils::GetPath(path, "from", from) && XMLUtils::GetPath(path, "to", to))
      {
        m_permanentCache[from] = to;
        m_temporaryCache[from] = to;
        m_temporaryCache[GetServerLookup(from)] = to;
      }
      path = path->NextSiblingElement("path");
    }
  }
  m_loaded = true;
}

void CPasswordManager::Save() const
{
  if (m_permanentCache.empty())
    return;

  CXBMCTinyXML2 doc;
  auto* rootElement = doc.NewElement("passwords");
  if (rootElement == nullptr)
    return;

  auto* root = doc.InsertEndChild(rootElement);
  if (root == nullptr)
    return;

  for (std::map<std::string, std::string>::const_iterator i = m_permanentCache.begin(); i != m_permanentCache.end(); ++i)
  {
    auto* pathElement = doc.NewElement("path");
    if (pathElement == nullptr)
      continue;

    auto* path = root->InsertEndChild(pathElement);
    if (path == nullptr)
      continue;

    XMLUtils::SetPath(path, "from", i->first);
    XMLUtils::SetPath(path, "to", i->second);
  }

  const std::shared_ptr<CProfileManager> profileManager = CServiceBroker::GetSettingsComponent()->GetProfileManager();

  doc.SaveFile(profileManager->GetUserDataItem("passwords.xml"));
}

std::string CPasswordManager::GetLookupPath(const CURL &url) const
{
  if (url.IsProtocol("sftp"))
    return GetServerLookup(url.Get());

  return url.GetProtocol() + "://" + url.GetHostName() + "/" + url.GetShareName();
}

std::string CPasswordManager::GetServerLookup(const std::string &path) const
{
  CURL url(path);
  return url.GetProtocol() + "://" + url.GetHostName() + "/";
}
