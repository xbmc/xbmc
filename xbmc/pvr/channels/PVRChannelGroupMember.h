/*
 *  Copyright (C) 2012-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "pvr/channels/PVRChannelNumber.h"
#include "utils/ISerializable.h"
#include "utils/ISortable.h"

#include <memory>
#include <string>

namespace PVR
{

class CPVRChannel;

class CPVRChannelGroupMember : public ISerializable, public ISortable
{
  friend class CPVRDatabase;

public:
  CPVRChannelGroupMember() : m_bNeedsSave(false) {}

  CPVRChannelGroupMember(const std::string& groupName,
                         int groupClientID,
                         int order,
                         const std::shared_ptr<CPVRChannel>& channel);

  CPVRChannelGroupMember(int iGroupID,
                         const std::string& groupName,
                         int groupClientID,
                         const std::shared_ptr<CPVRChannel>& channel);

  virtual ~CPVRChannelGroupMember() = default;

  // ISerializable implementation
  void Serialize(CVariant& value) const override;

  // ISortable implementation
  void ToSortable(SortItem& sortable, Field field) const override;

  std::shared_ptr<CPVRChannel> Channel() const { return m_channel; }
  void SetChannel(const std::shared_ptr<CPVRChannel>& channel);

  int GroupID() const { return m_iGroupID; }
  void SetGroupID(int iGroupID);

  const std::string& Path() const { return m_path; }
  void SetGroupName(const std::string& groupName);

  const CPVRChannelNumber& ChannelNumber() const { return m_channelNumber; }
  void SetChannelNumber(const CPVRChannelNumber& channelNumber);

  const CPVRChannelNumber& ClientChannelNumber() const { return m_clientChannelNumber; }
  void SetClientChannelNumber(const CPVRChannelNumber& clientChannelNumber);

  int ClientPriority() const { return m_iClientPriority; }
  void SetClientPriority(int iClientPriority);

  int Order() const { return m_iOrder; }
  void SetOrder(int iOrder);

  bool NeedsSave() const { return m_bNeedsSave; }
  void SetSaved() { m_bNeedsSave = false; }

  int ChannelClientID() const { return m_iChannelClientID; }

  int ChannelUID() const { return m_iChannelUID; }

  int ChannelDatabaseID() const { return m_iChannelDatabaseID; }

  bool IsRadio() const { return m_bIsRadio; }

  /*!
   * @brief Delete this group member from the database.
   * @return True if it was deleted successfully, false otherwise.
   */
  bool QueueDelete();

private:
  int m_iGroupID = -1;
  int m_iGroupClientID = -1;
  int m_iChannelClientID = -1;
  int m_iChannelUID = -1;
  int m_iChannelDatabaseID = -1;
  bool m_bIsRadio = false;
  std::shared_ptr<CPVRChannel> m_channel;
  std::string m_path;
  CPVRChannelNumber m_channelNumber; // the channel number this channel has in the group
  CPVRChannelNumber
      m_clientChannelNumber; // the client channel number this channel has in the group
  int m_iClientPriority = 0;
  int m_iOrder = 0; // The value denoting the order of this member in the group

  bool m_bNeedsSave = true;
};

} // namespace PVR
