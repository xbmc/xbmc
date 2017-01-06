#pragma once
/*
 *      Copyright (C) 2005-2016 Team Kodi
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

extern "C"
{

/*
 * For interface between add-on and kodi.
 *
 * In this structure becomes the addresses of functions inside Kodi stored who
 * then available for the add-on to call.
 *
 * All function pointers there are used by the C++ interface functions below.
 * You find the set of them on xbmc/addons/interfaces/kodi/General.cpp
 *
 * Note: For add-on development itself not needed, thats why with '*' here in
 * text.
 */
typedef struct AddonToKodiFuncTable_kodi
{
  bool (*get_setting)(void* kodiBase, const char* settingName, void *settingValue);
  void (*open_settings_dialog)(void* kodiBase);
} AddonToKodiFuncTable_kodi;

} /* extern "C" */

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
  inline bool CheckSettingUInt(const std::string& settingName, unsigned int& settingValue)
  {
    return ::kodi::addon::CAddonBase::m_interface->toKodi.kodi->get_setting(::kodi::addon::CAddonBase::m_interface->toKodi.kodiBase, settingName.c_str(), &settingValue);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  ///
  inline unsigned int GetSettingUInt(const std::string& settingName)
  {
    unsigned int settingValue = 0;
    CheckSettingUInt(settingName, settingValue);
    return settingValue;
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
  inline void OpenSettings()
  {
    ::kodi::addon::CAddonBase::m_interface->toKodi.kodi->open_settings_dialog(::kodi::addon::CAddonBase::m_interface->toKodi.kodiBase);
  }
  //----------------------------------------------------------------------------

} /* namespace kodi */
