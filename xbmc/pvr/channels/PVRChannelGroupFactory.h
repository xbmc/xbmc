/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>
#include <vector>

struct PVR_CHANNEL_GROUP;

namespace PVR
{

class CPVRChannelGroup;
class CPVRChannelsPath;

class CPVRChannelGroupFactory
{
public:
  CPVRChannelGroupFactory() = default;
  virtual ~CPVRChannelGroupFactory() = default;

  /*!
   * @brief Create an all channels group instance.
   * @param isRadio Whether Radio or TV.
   * @return The new group.
   */
  std::shared_ptr<CPVRChannelGroup> CreateAllChannelsGroup(bool isRadio);

  /*!
   * @brief Create an instance for a group provided by a PVR client add-on.
   * @param groupData The group data.
   * @param clientID The id of the client that provided the group.
   * @param allChannels The all channels group.
   * @return The new group.
   */
  std::shared_ptr<CPVRChannelGroup> CreateClientGroup(
      const PVR_CHANNEL_GROUP& groupData,
      int clientID,
      const std::shared_ptr<const CPVRChannelGroup>& allChannels);

  /*!
   * @brief Create an instance for a group created by the user.
   * @param isRadio Whether Radio or TV.
   * @param name The name for the group.
   * @param allChannels The all channels group.
   * @return The new group.
   */
  std::shared_ptr<CPVRChannelGroup> CreateUserGroup(
      bool isRadio,
      const std::string& name,
      const std::shared_ptr<const CPVRChannelGroup>& allChannels);

  /*!
   * @brief Create a channel group matching the given type.
   * @param groupType The type of the group.
   * @param groupPath The path of the group.
   * @param allChannels The all channels group.
   * @return The new group.
   */
  std::shared_ptr<CPVRChannelGroup> CreateGroup(
      int groupType,
      const CPVRChannelsPath& groupPath,
      const std::shared_ptr<const CPVRChannelGroup>& allChannels);

  /*!
   * @brief Get the priority (e.g. for sorting groups) for the type of the given group. Lower number means higer priority.
   * @param group The group.
   * @return The group's type priority.
   */
  int GetGroupTypePriority(const std::shared_ptr<const CPVRChannelGroup>& group) const;

  /*!
   * @brief Create any missing channel groups (e.g. after an update of groups/members/clients).
   * @param allChannelsGroup The all channels group.
   * @param allChannelGroups All channel groups.
   * @return The newly created groups, if any.
   */
  std::vector<std::shared_ptr<CPVRChannelGroup>> CreateMissingGroups(
      const std::shared_ptr<CPVRChannelGroup>& allChannelsGroup,
      const std::vector<std::shared_ptr<CPVRChannelGroup>>& allChannelGroups);
};
} // namespace PVR
