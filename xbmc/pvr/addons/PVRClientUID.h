/*
 *  Copyright (C) 2012-2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/IAddon.h"

#include <string>

namespace PVR
{
class CPVRClientUID final
{
public:
  CPVRClientUID(const std::string& addonID, ADDON::AddonInstanceId instanceID)
    : m_addonID(addonID), m_instanceID(instanceID)
  {
  }

  virtual ~CPVRClientUID() = default;

  /*!
   * @brief Return the numeric UID.
   * @return The numeric UID.
   */
  int GetUID() const;

  /*!
   * @brief Return the numeric legacy UID (compatibility/migration purposes only).
   * @return The numeric legacy UID.
   */
  int GetLegacyUID() const;

private:
  CPVRClientUID() = delete;

  std::string m_addonID;
  ADDON::AddonInstanceId m_instanceID{ADDON::ADDON_SINGLETON_INSTANCE_ID};

  mutable bool m_uidCreated{false};
  mutable int m_uid{0};
};
} // namespace PVR
