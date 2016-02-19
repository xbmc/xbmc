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

#include "kodi/api2/.internal/AddonLib_internal.hpp"

#include <string>

extern "C"
{

  struct CKODIAddon_InterProcess_Player_Player
  {
    std::string AddonPlayer_GetSupportedMedia(unsigned int mediaType);

    PLAYERHANDLE AddonPlayer_New();
    void AddonPlayer_Delete(PLAYERHANDLE player);
    void AddonPlayer_SetCallbacks(
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
        void      (*CBOnPlayBackSeekChapter) (PLAYERHANDLE cbhdl, int iChapter));
    bool AddonPlayer_PlayFile(PLAYERHANDLE player, const std::string& item, bool windowed);
    bool AddonPlayer_PlayFileItem(PLAYERHANDLE player, const GUIHANDLE listitem, bool windowed);
    bool AddonPlayer_PlayList(PLAYERHANDLE player, const GUIHANDLE list, int playlist, bool windowed, int startpos);
    void AddonPlayer_Stop(PLAYERHANDLE player);
    void AddonPlayer_Pause(PLAYERHANDLE player);
    void AddonPlayer_PlayNext(PLAYERHANDLE player);
    void AddonPlayer_PlayPrevious(PLAYERHANDLE player);
    void AddonPlayer_PlaySelected(PLAYERHANDLE player, int selected);
    bool AddonPlayer_IsPlaying(PLAYERHANDLE player);
    bool AddonPlayer_IsPlayingAudio(PLAYERHANDLE player);
    bool AddonPlayer_IsPlayingVideo(PLAYERHANDLE player);
    bool AddonPlayer_IsPlayingRDS(PLAYERHANDLE player);

    bool AddonPlayer_GetPlayingFile(PLAYERHANDLE player, std::string& file);
    double AddonPlayer_GetTotalTime(PLAYERHANDLE player);
    double AddonPlayer_GetTime(PLAYERHANDLE player);
    void AddonPlayer_SeekTime(PLAYERHANDLE player, double seekTime);

    bool AddonPlayer_GetAvailableAudioStreams(PLAYERHANDLE player, std::vector<std::string> &streams);
    void AddonPlayer_SetAudioStream(PLAYERHANDLE player, int iStream);

    bool AddonPlayer_GetAvailableSubtitleStreams(PLAYERHANDLE player, std::vector<std::string> &streams);
    void AddonPlayer_SetSubtitleStream(PLAYERHANDLE player, int iStream);
    void AddonPlayer_ShowSubtitles(PLAYERHANDLE player, bool bVisible);
    bool AddonPlayer_GetCurrentSubtitleName(PLAYERHANDLE player, std::string& name);
    void AddonPlayer_AddSubtitle(PLAYERHANDLE player, const std::string& subPath);
  };

}; /* extern "C" */
