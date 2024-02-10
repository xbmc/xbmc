/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ServicesSettings.h"

#include "filesystem/IFileTypes.h"
#include "guilib/LocalizeStrings.h"
#include "utils/StringUtils.h"

using namespace XFILE;

void CServicesSettings::SettingOptionsChunkSizesFiller(const SettingConstPtr& setting,
                                                       std::vector<IntegerSettingOption>& list,
                                                       int& current,
                                                       void* data)
{
  const auto& kb = g_localizeStrings.Get(37121);
  const auto& mb = g_localizeStrings.Get(37122);

  list.emplace_back(StringUtils::Format(kb, 16), 16);
  list.emplace_back(StringUtils::Format(kb, 32), 32);
  list.emplace_back(StringUtils::Format(kb, 64), 64);
  list.emplace_back(StringUtils::Format(kb, 128), 128);
  list.emplace_back(StringUtils::Format(kb, 256), 256);
  list.emplace_back(StringUtils::Format(kb, 512), 512);
  list.emplace_back(StringUtils::Format(mb, 1), 1024);
}

void CServicesSettings::SettingOptionsBufferModesFiller(const SettingConstPtr& setting,
                                                        std::vector<IntegerSettingOption>& list,
                                                        int& current,
                                                        void* data)
{
  list.emplace_back(g_localizeStrings.Get(37110), CACHE_BUFFER_MODE_NONE);
  list.emplace_back(g_localizeStrings.Get(37111), CACHE_BUFFER_MODE_TRUE_INTERNET);
  list.emplace_back(g_localizeStrings.Get(37112), CACHE_BUFFER_MODE_INTERNET);
  list.emplace_back(g_localizeStrings.Get(37113), CACHE_BUFFER_MODE_NETWORK);
  list.emplace_back(g_localizeStrings.Get(37114), CACHE_BUFFER_MODE_ALL);
}

void CServicesSettings::SettingOptionsMemorySizesFiller(const SettingConstPtr& setting,
                                                        std::vector<IntegerSettingOption>& list,
                                                        int& current,
                                                        void* data)
{
  const auto& mb = g_localizeStrings.Get(37122);
  const auto& gb = g_localizeStrings.Get(37123);

  list.emplace_back(StringUtils::Format(mb, 16), 16);
  list.emplace_back(StringUtils::Format(mb, 20), 20);
  list.emplace_back(StringUtils::Format(mb, 24), 24);
  list.emplace_back(StringUtils::Format(mb, 32), 32);
  list.emplace_back(StringUtils::Format(mb, 48), 48);
  list.emplace_back(StringUtils::Format(mb, 64), 64);
  list.emplace_back(StringUtils::Format(mb, 96), 96);
  list.emplace_back(StringUtils::Format(mb, 128), 128);
  list.emplace_back(StringUtils::Format(mb, 192), 192);
  list.emplace_back(StringUtils::Format(mb, 256), 256);
  list.emplace_back(StringUtils::Format(mb, 384), 384);
  list.emplace_back(StringUtils::Format(mb, 512), 512);
  list.emplace_back(StringUtils::Format(mb, 768), 768);
  list.emplace_back(StringUtils::Format(gb, 1), 1024);
  list.emplace_back(g_localizeStrings.Get(37115), 0);
}

void CServicesSettings::SettingOptionsReadFactorsFiller(const SettingConstPtr& setting,
                                                        std::vector<IntegerSettingOption>& list,
                                                        int& current,
                                                        void* data)
{
  list.emplace_back(g_localizeStrings.Get(37116), 0);
  list.emplace_back("1.1x", 110);
  list.emplace_back("1.25x", 125);
  list.emplace_back("1.5x", 150);
  list.emplace_back("1.75x", 175);
  list.emplace_back("2x", 200);
  list.emplace_back("2.5x", 250);
  list.emplace_back("3x", 300);
  list.emplace_back("4x", 400);
  list.emplace_back("5x", 500);
  list.emplace_back("7x", 700);
  list.emplace_back("10x", 1000);
  list.emplace_back("15x", 1500);
  list.emplace_back("20x", 2000);
  list.emplace_back("30x", 3000);
  list.emplace_back("50x", 5000);
}

void CServicesSettings::SettingOptionsCacheChunkSizesFiller(const SettingConstPtr& setting,
                                                            std::vector<IntegerSettingOption>& list,
                                                            int& current,
                                                            void* data)
{
  const auto& byte = g_localizeStrings.Get(37120);
  const auto& kb = g_localizeStrings.Get(37121);
  const auto& mb = g_localizeStrings.Get(37122);

  list.emplace_back(StringUtils::Format(byte, 256), 256);
  list.emplace_back(StringUtils::Format(byte, 512), 512);
  list.emplace_back(StringUtils::Format(kb, 1), 1024);
  list.emplace_back(StringUtils::Format(kb, 2), 2 * 1024);
  list.emplace_back(StringUtils::Format(kb, 4), 4 * 1024);
  list.emplace_back(StringUtils::Format(kb, 8), 8 * 1024);
  list.emplace_back(StringUtils::Format(kb, 16), 16 * 1024);
  list.emplace_back(StringUtils::Format(kb, 32), 32 * 1024);
  list.emplace_back(StringUtils::Format(kb, 64), 64 * 1024);
  list.emplace_back(StringUtils::Format(kb, 128), 128 * 1024);
  list.emplace_back(StringUtils::Format(kb, 256), 256 * 1024);
  list.emplace_back(StringUtils::Format(kb, 512), 512 * 1024);
  list.emplace_back(StringUtils::Format(mb, 1), 1024 * 1024);
}
