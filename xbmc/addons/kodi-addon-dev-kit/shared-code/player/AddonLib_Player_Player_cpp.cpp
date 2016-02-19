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
#include "kodi/api2/player/Player.hpp"

using namespace V2::KodiAPI::GUI;

namespace V2
{
namespace KodiAPI
{

namespace Player
{
  CPlayer::CPlayer()
  {
    m_ControlHandle = g_interProcess.AddonPlayer_New();
    if (m_ControlHandle)
      g_interProcess.AddonPlayer_SetCallbacks(m_ControlHandle, this,
                                     CBOnPlayBackStarted, CBOnPlayBackEnded,
                                     CBOnPlayBackStopped, CBOnPlayBackPaused,
                                     CBOnPlayBackResumed, CBOnQueueNextItem,
                                     CBOnPlayBackSpeedChanged, CBOnPlayBackSeek,
                                     CBOnPlayBackSeekChapter);
    else
      fprintf(stderr, "ERROR: CPlayer can't create control class from Kodi !!!\n");
  }

  CPlayer::~CPlayer()
  {
    g_interProcess.AddonPlayer_Delete(m_ControlHandle);
  }

  std::string CPlayer::GetSupportedMedia(AddonPlayListType mediaType)
  {
    return g_interProcess.AddonPlayer_GetSupportedMedia(mediaType);
  }

  bool CPlayer::Play(const std::string& item, bool windowed)
  {
    return g_interProcess.AddonPlayer_PlayFile(m_ControlHandle, item, windowed);
  }

  bool CPlayer::Play(const CListItem *listitem, bool windowed)
  {
    return g_interProcess.AddonPlayer_PlayFileItem(m_ControlHandle, listitem->GetListItemHandle(), windowed);
  }

  bool CPlayer::Play(const CPlayList *list, bool windowed, int startpos)
  {
    return g_interProcess.AddonPlayer_PlayList(m_ControlHandle, list->GetListHandle(), list->GetListType(), windowed, startpos);
  }

  void CPlayer::Stop()
  {
    g_interProcess.AddonPlayer_Stop(m_ControlHandle);
  }

  void CPlayer::Pause()
  {
    g_interProcess.AddonPlayer_Pause(m_ControlHandle);
  }

  void CPlayer::PlayNext()
  {
    g_interProcess.AddonPlayer_PlayNext(m_ControlHandle);
  }

  void CPlayer::PlayPrevious()
  {
    g_interProcess.AddonPlayer_PlayPrevious(m_ControlHandle);
  }

  void CPlayer::PlaySelected(int selected)
  {
    g_interProcess.AddonPlayer_PlaySelected(m_ControlHandle, selected);
  }

  bool CPlayer::IsPlaying()
  {
    return g_interProcess.AddonPlayer_IsPlaying(m_ControlHandle);
  }

  bool CPlayer::IsPlayingAudio()
  {
    return g_interProcess.AddonPlayer_IsPlayingAudio(m_ControlHandle);
  }

  bool CPlayer::IsPlayingVideo()
  {
    return g_interProcess.AddonPlayer_IsPlayingVideo(m_ControlHandle);
  }

  bool CPlayer::IsPlayingRDS()
  {
    return g_interProcess.AddonPlayer_IsPlayingRDS(m_ControlHandle);
  }

  bool CPlayer::GetPlayingFile(std::string& file)
  {
    return g_interProcess.AddonPlayer_GetPlayingFile(m_ControlHandle, file);
  }

  double CPlayer::GetTotalTime()
  {
    return g_interProcess.AddonPlayer_GetTotalTime(m_ControlHandle);
  }
  
  double CPlayer::GetTime()
  {
    return g_interProcess.AddonPlayer_GetTime(m_ControlHandle);
  }
  
  void CPlayer::SeekTime(double seekTime)
  {
    g_interProcess.AddonPlayer_SeekTime(m_ControlHandle, seekTime);
  }

  bool CPlayer::GetAvailableAudioStreams(std::vector<std::string> &streams)
  {
    return g_interProcess.AddonPlayer_GetAvailableAudioStreams(m_ControlHandle, streams);
  }
    
  void CPlayer::SetAudioStream(int iStream)
  {
    g_interProcess.AddonPlayer_SetAudioStream(m_ControlHandle, iStream);
  }

  bool CPlayer::GetAvailableSubtitleStreams(std::vector<std::string> &streams)
  {
    return g_interProcess.AddonPlayer_GetAvailableSubtitleStreams(m_ControlHandle, streams);
  }

  void CPlayer::SetSubtitleStream(int iStream)
  {
    g_interProcess.AddonPlayer_SetSubtitleStream(m_ControlHandle, iStream);
  }
  
  void CPlayer::ShowSubtitles(bool bVisible)
  {
    g_interProcess.AddonPlayer_ShowSubtitles(m_ControlHandle, bVisible);
  }

