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

#include "KeyringManager.h"
#include "settings/Settings.h"
#include "XMLKeyringManager.h"

using namespace std;

CKeyringManager::CKeyringManager()
{
  m_persistantKeyringManager = NULL;
}

CKeyringManager::~CKeyringManager()
{
  delete m_persistantKeyringManager;
}

void CKeyringManager::Initialize()
{
  // TODO Platform specific implementation

  if (m_persistantKeyringManager == NULL)
    m_persistantKeyringManager = new CXMLKeyringManager(g_settings.GetUserDataItem("secrets.xml").c_str());
}

bool CKeyringManager::FindSecret(const string &keyring, const string &key, CVariant &secret)
{
  if (!m_temporaryKeyringManager.FindSecret(keyring, key, secret))
  {
    if (m_persistantKeyringManager && m_persistantKeyringManager->FindSecret(keyring, key, secret))
    {
      m_temporaryKeyringManager.StoreSecret(keyring, key, secret);
      return true;
    }
    else
      return false;
  }
  else
    return true;
}

bool CKeyringManager::EraseSecret(const string &keyring, const string &key)
{
  bool success = m_temporaryKeyringManager.EraseSecret(keyring, key);
  if (m_persistantKeyringManager)
    return m_persistantKeyringManager->EraseSecret(keyring, key);
  else
    return success;
}

bool CKeyringManager::StoreSecret(const string &keyring, const string &key, const CVariant &secret, bool persistant)
{
  bool success = m_temporaryKeyringManager.StoreSecret(keyring, key, secret);
  if (persistant && m_persistantKeyringManager)
    return m_persistantKeyringManager->StoreSecret(keyring, key, secret);
  else
    return success;
}
