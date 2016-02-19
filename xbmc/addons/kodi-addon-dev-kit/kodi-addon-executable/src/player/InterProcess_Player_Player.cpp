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

#include "InterProcess_Player_Player.h"
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

std::string CKODIAddon_InterProcess_Player_Player::AddonPlayer_GetSupportedMedia(unsigned int mediaType)
{
}

PLAYERHANDLE CKODIAddon_InterProcess_Player_Player::AddonPlayer_New()
{
}

void CKODIAddon_InterProcess_Player_Player::AddonPlayer_Delete(PLAYERHANDLE player)
{
}

void CKODIAddon_InterProcess_Player_Player::AddonPlayer_SetCallbacks(
        PLAYERHANDLE      player,
        PLAYERHANDLE      cbhdl,
        void      (*CBOnPlayBackStarted)     (PLAYERHANDLE cbhdl),
        void      (*CBOnPlayBackEnded)       (PLAYERHANDLE cbhdl),
        void      (*CBOnPlayBackStopped)     (PLAYERHANDLE cbhdl),
        void      (*CBOnPlayBackPaused)      (PLAYERHANDLE cbhdl),
        void      (*CBOnPlayBackResumed)     (PLAYERHANDLE cbhdl),
        void      (*CBOnQueueNextItem)       (PLAYERHANDLE cbhdl),
        void      (*CBOnPlayBackSpeedChanged)(PLAYERHANDLE cbhdl, int iSpeed),
        void      (*CBOnPlayBackSeek)        (PLAYERHANDLE cbhdl, int iTime, int seekOffset),
        void      (*CBOnPlayBackSeekChapter) (PLAYERHANDLE cbhdl, int iChapter))
{
}

bool CKODIAddon_InterProcess_Player_Player::AddonPlayer_PlayFile(PLAYERHANDLE player, const std::string& item, bool windowed)
{
}

bool CKODIAddon_InterProcess_Player_Player::AddonPlayer_PlayFileItem(PLAYERHANDLE player, const GUIHANDLE listitem, bool windowed)
{
}

bool CKODIAddon_InterProcess_Player_Player::AddonPlayer_PlayList(PLAYERHANDLE player, const GUIHANDLE list, int playlist, bool windowed, int startpos)
{
}

void CKODIAddon_InterProcess_Player_Player::AddonPlayer_Stop(PLAYERHANDLE player)
{
}

void CKODIAddon_InterProcess_Player_Player::AddonPlayer_Pause(PLAYERHANDLE player)
{
}

void CKODIAddon_InterProcess_Player_Player::AddonPlayer_PlayNext(PLAYERHANDLE player)
{
}

void CKODIAddon_InterProcess_Player_Player::AddonPlayer_PlayPrevious(PLAYERHANDLE player)
{
}

void CKODIAddon_InterProcess_Player_Player::AddonPlayer_PlaySelected(PLAYERHANDLE player, int selected)
{
}

bool CKODIAddon_InterProcess_Player_Player::AddonPlayer_IsPlaying(PLAYERHANDLE player)
{
}

bool CKODIAddon_InterProcess_Player_Player::AddonPlayer_IsPlayingAudio(PLAYERHANDLE player)
{
}

bool CKODIAddon_InterProcess_Player_Player::AddonPlayer_IsPlayingVideo(PLAYERHANDLE player)
{
}

bool CKODIAddon_InterProcess_Player_Player::AddonPlayer_IsPlayingRDS(PLAYERHANDLE player)
{
}

bool CKODIAddon_InterProcess_Player_Player::AddonPlayer_GetPlayingFile(PLAYERHANDLE player, std::string& file)
{
}

double CKODIAddon_InterProcess_Player_Player::AddonPlayer_GetTotalTime(PLAYERHANDLE player)
{
}

double CKODIAddon_InterProcess_Player_Player::AddonPlayer_GetTime(PLAYERHANDLE player)
{
}

void CKODIAddon_InterProcess_Player_Player::AddonPlayer_SeekTime(PLAYERHANDLE player, double seekTime)
{
}

bool CKODIAddon_InterProcess_Player_Player::AddonPlayer_GetAvailableVideoStreams(PLAYERHANDLE player, std::vector<std::string> &streams)
{
}

void CKODIAddon_InterProcess_Player_Player::AddonPlayer_SetVideoStream(PLAYERHANDLE player, int iStream)
{
}

bool CKODIAddon_InterProcess_Player_Player::AddonPlayer_GetAvailableAudioStreams(PLAYERHANDLE player, std::vector<std::string> &streams)
{
}

void CKODIAddon_InterProcess_Player_Player::AddonPlayer_SetAudioStream(PLAYERHANDLE player, int iStream)
{
}

bool CKODIAddon_InterProcess_Player_Player::AddonPlayer_GetAvailableSubtitleStreams(PLAYERHANDLE player, std::vector<std::string> &streams)
{
}

void CKODIAddon_InterProcess_Player_Player::AddonPlayer_SetSubtitleStream(PLAYERHANDLE player, int iStream)
{
}

void CKODIAddon_InterProcess_Player_Player::AddonPlayer_ShowSubtitles(PLAYERHANDLE player, bool bVisible)
{
}

bool CKODIAddon_InterProcess_Player_Player::AddonPlayer_GetCurrentSubtitleName(PLAYERHANDLE player, std::string& name)
{
}

void CKODIAddon_InterProcess_Player_Player::AddonPlayer_AddSubtitle(PLAYERHANDLE player, const std::string& subPath)
{
}

}; /* extern "C" */
