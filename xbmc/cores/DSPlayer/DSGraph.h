#pragma once

/*
 *      Copyright (C) 2005-2009 Team XBMC
 *      http://www.xbmc.org
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

#include "StdString.h"
#include "StringUtils.h"

#include <dshow.h> //needed for CLSID_VideoRenderer
#include <initguid.h>
#include <dvdmedia.h>
#include <strmif.h>
#include <mtype.h>
#include <wxdebug.h>
#include <combase.h>
#include "util.h"
#include "dsconfig.h"
#include "fgmanager.h"

#ifdef HAS_VIDEO_PLAYBACK
  #include "cores/VideoRenderers/RenderManager.h"
#endif

#include "cores/IPlayer.h"
#include "File.h"



using namespace XFILE;

#define WM_GRAPHEVENT	WM_USER + 13
#define TIME_FORMAT_TO_MS 10000      // 10 ^ 4

/** Video state mode */
enum VideoStateMode { MOVIE_NOTOPENED = 0x00,
                  MOVIE_OPENED    = 0x01,
                  MOVIE_PLAYING   = 0x02,
                  MOVIE_STOPPED   = 0x03,
                  MOVIE_PAUSED    = 0x04 };

class CFGManager;

/** @brief Graph management helper
 *
 * Provide function for playing, pausing, stopping, handling the graph
 */
class CDSGraph
{
public:
  CDSGraph();
  virtual ~CDSGraph();

  /** Determine if the graph can seek
   * @return True is the graph can seek, false else
   * @remarks If True, it means that the graph can seek forward, backward and absolute
   */
  virtual bool CanSeek();
  //void SetDynamicRangeCompression(long drc);
  
  bool InitializedOutputDevice();

  virtual void ProcessDsWmCommand(WPARAM wParam, LPARAM lParam);
  virtual HRESULT HandleGraphEvent();
  /** @return True if the file reached end, false else */
  bool FileReachedEnd(){ return m_bReachedEnd; };

  /** @return True if the graph is paused, false else */
  virtual bool IsPaused() const;
  /** @return Current play speed */
  virtual double GetPlaySpeed() { return m_currentSpeed; };

  /** Perform a Fast Forward
   * @param[in] currentSpeed Fast Forward speed
   */
  virtual void DoFFRW(int currentSpeed);
  /** Performs a Seek
   * @param[in] bPlus If true, performs a positive seek. If false, performs a negative seek
   * @param[in] bLargeStep If true, performs a large seek
   */
  virtual void Seek(bool bPlus, bool bLargeStep);
  /** Performs a Seek
   * @param[in] sec Time to seek (in millisecond)
   */
  virtual void SeekInMilliSec(double sec);
  /// Play the graph
  virtual void Play();
  /// Pause the graph
  virtual void Pause();
  /** Stop the graph
   * @param[in] rewind If true, the graph is rewinded
   */
  virtual void Stop(bool rewind = false);
  /// Update current playing time
  virtual void UpdateTime();
  /// Update current player state
  virtual void UpdateState();
  /// Update current video informations (filters name)
  virtual void UpdateCurrentVideoInfo();
  /// @return Current playing time
  virtual __int64 GetTime();
  /// @return Total playing time in second (media length)
  virtual int GetTotalTime();
  /// @return Total playing time in millisecond (media length)
  __int64 GetTotalTimeInMsec();
  /// @return Current position in percent
  virtual float GetPercentage();

  /** Create the graph and open the file
   * @param[in] file File to play
   * @param[in] options Playing options
   * @return An HRESULT code
   */
  HRESULT SetFile(const CFileItem& file, const CPlayerOptions &options);
  /** Close the file, clean the graph and free resources */
  void CloseFile();

  /// @return Path of the current playing file, or an empty string if there's no playing file
  CStdString GetCurrentFile(void) { return m_Filename; }

  /** Sets the volume (amplitude) of the audio signal 
   * @param[in] nVolume The volume level in 1/100ths dB Valid values range from -10,000 (silence) to 0 (full volume) 0 = 0 dB -10000 = -100 dB
   */
  void SetVolume(long nVolume);

  /// @return General informations about filters
  CStdString GetGeneralInfo();
  /// @return Informations about the current audio track
  CStdString GetAudioInfo();
  /// @return Informations about the current video track
  CStdString GetVideoInfo();  
 
protected:

  /** Unload the graph and release all the filters
   * @return A HRESULT code
   */
  HRESULT UnloadGraph();
  //void WaitForRendererToShutDown();
  
private:
  //Direct Show Filters
  CFGManager*                     m_pGraphBuilder;
  IMediaControl*                  m_pMediaControl;  
  IMediaEventEx*                  m_pMediaEvent;
  IMediaSeeking*                  m_pMediaSeeking;
  IBasicAudio*                    m_pBasicAudio;
  IQualProp*                      m_pQualProp;

  bool m_bAllowFullscreen;
  bool m_bReachedEnd;
  CStdString m_Filename;
  int m_PlaybackRate;
  int m_currentSpeed;
  float m_fFrameRate;
  bool m_bChangingAudioStream;
  CFile m_File;   
  DWORD_PTR g_userId;
  CCritSec m_ObjectLock;
  CStdString m_pStrCurrentFrameRate;
  int        m_iCurrentFrameRefreshCycle;
  struct SPlayerState
  {
    void Clear()
    {
      timestamp     = 0;
      time    = 0;
      time_total      = 0;
      player_state  = "";
    }
    double timestamp;         // last time of update

    double time;              // current playback time
    double time_total;        // total playback time
    FILTER_STATE current_filter_state;

    std::string player_state;  // full player state
  } m_State;

  struct SVideoInfo
  {
    void Clear()
    {
      time_total    = 0;
      codec_video   = "";
      codec_audio   = "";
      dxva_info     = "";
      filter_audio_dec = "";
      filter_audio_renderer = "";
      filter_video_dec = "";
      filter_source = "";
      filter_splitter = "";
      time_format   = GUID_NULL;
    }
    double time_total;        // total playback time
    CStdString codec_video;
    CStdString codec_audio;

    CStdString filter_audio_dec;
    CStdString filter_audio_renderer;
    CStdString filter_video_dec;
    CStdString filter_source;
    CStdString filter_splitter;
    CStdString dxva_info;

    GUID time_format;
  } m_VideoInfo;
};
