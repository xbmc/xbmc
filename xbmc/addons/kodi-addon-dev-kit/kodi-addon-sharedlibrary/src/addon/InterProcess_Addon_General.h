#pragma once
/*
 *      Copyright (C) 2016 Team KODI
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
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "kodi/api2/AddonLib.hpp"
#include "kodi/api2/.internal/AddonLib_internal.hpp"

#include <string>

extern "C"
{

  struct CKODIAddon_InterProcess_Addon_General
  {
    bool GetSettingString(const std::string& settingName, std::string& settingValue, bool global);
    bool GetSettingBoolean(const std::string& settingName, bool* settingValue, bool global);
    bool GetSettingInt(const std::string& settingName, int* settingValue, bool global);
    bool GetSettingFloat(const std::string& settingName, float* settingValue, bool global);
    void OpenSettings();
    std::string GetAddonInfo(const std::string& id) const;
    void QueueFormattedNotification(const queue_msg type, const char* aDescription);
    void QueueNotification(
      const queue_msg     type,
      const std::string&  aCaption,
      const std::string&  aDescription,
      unsigned int        displayTime,
      bool                withSound,
      unsigned int        messageTime);
    void QueueNotification(const std::string& aCaption, const std::string& aDescription);
    void QueueNotification(
      const std::string&  aImageFile,
      const std::string&  aCaption,
      const std::string&  aDescription,
      unsigned int        displayTime,
      bool                withSound,
      unsigned int        messageTime);
    void GetMD5(const std::string& text, std::string& md5);
    bool UnknownToUTF8(const std::string& stringSrc, std::string& utf8StringDst, bool failOnBadChar);
    std::string GetLocalizedString(uint32_t labelId, const std::string& strDefault = "");
    std::string GetLanguage(lang_formats format, bool region);
    std::string GetDVDMenuLanguage();
    bool StartServer(eservers typ, bool start, bool wait);
    void AudioSuspend();
    void AudioResume();
    float GetVolume(bool percentage);
    void SetVolume(float value, bool isPercentage);
    bool IsMuted();
    void ToggleMute();
    void SetMute(bool mute);
    dvd_state GetOpticalDriveState();
    bool EjectOpticalDrive();
    void KodiVersion(kodi_version& version);
    void KodiQuit();
    void HTPCShutdown();
    void HTPCRestart();
    void ExecuteScript(const std::string& script);
    void ExecuteBuiltin(const std::string& function, bool wait);
    std::string ExecuteJSONRPC(const std::string& jsonrpccommand);
    std::string GetRegion(const std::string& id);
    long GetFreeMem();
    int GetGlobalIdleTime();
    std::string GetAddonInfo(const std::string& id);
    std::string TranslatePath(const std::string& path);
  };

}; /* extern "C" */
