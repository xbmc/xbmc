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

#include "InterProcess.h"
#include "kodi/api2/addon/General.hpp"

#include <string>
#include <stdarg.h>

namespace V2
{
namespace KodiAPI
{

namespace AddOn
{
namespace General
{

  bool GetSettingString(
      const std::string& settingName,
      std::string&       settingValue,
      bool               global)
  {
    return g_interProcess.GetSettingString(settingName, settingValue, global);
  }

  bool GetSettingInt(
      const std::string& settingName,
      int&               settingValue,
      bool               global)
  {
    return g_interProcess.GetSettingInt(settingName, &settingValue, global);
  }

  bool GetSettingBoolean(
      const std::string& settingName,
      bool&              settingValue,
      bool               global)
  {
    return g_interProcess.GetSettingBoolean(settingName, &settingValue, global);
  }

  bool GetSettingFloat(
      const std::string& settingName,
      float&             settingValue,
      bool               global)
  {
    return g_interProcess.GetSettingFloat(settingName, &settingValue, global);
  }

  void OpenSettings()
  {
    return g_interProcess.OpenSettings();
  }

  void QueueFormattedNotification(const queue_msg type, const char *format, ... )
  {
    va_list args;
    char buffer[16384];
    va_start(args, format);
    vsprintf(buffer, format, args);
    va_end(args);
    g_interProcess.QueueFormattedNotification(type, buffer);
  }

  void QueueNotification(
      const queue_msg  type,
      const std::string& aCaption,
      const std::string& aDescription,
      unsigned int       displayTime,
      bool               withSound,
      unsigned int       messageTime)
  {
    g_interProcess.QueueNotification(
                            type, aCaption, aDescription,
                            displayTime, withSound, messageTime);
  }

  void QueueNotification(
    const std::string& aCaption,
    const std::string& aDescription)
  {
    g_interProcess.QueueNotification(aCaption, aDescription);
  }

  void QueueNotification(
      const std::string& aImageFile,
      const std::string& aCaption,
      const std::string& aDescription,
      unsigned int       displayTime,
      bool               withSound,
      unsigned int       messageTime)
  {
    g_interProcess.QueueNotification(
                            aImageFile, aCaption, aDescription,
                            displayTime, withSound, messageTime);
  }

  void GetMD5(const std::string& text, std::string& md5)
  {
    g_interProcess.GetMD5(text, md5);
  }

  bool UnknownToUTF8(
      const std::string&  stringSrc,
      std::string&        utf8StringDst,
      bool                failOnBadChar)
  {
    return g_interProcess.UnknownToUTF8(stringSrc, utf8StringDst, failOnBadChar);
  }

  std::string GetLocalizedString(uint32_t labelId, const std::string& strDefault)
  {
    return g_interProcess.GetLocalizedString(labelId, strDefault);
  }

  std::string GetLanguage(lang_formats format, bool region)
  {
    return g_interProcess.GetLanguage(format, region);
  }

  std::string GetDVDMenuLanguage()
  {
    return g_interProcess.GetDVDMenuLanguage();
  }

  bool StartServer(eservers typ, bool start, bool wait)
  {
    return g_interProcess.StartServer(typ, start, wait);
  }

  void AudioSuspend()
  {
    g_interProcess.AudioSuspend();
  }

  void AudioResume()
  {
    g_interProcess.AudioResume();
  }

  float GetVolume(bool percentage)
  {
    return g_interProcess.GetVolume(percentage);
  }

  void SetVolume(float value, bool isPercentage)
  {
    g_interProcess.SetVolume(value, isPercentage);
  }

  bool IsMuted()
  {
    return g_interProcess.IsMuted();
  }

  void ToggleMute()
  {
    g_interProcess.ToggleMute();
  }

  void SetMute(bool mute)
  {
    g_interProcess.SetMute(mute);
  }

  dvd_state GetOpticalDriveState()
  {
    return g_interProcess.GetOpticalDriveState();
  }

  bool EjectOpticalDrive()
  {
    return g_interProcess.EjectOpticalDrive();
  }

  void KodiVersion(kodi_version& version)
  {
    g_interProcess.KodiVersion(version);
  }

  void KodiQuit()
  {
    g_interProcess.KodiQuit();
  }

  void HTPCShutdown()
  {
    g_interProcess.HTPCShutdown();
  }

  void HTPCRestart()
  {
    g_interProcess.HTPCRestart();
  }

  void ExecuteScript(const std::string& script)
  {
    g_interProcess.ExecuteScript(script);
  }

  void ExecuteBuiltin(const std::string& function, bool wait)
  {
    g_interProcess.ExecuteBuiltin(function, wait);
  }

  std::string ExecuteJSONRPC(const std::string& jsonrpccommand)
  {
    return g_interProcess.ExecuteJSONRPC(jsonrpccommand);
  }

  std::string GetRegion(const std::string& id)
  {
    return g_interProcess.GetRegion(id);
  }

  long GetFreeMem()
  {
    return g_interProcess.GetFreeMem();
  }

  int GetGlobalIdleTime()
  {
    return g_interProcess.GetGlobalIdleTime();
  }

  std::string GetAddonInfo(const std::string& id)
  {
    return g_interProcess.GetAddonInfo(id);
  }

  std::string TranslatePath(const std::string& path)
  {
    return g_interProcess.TranslatePath(path);
  }

  std::string TranslateAddonStatus(ADDON_STATUS status)
  {
    switch (status)
    {
      case ADDON_STATUS_OK:                 return "OK";
      case ADDON_STATUS_LOST_CONNECTION:    return "Lost connection";
      case ADDON_STATUS_NEED_RESTART:       return "Needs restart";
      case ADDON_STATUS_NEED_SETTINGS:      return "Needs settngs";
      case ADDON_STATUS_UNKNOWN:            return "Unknown";
      case ADDON_STATUS_NEED_SAVEDSETTINGS: return "Needs saved settings";
      case ADDON_STATUS_PERMANENT_FAILURE:  return "Permanent failure";
      default:
        break;
    }
    return "";
  }

}; /* namespace General */
}; /* namespace AddOn */

}; /* namespace KodiAPI */
}; /* namespace V2 */
