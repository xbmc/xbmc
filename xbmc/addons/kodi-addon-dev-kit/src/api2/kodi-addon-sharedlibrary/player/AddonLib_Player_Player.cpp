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
#include KITINCLUDE(ADDON_API_LEVEL, player/Player.hpp)

#include <cstring>

using namespace API_NAMESPACE_NAME::KodiAPI::GUI;

API_NAMESPACE

namespace KodiAPI
{

namespace Player
{
  CPlayer::CPlayer()
  {
    m_ControlHandle = g_interProcess.m_Callbacks->AddonPlayer.New(g_interProcess.m_Handle);
    if (m_ControlHandle)
      g_interProcess.m_Callbacks->AddonPlayer.SetCallbacks(g_interProcess.m_Handle, m_ControlHandle, this,
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
    g_interProcess.m_Callbacks->AddonPlayer.Delete(g_interProcess.m_Handle, m_ControlHandle);
  }

  std::string CPlayer::GetSupportedMedia(AddonPlayListType mediaType)
  {
    std::string strReturn;
    char* strMsg = g_interProcess.m_Callbacks->AddonPlayer.GetSupportedMedia(g_interProcess.m_Handle, mediaType);
    if (strMsg != nullptr)
    {
      if (std::strlen(strMsg))
        strReturn = strMsg;
      g_interProcess.m_Callbacks->free_string(g_interProcess.m_Handle, strMsg);
    }
    return strReturn;
  }

  bool CPlayer::Play(const std::string& item, bool windowed)
  {
    return g_interProcess.m_Callbacks->AddonPlayer.PlayFile(g_interProcess.m_Handle, m_ControlHandle, item.c_str(), windowed);
  }

  bool CPlayer::Play(const CListItem *listitem, bool windowed)
  {
    return g_interProcess.m_Callbacks->AddonPlayer.PlayFileItem(g_interProcess.m_Handle, m_ControlHandle, listitem->GetListItemHandle(), windowed);
  }

  bool CPlayer::Play(const CPlayList *list, bool windowed, int startpos)
  {
    return g_interProcess.m_Callbacks->AddonPlayer.PlayList(g_interProcess.m_Handle, m_ControlHandle, list->GetListHandle(), list->GetListType(), windowed, startpos);
  }

  void CPlayer::Stop()
  {
    g_interProcess.m_Callbacks->AddonPlayer.Stop(g_interProcess.m_Handle, m_ControlHandle);
  }

  void CPlayer::Pause()
  {
    g_interProcess.m_Callbacks->AddonPlayer.Pause(g_interProcess.m_Handle, m_ControlHandle);
  }

  void CPlayer::PlayNext()
  {
    g_interProcess.m_Callbacks->AddonPlayer.PlayNext(g_interProcess.m_Handle, m_ControlHandle);
  }

  void CPlayer::PlayPrevious()
  {
    g_interProcess.m_Callbacks->AddonPlayer.PlayPrevious(g_interProcess.m_Handle, m_ControlHandle);
  }

  void CPlayer::PlaySelected(int selected)
  {
    g_interProcess.m_Callbacks->AddonPlayer.PlaySelected(g_interProcess.m_Handle, m_ControlHandle, selected);
  }

  bool CPlayer::IsPlaying()
  {
    return g_interProcess.m_Callbacks->AddonPlayer.IsPlaying(g_interProcess.m_Handle, m_ControlHandle);
  }

  bool CPlayer::IsPlayingAudio()
  {
    return g_interProcess.m_Callbacks->AddonPlayer.IsPlayingAudio(g_interProcess.m_Handle, m_ControlHandle);
  }

  bool CPlayer::IsPlayingVideo()
  {
    return g_interProcess.m_Callbacks->AddonPlayer.IsPlayingVideo(g_interProcess.m_Handle, m_ControlHandle);
  }

  bool CPlayer::IsPlayingRDS()
  {
    return g_interProcess.m_Callbacks->AddonPlayer.IsPlayingRDS(g_interProcess.m_Handle, m_ControlHandle);
  }

  bool CPlayer::GetPlayingFile(std::string& file)
  {
    file.resize(1024);
    unsigned int size = (unsigned int)file.capacity();
    bool ret = g_interProcess.m_Callbacks->AddonPlayer.GetPlayingFile(g_interProcess.m_Handle, m_ControlHandle, file[0], size);
    file.resize(size);
    file.shrink_to_fit();
    return ret;
  }

  double CPlayer::GetTotalTime()
  {
    return g_interProcess.m_Callbacks->AddonPlayer.GetTotalTime(g_interProcess.m_Handle, m_ControlHandle);
  }
  
  double CPlayer::GetTime()
  {
    return g_interProcess.m_Callbacks->AddonPlayer.GetTime(g_interProcess.m_Handle, m_ControlHandle);
  }
  
  void CPlayer::SeekTime(double seekTime)
  {
    g_interProcess.m_Callbacks->AddonPlayer.SeekTime(g_interProcess.m_Handle, m_ControlHandle, seekTime);
  }

  bool CPlayer::GetAvailableAudioStreams(std::vector<std::string> &streams)
  {
    char** list;
    unsigned int listSize = 0;
    bool ret = g_interProcess.m_Callbacks->AddonPlayer.GetAvailableAudioStreams(g_interProcess.m_Handle, m_ControlHandle, list, listSize);
    if (ret)
    {
      for (unsigned int i = 0; i < listSize; ++i)
        streams.push_back(list[i]);
      g_interProcess.m_Callbacks->AddonPlayer.ClearList(list, listSize);
    }
    return ret;
  }

  void CPlayer::SetAudioStream(int iStream)
  {
    g_interProcess.m_Callbacks->AddonPlayer.SetAudioStream(g_interProcess.m_Handle, m_ControlHandle, iStream);
  }

  bool CPlayer::GetAvailableSubtitleStreams(std::vector<std::string> &streams)
  {
    char** list;
    unsigned int listSize = 0;
    bool ret = g_interProcess.m_Callbacks->AddonPlayer.GetAvailableSubtitleStreams(g_interProcess.m_Handle, m_ControlHandle, list, listSize);
    if (ret)
    {
      for (unsigned int i = 0; i < listSize; ++i)
        streams.push_back(list[i]);
      g_interProcess.m_Callbacks->AddonPlayer.ClearList(list, listSize);
    }
    return ret;
  }

  void CPlayer::SetSubtitleStream(int iStream)
  {
    g_interProcess.m_Callbacks->AddonPlayer.SetSubtitleStream(g_interProcess.m_Handle, m_ControlHandle, iStream);
  }
  
  void CPlayer::ShowSubtitles(bool bVisible)
  {
    g_interProcess.m_Callbacks->AddonPlayer.ShowSubtitles(g_interProcess.m_Handle, m_ControlHandle, bVisible);
  }

  bool CPlayer::GetCurrentSubtitleName(std::string& name)
  {
    name.resize(1024);
    unsigned int size = (unsigned int)name.capacity();
    bool ret = g_interProcess.m_Callbacks->AddonPlayer.GetCurrentSubtitleName(g_interProcess.m_Handle, m_ControlHandle, name[0], size);
    name.resize(size);
    name.shrink_to_fit();
    return ret;
  }

  void CPlayer::AddSubtitle(const std::string& subPath)
  {
    g_interProcess.m_Callbacks->AddonPlayer.AddSubtitle(g_interProcess.m_Handle, m_ControlHandle, subPath.c_str());
  }

  /*!
   * Kodi to add-on override definition function to use if class becomes used
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

    g_interProcess.m_Callbacks->AddonPlayer.SetCallbacks(g_interProcess.m_Handle, m_ControlHandle, cbhdl,
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

} /* namespace Player */
} /* namespace KodiAPI */

END_NAMESPACE()
