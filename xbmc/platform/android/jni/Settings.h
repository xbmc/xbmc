#pragma once
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

#include <string>

namespace jni
{

class CJNISettings
{
  public:
    static void PopulateStaticFields();

    static std::string ACTION_ACCESSIBILITY_SETTINGS;
    static std::string ACTION_ADD_ACCOUNT;
    static std::string ACTION_AIRPLANE_MODE_SETTINGS;
    static std::string ACTION_APN_SETTINGS;
    static std::string ACTION_APPLICATION_DETAILS_SETTINGS;
    static std::string ACTION_APPLICATION_DEVELOPMENT_SETTINGS;
    static std::string ACTION_APPLICATION_SETTINGS;
    static std::string ACTION_BLUETOOTH_SETTINGS;
    static std::string ACTION_DATA_ROAMING_SETTINGS;
    static std::string ACTION_DATE_SETTINGS;
    static std::string ACTION_DEVICE_INFO_SETTINGS;
    static std::string ACTION_DISPLAY_SETTINGS;
    static std::string ACTION_INPUT_METHOD_SETTINGS;
    static std::string ACTION_INPUT_METHOD_SUBTYPE_SETTINGS;
    static std::string ACTION_INTERNAL_STORAGE_SETTINGS;
    static std::string ACTION_LOCALE_SETTINGS;
    static std::string ACTION_LOCATION_SOURCE_SETTINGS;
    static std::string ACTION_MANAGE_ALL_APPLICATIONS_SETTINGS;
    static std::string ACTION_MANAGE_APPLICATIONS_SETTINGS;
    static std::string ACTION_MEMORY_CARD_SETTINGS;
    static std::string ACTION_NETWORK_OPERATOR_SETTINGS;
    static std::string ACTION_NFCSHARING_SETTINGS;
    static std::string ACTION_PRIVACY_SETTINGS;
    static std::string ACTION_QUICK_LAUNCH_SETTINGS;
    static std::string ACTION_SEARCH_SETTINGS;
    static std::string ACTION_SECURITY_SETTINGS;
    static std::string ACTION_SETTINGS;
    static std::string ACTION_SOUND_SETTINGS;
    static std::string ACTION_SYNC_SETTINGS;
    static std::string ACTION_USER_DICTIONARY_SETTINGS;
    static std::string ACTION_WIFI_IP_SETTINGS;
    static std::string ACTION_WIFI_SETTINGS;
    static std::string ACTION_WIRELESS_SETTINGS;
    static std::string AUTHORITY;

    // API 16
    static std::string ACTION_NFC_SETTINGS;
    // API 18
    static std::string ACTION_DREAM_SETTINGS;
    // API 19
    static std::string ACTION_CAPTIONING_SETTINGS;
    static std::string ACTION_NFC_PAYMENT_SETTINGS;
    static std::string ACTION_PRINT_SETTINGS;
    // API 21
    static std::string ACTION_CAST_SETTINGS;
    static std::string ACTION_HOME_SETTINGS;
    static std::string ACTION_SHOW_REGULATORY_INFO;
    static std::string ACTION_USAGE_ACCESS_SETTINGS;
    static std::string ACTION_VOICE_INPUT_SETTINGS;

};
}

