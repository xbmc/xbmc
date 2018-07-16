/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/**
* Playback settings
*/
enum BDPlaybackMode
{
  BD_PLAYBACK_SIMPLE_MENU = 0,
  BD_PLAYBACK_DISC_MENU,
  BD_PLAYBACK_MAIN_TITLE,
};

#include "settings/lib/ISettingCallback.h"

class CDiscSettings : public ISettingCallback
{
public:
  /* ISettingCallback*/

  static CDiscSettings& GetInstance();
  void OnSettingChanged(std::shared_ptr<const CSetting> setting) override;

private:
  CDiscSettings() = default;
  ~CDiscSettings() = default;
};
