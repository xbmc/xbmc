/*
 *  Copyright (C) 2012-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "pvr/settings/PVRSettings.h"
#include "settings/lib/ISettingCallback.h"

#include <memory>
#include <set>

namespace PVR
{

class IChannelGroupSettingsCallback
{
public:
  virtual ~IChannelGroupSettingsCallback() = default;

  virtual void UseBackendChannelOrderChanged() {}
  virtual void UseBackendChannelNumbersChanged() {}
  virtual void StartGroupChannelNumbersFromOneChanged() {}
};

class CPVRChannelGroupSettings : public ISettingCallback
{
public:
  CPVRChannelGroupSettings();
  virtual ~CPVRChannelGroupSettings();

  void OnSettingChanged(const std::shared_ptr<const CSetting>& setting) override;

  void RegisterCallback(IChannelGroupSettingsCallback* callback);
  void UnregisterCallback(IChannelGroupSettingsCallback* callback);

  bool UseBackendChannelOrder() const { return m_bUseBackendChannelOrder; }
  bool UseBackendChannelNumbers() const { return m_bUseBackendChannelNumbers; }
  bool StartGroupChannelNumbersFromOne() const { return m_bStartGroupChannelNumbersFromOne; }

private:
  bool UpdateUseBackendChannelOrder();
  bool UpdateUseBackendChannelNumbers();
  bool UpdateStartGroupChannelNumbersFromOne();

  bool m_bUseBackendChannelOrder = false;
  bool m_bUseBackendChannelNumbers = false;
  bool m_bStartGroupChannelNumbersFromOne = false;

  CPVRSettings m_settings;
  std::set<IChannelGroupSettingsCallback*> m_callbacks;
};

} // namespace PVR
