#pragma once
/*
 *      Copyright (C) 2015-2016 Team KODI
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

#include "cores/IPlayerCallback.h"
#include "threads/CriticalSection.h"

#include <string>
#include <vector>

class CFileItem;

namespace ADDON { class CAddon; }
namespace PLAYLIST { class CPlayList; }

namespace V2
{
namespace KodiAPI
{

struct CB_AddOnLib;

namespace Player
{
extern "C"
{

  class CAddOnPlayer
  {
  public:
    static void Init(struct CB_AddOnLib *interfaces);

    static char* GetSupportedMedia(void* hdl, int mediaType);

    static void* New(void *hdl);
    static void Delete(void *hdl, void* handle);
    static void SetCallbacks(
        void*      addonData,
        void*      handle,
        void*      cbhdl,
        void      (*CBOnPlayBackStarted)     (void* cbhdl),
        void      (*CBOnPlayBackEnded)       (void* cbhdl),
        void      (*CBOnPlayBackStopped)     (void* cbhdl),
        void      (*CBOnPlayBackPaused)      (void* cbhdl),
        void      (*CBOnPlayBackResumed)     (void* cbhdl),
        void      (*CBOnQueueNextItem)       (void* cbhdl),
        void      (*CBOnPlayBackSpeedChanged)(void* cbhdl, int iSpeed),
        void      (*CBOnPlayBackSeek)        (void* cbhdl, int iTime, int seekOffset),
        void      (*CBOnPlayBackSeekChapter) (void* cbhdl, int iChapter));

    static bool PlayFile(void *hdl, void* handle, const char* item, bool windowed);
    static bool PlayFileItem(void *hdl, void* handle, const void* listitem, bool windowed);
    static bool PlayList(void *hdl, void* handle, const void* list, int playListId, bool windowed, int startpos);
    static void Stop(void *hdl, void* handle);
    static void Pause(void *hdl, void* handle);
    static void PlayNext(void *hdl, void* handle);
    static void PlayPrevious(void *hdl, void* handle);
    static void PlaySelected(void *hdl, void* handle, int selected);

    static bool IsPlaying(void *hdl, void* handle);
    static bool IsPlayingAudio(void *hdl, void* handle);
    static bool IsPlayingVideo(void *hdl, void* handle);
    static bool IsPlayingRDS(void *hdl, void* handle);

    static bool GetPlayingFile(void *hdl, void* handle, char& file, unsigned int& iMaxStringSize);

    static double GetTotalTime(void *hdl, void* handle);
    static double GetTime(void *hdl, void* handle);
    static void SeekTime(void *hdl, void* handle, double seekTime);

    static bool GetAvailableVideoStreams(void *hdl, void* handle, char**& streams, unsigned int& entries);
    static void SetVideoStream(void *hdl, void* handle, int iStream);

    static bool GetAvailableAudioStreams(void *hdl, void* handle, char**& streams, unsigned int& entries);
    static void SetAudioStream(void *hdl, void* handle, int iStream);

    static bool GetAvailableSubtitleStreams(void *hdl, void* handle, char**& streams, unsigned int& entries);
    static void SetSubtitleStream(void *hdl, void* handle, int iStream);
    static void ShowSubtitles(void *hdl, void* handle, bool bVisible);
    static bool GetCurrentSubtitleName(void *hdl, void* handle, char& name, unsigned int& iMaxStringSize);
    static void AddSubtitle(void *hdl, void* handle, const char* strSubPath);

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
    void (*CBOnPlayBackStarted)(void* cbhdl);
    void (*CBOnPlayBackEnded)(void* cbhdl);
    void (*CBOnPlayBackStopped)(void* cbhdl);
    void (*CBOnPlayBackPaused)(void* cbhdl);
    void (*CBOnPlayBackResumed)(void* cbhdl);
    void (*CBOnQueueNextItem)(void* cbhdl);
    void (*CBOnPlayBackSpeedChanged)(void* cbhdl, int iSpeed);
    void (*CBOnPlayBackSeek)(void* cbhdl, int iTime, int seekOffset);
    void (*CBOnPlayBackSeekChapter)(void* cbhdl, int iChapter);
    void* m_cbhdl;

  private:
    ADDON::CAddon *m_addon;
    CCriticalSection m_playStateMutex;
    int m_playList;
  };

} /* extern "C" */
} /* namespace Player */

} /* namespace KodiAPI */
} /* namespace V2 */
