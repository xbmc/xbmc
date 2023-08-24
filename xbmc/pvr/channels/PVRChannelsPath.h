/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/IAddon.h"

class CDateTime;

namespace PVR
{
  class CPVRChannelsPath
  {
  public:
    static const std::string PATH_TV_CHANNELS;
    static const std::string PATH_RADIO_CHANNELS;

    explicit CPVRChannelsPath(const std::string& strPath);
    CPVRChannelsPath(bool bRadio, const std::string& strGroupName, int iGroupClientID);
    CPVRChannelsPath(bool bRadio,
                     bool bHidden,
                     const std::string& strGroupName,
                     int iGroupClientID);
    CPVRChannelsPath(bool bRadio,
                     const std::string& strGroupName,
                     int iGroupClientID,
                     const std::string& strAddonID,
                     ADDON::AddonInstanceId instanceID,
                     int iChannelUID);

    operator std::string() const { return m_path; }
    bool operator ==(const CPVRChannelsPath& right) const { return m_path == right.m_path; }
    bool operator !=(const CPVRChannelsPath& right) const { return !(*this == right); }

    bool IsValid() const { return m_kind > Kind::PROTO; }

    bool IsEmpty() const { return m_kind == Kind::EMPTY; }
    bool IsChannelsRoot() const { return m_kind == Kind::ROOT; }
    bool IsChannelGroup() const { return m_kind == Kind::GROUP; }
    bool IsChannel() const { return m_kind == Kind::CHANNEL; }

    bool IsHiddenChannelGroup() const;

    bool IsRadio() const { return m_bRadio; }

    const std::string& GetGroupName() const { return m_groupName; }
    int GetGroupClientID() const { return m_groupClientID; }
    const std::string& GetAddonID() const { return m_addonID; }
    ADDON::AddonInstanceId GetInstanceID() const { return m_instanceID; }
    int GetChannelUID() const { return m_iChannelUID; }

  private:
    static std::string TrimSlashes(const std::string& strString);

    enum class Kind
    {
      INVALID,
      PROTO,
      EMPTY,
      ROOT,
      GROUP,
      CHANNEL,
    };

    Kind m_kind = Kind::INVALID;
    bool m_bRadio = false;;
    std::string m_path;
    std::string m_groupName;
    int m_groupClientID{-1};
    std::string m_addonID;
    ADDON::AddonInstanceId m_instanceID{ADDON::ADDON_SINGLETON_INSTANCE_ID};
    int m_iChannelUID = -1;
  };
}
