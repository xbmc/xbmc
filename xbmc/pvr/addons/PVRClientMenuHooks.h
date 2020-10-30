/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

struct PVR_MENUHOOK;

namespace PVR
{
  class CPVRClientMenuHook
  {
  public:
    CPVRClientMenuHook() = delete;
    virtual ~CPVRClientMenuHook() = default;

    CPVRClientMenuHook(const std::string& addonId, const PVR_MENUHOOK& hook);

    bool operator ==(const CPVRClientMenuHook& right) const;

    bool IsAllHook() const;
    bool IsChannelHook() const;
    bool IsTimerHook() const;
    bool IsEpgHook() const;
    bool IsRecordingHook() const;
    bool IsDeletedRecordingHook() const;
    bool IsSettingsHook() const;

    unsigned int GetId() const;
    unsigned int GetLabelId() const;
    std::string GetLabel() const;

  private:
    std::string m_addonId;
    std::shared_ptr<PVR_MENUHOOK> m_hook;
  };

  class CPVRClientMenuHooks
  {
  public:
    CPVRClientMenuHooks() = default;
    virtual ~CPVRClientMenuHooks() = default;

    explicit CPVRClientMenuHooks(const std::string& addonId) : m_addonId(addonId) {}

    void AddHook(const PVR_MENUHOOK& addonHook);
    void Clear();

    std::vector<CPVRClientMenuHook> GetChannelHooks() const;
    std::vector<CPVRClientMenuHook> GetTimerHooks() const;
    std::vector<CPVRClientMenuHook> GetEpgHooks() const;
    std::vector<CPVRClientMenuHook> GetRecordingHooks() const;
    std::vector<CPVRClientMenuHook> GetDeletedRecordingHooks() const;
    std::vector<CPVRClientMenuHook> GetSettingsHooks() const;

  private:
    std::vector<CPVRClientMenuHook> GetHooks(
        const std::function<bool(const CPVRClientMenuHook& hook)>& function) const;

    std::string m_addonId;
    std::unique_ptr<std::vector<CPVRClientMenuHook>> m_hooks;
  };
}
