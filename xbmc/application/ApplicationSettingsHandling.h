/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "settings/ISubSettings.h"
#include "settings/lib/ISettingCallback.h"
#include "settings/lib/ISettingsHandler.h"

class CApplicationPlayer;
class CApplicationPowerHandling;
class CApplicationVolumeHandling;
class CApplicationSkinHandling;

/*!
 * \brief Class handling application support for settings.
 */

class CApplicationSettingsHandling : public ISettingCallback,
                                     public ISettingsHandler,
                                     public ISubSettings
{
public:
  CApplicationSettingsHandling(CApplicationPlayer& appPlayer,
                               CApplicationPowerHandling& powerHandling,
                               CApplicationSkinHandling& skinHandling,
                               CApplicationVolumeHandling& volumeHandling);

protected:
  void RegisterSettings();
  void UnregisterSettings();

  bool Load(const TiXmlNode* settings) override;
  bool Save(TiXmlNode* settings) const override;
  void OnSettingChanged(const std::shared_ptr<const CSetting>& setting) override;
  void OnSettingAction(const std::shared_ptr<const CSetting>& setting) override;
  bool OnSettingUpdate(const std::shared_ptr<CSetting>& setting,
                       const char* oldSettingId,
                       const TiXmlNode* oldSettingNode) override;

  CApplicationPlayer& m_appPlayerRef;
  CApplicationPowerHandling& m_powerHandling;
  CApplicationSkinHandling& m_skinHandling;
  CApplicationVolumeHandling& m_volumeHandling;
};
