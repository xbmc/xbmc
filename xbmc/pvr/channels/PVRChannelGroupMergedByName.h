/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "pvr/channels/PVRChannelGroup.h"

namespace PVR
{

class CPVRChannelGroupMergedByName : public CPVRChannelGroup
{
public:
  /*!
   * @brief Create a new channel group instance.
   * @param path The channel group path.
   * @param allChannelsGroup The channel group containing all TV or radio channels.
   */
  CPVRChannelGroupMergedByName(const CPVRChannelsPath& path,
                               const std::shared_ptr<const CPVRChannelGroup>& allChannelsGroup)
    : CPVRChannelGroup(path, allChannelsGroup)
  {
  }

  /*!
   * @brief Get the group's origin.
   * @return The origin.
   */
  Origin GetOrigin() const override { return Origin::SYSTEM; }

  /*!
   * @brief Return the type of this group.
   */
  int GroupType() const override { return PVR_GROUP_TYPE_SYSTEM_MERGED_BY_NAME; }

  /*!
   * @brief Check whether this group could be deleted by the user.
   * @return True if the group could be deleted, false otherwise.
   */
  bool SupportsDelete() const override { return false; }

  /*!
   * @brief Check whether members could be added to this group by the user.
   * @return True if members could be added, false otherwise.
   */
  bool SupportsMemberAdd() const override { return false; }

  /*!
   * @brief Check whether members could be removed from this group by the user.
   * @return True if members could be removed, false otherwise.
   */
  bool SupportsMemberRemove() const override { return false; }

  /*!
   * @brief Check whether this group is owner of the channel instances it contains.
   * @return True if owner, false otherwise.
   */
  bool IsChannelsOwner() const override { return false; }

  /*!
   * @brief Update data with channel group members from the given clients, sync with local data.
   * @param clients The clients to fetch data from. Leave empty to fetch data from all created clients.
   * @return True on success, false otherwise.
   */
  bool UpdateFromClients(const std::vector<std::shared_ptr<CPVRClient>>& clients) override
  {
    return true; // Nothing to update from any client for locally managed groups.
  }

  /*!
   * @brief Check whether this group should be ignored, e.g. not presented to the user and API.
   * @param allChannelGroups All available channel groups.
   * @return True if to be ignored, false otherwise.
   */
  bool ShouldBeIgnored(
      const std::vector<std::shared_ptr<CPVRChannelGroup>>& allChannelGroups) const override;

  /*!
   * @brief Create any missing channel groups (e.g. after an update of groups/members/clients).
   * @param allChannelsGroup The all channels group.
   * @param allChannelGroups All channel groups.
   * @return The newly created groups, if any.
   */
  static std::vector<std::shared_ptr<CPVRChannelGroup>> CreateMissingGroups(
      const std::shared_ptr<CPVRChannelGroup>& allChannelsGroup,
      const std::vector<std::shared_ptr<CPVRChannelGroup>>& allChannelGroups);

  /*!
   * @brief Update all group members.
   * @param allChannelsGroup The all channels group.
   * @param allChannelGroups All available channel groups.
   * @return True on success, false otherwise.
   */
  bool UpdateGroupMembers(
      const std::shared_ptr<CPVRChannelGroup>& allChannelsGroup,
      const std::vector<std::shared_ptr<CPVRChannelGroup>>& allChannelGroups) override;
};
} // namespace PVR