  bool CPlayer::GetCurrentSubtitleName(std::string& name)
  {
    return g_interProcess.AddonPlayer_GetCurrentSubtitleName(m_ControlHandle, name);
  }

  void CPlayer::AddSubtitle(const std::string& subPath)
  {
    g_interProcess.AddonPlayer_AddSubtitle(m_ControlHandle, subPath);
  }

  /*!
   * Kodi to add-on override defination function to use if class becomes used
   * independet.
   */

  void CPlayer::SetIndependentCallbacks(
        PLAYERHANDLE     cbhdl,
        void      (*IndptCBOnPlayBackStarted)     (PLAYERHANDLE cbhdl),
        void      (*IndptCBOnPlayBackEnded)       (PLAYERHANDLE cbhdl),
        void      (*IndptCBOnPlayBackStopped)     (PLAYERHANDLE cbhdl),
        void      (*IndptCBOnPlayBackPaused)      (PLAYERHANDLE cbhdl),
        void      (*IndptCBOnPlayBackResumed)     (PLAYERHANDLE cbhdl),
        void      (*IndptCBOnQueueNextItem)       (PLAYERHANDLE cbhdl),
        void      (*IndptCBOnPlayBackSpeedChanged)(PLAYERHANDLE cbhdl, int iSpeed),
        void      (*IndptCBOnPlayBackSeek)        (PLAYERHANDLE cbhdl, int iTime, int seekOffset),
        void      (*IndptCBOnPlayBackSeekChapter) (PLAYERHANDLE cbhdl, int iChapter))
  {
    if (!cbhdl ||
        !IndptCBOnPlayBackStarted || !IndptCBOnPlayBackEnded || !IndptCBOnPlayBackStopped || !IndptCBOnPlayBackPaused ||
        !IndptCBOnPlayBackResumed || !IndptCBOnQueueNextItem || !IndptCBOnPlayBackSpeedChanged ||
        !IndptCBOnPlayBackSeek    || !IndptCBOnPlayBackSeekChapter)
    {
        fprintf(stderr, "ERROR: CPlayer - SetIndependentCallbacks: called with nullptr !!!\n");
        return;
    }

    g_interProcess.AddonPlayer_SetCallbacks(m_ControlHandle, cbhdl,
                                            IndptCBOnPlayBackStarted, IndptCBOnPlayBackEnded,
                                            IndptCBOnPlayBackStopped, IndptCBOnPlayBackPaused,
                                            IndptCBOnPlayBackResumed, IndptCBOnQueueNextItem,
                                            IndptCBOnPlayBackSpeedChanged, IndptCBOnPlayBackSeek,
                                            IndptCBOnPlayBackSeekChapter);
  }

  /*!
   * Defined callback functions from Kodi to add-on, for use in parent / child system
   * (is private)!
   */

  void CPlayer::CBOnPlayBackStarted(PLAYERHANDLE cbhdl)
  {
    static_cast<CPlayer*>(cbhdl)->OnPlayBackStarted();
  }
  
  void CPlayer::CBOnPlayBackEnded(PLAYERHANDLE cbhdl)
  {
    static_cast<CPlayer*>(cbhdl)->OnPlayBackEnded();
  }
  
  void CPlayer::CBOnPlayBackStopped(PLAYERHANDLE cbhdl)
  {
    static_cast<CPlayer*>(cbhdl)->OnPlayBackStopped();
  }
  
  void CPlayer::CBOnPlayBackPaused(PLAYERHANDLE cbhdl)
  {
    static_cast<CPlayer*>(cbhdl)->OnPlayBackPaused();
  }
  
  void CPlayer::CBOnPlayBackResumed(PLAYERHANDLE cbhdl)
  {
    static_cast<CPlayer*>(cbhdl)->OnPlayBackResumed();
  }
  
  void CPlayer::CBOnQueueNextItem(PLAYERHANDLE cbhdl)
  {
    static_cast<CPlayer*>(cbhdl)->OnQueueNextItem();
  }
  
  void CPlayer::CBOnPlayBackSpeedChanged(PLAYERHANDLE cbhdl, int iSpeed)
  {
    static_cast<CPlayer*>(cbhdl)->OnPlayBackSpeedChanged(iSpeed);
  }
  
  void CPlayer::CBOnPlayBackSeek(PLAYERHANDLE cbhdl, int iTime, int seekOffset)
  {
    static_cast<CPlayer*>(cbhdl)->OnPlayBackSeek(iTime, seekOffset);
  }
  
  void CPlayer::CBOnPlayBackSeekChapter(PLAYERHANDLE cbhdl, int iChapter)
  {
    static_cast<CPlayer*>(cbhdl)->OnPlayBackSeekChapter(iChapter);
  }

}; /* namespace Player */

}; /* namespace KodiAPI */
}; /* namespace V2 */
