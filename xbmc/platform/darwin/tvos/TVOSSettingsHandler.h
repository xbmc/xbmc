/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "settings/lib/ISettingCallback.h"

class CTVOSInputSettings : public ISettingCallback
{
 public:
  static CTVOSInputSettings& GetInstance();

  void Initialize();

    virtual void OnSettingChanged(std::shared_ptr<const CSetting> setting) override;

private:
  CTVOSInputSettings();
  CTVOSInputSettings(CTVOSInputSettings const& );
  CTVOSInputSettings& operator=(CTVOSInputSettings const&);

  static CTVOSInputSettings* m_instance;
};
