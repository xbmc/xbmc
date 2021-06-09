/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "settings/lib/ISettingCallback.h"
#include "settings/lib/ISettingsHandler.h"
#include "threads/CriticalSection.h"

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

class CSetting;

struct IntegerSettingOption;

namespace PVR
{
  class CPVRSettings : private ISettingsHandler, private ISettingCallback
  {
  public:
    explicit CPVRSettings(const std::set<std::string>& settingNames);
    ~CPVRSettings() override;

    void RegisterCallback(ISettingCallback* callback);
    void UnregisterCallback(ISettingCallback* callback);

    // ISettingsHandler implementation
    void OnSettingsLoaded() override;

    // ISettingCallback implementation
    void OnSettingChanged(const std::shared_ptr<const CSetting>& setting) override;

    bool GetBoolValue(const std::string& settingName) const;
    int GetIntValue(const std::string& settingName) const;
    std::string GetStringValue(const std::string& settingName) const;

    // settings value filler for start/end recording margin time for PVR timers.
    static void MarginTimeFiller(const std::shared_ptr<const CSetting>& setting,
                                 std::vector<IntegerSettingOption>& list,
                                 int& current,
                                 void* data);

    // Dynamically hide or show settings.
    static bool IsSettingVisible(const std::string& condition,
                                 const std::string& value,
                                 const std::shared_ptr<const CSetting>& setting,
                                 void* data);

    // Do parental PIN check.
    static bool CheckParentalPin(const std::string& condition,
                                 const std::string& value,
                                 const std::shared_ptr<const CSetting>& setting,
                                 void* data);

  private:
    CPVRSettings(const CPVRSettings&) = delete;
    CPVRSettings& operator=(CPVRSettings const&) = delete;

    void Init(const std::set<std::string>& settingNames);

    mutable CCriticalSection m_critSection;
    std::map<std::string, std::shared_ptr<CSetting>> m_settings;
    std::set<ISettingCallback*> m_callbacks;

    static unsigned int m_iInstances;
  };
}
