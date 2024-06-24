/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "pvr/channels/PVRChannelGroup.h"

namespace PVR
{

class CPVRChannelGroupAllChannels : public CPVRChannelGroup
{
public:
  CPVRChannelGroupAllChannels() = delete;

  /*!
   * @brief Create a new all channels channel group.
   * @param bRadio True if this group holds radio channels.
   */
  explicit CPVRChannelGroupAllChannels(bool bRadio);

  /*!
   * @brief Create a new all channels channel group.
   * @param path The path for the new group.
   */
  explicit CPVRChannelGroupAllChannels(const CPVRChannelsPath& path);

  ~CPVRChannelGroupAllChannels() override;

  /*!
   * @see CPVRChannelGroup::IsGroupMember
   */
  bool IsGroupMember(
      const std::shared_ptr<const CPVRChannelGroupMember>& groupMember) const override;

  /*!
   * @see CPVRChannelGroup::AppendToGroup
   */
  bool AppendToGroup(const std::shared_ptr<const CPVRChannelGroupMember>& groupMember) override;

  /*!
   * @see CPVRChannelGroup::RemoveFromGroup
   */
  bool RemoveFromGroup(const std::shared_ptr<const CPVRChannelGroupMember>& groupMember) override;

  /*!
   * @brief Check whether the group name is still correct after the language setting changed.
   */
  void CheckGroupName();

  /*!
   * @brief Get the group's origin.
   * @return The origin.
   */
  Origin GetOrigin() const override { return Origin::SYSTEM; }

  /*!
   * @brief Return the type of this group.
   */
  int GroupType() const override { return PVR_GROUP_TYPE_SYSTEM_ALL_CHANNELS; }

  /*!
   * @brief Check whether this group could be deleted by the user.
   * @return True if the group could be deleted, false otherwise.
   */
  bool SupportsDelete() const override { return false; }

  /*!
   * @brief Check whether members could be added to this group by the user.
   * @return True if members could be added, false otherwise.
   */
  bool SupportsMemberAdd() const override { return true; }

  /*!
   * @brief Check whether members could be removed from this group by the user.
   * @return True if members could be removed, false otherwise.
   */
  bool SupportsMemberRemove() const override { return true; }

  /*!
   * @brief Check whether this group is owner of the channel instances it contains.
   * @return True if owner, false otherwise.
   */
  bool IsChannelsOwner() const override { return true; }

  /*!
   * @brief Check whether this group should be ignored, e.g. not presented to the user and API.
   * @param allChannelGroups All available channel groups.
   * @return True if to be ignored, false otherwise.
   */
  bool ShouldBeIgnored(
      const std::vector<std::shared_ptr<CPVRChannelGroup>>& allChannelGroups) const override
  {
    // "All channels" groups must always be present.
    return false;
  }

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
};
} // namespace PVR
