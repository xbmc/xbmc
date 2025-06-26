/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PlayerSettings.h"

#include "guilib/LocalizeStrings.h"
#include "utils/StringUtils.h"

void CPlayerSettings::SettingOptionsQueueTimeSizesFiller(const SettingConstPtr& setting,
                                                         std::vector<IntegerSettingOption>& list,
                                                         int& current)
{
  const std::string& secFloat = g_localizeStrings.Get(13553);
  const std::string& seconds = g_localizeStrings.Get(37129);
  const std::string& second = g_localizeStrings.Get(37128);

  list.emplace_back(StringUtils::Format(secFloat, 0.5), 5);
  list.emplace_back(StringUtils::Format(second, 1), 10);
  list.emplace_back(StringUtils::Format(seconds, 2), 20);
  list.emplace_back(StringUtils::Format(seconds, 4), 40);
  list.emplace_back(StringUtils::Format(seconds, 8), 80);
  list.emplace_back(StringUtils::Format(seconds, 16), 160);
}

void CPlayerSettings::SettingOptionsQueueDataSizesFiller(const SettingConstPtr& /*setting*/,
                                                         std::vector<IntegerSettingOption>& list,
                                                         int& /*current*/)
{
  const std::string& mb = g_localizeStrings.Get(37122);
  const std::string& gb = g_localizeStrings.Get(37123);

  list.emplace_back(StringUtils::Format(mb, 16), 16);
  list.emplace_back(StringUtils::Format(mb, 32), 32);
  list.emplace_back(StringUtils::Format(mb, 64), 64);
  list.emplace_back(StringUtils::Format(mb, 128), 128);
  list.emplace_back(StringUtils::Format(mb, 256), 256);
  list.emplace_back(StringUtils::Format(mb, 512), 512);
  list.emplace_back(StringUtils::Format(gb, 1), 1024);
}
