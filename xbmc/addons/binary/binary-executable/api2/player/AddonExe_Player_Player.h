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

struct CAddonExeCB_Player_Player
{
  static bool AddonPlayer_GetSupportedMedia(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool AddonPlayer_New(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool AddonPlayer_Delete(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool AddonPlayer_SetCallbacks(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool AddonPlayer_PlayFile(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool AddonPlayer_PlayFileItem(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool AddonPlayer_PlayList(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool AddonPlayer_Stop(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool AddonPlayer_Pause(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool AddonPlayer_PlayNext(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool AddonPlayer_PlayPrevious(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool AddonPlayer_PlaySelected(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool AddonPlayer_IsPlaying(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool AddonPlayer_IsPlayingAudio(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool AddonPlayer_IsPlayingVideo(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool AddonPlayer_IsPlayingRDS(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool AddonPlayer_GetPlayingFile(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool AddonPlayer_GetTotalTime(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool AddonPlayer_GetTime(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool AddonPlayer_SeekTime(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool AddonPlayer_GetAvailableVideoStreams(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool AddonPlayer_SetVideoStream(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool AddonPlayer_GetAvailableAudioStreams(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool AddonPlayer_SetAudioStream(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool AddonPlayer_GetAvailableSubtitleStreams(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool AddonPlayer_SetSubtitleStream(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool AddonPlayer_ShowSubtitles(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool AddonPlayer_GetCurrentSubtitleName(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool AddonPlayer_AddSubtitle(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
};

}; /* namespace KodiAPI */
}; /* namespace V2 */
