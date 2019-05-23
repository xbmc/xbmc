/*
 *  Copyright (C) 2012-2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>
#include <vector>

namespace PVR
{

class CPVRChannelGroup;

class CPVRGUIChannelIconUpdater
{
public:
  /*!
   * @brief ctor.
   * @param groups The channel groups for which the channel icons shall be updated.
   * @param bUpdateDb If true, persist the changed values in the PVR database.
   */
  CPVRGUIChannelIconUpdater(const std::vector<std::shared_ptr<CPVRChannelGroup>>& groups, bool bUpdateDb)
  : m_groups(groups), m_bUpdateDb(bUpdateDb) {}

  CPVRGUIChannelIconUpdater() = delete;
  CPVRGUIChannelIconUpdater(const CPVRGUIChannelIconUpdater&) = delete;
  CPVRGUIChannelIconUpdater& operator=(const CPVRGUIChannelIconUpdater&) = delete;

  virtual ~CPVRGUIChannelIconUpdater() = default;

  /*!
   * @brief Search and update missing channel icons.
   */
  void SearchAndUpdateMissingChannelIcons() const;

private:
  const std::vector<std::shared_ptr<CPVRChannelGroup>> m_groups;
  const bool m_bUpdateDb = false;
};

}
