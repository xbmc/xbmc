/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "pvr/channels/PVRChannelGroup.h"

#include <memory>
#include <vector>

namespace PVR
{
class CPVRChannel;
class CPVRChannelNumber;

class CPVRChannelGroupInternal : public CPVRChannelGroup
{
public:
  CPVRChannelGroupInternal() = delete;

  /*!
   * @brief Create a new internal channel group.
   * @param bRadio True if this group holds radio channels.
   */
  explicit CPVRChannelGroupInternal(bool bRadio);

  /*!
   * @brief Create a new internal channel group.
   * @param path The path for the new group.
   */
  explicit CPVRChannelGroupInternal(const CPVRChannelsPath& path);

  ~CPVRChannelGroupInternal() override;

  /*!
   * @see CPVRChannelGroup::IsGroupMember
   */
  bool IsGroupMember(const std::shared_ptr<CPVRChannelGroupMember>& groupMember) const override;

  /*!
   * @see CPVRChannelGroup::AppendToGroup
   */
  bool AppendToGroup(const std::shared_ptr<CPVRChannelGroupMember>& groupMember) override;

  /*!
   * @see CPVRChannelGroup::RemoveFromGroup
   */
  bool RemoveFromGroup(const std::shared_ptr<CPVRChannelGroupMember>& groupMember) override;

  /*!
   * @brief Check whether the group name is still correct after the language setting changed.
   */
  void CheckGroupName();

protected:
  /*!
   * @brief Remove deleted group members from this group. Delete stale channels.
   * @param groupMembers The group members to use to update this list.
   * @return The removed members .
   */
  std::vector<std::shared_ptr<CPVRChannelGroupMember>> RemoveDeletedGroupMembers(
      const std::vector<std::shared_ptr<CPVRChannelGroupMember>>& groupMembers) override;

  /*!
   * @brief Update data with 'all channels' group members from the given clients, sync with local data.
   * @param clients The clients to fetch data from. Leave empty to fetch data from all created clients.
   * @return True on success, false otherwise.
   */
  bool UpdateFromClients(const std::vector<std::shared_ptr<CPVRClient>>& clients) override;

  /*!
   * @brief Clear all data.
   */
  void Unload() override;
};
} // namespace PVR
