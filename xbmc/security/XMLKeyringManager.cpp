/*
 *      Copyright (C) 2005-2011 Team XBMC
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

#include "XMLKeyringManager.h"
#include "filesystem/File.h"
#include "utils/XMLUtils.h"
#include "utils/XMLVariantParser.h"
#include "utils/XMLVariantWriter.h"
#include "utils/log.h"

using namespace std;

CXMLKeyringManager::CXMLKeyringManager(const char *store) : CMemoryKeyringManager(), m_store(store)
{
  Load();
}

bool CXMLKeyringManager::StoreSecret(const std::string &keyring, const std::string &key, const CVariant &secret)
{
  return CMemoryKeyringManager::StoreSecret(keyring, key, secret) && Save();
}

bool CXMLKeyringManager::Load()
{
  if (XFILE::CFile::Exists(m_store))
  {
    TiXmlDocument doc;
    if (!doc.LoadFile(m_store))
    {
      CLog::Log(LOGERROR, "XMLKeyringManager: Unable to load: %s, Line %d\n%s", m_store.c_str(), doc.ErrorRow(), doc.ErrorDesc());
      return false;
    }
    const TiXmlElement *root = doc.RootElement();
    if (root->ValueStr() != "keyrings")
    {
      CLog::Log(LOGERROR, "XMLKeyringManager: Failed to find root document");
      return false;
    }

    const TiXmlElement *keyringNode = root->FirstChildElement("keyring");
    while (keyringNode)
    {
      const char *keyring = keyringNode->Attribute("name");
      const TiXmlElement *secretNode = keyringNode->FirstChildElement("secret");
      while (secretNode)
      {
        const char *key = secretNode->Attribute("key");
        CVariant secret;
        CXMLVariantParser::Parse(secretNode->FirstChildElement("value"), secret);

        StoreSecret(keyring, key, secret);

        secretNode = secretNode->NextSiblingElement("secret");
      }

      keyringNode = keyringNode->NextSiblingElement("keyring");
    }
  }

  return true;
}

bool CXMLKeyringManager::Save()
{
  TiXmlDocument doc;
  TiXmlElement rootElement("passwords");
  TiXmlNode *root = doc.InsertEndChild(TiXmlElement("keyrings"));
  if (!root)
    return false;

  bool success = true;

  for (map<string, Keyring>::const_iterator keyring_itr = m_keyrings.begin(); keyring_itr != m_keyrings.end() && success; keyring_itr++)
  {
    TiXmlElement keyring("keyring");
    keyring.SetAttribute("name", keyring_itr->first.c_str());

    for (Keyring::const_iterator secret_itr = keyring_itr->second.begin(); secret_itr != keyring_itr->second.end() && success; secret_itr++)
    {
      TiXmlElement secret("secret");
      secret.SetAttribute("key", secret_itr->first.c_str());

      success &= CXMLVariantWriter::Write(&secret, secret_itr->second);

      success &= keyring.InsertEndChild(secret) != NULL;
    }

    success &= root->InsertEndChild(keyring) != NULL;
  }

  if (success)
    doc.SaveFile(m_store);
  else
  {
    CLog::Log(LOGERROR, "XMLKeyringManager: Failed to generate xml for storage");
    return false;
  }

  return true;
}
