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
#include "RequestPacket.h"
#include "ResponsePacket.h"

#include <p8-platform/util/StringUtils.h>
#include <cstring>
#include <iostream>       // std::cerr
#include <stdexcept>      // std::out_of_range

using namespace P8PLATFORM;

extern "C"
{

  bool CKODIAddon_InterProcess_Addon_General::GetSettingString(
      const std::string& settingName,
      std::string&       settingValue,
      bool               global)
  {
    try
    {
      char* retString;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_AddOn_General_GetSettingString, session);
      vrp.push(API_STRING,  settingName.c_str());
      vrp.push(API_BOOLEAN, &global);
      uint32_t retCode = PROCESS_ADDON_CALL_WITH_RETURN_STRING(session, vrp, &retString);
      if (retCode == API_ERR_VALUE_NOT_AVAILABLE)
        return false;
      if (retCode != API_SUCCESS)
        throw retCode;
      settingValue = retString;
      return true;
    }
    PROCESS_HANDLE_EXCEPTION;
    return false;
  }

  bool CKODIAddon_InterProcess_Addon_General::GetSettingBoolean(
      const std::string& settingName,
      bool*              settingValue,
      bool               global)
  {
    try
    {
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_AddOn_General_GetSettingBoolean, session);
      vrp.push(API_STRING,  settingName.c_str());
      vrp.push(API_BOOLEAN, &global);
      uint32_t retCode = PROCESS_ADDON_CALL_WITH_RETURN_BOOLEAN(session, vrp, settingValue);
      if (retCode == API_ERR_VALUE_NOT_AVAILABLE)
        return false;
      if (retCode != API_SUCCESS)
        throw retCode;
      return true;
    }
    PROCESS_HANDLE_EXCEPTION;
    return false;
  }

  bool CKODIAddon_InterProcess_Addon_General::GetSettingInt(
      const std::string& settingName,
      int*               settingValue,
      bool               global)
  {
    try
    {
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_AddOn_General_GetSettingInt, session);
      vrp.push(API_STRING,  settingName.c_str());
      vrp.push(API_BOOLEAN, &global);
      uint32_t retCode = PROCESS_ADDON_CALL_WITH_RETURN_INTEGER(session, vrp, settingValue);
      if (retCode == API_ERR_VALUE_NOT_AVAILABLE)
        return false;
      if (retCode != API_SUCCESS)
        throw retCode;
      return true;
    }
    PROCESS_HANDLE_EXCEPTION;
    return false;
  }

  bool CKODIAddon_InterProcess_Addon_General::GetSettingFloat(
      const std::string& settingName,
      float*             settingValue,
      bool               global)
  {
    try
    {
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_AddOn_General_GetSettingInt, session);
      vrp.push(API_STRING,  settingName.c_str());
      vrp.push(API_BOOLEAN, &global);
      uint32_t retCode = PROCESS_ADDON_CALL_WITH_RETURN_FLOAT(session, vrp, settingValue);
      if (retCode == API_ERR_VALUE_NOT_AVAILABLE)
        return false;
      if (retCode != API_SUCCESS)
        throw retCode;
      return true;
    }
    PROCESS_HANDLE_EXCEPTION;
    return false;
  }

  void CKODIAddon_InterProcess_Addon_General::OpenSettings()
  {
    try
    {
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_AddOn_General_OpenSettings, session);
      uint32_t retCode = PROCESS_ADDON_CALL(session, vrp);
      if (retCode != API_SUCCESS)
        throw retCode;
    }
    PROCESS_HANDLE_EXCEPTION;
  }

  std::string CKODIAddon_InterProcess_Addon_General::GetAddonInfo(const std::string& id)
  {
    try
    {
      char* retString;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_AddOn_General_GetAddonInfo, session);
      vrp.push(API_STRING, id.c_str());
      uint32_t retCode = PROCESS_ADDON_CALL_WITH_RETURN_STRING(session, vrp, &retString);
      if (retCode != API_SUCCESS)
        throw retCode;

      return retString;
    }
    PROCESS_HANDLE_EXCEPTION;
    return "";
  }

  void CKODIAddon_InterProcess_Addon_General::QueueFormattedNotification(const queue_msg type, const char *aDescription)
  {
    try
    {
      int usedType = type;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_AddOn_General_QueueFormattedNotification, session);
      vrp.push(API_INT, &usedType);
      vrp.push(API_STRING, aDescription);
      uint32_t retCode = PROCESS_ADDON_CALL(session, vrp);
      if (retCode != API_SUCCESS)
        throw retCode;
    }
    PROCESS_HANDLE_EXCEPTION;
  }

  void CKODIAddon_InterProcess_Addon_General::QueueNotification(
    const queue_msg  type,
    const std::string& aCaption,
    const std::string& aDescription,
    unsigned int       displayTime,
    bool               withSound,
    unsigned int       messageTime)
  {
    try
    {
      int usedType = type;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_AddOn_General_QueueFormattedNotification, session);
      vrp.push(API_INT,          &usedType);
      vrp.push(API_STRING,        aCaption.c_str());
      vrp.push(API_STRING,        aDescription.c_str());
      vrp.push(API_UNSIGNED_INT, &displayTime);
      vrp.push(API_BOOLEAN,      &withSound);
      vrp.push(API_UNSIGNED_INT, &messageTime);
      uint32_t retCode = PROCESS_ADDON_CALL(session, vrp);
      if (retCode != API_SUCCESS)
        throw retCode;
    }
    PROCESS_HANDLE_EXCEPTION;
  }

  void CKODIAddon_InterProcess_Addon_General::QueueNotification(
    const std::string& aCaption,
    const std::string& aDescription)
  {
    try
    {
      const char*   image       = "";
      unsigned int  displayTime = 5000;
      bool          withSound   = false;
      unsigned int  messageTime = 1000;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_AddOn_General_QueueNotificationWithImage, session);
      vrp.push(API_STRING,        image);
      vrp.push(API_STRING,        aCaption.c_str());
      vrp.push(API_STRING,        aDescription.c_str());
      vrp.push(API_UNSIGNED_INT, &displayTime);
      vrp.push(API_BOOLEAN,      &withSound);
      vrp.push(API_UNSIGNED_INT, &messageTime);
      uint32_t retCode = PROCESS_ADDON_CALL(session, vrp);
      if (retCode != API_SUCCESS)
        throw retCode;
    }
    PROCESS_HANDLE_EXCEPTION;
  }

  void CKODIAddon_InterProcess_Addon_General::QueueNotification(
    const std::string& aImageFile,
    const std::string& aCaption,
    const std::string& aDescription,
    unsigned int       displayTime,
    bool               withSound,
    unsigned int       messageTime)
  {
    try
    {
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_AddOn_General_QueueNotificationWithImage, session);
      vrp.push(API_STRING,        aImageFile.c_str());
      vrp.push(API_STRING,        aCaption.c_str());
      vrp.push(API_STRING,        aDescription.c_str());
      vrp.push(API_UNSIGNED_INT, &displayTime);
      vrp.push(API_BOOLEAN,      &withSound);
      vrp.push(API_UNSIGNED_INT, &messageTime);
      uint32_t retCode = PROCESS_ADDON_CALL(session, vrp);
      if (retCode != API_SUCCESS)
        throw retCode;
    }
    PROCESS_HANDLE_EXCEPTION;
  }

  void CKODIAddon_InterProcess_Addon_General::GetMD5(const std::string& text, std::string& md5)
  {
    try
    {
      char* retString;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_AddOn_General_GetMD5, session);
      vrp.push(API_STRING, text.c_str());
      uint32_t retCode = PROCESS_ADDON_CALL_WITH_RETURN_STRING(session, vrp, &retString);
      if (retCode != API_SUCCESS)
        throw retCode;
      md5 = retString;
    }
    PROCESS_HANDLE_EXCEPTION;
  }

  bool CKODIAddon_InterProcess_Addon_General::UnknownToUTF8(
    const std::string&  stringSrc,
    std::string&        utf8StringDst,
    bool                failOnBadChar)
  {
    try
    {
      char* retString;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_AddOn_General_UnknownToUTF8, session);
      vrp.push(API_STRING,      stringSrc.c_str());
      vrp.push(API_BOOLEAN,    &failOnBadChar);
      uint32_t retCode = PROCESS_ADDON_CALL_WITH_RETURN_STRING(session, vrp, &retString);
      if (retCode != API_SUCCESS)
        throw retCode;
      utf8StringDst = retString;
      return true;
    }
    PROCESS_HANDLE_EXCEPTION;
    return false;
  }

  std::string CKODIAddon_InterProcess_Addon_General::GetLocalizedString(uint32_t labelId, const std::string& strDefault)
  {
    try
    {
      char* retString;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_AddOn_General_GetLocalizedString, session);
      vrp.push(API_UINT32_T, &labelId);
      uint32_t retCode = PROCESS_ADDON_CALL_WITH_RETURN_STRING(session, vrp, &retString);
      if (retCode != API_SUCCESS)
        throw retCode;
      if (retString == nullptr || strlen(retString) == 0)
        return strDefault;
      return retString;
    }
    PROCESS_HANDLE_EXCEPTION;
    return "";
  }

  std::string CKODIAddon_InterProcess_Addon_General::GetLanguage(lang_formats format, bool region)
  {
    try
    {
      char* retString;
      int lang_format = format;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_AddOn_General_GetLanguage, session);
      vrp.push(API_INT,     &lang_format);
      vrp.push(API_BOOLEAN, &region);
      uint32_t retCode = PROCESS_ADDON_CALL_WITH_RETURN_STRING(session, vrp, &retString);
      if (retCode != API_SUCCESS)
        throw retCode;
      return retString;
    }
    PROCESS_HANDLE_EXCEPTION;
    return "";
  }

  std::string CKODIAddon_InterProcess_Addon_General::GetDVDMenuLanguage()
  {
    try
    {
      char* retString;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_AddOn_General_GetDVDMenuLanguage, session);
      uint32_t retCode = PROCESS_ADDON_CALL_WITH_RETURN_STRING(session, vrp, &retString);
      if (retCode != API_SUCCESS)
        throw retCode;
      return retString;
    }
    PROCESS_HANDLE_EXCEPTION;
    return "";
  }

  bool CKODIAddon_InterProcess_Addon_General::StartServer(eservers typ, bool start, bool wait)
  {
    try
    {
      int server = typ;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_AddOn_General_StartServer, session);
      vrp.push(API_INT,     &server);
      vrp.push(API_BOOLEAN, &start);
      vrp.push(API_BOOLEAN, &wait);
      uint32_t retCode = PROCESS_ADDON_CALL(session, vrp);
      return (retCode == API_SUCCESS ? true : false);
    }
    PROCESS_HANDLE_EXCEPTION;
    return false;
  }

  void CKODIAddon_InterProcess_Addon_General::AudioSuspend()
  {
    try
    {
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_AddOn_General_AudioSuspend, session);
      uint32_t retCode = PROCESS_ADDON_CALL(session, vrp);
      if (retCode != API_SUCCESS)
        throw retCode;
    }
    PROCESS_HANDLE_EXCEPTION;
  }

  void CKODIAddon_InterProcess_Addon_General::AudioResume()
  {
    try
    {
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_AddOn_General_AudioResume, session);
      uint32_t retCode = PROCESS_ADDON_CALL(session, vrp);
      if (retCode != API_SUCCESS)
        throw retCode;
    }
    PROCESS_HANDLE_EXCEPTION;
  }

  float CKODIAddon_InterProcess_Addon_General::GetVolume(bool percentage)
  {
    try
    {
      float volume;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_AddOn_General_GetVolume, session);
      vrp.push(API_BOOLEAN, &percentage);
      uint32_t retCode = PROCESS_ADDON_CALL_WITH_RETURN_FLOAT(session, vrp, &volume);
      if (retCode != API_SUCCESS)
        throw retCode;
      return volume;
    }
    PROCESS_HANDLE_EXCEPTION;
    return 0.0f;
  }

  void CKODIAddon_InterProcess_Addon_General::SetVolume(float value, bool isPercentage)
  {
    try
    {
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_AddOn_General_SetVolume, session);
      vrp.push(API_FLOAT,   &value);
      vrp.push(API_BOOLEAN, &isPercentage);
      uint32_t retCode = PROCESS_ADDON_CALL(session, vrp);
      if (retCode != API_SUCCESS)
        throw retCode;
    }
    PROCESS_HANDLE_EXCEPTION;
  }

  bool CKODIAddon_InterProcess_Addon_General::IsMuted()
  {
    try
    {
      bool isMuted;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_AddOn_General_IsMuted, session);
      uint32_t retCode = PROCESS_ADDON_CALL_WITH_RETURN_BOOLEAN(session, vrp, &isMuted);
      if (retCode != API_SUCCESS)
        throw retCode;
      return isMuted;
    }
    PROCESS_HANDLE_EXCEPTION;
    return false;
  }

  void CKODIAddon_InterProcess_Addon_General::ToggleMute()
  {
    try
    {
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_AddOn_General_ToggleMute, session);
      uint32_t retCode = PROCESS_ADDON_CALL(session, vrp);
      if (retCode != API_SUCCESS)
        throw retCode;
    }
    PROCESS_HANDLE_EXCEPTION;
  }

  void CKODIAddon_InterProcess_Addon_General::SetMute(bool mute)
  {
    try
    {
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_AddOn_General_SetMute, session);
      vrp.push(API_BOOLEAN, &mute);
      uint32_t retCode = PROCESS_ADDON_CALL(session, vrp);
      if (retCode != API_SUCCESS)
        throw retCode;
    }
    PROCESS_HANDLE_EXCEPTION;
  }

  dvd_state CKODIAddon_InterProcess_Addon_General::GetOpticalDriveState()
  {
    try
    {
      long ret;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_AddOn_General_GetOpticalDriveState, session);
      uint32_t retCode = PROCESS_ADDON_CALL_WITH_RETURN_LONG(session, vrp, &ret);
      if (retCode != API_SUCCESS)
        throw retCode;
      return (dvd_state)ret;
    }
    PROCESS_HANDLE_EXCEPTION;

    return ADDON_DRIVE_NOT_READY;
  }

  bool CKODIAddon_InterProcess_Addon_General::EjectOpticalDrive()
  {
    try
    {
      bool ret;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_AddOn_General_EjectOpticalDrive, session);
      uint32_t retCode = PROCESS_ADDON_CALL_WITH_RETURN_BOOLEAN(session, vrp, &ret);
      if (retCode != API_SUCCESS)
        throw retCode;
      return ret;
    }
    PROCESS_HANDLE_EXCEPTION;
    return false;
  }

  void CKODIAddon_InterProcess_Addon_General::KodiVersion(kodi_version& version)
  {
    try
    {
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_AddOn_General_KodiVersion, session);

      uint32_t retCode;
      CLockObject lock(session->m_callMutex);
      std::unique_ptr<CResponsePacket> vresp(session->ReadResult(&vrp));
      if (!vresp)
        throw API_ERR_BUFFER;
      vresp->pop(API_UINT32_T, &retCode);
      char* compile_name; vresp->pop(API_STRING,   &compile_name); version.compile_name = compile_name;
      vresp->pop(API_STRING,   &version.major);
      vresp->pop(API_STRING,   &version.minor);
      char* revision;     vresp->pop(API_STRING,   &compile_name); version.revision     = revision;
      char* tag;          vresp->pop(API_STRING,   &tag);          version.tag          = tag;
      char* tag_revision; vresp->pop(API_STRING,   &tag_revision); version.tag_revision = compile_name;
      if (retCode != API_SUCCESS)
        throw retCode;
    }
    PROCESS_HANDLE_EXCEPTION;
  }

  void CKODIAddon_InterProcess_Addon_General::KodiQuit()
  {
    try
    {
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_AddOn_General_KodiQuit, session);
      uint32_t retCode = PROCESS_ADDON_CALL(session, vrp);
      if (retCode != API_SUCCESS)
        throw retCode;
    }
    PROCESS_HANDLE_EXCEPTION;
  }

  void CKODIAddon_InterProcess_Addon_General::HTPCShutdown()
  {
    try
    {
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_AddOn_General_HTPCShutdown, session);
      uint32_t retCode = PROCESS_ADDON_CALL(session, vrp);
      if (retCode != API_SUCCESS)
        throw retCode;
    }
    PROCESS_HANDLE_EXCEPTION;
  }

  void CKODIAddon_InterProcess_Addon_General::HTPCRestart()
  {
    try
    {
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_AddOn_General_HTPCRestart, session);
      uint32_t retCode = PROCESS_ADDON_CALL(session, vrp);
      if (retCode != API_SUCCESS)
        throw retCode;
    }
    PROCESS_HANDLE_EXCEPTION;
  }

  void CKODIAddon_InterProcess_Addon_General::ExecuteScript(const std::string& script)
  {
    try
    {
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_AddOn_General_ExecuteScript, session);
      vrp.push(API_STRING, script.c_str());
      uint32_t retCode = PROCESS_ADDON_CALL(session, vrp);
      if (retCode != API_SUCCESS)
        throw retCode;
    }
    PROCESS_HANDLE_EXCEPTION;
  }

  void CKODIAddon_InterProcess_Addon_General::ExecuteBuiltin(const std::string& function, bool wait)
  {
    try
    {
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_AddOn_General_ExecuteBuiltin, session);
      vrp.push(API_STRING, function.c_str());
      vrp.push(API_BOOLEAN, &wait);
      uint32_t retCode = PROCESS_ADDON_CALL(session, vrp);
      if (retCode != API_SUCCESS)
        throw retCode;
    }
    PROCESS_HANDLE_EXCEPTION;
  }

  std::string CKODIAddon_InterProcess_Addon_General::ExecuteJSONRPC(const std::string& jsonrpccommand)
  {
    try
    {
      char* retString;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_AddOn_General_ExecuteJSONRPC, session);
      vrp.push(API_STRING, jsonrpccommand.c_str());
      uint32_t retCode = PROCESS_ADDON_CALL_WITH_RETURN_STRING(session, vrp, &retString);
      if (retCode != API_SUCCESS)
        throw retCode;
      return retString;
    }
    PROCESS_HANDLE_EXCEPTION;
    return "";
  }

  std::string CKODIAddon_InterProcess_Addon_General::GetRegion(const std::string& id)
  {
    try
    {
      char* retString;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_AddOn_General_GetRegion, session);
      vrp.push(API_STRING, id.c_str());
      uint32_t retCode = PROCESS_ADDON_CALL_WITH_RETURN_STRING(session, vrp, &retString);
      if (retCode != API_SUCCESS)
        throw retCode;
      return retString;
    }
    PROCESS_HANDLE_EXCEPTION;
    return "";
  }

  long CKODIAddon_InterProcess_Addon_General::GetFreeMem()
  {
    try
    {
      long ret;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_AddOn_General_TranslatePath, session);
      uint32_t retCode = PROCESS_ADDON_CALL_WITH_RETURN_LONG(session, vrp, &ret);
      if (retCode != API_SUCCESS)
        throw retCode;
      return ret;
    }
    PROCESS_HANDLE_EXCEPTION;
    return -1;
  }

  int CKODIAddon_InterProcess_Addon_General::GetGlobalIdleTime()
  {
    try
    {
      int ret;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_AddOn_General_TranslatePath, session);
      uint32_t retCode = PROCESS_ADDON_CALL_WITH_RETURN_INTEGER(session, vrp, &ret);
      if (retCode != API_SUCCESS)
        throw retCode;
      return ret;
    }
    PROCESS_HANDLE_EXCEPTION;
    return -1;
  }

  std::string CKODIAddon_InterProcess_Addon_General::TranslatePath(const std::string& path)
  {
    try
    {
      char* retString;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_AddOn_General_TranslatePath, session);
      vrp.push(API_STRING, path.c_str());
      uint32_t retCode = PROCESS_ADDON_CALL_WITH_RETURN_STRING(session, vrp, &retString);
      if (retCode != API_SUCCESS)
        throw retCode;
      return retString;
    }
    PROCESS_HANDLE_EXCEPTION;
    return "";
  }

}; /* extern "C" */
