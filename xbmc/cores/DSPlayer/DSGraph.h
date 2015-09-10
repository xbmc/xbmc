#pragma once

/*
 *      Copyright (C) 2005-2009 Team XBMC
 *      http://www.xbmc.org
 *
 *		Copyright (C) 2010-2013 Eduard Kytmanov
 *		http://www.avmedia.su
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#ifndef HAS_DS_PLAYER
#error DSPlayer's header file included without HAS_DS_PLAYER defined
#endif

#include "utils/StdString.h"
#include "utils/StringUtils.h"

#include <dshow.h> //needed for CLSID_VideoRenderer
#include <initguid.h>
#include <dvdmedia.h>
#include "util.h"
#include "DVDClock.h"
#include "fgmanager.h"
#include "DSUtil/DSUtil.h"
#include "DSUtil/SmartPtr.h"
#include "windowing/windows/winsystemwin32.h"
#include "cores/IPlayer.h"
#include "filesystem/File.h"

#include "DSMessage.h"

using namespace XFILE;

#define WM_GRAPHEVENT  WM_USER + 13
#define WM_GRAPHMESSAGE  WM_USER + 5

/** Video state mode */
enum VideoStateMode {
  MOVIE_NOTOPENED = 0x00,
  MOVIE_OPENED = 0x01,
  MOVIE_PLAYING = 0x02,
  MOVIE_STOPPED = 0x03,
  MOVIE_PAUSED = 0x04
};

class CFGManager;

/** @brief Graph management helper
 *
 * Provide function for playing, pausing, stopping, handling the graph
 */
class CDSGraph
{
public:
  CDSGraph(CDVDClock* pClock, IPlayerCallback& callback);
  virtual ~CDSGraph();

  /** Determine if the graph can seek
   * @return True is the graph can seek, false else
   * @remarks If True, it means that the graph can seek forward, backward and absolute
   */
  virtual bool CanSeek();
  //void SetDynamicRangeCompression(long drc);

  virtual HRESULT HandleGraphEvent();

  /** @return True if the graph is paused, false else */
  virtual bool IsPaused() const;

  virtual bool IsDvd() { return m_VideoInfo.isDVD; };
  bool IsEof() { return m_State.eof; };
  bool IsInMenu() const;
  bool OnMouseMove(tagPOINT pt);
  bool OnMouseClick(tagPOINT pt);

  /** Performs a Seek
   * @param[in] bPlus If true, performs a positive seek. If false, performs a negative seek
   * @param[in] bLargeStep If true, performs a large seek
   */
  virtual void Seek(bool bPlus, bool bLargeStep);
  /** Performs a Seek
   * @param[in] position Time to seek (in millisecond)
   */
  virtual void SeekInMilliSec(double position);
  /** Performs a Seek
   * @param[in] position Where to seek (DS_TIME_BASE unit)
   * @param[in] flags DirectShow IMediaSeeking::SetPositions flags
   * @param[in] showPopup Do we show a seeking popup?
   */
  virtual void Seek(uint64_t position, uint32_t flags = AM_SEEKING_AbsolutePositioning, bool showPopup = true);
  virtual void SeekPercentage(float iPercent);
  /// Play the graph
  virtual void Play(bool force = false);
  /// Pause the graph
  virtual void Pause();
  /** Stop the graph
   * @param[in] rewind If true, the graph is rewinded
   */
  virtual void Stop(bool rewind = false);
  /// Update total playing time
  virtual void UpdateTotalTime();
  /// Update current playing time
  virtual void UpdateTime();
  /// Update Dvd state
  virtual void UpdateDvdState();
  // Updating the video position is needed for dvd menu navigation
  virtual void UpdateWindowPosition();
  // Updating the madVR video position
  virtual void UpdateMadvrWindowPosition();
  /// Update current player state
  virtual void UpdateState();
  /// @return Current playing time in DS_TIME_BASE unit
  virtual uint64_t GetTime(bool forcePlaybackTime = false);
  /// @return Total playing time in DS_TIME_BASE unit (media length)
  virtual uint64_t GetTotalTime(bool forcePlaybackTime = false);
  /// @return Current position in percent
  virtual float GetPercentage();

  virtual float GetCachePercentage();

  /** Create the graph and open the file
   * @param[in] file File to play
   * @param[in] options Playing options
   * @return An HRESULT code
   */
  HRESULT SetFile(const CFileItem& file, const CPlayerOptions &options);
  /** Close the file, clean the graph and free resources */
  void CloseFile();

  /** Sets the volume (amplitude) of the audio signal
   * @param[in] nVolume The volume level in 1/100ths dB Valid values range from -10,000 (silence) to 0 (full volume) 0 = 0 dB -10000 = -100 dB
   */
  void SetVolume(float nVolume);

  /// @return General informations about filters
  CStdString GetGeneralInfo();
  /// @return Informations about the current audio track
  CStdString GetAudioInfo();
  /// @return Informations about the current video track
  CStdString GetVideoInfo();

  Com::SmartPtr<IFilterGraph2> pFilterGraph;

private:
  //Direct Show Filters
  CFGManager*                           m_pGraphBuilder;
  Com::SmartQIPtr<IMediaControl>        m_pMediaControl;
  Com::SmartQIPtr<IMediaEventEx>        m_pMediaEvent;
  Com::SmartQIPtr<IMediaSeeking>        m_pMediaSeeking;
  Com::SmartQIPtr<IBasicAudio>          m_pBasicAudio;
  Com::SmartQIPtr<IAMOpenProgress>		m_pAMOpenProgress;
  //dvd stuff
  Com::SmartQIPtr<IVideoWindow>         m_pVideoWindow;
  Com::SmartQIPtr<IBasicVideo>          m_pBasicVideo;
  Com::SmartPtr<IDvdState>              m_pDvdState;
  DVD_STATUS	                          m_pDvdStatus;
  std::vector<DvdTitle*>                m_pDvdTitles;
  int8_t m_canSeek; //<-1: not queried; 0: false; 1: true
  float m_currentVolume;

  CCriticalSection m_ObjectLock;

  CStdString m_pStrCurrentFrameRate;
  int m_iCurrentFrameRefreshCycle;

  IPlayerCallback& m_callback;

  struct SPlayerState
  {
    SPlayerState()
    {
      Clear();
    }

    void Clear()
    {
      eof = false;
      isPVR = false;
      time = 0;
      time_total = 0;
      time_live = 0;
      time_total_live = 0;
      player_state = "";

      cache_offset = 0.0;
      current_filter_state = State_Stopped;
    }
    bool eof;
    bool isPVR;
    uint64_t time;              // current playback time in millisec
    uint64_t time_total;        // total playback time in millisec
    uint64_t time_live;         // current LiveTV EPG time in millisec
    uint64_t time_total_live;   // end LiveTV EPG time in millisec
    double  cache_offset;       // percentage of file ahead of current position
    FILTER_STATE current_filter_state;

    std::string player_state;  // full player state
  } m_State;

  struct SDvdState
  {
    void Clear()
    {
      isInMenu = false;
    }
    bool isInMenu;
  } m_DvdState;

  struct SVideoInfo
  {
    SVideoInfo()
    {
      Clear();
    }
    void Clear()
    {
      time_total = 0;
      time_format = GUID_NULL;
      isDVD = false;
    }
    double time_total;        // total playback time
    GUID time_format;
    bool isDVD;
  } m_VideoInfo;
};

extern CDSGraph* g_dsGraph;
