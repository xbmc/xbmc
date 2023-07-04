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

namespace tinyxml2
{
class XMLNode;
}

/*!
 * \brief Class handling application support for settings.
 */

class CApplicationSettingsHandling : public ISettingCallback,
                                     public ISettingsHandler,
                                     public ISubSettings
{
protected:
  void RegisterSettings();
  void UnregisterSettings();

  bool Load(const tinyxml2::XMLNode* settings) override;
  bool Save(tinyxml2::XMLNode* settings) const override;
  void OnSettingChanged(const std::shared_ptr<const CSetting>& setting) override;
  void OnSettingAction(const std::shared_ptr<const CSetting>& setting) override;
  bool OnSettingUpdate(const std::shared_ptr<CSetting>& setting,
                       const char* oldSettingId,
                       const tinyxml2::XMLNode* oldSettingNode) override;
};
