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

#include "addons/binary/binary-executable/requestpacket.h"
#include "addons/binary/binary-executable/responsepacket.h"

struct AddonCB;

namespace V2
{
namespace KodiAPI
{

struct CAddonExeCB_Addon_General
{
  static bool GetSettingString(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool GetSettingInt(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool GetSettingBoolean(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool GetSettingFloat(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool OpenSettings(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool GetAddonInfo(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool QueueFormattedNotification(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool QueueNotificationFromType(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool QueueNotificationWithImage(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool GetMD5(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool UnknownToUTF8(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool GetLocalizedString(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool GetLanguage(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool GetDVDMenuLanguage(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool StartServer(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool AudioSuspend(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool AudioResume(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool GetVolume(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool SetVolume(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool IsMuted(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool ToggleMute(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool SetMute(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool GetOpticalDriveState(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool EjectOpticalDrive(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool KodiVersion(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool KodiQuit(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool HTPCShutdown(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool HTPCRestart(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool ExecuteScript(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool ExecuteBuiltin(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool ExecuteJSONRPC(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool GetRegion(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool GetFreeMem(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool GetGlobalIdleTime(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool TranslatePath(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
};

}; /* namespace KodiAPI */
}; /* namespace V2 */
