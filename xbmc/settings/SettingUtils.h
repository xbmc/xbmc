/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>
#include <vector>

class CVariant;
class CSettingList;
class CSetting;

class CSettingUtils
{
public:
  /*!
   \brief Gets the values of the given list setting.

   \param settingList List setting
   \return List of values of the given list setting
   */
  static std::vector<CVariant> GetList(std::shared_ptr<const CSettingList> settingList);
  /*!
   \brief Sets the values of the given list setting.

   \param settingList List setting
   \param value Values to set
   \return True if setting the values was successful, false otherwise
   */
  static bool SetList(std::shared_ptr<CSettingList> settingList, const std::vector<CVariant> &value);

  static std::vector<CVariant> ListToValues(std::shared_ptr<const CSettingList> setting, const std::vector< std::shared_ptr<CSetting> > &values);
  static bool ValuesToList(std::shared_ptr<const CSettingList> setting, const std::vector<CVariant> &values, std::vector< std::shared_ptr<CSetting> > &newValues);
};
