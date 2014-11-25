/*
 *      Copyright (C) 2013 Team XBMC
 *      http://xbmc.org
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

#include "Settings.h"
#include "JNIBase.h"
#include "jutils/jutils-details.hpp"

using namespace jni;

std::string CJNISettings::ACTION_ACCESSIBILITY_SETTINGS;
std::string CJNISettings::ACTION_ADD_ACCOUNT;
std::string CJNISettings::ACTION_AIRPLANE_MODE_SETTINGS;
std::string CJNISettings::ACTION_APN_SETTINGS;
std::string CJNISettings::ACTION_APPLICATION_DETAILS_SETTINGS;
std::string CJNISettings::ACTION_APPLICATION_DEVELOPMENT_SETTINGS;
std::string CJNISettings::ACTION_APPLICATION_SETTINGS;
std::string CJNISettings::ACTION_BLUETOOTH_SETTINGS;
std::string CJNISettings::ACTION_DATA_ROAMING_SETTINGS;
std::string CJNISettings::ACTION_DATE_SETTINGS;
std::string CJNISettings::ACTION_DEVICE_INFO_SETTINGS;
std::string CJNISettings::ACTION_DISPLAY_SETTINGS;
std::string CJNISettings::ACTION_INPUT_METHOD_SETTINGS;
std::string CJNISettings::ACTION_INPUT_METHOD_SUBTYPE_SETTINGS;
std::string CJNISettings::ACTION_INTERNAL_STORAGE_SETTINGS;
std::string CJNISettings::ACTION_LOCALE_SETTINGS;
std::string CJNISettings::ACTION_LOCATION_SOURCE_SETTINGS;
std::string CJNISettings::ACTION_MANAGE_ALL_APPLICATIONS_SETTINGS;
std::string CJNISettings::ACTION_MANAGE_APPLICATIONS_SETTINGS;
std::string CJNISettings::ACTION_MEMORY_CARD_SETTINGS;
std::string CJNISettings::ACTION_NETWORK_OPERATOR_SETTINGS;
std::string CJNISettings::ACTION_NFCSHARING_SETTINGS;
std::string CJNISettings::ACTION_PRIVACY_SETTINGS;
std::string CJNISettings::ACTION_QUICK_LAUNCH_SETTINGS;
std::string CJNISettings::ACTION_SEARCH_SETTINGS;
std::string CJNISettings::ACTION_SECURITY_SETTINGS;
std::string CJNISettings::ACTION_SETTINGS;
std::string CJNISettings::ACTION_SOUND_SETTINGS;
std::string CJNISettings::ACTION_SYNC_SETTINGS;
std::string CJNISettings::ACTION_USER_DICTIONARY_SETTINGS;
std::string CJNISettings::ACTION_WIFI_IP_SETTINGS;
std::string CJNISettings::ACTION_WIFI_SETTINGS;
std::string CJNISettings::ACTION_WIRELESS_SETTINGS;
std::string CJNISettings::AUTHORITY;

// API 16
std::string CJNISettings::ACTION_NFC_SETTINGS;
// API 18
std::string CJNISettings::ACTION_DREAM_SETTINGS;
// API 19
std::string CJNISettings::ACTION_CAPTIONING_SETTINGS;
std::string CJNISettings::ACTION_NFC_PAYMENT_SETTINGS;
std::string CJNISettings::ACTION_PRINT_SETTINGS;
// API 21
std::string CJNISettings::ACTION_CAST_SETTINGS;
std::string CJNISettings::ACTION_HOME_SETTINGS;
std::string CJNISettings::ACTION_SHOW_REGULATORY_INFO;
std::string CJNISettings::ACTION_USAGE_ACCESS_SETTINGS;
std::string CJNISettings::ACTION_VOICE_INPUT_SETTINGS;

void CJNISettings::PopulateStaticFields()
{
  int sdk = CJNIBase::GetSDKVersion();

  jhclass clazz = find_class("android/provider/Settings");
  ACTION_ACCESSIBILITY_SETTINGS = (jcast<std::string>(get_static_field<jhstring>(clazz, "ACTION_ACCESSIBILITY_SETTINGS")));
  ACTION_ADD_ACCOUNT = (jcast<std::string>(get_static_field<jhstring>(clazz, "ACTION_ADD_ACCOUNT")));
  ACTION_AIRPLANE_MODE_SETTINGS = (jcast<std::string>(get_static_field<jhstring>(clazz, "ACTION_AIRPLANE_MODE_SETTINGS")));
  ACTION_APN_SETTINGS = (jcast<std::string>(get_static_field<jhstring>(clazz, "ACTION_APN_SETTINGS")));
  ACTION_APPLICATION_DETAILS_SETTINGS = (jcast<std::string>(get_static_field<jhstring>(clazz, "ACTION_APPLICATION_DETAILS_SETTINGS")));
  ACTION_APPLICATION_DEVELOPMENT_SETTINGS = (jcast<std::string>(get_static_field<jhstring>(clazz, "ACTION_APPLICATION_DEVELOPMENT_SETTINGS")));
  ACTION_APPLICATION_SETTINGS = (jcast<std::string>(get_static_field<jhstring>(clazz, "ACTION_APPLICATION_SETTINGS")));
  ACTION_BLUETOOTH_SETTINGS = (jcast<std::string>(get_static_field<jhstring>(clazz, "ACTION_BLUETOOTH_SETTINGS")));
  ACTION_DATA_ROAMING_SETTINGS = (jcast<std::string>(get_static_field<jhstring>(clazz, "ACTION_DATA_ROAMING_SETTINGS")));
  ACTION_DATE_SETTINGS = (jcast<std::string>(get_static_field<jhstring>(clazz, "ACTION_DATE_SETTINGS")));
  ACTION_DEVICE_INFO_SETTINGS = (jcast<std::string>(get_static_field<jhstring>(clazz, "ACTION_DEVICE_INFO_SETTINGS")));
  ACTION_DISPLAY_SETTINGS = (jcast<std::string>(get_static_field<jhstring>(clazz, "ACTION_DISPLAY_SETTINGS")));
  ACTION_INPUT_METHOD_SETTINGS = (jcast<std::string>(get_static_field<jhstring>(clazz, "ACTION_INPUT_METHOD_SETTINGS")));
  ACTION_INPUT_METHOD_SUBTYPE_SETTINGS = (jcast<std::string>(get_static_field<jhstring>(clazz, "ACTION_INPUT_METHOD_SUBTYPE_SETTINGS")));
  ACTION_INTERNAL_STORAGE_SETTINGS = (jcast<std::string>(get_static_field<jhstring>(clazz, "ACTION_INTERNAL_STORAGE_SETTINGS")));
  ACTION_LOCALE_SETTINGS = (jcast<std::string>(get_static_field<jhstring>(clazz, "ACTION_LOCALE_SETTINGS")));
  ACTION_LOCATION_SOURCE_SETTINGS = (jcast<std::string>(get_static_field<jhstring>(clazz, "ACTION_LOCATION_SOURCE_SETTINGS")));
  ACTION_MANAGE_ALL_APPLICATIONS_SETTINGS = (jcast<std::string>(get_static_field<jhstring>(clazz, "ACTION_MANAGE_ALL_APPLICATIONS_SETTINGS")));
  ACTION_MANAGE_APPLICATIONS_SETTINGS = (jcast<std::string>(get_static_field<jhstring>(clazz, "ACTION_MANAGE_APPLICATIONS_SETTINGS")));
  ACTION_MEMORY_CARD_SETTINGS = (jcast<std::string>(get_static_field<jhstring>(clazz, "ACTION_MEMORY_CARD_SETTINGS")));
  ACTION_NETWORK_OPERATOR_SETTINGS = (jcast<std::string>(get_static_field<jhstring>(clazz, "ACTION_NETWORK_OPERATOR_SETTINGS")));
  ACTION_NFCSHARING_SETTINGS = (jcast<std::string>(get_static_field<jhstring>(clazz, "ACTION_NFCSHARING_SETTINGS")));
  ACTION_PRIVACY_SETTINGS = (jcast<std::string>(get_static_field<jhstring>(clazz, "ACTION_PRIVACY_SETTINGS")));
  ACTION_QUICK_LAUNCH_SETTINGS = (jcast<std::string>(get_static_field<jhstring>(clazz, "ACTION_QUICK_LAUNCH_SETTINGS")));
  ACTION_SEARCH_SETTINGS = (jcast<std::string>(get_static_field<jhstring>(clazz, "ACTION_SEARCH_SETTINGS")));
  ACTION_SECURITY_SETTINGS = (jcast<std::string>(get_static_field<jhstring>(clazz, "ACTION_SECURITY_SETTINGS")));
  ACTION_SETTINGS = (jcast<std::string>(get_static_field<jhstring>(clazz, "ACTION_SETTINGS")));
  ACTION_SOUND_SETTINGS = (jcast<std::string>(get_static_field<jhstring>(clazz, "ACTION_SOUND_SETTINGS")));
  ACTION_SYNC_SETTINGS = (jcast<std::string>(get_static_field<jhstring>(clazz, "ACTION_SYNC_SETTINGS")));
  ACTION_USER_DICTIONARY_SETTINGS = (jcast<std::string>(get_static_field<jhstring>(clazz, "ACTION_USER_DICTIONARY_SETTINGS")));
  ACTION_WIFI_IP_SETTINGS = (jcast<std::string>(get_static_field<jhstring>(clazz, "ACTION_WIFI_IP_SETTINGS")));
  ACTION_WIFI_SETTINGS = (jcast<std::string>(get_static_field<jhstring>(clazz, "ACTION_WIFI_SETTINGS")));
  ACTION_WIRELESS_SETTINGS = (jcast<std::string>(get_static_field<jhstring>(clazz, "ACTION_WIRELESS_SETTINGS")));
  AUTHORITY = (jcast<std::string>(get_static_field<jhstring>(clazz, "AUTHORITY")));

  if (sdk >= 16)
  {
    ACTION_NFC_SETTINGS = (jcast<std::string>(get_static_field<jhstring>(clazz, "ACTION_NFC_SETTINGS")));
  }

  if (sdk >= 18)
  {
    ACTION_DREAM_SETTINGS = (jcast<std::string>(get_static_field<jhstring>(clazz, "ACTION_DREAM_SETTINGS")));
  }

  if (sdk >= 19)
  {
    ACTION_CAPTIONING_SETTINGS = (jcast<std::string>(get_static_field<jhstring>(clazz, "ACTION_CAPTIONING_SETTINGS")));
    ACTION_NFC_PAYMENT_SETTINGS = (jcast<std::string>(get_static_field<jhstring>(clazz, "ACTION_NFC_PAYMENT_SETTINGS")));
    ACTION_PRINT_SETTINGS = (jcast<std::string>(get_static_field<jhstring>(clazz, "ACTION_PRINT_SETTINGS")));
  }

  if (sdk >= 21)
  {
    ACTION_CAST_SETTINGS = (jcast<std::string>(get_static_field<jhstring>(clazz, "ACTION_CAST_SETTINGS")));
    ACTION_HOME_SETTINGS = (jcast<std::string>(get_static_field<jhstring>(clazz, "ACTION_HOME_SETTINGS")));
    ACTION_SHOW_REGULATORY_INFO = (jcast<std::string>(get_static_field<jhstring>(clazz, "ACTION_SHOW_REGULATORY_INFO")));
    ACTION_USAGE_ACCESS_SETTINGS = (jcast<std::string>(get_static_field<jhstring>(clazz, "ACTION_USAGE_ACCESS_SETTINGS")));
    ACTION_VOICE_INPUT_SETTINGS = (jcast<std::string>(get_static_field<jhstring>(clazz, "ACTION_VOICE_INPUT_SETTINGS")));
  }
}

