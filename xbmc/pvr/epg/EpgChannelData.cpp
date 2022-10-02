/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "EpgChannelData.h"

#include "XBDateTime.h"
#include "pvr/channels/PVRChannel.h"

using namespace PVR;

CPVREpgChannelData::CPVREpgChannelData(int iClientId, int iUniqueClientChannelId)
  : m_iClientId(iClientId), m_iUniqueClientChannelId(iUniqueClientChannelId)
{
}

CPVREpgChannelData::CPVREpgChannelData(const CPVRChannel& channel)
  : m_bIsRadio(channel.IsRadio()),
    m_iClientId(channel.ClientID()),
    m_iUniqueClientChannelId(channel.UniqueID()),
    m_bIsHidden(channel.IsHidden()),
    m_bIsLocked(channel.IsLocked()),
    m_bIsEPGEnabled(channel.EPGEnabled()),
    m_iChannelId(channel.ChannelID()),
    m_strChannelName(channel.ChannelName()),
    m_strChannelIconPath(channel.IconPath())
{
}

bool CPVREpgChannelData::IsRadio() const
{
  return m_bIsRadio;
}

int CPVREpgChannelData::ClientId() const
{
  return m_iClientId;
}

int CPVREpgChannelData::UniqueClientChannelId() const
{
  return m_iUniqueClientChannelId;
}

bool CPVREpgChannelData::IsHidden() const
{
  return m_bIsHidden;
}

void CPVREpgChannelData::SetHidden(bool bIsHidden)
{
  m_bIsHidden = bIsHidden;
}

bool CPVREpgChannelData::IsLocked() const
{
  return m_bIsLocked;
}

void CPVREpgChannelData::SetLocked(bool bIsLocked)
{
  m_bIsLocked = bIsLocked;
}

bool CPVREpgChannelData::IsEPGEnabled() const
{
  return m_bIsEPGEnabled;
}

void CPVREpgChannelData::SetEPGEnabled(bool bIsEPGEnabled)
{
  m_bIsEPGEnabled = bIsEPGEnabled;
}

int CPVREpgChannelData::ChannelId() const
{
  return m_iChannelId;
}

void CPVREpgChannelData::SetChannelId(int iChannelId)
{
  m_iChannelId = iChannelId;
}

const std::string& CPVREpgChannelData::ChannelName() const
{
  return m_strChannelName;
}

void CPVREpgChannelData::SetChannelName(const std::string& strChannelName)
{
  m_strChannelName = strChannelName;
}

const std::string& CPVREpgChannelData::ChannelIconPath() const
{
  return m_strChannelIconPath;
}

void CPVREpgChannelData::SetChannelIconPath(const std::string& strChannelIconPath)
{
  m_strChannelIconPath = strChannelIconPath;
}
