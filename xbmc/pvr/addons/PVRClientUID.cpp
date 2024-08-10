/*
 *  Copyright (C) 2012-2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRClientUID.h"

#include "pvr/PVRDatabase.h"
#include "utils/log.h"

#include <functional>
#include <map>
#include <utility>

using namespace PVR;

namespace
{
using ClientUIDParts = std::pair<std::string, ADDON::AddonInstanceId>;
static std::map<ClientUIDParts, int> s_idMap;
} // unnamed namespace

int CPVRClientUID::GetUID() const
{
  if (!m_uidCreated)
  {
    const auto it = s_idMap.find({m_addonID, m_instanceID});
    if (it != s_idMap.cend())
    {
      // Cache hit
      m_uid = (*it).second;
    }
    else
    {
      // Cache miss. Read from db and cache.
      CPVRDatabase db;
      if (!db.Open())
      {
        CLog::LogF(LOGERROR, "Unable to open TV database!");
        return -1;
      }

      m_uid = db.GetClientID(m_addonID, m_instanceID);
      if (m_uid == -1)
      {
        CLog::LogF(LOGERROR, "Unable to get client id from TV database!");
        return -1;
      }

      s_idMap.insert({{m_addonID, m_instanceID}, m_uid});
    }
    m_uidCreated = true;
  }

  return m_uid;
}

int CPVRClientUID::GetLegacyUID() const
{
  // Note: For database backwards compatibility reasons the hash of the first instance
  // must be calculated just from the addonId, not from addonId and instanceId.
  std::string prefix;
  if (m_instanceID > ADDON::ADDON_FIRST_INSTANCE_ID)
    prefix = std::to_string(m_instanceID) + "@";

  std::hash<std::string> hasher;
  int uid{static_cast<int>(hasher(prefix + m_addonID))};
  if (uid < 0)
    uid = -uid;

  return uid;
}
