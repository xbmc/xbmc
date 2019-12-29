/*
 *  Copyright (C) 2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "settings/lib/ISettingCallback.h"

class IOSSettingsHandler : public ISettingCallback
{
public:
  IOSSettingsHandler();
  virtual ~IOSSettingsHandler();

  void OnSettingAction(std::shared_ptr<const CSetting> setting) override;
};
