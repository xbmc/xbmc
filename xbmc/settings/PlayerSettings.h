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
                                                 int& current);
  static void SettingOptionsQueueDataSizesFiller(const SettingConstPtr& setting,
                                                 std::vector<IntegerSettingOption>& list,
                                                 int& current);
  static void SettingOptionsFastForwardSpeeds(const SettingConstPtr& setting,
                                              std::vector<IntegerSettingOption>& list,
                                              int& current);

  //! \brief The ratios a viewer may state their room rests at, which are the ones Kodi can
  //! label. Keyed in hundredths, so a stored choice survives a change to the vocabulary file.
  static void SettingOptionsRasterAspectRatios(const SettingConstPtr& setting,
                                               std::vector<IntegerSettingOption>& list,
                                               int& current);
};
