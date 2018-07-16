/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>
#include <string>

class CSetting;
class CSettingsManager;

/*!
 \ingroup settings
 \brief Interface for creating a new setting of a custom setting type.
 */
class ISettingCreator
{
public:
  virtual ~ISettingCreator() = default;

  /*!
   \brief Creates a new setting of the given custom setting type.

   \param settingType string representation of the setting type
   \param settingId Identifier of the setting to be created
   \param settingsManager Reference to the settings manager
   \return A new setting object of the given (custom) setting type or nullptr if the setting type is unknown
   */
  virtual std::shared_ptr<CSetting> CreateSetting(const std::string &settingType, const std::string &settingId, CSettingsManager *settingsManager = nullptr) const = 0;
};
