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

#include <cstring>

extern "C"
{

std::string AddonPlayer_GetSupportedMedia(unsigned int mediaType)
{
  std::string strReturn;
  char* strMsg = g_interProcess.m_Callbacks->AddonPlayer.GetSupportedMedia(g_interProcess.m_Handle->addonData, mediaType);
  if (strMsg != nullptr)
  {
    if (std::strlen(strMsg))
      strReturn = strMsg;
    g_interProcess.m_Callbacks->free_string(g_interProcess.m_Handle->addonData, strMsg);
  }
  return strReturn;
}

PLAYERHANDLE AddonPlayer_New()
{
  return g_interProcess.m_Callbacks->AddonPlayer.New(g_interProcess.m_Handle->addonData);
}

void AddonPlayer_Delete(PLAYERHANDLE player)
{
  g_interProcess.m_Callbacks->AddonPlayer.Delete(g_interProcess.m_Handle->addonData, player);
}

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
        void      (*CBOnPlayBackSeekChapter) (PLAYERHANDLE cbhdl, int iChapter))
{
  if (!player || !cbhdl ||
        !CBOnPlayBackStarted || !CBOnPlayBackEnded || !CBOnPlayBackStopped || !CBOnPlayBackPaused ||
        !CBOnPlayBackResumed || !CBOnQueueNextItem || !CBOnPlayBackSpeedChanged ||
        !CBOnPlayBackSeek    || !CBOnPlayBackSeekChapter)
  {
    fprintf(stderr, "ERROR: CPlayerLib_Player - SetIndependentCallbacks: called with nullptr !!!\n");
    return;
  }

  g_interProcess.m_Callbacks->AddonPlayer.SetCallbacks(g_interProcess.m_Handle->addonData, player, cbhdl,
                                   CBOnPlayBackStarted, CBOnPlayBackEnded,
                                   CBOnPlayBackStopped, CBOnPlayBackPaused,
                                   CBOnPlayBackResumed, CBOnQueueNextItem,
                                   CBOnPlayBackSpeedChanged, CBOnPlayBackSeek,
                                   CBOnPlayBackSeekChapter);
}

bool AddonPlayer_PlayFile(PLAYERHANDLE player, const std::string& item, bool windowed)
{
  return g_interProcess.m_Callbacks->AddonPlayer.PlayFile(g_interProcess.m_Handle->addonData, player, item.c_str(), windowed);
}

bool AddonPlayer_PlayFileItem(PLAYERHANDLE player, const GUIHANDLE listitem, bool windowed)
{
  return g_interProcess.m_Callbacks->AddonPlayer.PlayFileItem(g_interProcess.m_Handle->addonData, player, listitem, windowed);
}

bool AddonPlayer_PlayList(PLAYERHANDLE player, const GUIHANDLE list, int playlist, bool windowed, int startpos)
{
  return g_interProcess.m_Callbacks->AddonPlayer.PlayList(g_interProcess.m_Handle->addonData, player, list, playlist, windowed, startpos);
}

void AddonPlayer_Stop(PLAYERHANDLE player)
{
  g_interProcess.m_Callbacks->AddonPlayer.Stop(g_interProcess.m_Handle->addonData, player);
}

void AddonPlayer_Pause(PLAYERHANDLE player)
{
  g_interProcess.m_Callbacks->AddonPlayer.Pause(g_interProcess.m_Handle->addonData, player);
}

void AddonPlayer_PlayNext(PLAYERHANDLE player)
{
  g_interProcess.m_Callbacks->AddonPlayer.PlayNext(g_interProcess.m_Handle->addonData, player);
}

void AddonPlayer_PlayPrevious(PLAYERHANDLE player)
{
  g_interProcess.m_Callbacks->AddonPlayer.PlayPrevious(g_interProcess.m_Handle->addonData, player);
}

void AddonPlayer_PlaySelected(PLAYERHANDLE player, int selected)
{
  g_interProcess.m_Callbacks->AddonPlayer.PlaySelected(g_interProcess.m_Handle->addonData, player, selected);
}

bool AddonPlayer_IsPlaying(PLAYERHANDLE player)
{
  return g_interProcess.m_Callbacks->AddonPlayer.IsPlaying(g_interProcess.m_Handle->addonData, player);
}

bool AddonPlayer_IsPlayingAudio(PLAYERHANDLE player)
{
  return g_interProcess.m_Callbacks->AddonPlayer.IsPlayingAudio(g_interProcess.m_Handle->addonData, player);
}

bool AddonPlayer_IsPlayingVideo(PLAYERHANDLE player)
{
  return g_interProcess.m_Callbacks->AddonPlayer.IsPlayingVideo(g_interProcess.m_Handle->addonData, player);
}

