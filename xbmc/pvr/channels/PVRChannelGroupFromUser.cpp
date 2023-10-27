/*
 *  Copyright (C) 2012-2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRChannelGroupFromUser.h"

using namespace PVR;

CPVRChannelGroupFromUser::CPVRChannelGroupFromUser(
    const CPVRChannelsPath& path, const std::shared_ptr<const CPVRChannelGroup>& allChannelsGroup)
  : CPVRChannelGroup(path, allChannelsGroup)
{
}

bool CPVRChannelGroupFromUser::UpdateFromClients(
    const std::vector<std::shared_ptr<CPVRClient>>& clients)
{
  // Nothing to update from any client, because there is none for local groups.
  return true;
}
