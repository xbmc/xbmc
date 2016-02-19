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

#include "addons/kodi-addon-dev-kit/include/kodi/api2/.internal/AddonLib_LibFunc_Base.hpp"
#include "cores/IPlayerCallback.h"

class CFileItem;

namespace PLAYLIST
{
  class CPlayList;
};

namespace V2
{
namespace KodiAPI
{

namespace Player
{
extern "C"
{

  class CAddOnPlayer
  {
  public:
    static void Init(CB_AddOnLib *callbacks);

    static char* GetSupportedMedia(void* addonData, int mediaType);

    static PLAYERHANDLE New(void *addonData);
    static void Delete(void *addonData, PLAYERHANDLE handle);
    static void SetCallbacks(
        void*             addonData,
        PLAYERHANDLE      handle,
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

    static bool PlayFile(void *addonData, PLAYERHANDLE handle, const char* item, bool windowed);
    static bool PlayFileItem(void *addonData, PLAYERHANDLE handle, const GUIHANDLE listitem, bool windowed);
    static bool PlayList(void *addonData, PLAYERHANDLE handle, const PLAYERHANDLE list, int playListId, bool windowed, int startpos);
    static void Stop(void *addonData, PLAYERHANDLE handle);
    static void Pause(void *addonData, PLAYERHANDLE handle);
    static void PlayNext(void *addonData, PLAYERHANDLE handle);
    static void PlayPrevious(void *addonData, PLAYERHANDLE handle);
    static void PlaySelected(void *addonData, PLAYERHANDLE handle, int selected);

    static bool IsPlaying(void *addonData, PLAYERHANDLE handle);
    static bool IsPlayingAudio(void *addonData, PLAYERHANDLE handle);
    static bool IsPlayingVideo(void *addonData, PLAYERHANDLE handle);
    static bool IsPlayingRDS(void *addonData, PLAYERHANDLE handle);

    static bool GetPlayingFile(void *addonData, PLAYERHANDLE handle, char& file, unsigned int& iMaxStringSize);

    static double GetTotalTime(void *addonData, PLAYERHANDLE handle);
    static double GetTime(void *addonData, PLAYERHANDLE handle);
    static void SeekTime(void *addonData, PLAYERHANDLE handle, double seekTime);

    static bool GetAvailableVideoStreams(void *addonData, PLAYERHANDLE handle, char**& streams, unsigned int& entries);
    static void SetVideoStream(void *addonData, PLAYERHANDLE handle, int iStream);

    static bool GetAvailableAudioStreams(void *addonData, PLAYERHANDLE handle, char**& streams, unsigned int& entries);
    static void SetAudioStream(void *addonData, PLAYERHANDLE handle, int iStream);

    static bool GetAvailableSubtitleStreams(void *addonData, PLAYERHANDLE handle, char**& streams, unsigned int& entries);
    static void SetSubtitleStream(void *addonData, PLAYERHANDLE handle, int iStream);
    static void ShowSubtitles(void *addonData, PLAYERHANDLE handle, bool bVisible);
    static bool GetCurrentSubtitleName(void *addonData, PLAYERHANDLE handle, char& name, unsigned int& iMaxStringSize);
    static void AddSubtitle(void *addonData, PLAYERHANDLE handle, const char* strSubPath);

    static void ClearList(char**& path, unsigned int entries);
  };

  /*\___________________________________________________________________________
  \*/

  class CAddOnPlayerControl : public IPlayerCallback
  {
  friend class CAddOnPlayer;
  public:
    CAddOnPlayerControl(ADDON::CAddon* addon);
    virtual ~CAddOnPlayerControl();

    inline bool PlayFile(std::string item, bool windowed);
    inline bool PlayFileItem(const CFileItem* item, bool windowed);
    inline bool PlayList(const PLAYLIST::CPlayList* list, int playListId, bool windowed, int startpos);
    inline void Stop();
    inline void Pause();
    inline void PlayNext();
    inline void PlayPrevious();
    inline void PlaySelected(int selected);

    virtual void OnPlayBackEnded();
    virtual void OnPlayBackStarted();
    virtual void OnPlayBackPaused();
    virtual void OnPlayBackResumed();
    virtual void OnPlayBackStopped();
    virtual void OnQueueNextItem();
    virtual void OnPlayBackSeek(int iTime, int seekOffset);
    virtual void OnPlayBackSeekChapter(int iChapter);
    virtual void OnPlayBackSpeedChanged(int iSpeed);

    inline bool IsPlaying();
    inline bool IsPlayingAudio();
    inline bool IsPlayingVideo();
    inline bool IsPlayingRDS();

    inline std::string GetPlayingFile();

    inline double GetTotalTime();
    inline double GetTime();
    inline void SeekTime(double seekTime);

    inline std::vector<std::string> GetAvailableVideoStreams();
    inline void SetVideoStream(int iStream);

    inline std::vector<std::string> GetAvailableAudioStreams();
    inline void SetAudioStream(int iStream);

    inline std::vector<std::string> GetAvailableSubtitleStreams();
    inline void SetSubtitleStream(int iStream);
    inline void ShowSubtitles(bool bVisible);
    inline std::string GetCurrentSubtitleName();
    inline void AddSubtitle(const std::string& strSubPath);

  protected:
    void (*CBOnPlayBackStarted)(PLAYERHANDLE cbhdl);
    void (*CBOnPlayBackEnded)(PLAYERHANDLE cbhdl);
    void (*CBOnPlayBackStopped)(PLAYERHANDLE cbhdl);
    void (*CBOnPlayBackPaused)(PLAYERHANDLE cbhdl);
    void (*CBOnPlayBackResumed)(PLAYERHANDLE cbhdl);
    void (*CBOnQueueNextItem)(PLAYERHANDLE cbhdl);
    void (*CBOnPlayBackSpeedChanged)(PLAYERHANDLE cbhdl, int iSpeed);
    void (*CBOnPlayBackSeek)(PLAYERHANDLE cbhdl, int iTime, int seekOffset);
    void (*CBOnPlayBackSeekChapter)(PLAYERHANDLE cbhdl, int iChapter);
    PLAYERHANDLE m_cbhdl;

  private:
    ADDON::CAddon *m_addon;
    CCriticalSection m_playStateMutex;
    int m_playList;
  };

}; /* extern "C" */
}; /* namespace Player */

}; /* namespace KodiAPI */
}; /* namespace V2 */
