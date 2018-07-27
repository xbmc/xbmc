/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "interfaces/IActionListener.h"
#include "settings/lib/ISettingCallback.h"

namespace PVR
{

enum class ChannelSwitchMode;

class CPVRActionListener : public IActionListener, public ISettingCallback
{
public:
  CPVRActionListener();
  ~CPVRActionListener() override;

  // IActionListener implementation
  bool OnAction(const CAction &action) override;

  // ISettingCallback implementation
  void OnSettingChanged(std::shared_ptr<const CSetting> setting) override;
  void OnSettingAction(std::shared_ptr<const CSetting> setting) override;

private:
  CPVRActionListener(const CPVRActionListener&) = delete;
  CPVRActionListener& operator=(const CPVRActionListener&) = delete;

  static ChannelSwitchMode GetChannelSwitchMode(int iAction);
};

} // namespace PVR
