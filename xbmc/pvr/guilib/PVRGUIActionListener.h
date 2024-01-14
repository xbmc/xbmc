/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "input/actions/interfaces/IActionListener.h"
#include "settings/lib/ISettingCallback.h"

namespace PVR
{

class CPVRManager;
enum class ChannelSwitchMode;
enum class PVREvent;

class CPVRGUIActionListener : public KODI::ACTION::IActionListener, public ISettingCallback
{
public:
  CPVRGUIActionListener();
  ~CPVRGUIActionListener() override;

  void Init(CPVRManager& mgr);
  void Deinit(CPVRManager& mgr);

  // IActionListener implementation
  bool OnAction(const CAction& action) override;

  // ISettingCallback implementation
  void OnSettingChanged(const std::shared_ptr<const CSetting>& setting) override;
  void OnSettingAction(const std::shared_ptr<const CSetting>& setting) override;

  void OnPVRManagerEvent(const PVREvent& event);

private:
  CPVRGUIActionListener(const CPVRGUIActionListener&) = delete;
  CPVRGUIActionListener& operator=(const CPVRGUIActionListener&) = delete;

  static ChannelSwitchMode GetChannelSwitchMode(int iAction);
};

} // namespace PVR