bool AddonPlayer_IsPlayingRDS(PLAYERHANDLE player)
{
  return g_interProcess.m_Callbacks->AddonPlayer.IsPlayingRDS(g_interProcess.m_Handle->addonData, player);
}

bool AddonPlayer_GetPlayingFile(PLAYERHANDLE player, std::string& file)
{
  file.resize(1024);
  unsigned int size = (unsigned int)file.capacity();
  bool ret = g_interProcess.m_Callbacks->AddonPlayer.GetPlayingFile(g_interProcess.m_Handle->addonData, player, file[0], size);
  file.resize(size);
  file.shrink_to_fit();
  return ret;
}

double AddonPlayer_GetTotalTime(PLAYERHANDLE player)
{
  return g_interProcess.m_Callbacks->AddonPlayer.GetTotalTime(g_interProcess.m_Handle->addonData, player);
}

double AddonPlayer_GetTime(PLAYERHANDLE player)
{
  return g_interProcess.m_Callbacks->AddonPlayer.GetTime(g_interProcess.m_Handle->addonData, player);
}

void AddonPlayer_SeekTime(PLAYERHANDLE player, double seekTime)
{
  g_interProcess.m_Callbacks->AddonPlayer.SeekTime(g_interProcess.m_Handle->addonData, player, seekTime);
}

bool AddonPlayer_GetAvailableVideoStreams(PLAYERHANDLE player, std::vector<std::string> &streams)
{
  char** list;
  unsigned int listSize = 0;
  bool ret = g_interProcess.m_Callbacks->AddonPlayer.GetAvailableVideoStreams(g_interProcess.m_Handle->addonData, player, list, listSize);
  if (ret)
  {
    for (unsigned int i = 0; i < listSize; ++i)
      streams.push_back(list[i]);
    g_interProcess.m_Callbacks->AddonPlayer.ClearList(list, listSize);
  }
  return ret;
}

void AddonPlayer_SetVideoStream(PLAYERHANDLE player, int iStream)
{
  g_interProcess.m_Callbacks->AddonPlayer.SetVideoStream(g_interProcess.m_Handle->addonData, player, iStream);
}

bool AddonPlayer_GetAvailableAudioStreams(PLAYERHANDLE player, std::vector<std::string> &streams)
{
  char** list;
  unsigned int listSize = 0;
  bool ret = g_interProcess.m_Callbacks->AddonPlayer.GetAvailableAudioStreams(g_interProcess.m_Handle->addonData, player, list, listSize);
  if (ret)
  {
    for (unsigned int i = 0; i < listSize; ++i)
      streams.push_back(list[i]);
    g_interProcess.m_Callbacks->AddonPlayer.ClearList(list, listSize);
  }
  return ret;
}

void AddonPlayer_SetAudioStream(PLAYERHANDLE player, int iStream)
{
  g_interProcess.m_Callbacks->AddonPlayer.SetAudioStream(g_interProcess.m_Handle->addonData, player, iStream);
}

bool AddonPlayer_GetAvailableSubtitleStreams(PLAYERHANDLE player, std::vector<std::string> &streams)
{
  char** list;
  unsigned int listSize = 0;
  bool ret = g_interProcess.m_Callbacks->AddonPlayer.GetAvailableSubtitleStreams(g_interProcess.m_Handle->addonData, player, list, listSize);
  if (ret)
  {
    for (unsigned int i = 0; i < listSize; ++i)
      streams.push_back(list[i]);
    g_interProcess.m_Callbacks->AddonPlayer.ClearList(list, listSize);
  }
  return ret;
}

void AddonPlayer_SetSubtitleStream(PLAYERHANDLE player, int iStream)
{
  g_interProcess.m_Callbacks->AddonPlayer.SetSubtitleStream(g_interProcess.m_Handle->addonData, player, iStream);
}

void AddonPlayer_ShowSubtitles(PLAYERHANDLE player, bool bVisible)
{
  g_interProcess.m_Callbacks->AddonPlayer.ShowSubtitles(g_interProcess.m_Handle->addonData, player, bVisible);
}

bool AddonPlayer_GetCurrentSubtitleName(PLAYERHANDLE player, std::string& name)
{
  name.resize(1024);
  unsigned int size = (unsigned int)name.capacity();
  bool ret = g_interProcess.m_Callbacks->AddonPlayer.GetCurrentSubtitleName(g_interProcess.m_Handle->addonData, player, name[0], size);
  name.resize(size);
  name.shrink_to_fit();
  return ret;
}

void AddonPlayer_AddSubtitle(PLAYERHANDLE player, const std::string& subPath)
{
  g_interProcess.m_Callbacks->AddonPlayer.AddSubtitle(g_interProcess.m_Handle->addonData, player, subPath.c_str());
}

}; /* extern "C" */
