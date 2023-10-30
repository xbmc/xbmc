/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ServicesSettings.h"

void CServicesSettings::SettingOptionsChunkSizesFiller(const SettingConstPtr& setting,
                                                       std::vector<IntegerSettingOption>& list,
                                                       int& current,
                                                       void* data)
{
  list.emplace_back("16 KB", 16);
  list.emplace_back("32 KB", 32);
  list.emplace_back("64 KB", 64);
  list.emplace_back("128 KB", 128);
  list.emplace_back("256 KB", 256);
  list.emplace_back("512 KB", 512);
  list.emplace_back("1 MB", 1024);
}
