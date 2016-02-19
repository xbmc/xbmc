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

struct CAddonExeCB_AudioEngine_Stream
{
  static bool AE_MakeStream(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool AE_FreeStream(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool AE_Stream_GetSpace(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool AE_Stream_AddData(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool AE_Stream_GetDelay(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool AE_Stream_IsBuffering(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool AE_Stream_GetCacheTime(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool AE_Stream_GetCacheTotal(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool AE_Stream_Pause(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool AE_Stream_Resume(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool AE_Stream_Drain(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool AE_Stream_IsDraining(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool AE_Stream_IsDrained(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool AE_Stream_Flush(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool AE_Stream_GetVolume(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool AE_Stream_SetVolume(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool AE_Stream_GetAmplification(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool AE_Stream_SetAmplification(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool AE_Stream_GetFrameSize(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool AE_Stream_GetChannelCount(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool AE_Stream_GetSampleRate(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool AE_Stream_GetDataFormat(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool AE_Stream_GetResampleRatio(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool AE_Stream_SetResampleRatio(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
};

}; /* namespace KodiAPI */
}; /* namespace V2 */
