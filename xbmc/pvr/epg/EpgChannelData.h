/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <ctime>
#include <string>

namespace PVR
{
class CPVRChannel;

class CPVREpgChannelData
{
public:
  CPVREpgChannelData() = default;
  CPVREpgChannelData(int iClientId, int iUniqueClientChannelId);
  explicit CPVREpgChannelData(const CPVRChannel& channel);

  int ClientId() const;
  int UniqueClientChannelId() const;
  bool IsRadio() const;

  bool IsHidden() const;
  void SetHidden(bool bIsHidden);

  bool IsLocked() const;
  void SetLocked(bool bIsLocked);

  bool IsEPGEnabled() const;
  void SetEPGEnabled(bool bIsEPGEnabled);

  int ChannelId() const;
  void SetChannelId(int iChannelId);

  const std::string& ChannelName() const;
  void SetChannelName(const std::string& strChannelName);

  const std::string& ChannelIconPath() const;
  void SetChannelIconPath(const std::string& strChannelIconPath);

private:
  const bool m_bIsRadio = false;
  const int m_iClientId = -1;
  const int m_iUniqueClientChannelId = -1;

  bool m_bIsHidden = false;
  bool m_bIsLocked = false;
  bool m_bIsEPGEnabled = true;
  int m_iChannelId = -1;
  std::string m_strChannelName;
  std::string m_strChannelIconPath;
};
} // namespace PVR
