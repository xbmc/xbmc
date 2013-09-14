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

#include "MemoryKeyringManager.h"

using namespace std;

bool CMemoryKeyringManager::FindSecret(const string &keyring, const string &key, CVariant &secret)
{
  map<string, Keyring>::const_iterator keyring_itr = m_keyrings.find(keyring);

  if (keyring_itr != m_keyrings.end())
  {
    Keyring::const_iterator itr = keyring_itr->second.find(key);
    if (itr != keyring_itr->second.end())
    {
      secret = itr->second;
      return true;
    }
  }

  return false;
}

bool CMemoryKeyringManager::EraseSecret(const string &keyring, const string &key)
{
  map<string, Keyring>::iterator keyring_itr = m_keyrings.find(keyring);

  if (keyring_itr != m_keyrings.end())
  {
    Keyring::iterator itr = keyring_itr->second.find(key);
    if (itr != keyring_itr->second.end())
      keyring_itr->second.erase(itr);
  }

  return true;
}

bool CMemoryKeyringManager::StoreSecret(const string &keyring, const string &key, const CVariant &secret)
{
  m_keyrings[keyring][key] = secret;
  return true;
}
