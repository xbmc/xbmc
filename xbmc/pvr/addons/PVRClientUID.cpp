/*
 *  Copyright (C) 2012-2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRClientUID.h"

#include <functional>

using namespace PVR;

int CPVRClientUID::GetUID() const
{
  if (!m_uidCreated)
  {
    std::hash<std::string> hasher;

    // Note: For database backwards compatibility reasons the hash of the first instance
    // must be calculated just from the addonId, not from addonId and instanceId.
    m_uid = static_cast<int>(hasher(
        (m_instanceID > ADDON::ADDON_FIRST_INSTANCE_ID ? std::to_string(m_instanceID) + "@" : "") +
        m_addonID));
    if (m_uid < 0)
      m_uid = -m_uid;

    m_uidCreated = true;
  }

  return m_uid;
}
