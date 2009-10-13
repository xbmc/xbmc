/*
 *      Copyright (C) 2005-2008 Team XBMC
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

#include "PasswordManager.h"
#include "GUIDialogLockSettings.h"
#include "URL.h"
#include "Settings.h"
#include "XMLUtils.h"

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
  if (!m_loaded)
    Load();
  map<CStdString, CStdString>::const_iterator it = m_temporaryCache.find(GetLookupPath(url));
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
  CStdString passcode;
  CStdString username = url.GetUserName();
  CStdString share;
  url.GetURLWithoutUserDetails(share);
  CStdString path = GetLookupPath(url);

  bool saveDetails = false;
  if (!CGUIDialogLockSettings::ShowAndGetUserAndPassword(username, passcode, share, &saveDetails))
    return false;

  url.SetPassword(passcode);
  url.SetUserName(username);

  // save the information for later
  CStdString authenticatedPath;
  url.GetURL(authenticatedPath);

  if (!m_loaded)
    Load();

  if (saveDetails)
  { // write to some random XML file...
    m_permanentCache[path] = authenticatedPath;
    Save();
  }

  m_temporaryCache[path] = authenticatedPath;
  return true;
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
  TiXmlDocument doc;
  if (!doc.LoadFile(g_settings.GetUserDataItem("passwords.xml")))
    return;
  const TiXmlElement *root = doc.RootElement();
  if (root->ValueStr() != "passwords")
    return;
  // read in our passwords
  const TiXmlElement *path = root->FirstChildElement("path");
  while (path)
  {
    CStdString from, to;
    if (XMLUtils::GetPath(path, "from", from) && XMLUtils::GetPath(path, "to", to))
    {
      m_permanentCache[from] = to;
      m_temporaryCache[from] = to;
    }
    path = path->NextSiblingElement("path");
  }
  m_loaded = true;
}

void CPasswordManager::Save() const
{
  if (!m_permanentCache.size())
    return;

  TiXmlDocument doc;
  TiXmlElement rootElement("passwords");
  TiXmlNode *root = doc.InsertEndChild(rootElement);
  if (!root)
    return;

  for (map<CStdString, CStdString>::const_iterator i = m_permanentCache.begin(); i != m_permanentCache.end(); ++i)
  {
    TiXmlElement pathElement("path");
    TiXmlNode *path = root->InsertEndChild(pathElement);
    XMLUtils::SetPath(path, "from", i->first);
    XMLUtils::SetPath(path, "to", i->second);
  }

  doc.SaveFile(g_settings.GetUserDataItem("passwords.xml"));
}

CStdString CPasswordManager::GetLookupPath(const CURL &url) const
{
  return "smb://" + url.GetHostName() + "/" + url.GetShareName();
}
