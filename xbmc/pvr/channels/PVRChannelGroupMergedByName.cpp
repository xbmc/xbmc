/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRChannelGroupMergedByName.h"

#include "guilib/LocalizeStrings.h"
#include "pvr/channels/PVRChannelGroupMember.h"
#include "utils/StringUtils.h"

#include <unordered_map>
#include <utility>

using namespace PVR;

bool CPVRChannelGroupMergedByName::ShouldBeIgnored(
    const std::vector<std::shared_ptr<CPVRChannelGroup>>& allChannelGroups) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  // Ignore the group if it is empty or only members from one group are present.

  if (m_members.empty())
    return true;

  const std::string mergedGroupName{!ClientGroupName().empty() ? ClientGroupName() : GroupName()};
  static constexpr int NO_GROUP_FOUND{-1};
  int matchingGroup{NO_GROUP_FOUND};

  for (const auto& member : m_members)
  {
    for (const auto& group : allChannelGroups)
    {
      if (group->GetOrigin() != Origin::USER && group->GetOrigin() != Origin::CLIENT)
        continue;

      if (group->GroupName() != mergedGroupName && group->ClientGroupName() != mergedGroupName)
        continue;

      if (group->IsGroupMember(member.second))
      {
        if (matchingGroup != NO_GROUP_FOUND && matchingGroup != group->GroupID())
        {
          // Found a second group containing a member of this group. This group must not be ignored.
          return false;
        }
        // First match or no new group. Continue with next group.
        matchingGroup = group->GroupID();
      }
    }
  }
  return true;
}

namespace
{
std::vector<std::string> GetGroupNames(const std::vector<std::shared_ptr<CPVRChannelGroup>>& groups)
{
  std::unordered_map<std::string, unsigned int> knownNamesCountMap;

  // Structure group data for easy further processing.
  for (const auto& group : groups)
  {
    const std::string groupName{!group->ClientGroupName().empty() ? group->ClientGroupName()
                                                                  : group->GroupName()};
    switch (group->GetOrigin())
    {
      case CPVRChannelGroup::Origin::SYSTEM:
      {
        // Ignore system-created groups, except merged by name groups
        if (group->GroupType() == PVR_GROUP_TYPE_SYSTEM_MERGED_BY_NAME)
        {
          const auto it = knownNamesCountMap.find(groupName);
          if (it == knownNamesCountMap.end())
            knownNamesCountMap.insert({groupName, 0}); // remember we found a merged group
          else
            (*it).second = 0; // reset groups counter. we do not need a new merged group
        }
        break;
      }

      case CPVRChannelGroup::Origin::USER:
      case CPVRChannelGroup::Origin::CLIENT:
      {
        const auto it = knownNamesCountMap.find(groupName);
        if (it == knownNamesCountMap.end())
          knownNamesCountMap.insert({groupName, 1}); // first occurance
        else if ((*it).second > 0)
          (*it).second++; // second+ occurance
        break;
      }
      default:
        break;
    }
  }

  std::vector<std::string> names;
  for (const auto& name : knownNamesCountMap)
  {
    if (name.second > 1)
      names.emplace_back(name.first);
  }
  return names;
}
} // namespace

std::vector<std::shared_ptr<CPVRChannelGroup>> CPVRChannelGroupMergedByName::CreateMissingGroups(
    const std::shared_ptr<CPVRChannelGroup>& allChannelsGroup,
    const std::vector<std::shared_ptr<CPVRChannelGroup>>& allChannelGroups)
{
  std::vector<std::shared_ptr<CPVRChannelGroup>> addedGroups;

  const std::vector<std::string> names{GetGroupNames(allChannelGroups)};
  for (const auto& name : names)
  {
    const std::string groupName{StringUtils::Format(g_localizeStrings.Get(859), name)};
    const CPVRChannelsPath path{allChannelsGroup->IsRadio(), groupName, PVR_GROUP_CLIENT_ID_LOCAL};
    const std::shared_ptr<CPVRChannelGroup> mergedByNameGroup{
        std::make_shared<CPVRChannelGroupMergedByName>(path, allChannelsGroup)};
    mergedByNameGroup->SetClientGroupName(name);
    addedGroups.emplace_back(mergedByNameGroup);
  }

  return addedGroups;
}

bool CPVRChannelGroupMergedByName::UpdateGroupMembers(
    const std::shared_ptr<CPVRChannelGroup>& allChannelsGroup,
    const std::vector<std::shared_ptr<CPVRChannelGroup>>& allChannelGroups)
{
  std::vector<std::shared_ptr<CPVRChannelGroupMember>> groupMembers;

  // Collect and populate matching members.
  for (const auto& group : allChannelGroups)
  {
    const std::string groupName{!group->ClientGroupName().empty() ? group->ClientGroupName()
                                                                  : group->GroupName()};
    if (groupName != ClientGroupName())
      continue;

    switch (group->GetOrigin())
    {
      case CPVRChannelGroup::Origin::USER:
      case CPVRChannelGroup::Origin::CLIENT:
      {
        const auto members{group->GetMembers()};
        for (const auto& member : members)
        {
          groupMembers.emplace_back(std::make_shared<CPVRChannelGroupMember>(
              GroupID(), GroupName(), GetClientID(), member->Channel()));
        }
        break;
      }
      default:
        break;
    }
  }

  return UpdateGroupEntries(groupMembers);
}
