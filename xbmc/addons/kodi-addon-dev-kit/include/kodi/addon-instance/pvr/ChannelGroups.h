/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "../../AddonBase.h"
#include "../../c-api/addon-instance/pvr.h"

#ifdef __cplusplus

namespace kodi
{
namespace addon
{

class PVRChannelGroup : public CStructHdl<PVRChannelGroup, PVR_CHANNEL_GROUP>
{
public:
  PVRChannelGroup() { memset(m_cStructure, 0, sizeof(PVR_CHANNEL_GROUP)); }
  PVRChannelGroup(const PVRChannelGroup& channel) : CStructHdl(channel) {}
  PVRChannelGroup(const PVR_CHANNEL_GROUP* channel) : CStructHdl(channel) {}
  PVRChannelGroup(PVR_CHANNEL_GROUP* channel) : CStructHdl(channel) {}

  void SetGroupName(const std::string& groupName)
  {
    strncpy(m_cStructure->strGroupName, groupName.c_str(), sizeof(m_cStructure->strGroupName) - 1);
  }
  std::string GetGroupName() const { return m_cStructure->strGroupName; }

  void SetIsRadio(bool isRadio) { m_cStructure->bIsRadio = isRadio; }
  bool GetIsRadio() const { return m_cStructure->bIsRadio; }

  void SetPosition(unsigned int position) { m_cStructure->iPosition = position; }
  unsigned int GetPosition() const { return m_cStructure->iPosition; }
};

class PVRChannelGroupsResultSet
{
public:
  PVRChannelGroupsResultSet() = delete;
  PVRChannelGroupsResultSet(const AddonInstance_PVR* instance, ADDON_HANDLE handle)
    : m_instance(instance), m_handle(handle)
  {
  }

  void Add(const kodi::addon::PVRChannelGroup& tag)
  {
    m_instance->toKodi->TransferChannelGroup(m_instance->toKodi->kodiInstance, m_handle, tag);
  }

private:
  const AddonInstance_PVR* m_instance = nullptr;
  const ADDON_HANDLE m_handle;
};

class PVRChannelGroupMember : public CStructHdl<PVRChannelGroupMember, PVR_CHANNEL_GROUP_MEMBER>
{
public:
  PVRChannelGroupMember() { memset(m_cStructure, 0, sizeof(PVR_CHANNEL_GROUP_MEMBER)); }
  PVRChannelGroupMember(const PVRChannelGroupMember& channel) : CStructHdl(channel) {}
  PVRChannelGroupMember(const PVR_CHANNEL_GROUP_MEMBER* channel) : CStructHdl(channel) {}
  PVRChannelGroupMember(PVR_CHANNEL_GROUP_MEMBER* channel) : CStructHdl(channel) {}

  void SetGroupName(const std::string& groupName)
  {
    strncpy(m_cStructure->strGroupName, groupName.c_str(), sizeof(m_cStructure->strGroupName) - 1);
  }
  std::string GetGroupName() const { return m_cStructure->strGroupName; }

  void SetChannelUniqueId(unsigned int channelUniqueId)
  {
    m_cStructure->iChannelUniqueId = channelUniqueId;
  }
  unsigned int GetChannelUniqueId() const { return m_cStructure->iChannelUniqueId; }

  void SetChannelNumber(unsigned int channelNumber)
  {
    m_cStructure->iChannelNumber = channelNumber;
  }
  unsigned int GetChannelNumber() const { return m_cStructure->iChannelNumber; }

  void SetSubChannelNumber(unsigned int subChannelNumber)
  {
    m_cStructure->iSubChannelNumber = subChannelNumber;
  }
  unsigned int GetSubChannelNumber() const { return m_cStructure->iSubChannelNumber; }

  void SetOrder(bool order) { m_cStructure->iOrder = order; }
  bool GetOrder() const { return m_cStructure->iOrder; }
};

class PVRChannelGroupMembersResultSet
{
public:
  PVRChannelGroupMembersResultSet() = delete;
  PVRChannelGroupMembersResultSet(const AddonInstance_PVR* instance, ADDON_HANDLE handle)
    : m_instance(instance), m_handle(handle)
  {
  }

  void Add(const kodi::addon::PVRChannelGroupMember& tag)
  {
    m_instance->toKodi->TransferChannelGroupMember(m_instance->toKodi->kodiInstance, m_handle, tag);
  }

private:
  const AddonInstance_PVR* m_instance = nullptr;
  const ADDON_HANDLE m_handle;
};

} /* namespace addon */
} /* namespace kodi */

#endif /* __cplusplus */
