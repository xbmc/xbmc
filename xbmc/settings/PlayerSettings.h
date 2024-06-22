/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "settings/ISubSettings.h"
#include "settings/lib/Setting.h"

#include <vector>

class CPlayerSettings : public ISubSettings
{
public:
  static void SettingOptionsQueueTimeSizesFiller(const SettingConstPtr& setting,
                                                 std::vector<IntegerSettingOption>& list,
                                                 int& current,
                                                 void* data);
  static void SettingOptionsQueueDataSizesFiller(const SettingConstPtr& setting,
                                                 std::vector<IntegerSettingOption>& list,
                                                 int& current,
                                                 void* data);
};
