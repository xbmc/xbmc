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

#include "AddonExe_Addon_General.h"
#include "addons/Addon.h"
#include "addons/binary/callbacks/AddonCallbacksAddonBase.h"
#include "addons/binary/callbacks/api2/Addon/AddonCB_General.h"
#include "addons/binary/callbacks/api2/AddonCallbacksBase.h"

namespace V2
{
namespace KodiAPI
{

bool CAddonExeCB_Addon_General::GetSettingString(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  bool global;
  char* settingName;
  char* buffer = (char*) malloc(1024);
  buffer[0] = 0; /* Set the end of string */
  req.pop(API_STRING, &settingName);
  req.pop(API_BOOLEAN, &global);
  bool ret = AddOn::CAddOnGeneral::get_setting(addon, settingName, buffer, global);
  std::string retStr = buffer;
  free(buffer);
  PROCESS_ADDON_RETURN_CALL_WITH_STRING(ret ? API_SUCCESS : API_ERR_VALUE_NOT_AVAILABLE, retStr.c_str());
  return true;
}

bool CAddonExeCB_Addon_General::GetSettingInt(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  bool global;
  char* settingName;
  int retValue;
  req.pop(API_STRING, &settingName);
  req.pop(API_BOOLEAN, &global);
  bool ret = AddOn::CAddOnGeneral::get_setting(addon, settingName, &retValue, global);
  PROCESS_ADDON_RETURN_CALL_WITH_INT(ret ? API_SUCCESS : API_ERR_VALUE_NOT_AVAILABLE, &retValue);
  return true;
}

bool CAddonExeCB_Addon_General::GetSettingBoolean(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  bool global;
  char* settingName;
  bool retValue;
  req.pop(API_STRING, &settingName);
  req.pop(API_BOOLEAN, &global);
  bool ret = AddOn::CAddOnGeneral::get_setting(addon, settingName, &retValue, global);
  PROCESS_ADDON_RETURN_CALL_WITH_BOOLEAN(ret ? API_SUCCESS : API_ERR_VALUE_NOT_AVAILABLE, &retValue);
  return true;
}

bool CAddonExeCB_Addon_General::GetSettingFloat(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  bool global;
  char* settingName;
  float retValue;
  req.pop(API_STRING, &settingName);
  req.pop(API_BOOLEAN, &global);
  bool ret = AddOn::CAddOnGeneral::get_setting(addon, settingName, &retValue, global);
  PROCESS_ADDON_RETURN_CALL_WITH_FLOAT(ret ? API_SUCCESS : API_ERR_VALUE_NOT_AVAILABLE, &retValue);
  return true;
}

bool CAddonExeCB_Addon_General::OpenSettings(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  AddOn::CAddOnGeneral::open_settings_dialog(addon);
  PROCESS_ADDON_RETURN_CALL(API_SUCCESS);
  return true;
}

bool CAddonExeCB_Addon_General::GetAddonInfo(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  char* str;
  req.pop(API_STRING, &str);
  str = AddOn::CAddOnGeneral::get_addon_info(addon, str);
  std::string retStr = str;
  CAddonCallbacksAddon::free_string(addon, str);
  PROCESS_ADDON_RETURN_CALL_WITH_STRING(API_SUCCESS, retStr.c_str());
  return true;
}

bool CAddonExeCB_Addon_General::QueueFormattedNotification(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  int   usedType;
  char* str;
  req.pop(API_INT,    &usedType);
  req.pop(API_STRING, &str);
  AddOn::CAddOnGeneral::queue_notification(addon, (V2::KodiAPI::queue_msg)usedType, str);
  PROCESS_ADDON_RETURN_CALL(API_SUCCESS);
  return true;
}

bool CAddonExeCB_Addon_General::QueueNotificationFromType(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  int           usedType;
  char*         aCaption;
  char*         aDescription;
  unsigned int  displayTime;
  bool          withSound;
  unsigned int  messageTime;
  req.pop(API_INT,          &usedType);
  req.pop(API_STRING,       &aCaption);
  req.pop(API_STRING,       &aDescription);
  req.pop(API_UNSIGNED_INT, &displayTime);
  req.pop(API_BOOLEAN,      &withSound);
  req.pop(API_UNSIGNED_INT, &messageTime);
  AddOn::CAddOnGeneral::queue_notification_from_type(addon, (V2::KodiAPI::queue_msg)usedType, aCaption, aDescription, displayTime, withSound, messageTime);
  PROCESS_ADDON_RETURN_CALL(API_SUCCESS);
  return true;
}

bool CAddonExeCB_Addon_General::QueueNotificationWithImage(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  char*         aImageFile;
  char*         aCaption;
  char*         aDescription;
  unsigned int  displayTime;
  bool          withSound;
  unsigned int  messageTime;
  req.pop(API_STRING,       &aImageFile);
  req.pop(API_STRING,       &aCaption);
  req.pop(API_STRING,       &aDescription);
  req.pop(API_UNSIGNED_INT, &displayTime);
  req.pop(API_BOOLEAN,      &withSound);
  req.pop(API_UNSIGNED_INT, &messageTime);
  AddOn::CAddOnGeneral::queue_notification_with_image(addon, aImageFile, aCaption, aDescription, displayTime, withSound, messageTime);
  PROCESS_ADDON_RETURN_CALL(API_SUCCESS);
  return true;
}

bool CAddonExeCB_Addon_General::GetMD5(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  char* text;
  uint32_t retValue = API_SUCCESS;
  req.pop(API_STRING,       &text);
  char* md5 = (char*)malloc(40*sizeof(char)); // md5 size normally 32 bytes
  AddOn::CAddOnGeneral::get_md5(text, *md5);
  resp.init(req.getRequestID());
  resp.push(API_UINT32_T, &retValue);
  resp.push(API_STRING,    md5);
  resp.finalise();
  free(md5);
  return true;
}

bool CAddonExeCB_Addon_General::UnknownToUTF8(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  char* str;
  bool ret;
  bool failOnBadChar;
  std::string retStr;
  req.pop(API_STRING,    &str);
  req.pop(API_BOOLEAN,   &failOnBadChar);
  str = AddOn::CAddOnGeneral::unknown_to_utf8(addon, str, ret, failOnBadChar);
  if (str != nullptr)
  {
    retStr = str;
    CAddonCallbacksAddon::free_string(addon, str);
  }
  PROCESS_ADDON_RETURN_CALL_WITH_STRING((ret ? API_SUCCESS : API_ERR_TYPE), retStr.c_str());
  return true;
}

bool CAddonExeCB_Addon_General::GetLocalizedString(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  uint32_t labelId;
  std::string retStr;
  req.pop(API_UINT32_T, &labelId);
  char* strMsg = AddOn::CAddOnGeneral::get_localized_string(addon, labelId);
  if (strMsg != nullptr)
  {
    retStr = strMsg;
    CAddonCallbacksAddon::free_string(addon, strMsg);
  }
  PROCESS_ADDON_RETURN_CALL_WITH_STRING(API_SUCCESS, retStr.c_str());
  return true;
}

bool CAddonExeCB_Addon_General::GetLanguage(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  bool region;
  int lang_format;
  unsigned int iMaxStringSize = 256*sizeof(char);
  req.pop(API_INT,       &lang_format);
  req.pop(API_BOOLEAN,   &region);
  char* language = (char*)malloc(iMaxStringSize);
  AddOn::CAddOnGeneral::get_language(addon, *language, iMaxStringSize, lang_format, region);
  PROCESS_ADDON_RETURN_CALL_WITH_STRING(API_SUCCESS, language);
  free(language);
  return true;
}

bool CAddonExeCB_Addon_General::GetDVDMenuLanguage(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  unsigned int iMaxStringSize = 16*sizeof(char);
  char* language = (char*)malloc(iMaxStringSize);
  AddOn::CAddOnGeneral::get_dvd_menu_language(addon, *language, iMaxStringSize);
  PROCESS_ADDON_RETURN_CALL_WITH_STRING(API_SUCCESS, language);
  free(language);
  return true;
}

bool CAddonExeCB_Addon_General::StartServer(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  int typ;
  bool start;
  bool wait;
  req.pop(API_INT,       &typ);
  req.pop(API_BOOLEAN,   &start);
  req.pop(API_BOOLEAN,   &wait);
  bool ret = AddOn::CAddOnGeneral::start_server(addon, typ, start, wait);
  PROCESS_ADDON_RETURN_CALL(ret ? API_SUCCESS : API_ERR_OTHER);
  return true;
}

bool CAddonExeCB_Addon_General::AudioSuspend(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  AddOn::CAddOnGeneral::audio_suspend(addon);
  PROCESS_ADDON_RETURN_CALL(API_SUCCESS);
  return true;
}

bool CAddonExeCB_Addon_General::AudioResume(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  AddOn::CAddOnGeneral::audio_resume(addon);
  PROCESS_ADDON_RETURN_CALL(API_SUCCESS);
  return true;
}

bool CAddonExeCB_Addon_General::GetVolume(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  bool isPercentage;
  req.pop(API_BOOLEAN,   &isPercentage);
  float value = AddOn::CAddOnGeneral::get_volume(addon, isPercentage);
  PROCESS_ADDON_RETURN_CALL_WITH_FLOAT(API_SUCCESS, &value);
  return true;
}

bool CAddonExeCB_Addon_General::SetVolume(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  float value;
  bool isPercentage;
  req.pop(API_FLOAT,     &value);
  req.pop(API_BOOLEAN,   &isPercentage);
  AddOn::CAddOnGeneral::set_volume(addon, value, isPercentage);
  PROCESS_ADDON_RETURN_CALL(API_SUCCESS);
  return true;
}

bool CAddonExeCB_Addon_General::IsMuted(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  bool isMuted = AddOn::CAddOnGeneral::is_muted(addon);
  PROCESS_ADDON_RETURN_CALL_WITH_BOOLEAN(API_SUCCESS, &isMuted);
  return true;
}

bool CAddonExeCB_Addon_General::ToggleMute(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  AddOn::CAddOnGeneral::toggle_mute(addon);
  PROCESS_ADDON_RETURN_CALL(API_SUCCESS);
  return true;
}

bool CAddonExeCB_Addon_General::SetMute(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  bool mute;
  req.pop(API_BOOLEAN, &mute);
  if (AddOn::CAddOnGeneral::is_muted(addon) != mute)
    AddOn::CAddOnGeneral::toggle_mute(addon);
  PROCESS_ADDON_RETURN_CALL(API_SUCCESS);
  return true;
}

bool CAddonExeCB_Addon_General::GetOpticalDriveState(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  long ret = AddOn::CAddOnGeneral::get_optical_state(addon);
  PROCESS_ADDON_RETURN_CALL_WITH_LONG(API_SUCCESS, &ret);
  return true;
}

bool CAddonExeCB_Addon_General::EjectOpticalDrive(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  bool ret = AddOn::CAddOnGeneral::eject_optical_drive(addon);
  PROCESS_ADDON_RETURN_CALL_WITH_BOOLEAN(API_SUCCESS, &ret);
  return true;
}

bool CAddonExeCB_Addon_General::KodiVersion(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  char* compile_name = nullptr;
  char* revision     = nullptr;
  char* tag          = nullptr;
  char* tag_revision = nullptr;
  int major;
  int minor;

  AddOn::CAddOnGeneral::kodi_version(addon, compile_name, major, minor, revision, tag, tag_revision);
  resp.init(req.getRequestID());

  if (compile_name != nullptr)
  {
    resp.push(API_STRING, compile_name);
    CAddonCallbacksAddon::free_string(addon, compile_name);
  }
  else
    resp.push(API_STRING, "");

  resp.push(API_INT, &major);
  resp.push(API_INT, &minor);

  if (revision != nullptr)
  {
    resp.push(API_STRING, revision);
    CAddonCallbacksAddon::free_string(addon, revision);
  }
  else
    resp.push(API_STRING, "");
  if (tag != nullptr)
  {
    resp.push(API_STRING, tag);
    CAddonCallbacksAddon::free_string(addon, tag);
  }
  else
    resp.push(API_STRING, "");
  if (tag_revision != nullptr)
  {
    resp.push(API_STRING, tag_revision);
    CAddonCallbacksAddon::free_string(addon, tag_revision);
  }
  else
    resp.push(API_STRING, "");

  resp.finalise();
  return true;
}

bool CAddonExeCB_Addon_General::KodiQuit(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  AddOn::CAddOnGeneral::kodi_quit(addon);
  PROCESS_ADDON_RETURN_CALL(API_SUCCESS);
  return true;
}

bool CAddonExeCB_Addon_General::HTPCShutdown(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  AddOn::CAddOnGeneral::htpc_shutdown(addon);
  PROCESS_ADDON_RETURN_CALL(API_SUCCESS);
  return true;
}

bool CAddonExeCB_Addon_General::HTPCRestart(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  AddOn::CAddOnGeneral::htpc_restart(addon);
  PROCESS_ADDON_RETURN_CALL(API_SUCCESS);
  return true;
}

bool CAddonExeCB_Addon_General::ExecuteScript(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  char* script;
  req.pop(API_STRING,    &script);
  AddOn::CAddOnGeneral::execute_script(addon, script);
  PROCESS_ADDON_RETURN_CALL(API_SUCCESS);
  return true;
}

bool CAddonExeCB_Addon_General::ExecuteBuiltin(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  char* function;
  bool wait;
  req.pop(API_STRING,    &function);
  req.pop(API_BOOLEAN,   &wait);
  AddOn::CAddOnGeneral::execute_builtin(addon, function, wait);
  PROCESS_ADDON_RETURN_CALL(API_SUCCESS);
  return true;
}

bool CAddonExeCB_Addon_General::ExecuteJSONRPC(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  char* jsonrpccommand;
  std::string retStr;
  req.pop(API_STRING, &jsonrpccommand);
  char* strMsg = AddOn::CAddOnGeneral::execute_jsonrpc(addon, jsonrpccommand);
  if (strMsg != nullptr)
  {
    retStr = strMsg;
    CAddonCallbacksAddon::free_string(addon, strMsg);
  }
  PROCESS_ADDON_RETURN_CALL_WITH_STRING(API_SUCCESS, retStr.c_str());
  return true;
}

bool CAddonExeCB_Addon_General::GetRegion(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  char* id;
  std::string retStr;
  req.pop(API_STRING, &id);
  char* strMsg = AddOn::CAddOnGeneral::get_region(addon, id);
  if (strMsg != nullptr)
  {
    retStr = strMsg;
    CAddonCallbacksAddon::free_string(addon, strMsg);
  }
  PROCESS_ADDON_RETURN_CALL_WITH_STRING(API_SUCCESS, retStr.c_str());
  return true;
}

bool CAddonExeCB_Addon_General::GetFreeMem(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  long freeMem = AddOn::CAddOnGeneral::get_free_mem(addon);
  PROCESS_ADDON_RETURN_CALL_WITH_LONG(API_SUCCESS, &freeMem);
  return true;
}

bool CAddonExeCB_Addon_General::GetGlobalIdleTime(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  int time = AddOn::CAddOnGeneral::get_global_idle_time(addon);
  PROCESS_ADDON_RETURN_CALL_WITH_INT(API_SUCCESS, &time);
  return true;
}

bool CAddonExeCB_Addon_General::TranslatePath(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  char* path;
  std::string retStr;
  req.pop(API_STRING, &path);
  char* strMsg = AddOn::CAddOnGeneral::translate_path(addon, path);
  if (strMsg != nullptr)
  {
    retStr = strMsg;
    CAddonCallbacksAddon::free_string(addon, strMsg);
  }
  PROCESS_ADDON_RETURN_CALL_WITH_STRING(API_SUCCESS, retStr.c_str());
  return true;
}

}; /* namespace KodiAPI */
}; /* namespace V2 */
