#pragma once
/*
 *      Copyright (C) 2005-2017 Team Kodi
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "AddonBase.h"
#include "definitions.h"

namespace kodi
{
  //============================================================================
  ///
  inline bool CheckSettingString(const std::string& settingName, std::string& settingValue)
  {
    char * buffer = (char*) malloc(1024);
    buffer[0] = 0; /* Set the end of string */
    bool ret = ::kodi::addon::CAddonBase::m_interface->toKodi.kodi->get_setting(::kodi::addon::CAddonBase::m_interface->toKodi.kodiBase, settingName.c_str(), buffer);
    if (ret)
      settingValue = buffer;
    free(buffer);
    return ret;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  ///
  inline std::string GetSettingString(const std::string& settingName)
  {
    std::string settingValue;
    CheckSettingString(settingName, settingValue);
    return settingValue;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  ///
  inline void SetSettingString(const std::string& settingName, const std::string& settingValue)
  {
    ::kodi::addon::CAddonBase::m_interface->toKodi.kodi->set_setting(::kodi::addon::CAddonBase::m_interface->toKodi.kodiBase, settingName.c_str(), settingValue.c_str());
  }
  //----------------------------------------------------------------------------

  //============================================================================
  ///
  inline bool CheckSettingInt(const std::string& settingName, int& settingValue)
  {
    return ::kodi::addon::CAddonBase::m_interface->toKodi.kodi->get_setting(::kodi::addon::CAddonBase::m_interface->toKodi.kodiBase, settingName.c_str(), &settingValue);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  ///
  inline int GetSettingInt(const std::string& settingName)
  {
    int settingValue = 0;
    CheckSettingInt(settingName, settingValue);
    return settingValue;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  ///
  inline void SetSettingInt(const std::string& settingName, int settingValue)
  {
    char buffer[33];
    snprintf(buffer, sizeof(buffer), "%i", settingValue);
    ::kodi::addon::CAddonBase::m_interface->toKodi.kodi->set_setting(::kodi::addon::CAddonBase::m_interface->toKodi.kodiBase, settingName.c_str(), buffer);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  ///
  inline bool CheckSettingBoolean(const std::string& settingName, bool& settingValue)
  {
    return ::kodi::addon::CAddonBase::m_interface->toKodi.kodi->get_setting(::kodi::addon::CAddonBase::m_interface->toKodi.kodiBase, settingName.c_str(), &settingValue);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  ///
  inline bool GetSettingBoolean(const std::string& settingName)
  {
    bool settingValue = false;
    CheckSettingBoolean(settingName, settingValue);
    return settingValue;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  ///
  inline void SetSettingBoolean(const std::string& settingName, bool settingValue)
  {
    ::kodi::addon::CAddonBase::m_interface->toKodi.kodi->set_setting(::kodi::addon::CAddonBase::m_interface->toKodi.kodiBase, settingName.c_str(), settingValue ? "true" : "false");
  }
  //----------------------------------------------------------------------------

  //============================================================================
  ///
  inline bool CheckSettingFloat(const std::string& settingName, float& settingValue)
  {
    return ::kodi::addon::CAddonBase::m_interface->toKodi.kodi->get_setting(::kodi::addon::CAddonBase::m_interface->toKodi.kodiBase, settingName.c_str(), &settingValue);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  ///
  inline float GetSettingFloat(const std::string& settingName)
  {
    float settingValue = 0.0f;
    CheckSettingFloat(settingName, settingValue);
    return settingValue;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  ///
  inline void SetSettingFloat(const std::string& settingName, float settingValue)
  {
    char buffer[50];
    snprintf(buffer, sizeof(buffer), "%f", settingValue);
    ::kodi::addon::CAddonBase::m_interface->toKodi.kodi->set_setting(::kodi::addon::CAddonBase::m_interface->toKodi.kodiBase, settingName.c_str(), buffer);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  ///
  inline void OpenSettings()
  {
    ::kodi::addon::CAddonBase::m_interface->toKodi.kodi->open_settings_dialog(::kodi::addon::CAddonBase::m_interface->toKodi.kodiBase);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  ///
  inline std::string GetLocalizedString(uint32_t labelId, const std::string& defaultStr = "")
  {
    std::string retString = defaultStr;
    char* strMsg = ::kodi::addon::CAddonBase::m_interface->toKodi.kodi->get_localized_string(::kodi::addon::CAddonBase::m_interface->toKodi.kodiBase, labelId);
    if (strMsg != nullptr)
    {
      if (std::strlen(strMsg))
        retString = strMsg;
      ::kodi::addon::CAddonBase::m_interface->toKodi.free_string(::kodi::addon::CAddonBase::m_interface->toKodi.kodiBase,strMsg);
    }
    return retString;
  }
  //----------------------------------------------------------------------------

} /* namespace kodi */
