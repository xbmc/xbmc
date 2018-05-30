/*
 *      Copyright (C) 2013 Team XBMC
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include <vector>
#include <memory>

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
