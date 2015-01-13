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

#include "PasswordManager.h"
#include "profiles/ProfilesManager.h"
#include "profiles/dialogs/GUIDialogLockSettings.h"
#include "URL.h"
#include "utils/XMLUtils.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "filesystem/File.h"

using namespace std;

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
  CSingleLock lock(m_critSection);

  if (!m_loaded)
    Load();
  std::string lookup(GetLookupPath(url));
  map<std::string, std::string>::const_iterator it = m_temporaryCache.find(lookup);
  if (it == m_temporaryCache.end())
  { // second step, try something that doesn't quite match
    it = m_temporaryCache.find(GetServerLookup(lookup));
  }
  if (it != m_temporaryCache.end())
  {
    CURL auth(it->second);
    url.SetPassword(auth.GetPassWord());
    url.SetUserName(auth.GetUserName());
    return true;
  }
  return false;
}

bool CPasswordManager::PromptToAuthenticateURL(CURL &url)
{
  CSingleLock lock(m_critSection);

  std::string passcode;
  std::string username = url.GetUserName();

  bool saveDetails = false;
  if (!CGUIDialogLockSettings::ShowAndGetUserAndPassword(username, passcode, url.GetWithoutUserDetails(), &saveDetails))
    return false;

  url.SetPassword(passcode);
  url.SetUserName(username);

  // save the information for later
  SaveAuthenticatedURL(url, saveDetails);
  return true;
}

void CPasswordManager::SaveAuthenticatedURL(const CURL &url, bool saveToProfile)
{
  // don't store/save authenticated url if it doesn't contain username
  if (url.GetUserName().empty())
    return;

  CSingleLock lock(m_critSection);

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

void CPasswordManager::Clear()
{
  m_temporaryCache.clear();
  m_permanentCache.clear();
  m_loaded = false;
}

void CPasswordManager::Load()
{
  Clear();
  std::string passwordsFile = CProfilesManager::Get().GetUserDataItem("passwords.xml");
  if (XFILE::CFile::Exists(passwordsFile))
  {
    CXBMCTinyXML doc;
    if (!doc.LoadFile(passwordsFile))
    {
      CLog::Log(LOGERROR, "%s - Unable to load: %s, Line %d\n%s", 
        __FUNCTION__, passwordsFile.c_str(), doc.ErrorRow(), doc.ErrorDesc());
      return;
    }
    const TiXmlElement *root = doc.RootElement();
    if (root->ValueStr() != "passwords")
      return;
    // read in our passwords
    const TiXmlElement *path = root->FirstChildElement("path");
    while (path)
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

  CXBMCTinyXML doc;
  TiXmlElement rootElement("passwords");
  TiXmlNode *root = doc.InsertEndChild(rootElement);
  if (!root)
    return;

  for (map<std::string, std::string>::const_iterator i = m_permanentCache.begin(); i != m_permanentCache.end(); ++i)
  {
    TiXmlElement pathElement("path");
    TiXmlNode *path = root->InsertEndChild(pathElement);
    XMLUtils::SetPath(path, "from", i->first);
    XMLUtils::SetPath(path, "to", i->second);
  }

  doc.SaveFile(CProfilesManager::Get().GetUserDataItem("passwords.xml"));
}

std::string CPasswordManager::GetLookupPath(const CURL &url) const
{
  return "smb://" + url.GetHostName() + "/" + url.GetShareName();
}

std::string CPasswordManager::GetServerLookup(const std::string &path) const
{
  CURL url(path);
  return "smb://" + url.GetHostName() + "/";
}
