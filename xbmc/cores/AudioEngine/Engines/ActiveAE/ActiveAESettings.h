/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "settings/lib/ISettingCallback.h"
#include "threads/CriticalSection.h"

#include <atomic>
#include <memory>
#include <string>
#include <vector>

class CSetting;
class CAEStreamInfo;
struct IntegerSettingOption;
struct StringSettingOption;

namespace ActiveAE
{
class CActiveAE;

class CActiveAESettings : public ISettingCallback
{
public:
  CActiveAESettings(CActiveAE &ae);
  ~CActiveAESettings() override;

  void OnSettingChanged(const std::shared_ptr<const CSetting>& setting) override;

  static void SettingOptionsAudioDevicesFiller(const std::shared_ptr<const CSetting>& setting,
                                               std::vector<StringSettingOption>& list,
                                               std::string& current,
                                               void* data);
  static void SettingOptionsAudioDevicesPassthroughFiller(
      const std::shared_ptr<const CSetting>& setting,
      std::vector<StringSettingOption>& list,
      std::string& current,
      void* data);
  static void SettingOptionsAudioQualityLevelsFiller(const std::shared_ptr<const CSetting>& setting,
                                                     std::vector<IntegerSettingOption>& list,
                                                     int& current,
                                                     void* data);
  static void SettingOptionsAudioStreamsilenceFiller(const std::shared_ptr<const CSetting>& setting,
                                                     std::vector<IntegerSettingOption>& list,
                                                     int& current,
                                                     void* data);
  static bool IsSettingVisible(const std::string& condition,
                               const std::string& value,
                               const std::shared_ptr<const CSetting>& setting,
                               void* data);

protected:
  static void SettingOptionsAudioDevicesFillerGeneral(
      const std::shared_ptr<const CSetting>& setting,
      std::vector<StringSettingOption>& list,
      std::string& current,
      bool passthrough);

  CActiveAE &m_audioEngine;
  CCriticalSection m_cs;
  static CActiveAESettings* m_instance;
};
};
