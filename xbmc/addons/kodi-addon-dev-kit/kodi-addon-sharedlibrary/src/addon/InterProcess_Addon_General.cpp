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

#include "InterProcess_Addon_General.h"
#include "InterProcess.h"

#include <cstring>

extern "C"
{
  bool CKODIAddon_InterProcess_Addon_General::GetSettingString(
      const std::string& settingName,
      std::string&       settingValue,
      bool               global)
  {
    char * buffer = (char*) malloc(1024);
    buffer[0] = 0; /* Set the end of string */
    bool ret = g_interProcess.m_Callbacks->General.get_setting(g_interProcess.m_Handle, settingName.c_str(), buffer, global);
    if (ret)
      settingValue = buffer;
    free(buffer);
    return ret;
  }

  bool CKODIAddon_InterProcess_Addon_General::GetSettingBoolean(
      const std::string& settingName,
      bool*              settingValue,
      bool               global)
  {
    return g_interProcess.m_Callbacks->General.get_setting(g_interProcess.m_Handle, settingName.c_str(), settingValue, global);
  }

  bool CKODIAddon_InterProcess_Addon_General::GetSettingInt(
      const std::string& settingName,
      int*               settingValue,
      bool               global)
  {
    return g_interProcess.m_Callbacks->General.get_setting(g_interProcess.m_Handle, settingName.c_str(), settingValue, global);
  }

  bool CKODIAddon_InterProcess_Addon_General::GetSettingFloat(
      const std::string& settingName,
      float*             settingValue,
      bool               global)
  {
    return g_interProcess.m_Callbacks->General.get_setting(g_interProcess.m_Handle, settingName.c_str(), settingValue, global);
  }

  void CKODIAddon_InterProcess_Addon_General::OpenSettings()
  {
    g_interProcess.m_Callbacks->General.open_settings_dialog(g_interProcess.m_Handle);
  }

  std::string CKODIAddon_InterProcess_Addon_General::GetAddonInfo(const std::string& id) const
  {
    std::string strReturn;
    char* strMsg = g_interProcess.m_Callbacks->General.get_addon_info(g_interProcess.m_Handle, id.c_str());
    if (strMsg != nullptr)
    {
      if (std::strlen(strMsg))
        strReturn = strMsg;
      g_interProcess.m_Callbacks->free_string(g_interProcess.m_Handle, strMsg);
    }
    return strReturn;
  }

  void CKODIAddon_InterProcess_Addon_General::QueueFormattedNotification(const queue_msg type, const char *aDescription)
  {
    g_interProcess.m_Callbacks->General.queue_notification(g_interProcess.m_Handle, type, aDescription);
  }

  void CKODIAddon_InterProcess_Addon_General::QueueNotification(
    const queue_msg  type,
    const std::string& aCaption,
    const std::string& aDescription,
    unsigned int       displayTime,
    bool               withSound,
    unsigned int       messageTime)
  {
    g_interProcess.m_Callbacks->General.queue_notification_from_type(g_interProcess.m_Handle,
                                      type, aCaption.c_str(), aDescription.c_str(),
                                      displayTime, withSound, messageTime);
  }

  void CKODIAddon_InterProcess_Addon_General::QueueNotification(
    const std::string& aCaption,
    const std::string& aDescription)
  {
    g_interProcess.m_Callbacks->General.queue_notification_with_image(g_interProcess.m_Handle, "",
                                      aCaption.c_str(), aDescription.c_str(),
                                      5000, false, 1000);
  }

  void CKODIAddon_InterProcess_Addon_General::QueueNotification(
    const std::string& aImageFile,
    const std::string& aCaption,
    const std::string& aDescription,
    unsigned int       displayTime,
    bool               withSound,
    unsigned int       messageTime)
  {
    g_interProcess.m_Callbacks->General.queue_notification_with_image(g_interProcess.m_Handle, aImageFile.c_str(),
                                      aCaption.c_str(), aDescription.c_str(),
                                      displayTime, withSound, messageTime);
  }

  void CKODIAddon_InterProcess_Addon_General::GetMD5(const std::string& text, std::string& md5)
  {
    char* md5ret = (char*)malloc(40*sizeof(char)); // md5 size normally 32 bytes
    g_interProcess.m_Callbacks->General.get_md5(text.c_str(), *md5ret);
    md5 = md5ret;
    free(md5ret);
  }

  bool CKODIAddon_InterProcess_Addon_General::UnknownToUTF8(
    const std::string&  stringSrc,
    std::string&        utf8StringDst,
    bool                failOnBadChar)
  {
    bool ret = false;
    char* retString = g_interProcess.m_Callbacks->General.unknown_to_utf8(g_interProcess.m_Handle, stringSrc.c_str(), ret, failOnBadChar);
    if (retString != nullptr)
    {
      if (ret)
        utf8StringDst = retString;
      g_interProcess.m_Callbacks->free_string(g_interProcess.m_Handle, retString);
    }
    return ret;
  }

  std::string CKODIAddon_InterProcess_Addon_General::GetLocalizedString(uint32_t labelId, const std::string& strDefault)
  {
    std::string strReturn = strDefault;
    char* strMsg = g_interProcess.m_Callbacks->General.get_localized_string(g_interProcess.m_Handle, labelId);
    if (strMsg != nullptr)
    {
      if (std::strlen(strMsg))
        strReturn = strMsg;
      g_interProcess.m_Callbacks->free_string(g_interProcess.m_Handle, strMsg);
    }
    return strReturn;
  }

  std::string CKODIAddon_InterProcess_Addon_General::GetLanguage(lang_formats format, bool region)
  {
    std::string language;
    language.resize(256);
    unsigned int size = (unsigned int)language.capacity();
    g_interProcess.m_Callbacks->General.get_language(g_interProcess.m_Handle, language[0], size, format, region);
    language.resize(size);
    language.shrink_to_fit();
    return language;
  }

  std::string CKODIAddon_InterProcess_Addon_General::GetDVDMenuLanguage()
  {
    std::string language;
    language.resize(16);
    unsigned int size = (unsigned int)language.capacity();
    g_interProcess.m_Callbacks->General.get_dvd_menu_language(g_interProcess.m_Handle, language[0], size);
    language.resize(size);
    language.shrink_to_fit();
    return language;
  }

  bool CKODIAddon_InterProcess_Addon_General::StartServer(eservers typ, bool start, bool wait)
  {
    return g_interProcess.m_Callbacks->General.start_server(g_interProcess.m_Handle, (int)typ, start, wait);
  }

  void CKODIAddon_InterProcess_Addon_General::AudioSuspend()
  {
    g_interProcess.m_Callbacks->General.audio_suspend(g_interProcess.m_Handle);
  }

  void CKODIAddon_InterProcess_Addon_General::AudioResume()
  {
    g_interProcess.m_Callbacks->General.audio_resume(g_interProcess.m_Handle);
  }

  float CKODIAddon_InterProcess_Addon_General::GetVolume(bool percentage)
  {
    return g_interProcess.m_Callbacks->General.get_volume(g_interProcess.m_Handle, percentage);
  }

  void CKODIAddon_InterProcess_Addon_General::SetVolume(float value, bool isPercentage)
  {
    g_interProcess.m_Callbacks->General.set_volume(g_interProcess.m_Handle, value, isPercentage);
  }

  bool CKODIAddon_InterProcess_Addon_General::IsMuted()
  {
    return g_interProcess.m_Callbacks->General.is_muted(g_interProcess.m_Handle);
  }

  void CKODIAddon_InterProcess_Addon_General::ToggleMute()
  {
    g_interProcess.m_Callbacks->General.toggle_mute(g_interProcess.m_Handle);
  }

  void CKODIAddon_InterProcess_Addon_General::SetMute(bool mute)
  {
    if (g_interProcess.m_Callbacks->General.is_muted(g_interProcess.m_Handle) != mute)
      g_interProcess.m_Callbacks->General.toggle_mute(g_interProcess.m_Handle);
  }

  dvd_state CKODIAddon_InterProcess_Addon_General::GetOpticalDriveState()
  {
    return (dvd_state)g_interProcess.m_Callbacks->General.get_optical_state(g_interProcess.m_Handle);
  }

  bool CKODIAddon_InterProcess_Addon_General::EjectOpticalDrive()
  {
    return g_interProcess.m_Callbacks->General.eject_optical_drive(g_interProcess.m_Handle);
  }

  void CKODIAddon_InterProcess_Addon_General::KodiVersion(kodi_version& version)
  {
    char* compile_name = nullptr;
    char* revision     = nullptr;
    char* tag          = nullptr;
    char* tag_revision = nullptr;

    g_interProcess.m_Callbacks->General.kodi_version(g_interProcess.m_Handle, compile_name, version.major, version.minor, revision, tag, tag_revision);
    if (compile_name != nullptr)
    {
      version.compile_name  = compile_name;
      g_interProcess.m_Callbacks->free_string(g_interProcess.m_Handle, compile_name);
    }
    if (revision != nullptr)
    {
      version.revision = revision;
      g_interProcess.m_Callbacks->free_string(g_interProcess.m_Handle, revision);
    }
    if (tag != nullptr)
    {
      version.tag = tag;
      g_interProcess.m_Callbacks->free_string(g_interProcess.m_Handle, tag);
    }
    if (tag_revision != nullptr)
    {
      version.tag_revision = tag_revision;
      g_interProcess.m_Callbacks->free_string(g_interProcess.m_Handle, tag_revision);
    }
  }

  void CKODIAddon_InterProcess_Addon_General::KodiQuit()
  {
    g_interProcess.m_Callbacks->General.kodi_quit(g_interProcess.m_Handle);
  }

  void CKODIAddon_InterProcess_Addon_General::HTPCShutdown()
  {
    g_interProcess.m_Callbacks->General.htpc_shutdown(g_interProcess.m_Handle);
  }

  void CKODIAddon_InterProcess_Addon_General::HTPCRestart()
  {
    g_interProcess.m_Callbacks->General.htpc_restart(g_interProcess.m_Handle);
  }

  void CKODIAddon_InterProcess_Addon_General::ExecuteScript(const std::string& script)
  {
    g_interProcess.m_Callbacks->General.execute_script(g_interProcess.m_Handle, script.c_str());
  }

  void CKODIAddon_InterProcess_Addon_General::ExecuteBuiltin(const std::string& function, bool wait)
  {
    g_interProcess.m_Callbacks->General.execute_builtin(g_interProcess.m_Handle, function.c_str(), wait);
  }

  std::string CKODIAddon_InterProcess_Addon_General::ExecuteJSONRPC(const std::string& jsonrpccommand)
  {
    std::string strReturn;
    char* strMsg = g_interProcess.m_Callbacks->General.execute_jsonrpc(g_interProcess.m_Handle, jsonrpccommand.c_str());
    if (strMsg != nullptr)
    {
      if (std::strlen(strMsg))
        strReturn = strMsg;
      g_interProcess.m_Callbacks->free_string(g_interProcess.m_Handle, strMsg);
    }
    return strReturn;
  }

  std::string CKODIAddon_InterProcess_Addon_General::GetRegion(const std::string& id)
  {
    std::string strReturn;
    char* strMsg = g_interProcess.m_Callbacks->General.get_region(g_interProcess.m_Handle, id.c_str());
    if (strMsg != nullptr)
    {
      if (std::strlen(strMsg))
        strReturn = strMsg;
      g_interProcess.m_Callbacks->free_string(g_interProcess.m_Handle, strMsg);
    }
    return strReturn;
  }

  long CKODIAddon_InterProcess_Addon_General::GetFreeMem()
  {
    return g_interProcess.m_Callbacks->General.get_free_mem(g_interProcess.m_Handle);
  }

  int CKODIAddon_InterProcess_Addon_General::GetGlobalIdleTime()
  {
    return g_interProcess.m_Callbacks->General.get_global_idle_time(g_interProcess.m_Handle);
  }

  std::string CKODIAddon_InterProcess_Addon_General::GetAddonInfo(const std::string& id)
  {
    std::string strReturn;
    char* strMsg = g_interProcess.m_Callbacks->General.get_addon_info(g_interProcess.m_Handle, id.c_str());
    if (strMsg != nullptr)
    {
      if (std::strlen(strMsg))
        strReturn = strMsg;
      g_interProcess.m_Callbacks->free_string(g_interProcess.m_Handle, strMsg);
    }
    return strReturn;
  }

  std::string CKODIAddon_InterProcess_Addon_General::TranslatePath(const std::string& path)
  {
    std::string strReturn;
    char* strMsg = g_interProcess.m_Callbacks->General.translate_path(g_interProcess.m_Handle, path.c_str());
    if (strMsg != nullptr)
    {
      if (std::strlen(strMsg))
        strReturn = strMsg;
      g_interProcess.m_Callbacks->free_string(g_interProcess.m_Handle, strMsg);
    }
    return strReturn;
  }

}; /* extern "C" */
