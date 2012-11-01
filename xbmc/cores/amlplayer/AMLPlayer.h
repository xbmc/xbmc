#pragma once
/*
 *      Copyright (C) 2011-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "FileItem.h"
#include "cores/IPlayer.h"
#include "dialogs/GUIDialogBusy.h"
#include "threads/Thread.h"

typedef struct AMLChapterInfo AMLChapterInfo;
typedef struct AMLPlayerStreamInfo AMLPlayerStreamInfo;
typedef struct player_info player_info_t;

struct AMLSubtitle
{
  int64_t     bgntime;
  int64_t     endtime;
  CStdString  string;
};

class DllLibAmplayer;

class CAMLSubTitleThread : public CThread
{
public:
  CAMLSubTitleThread(DllLibAmplayer* dll);
  virtual ~CAMLSubTitleThread();

  void         UpdateSubtitle(CStdString &subtitle, int64_t elapsed_ms);
protected:
  virtual void Process(void);

  DllLibAmplayer           *m_dll;
  int                       m_subtitle_codec;
  std::deque<AMLSubtitle*>  m_subtitle_strings;
  CCriticalSection          m_subtitle_csection;
};

class CDVDPlayerSubtitle;
class CDVDOverlayContainer;

class CAMLPlayer : public IPlayer, public CThread
{
public:

  CAMLPlayer(IPlayerCallback &callback);
  virtual ~CAMLPlayer();
  
  virtual void  RegisterAudioCallback(IAudioCallback* pCallback) {}
  virtual void  UnRegisterAudioCallback()                        {}
  virtual bool  OpenFile(const CFileItem &file, const CPlayerOptions &options);
  virtual bool  QueueNextFile(const CFileItem &file)             {return false;}
  virtual void  OnNothingToQueueNotify()                         {}
  virtual bool  CloseFile();
  virtual bool  IsPlaying() const;
  virtual void  Pause();
  virtual bool  IsPaused() const;
  virtual bool  HasVideo() const;
  virtual bool  HasAudio() const;
  virtual void  ToggleFrameDrop();
  virtual bool  CanSeek();
  virtual void  Seek(bool bPlus = true, bool bLargeStep = false);
  virtual bool  SeekScene(bool bPlus = true);
  virtual void  SeekPercentage(float fPercent = 0.0f);
  virtual float GetPercentage();
  virtual void  SetVolume(float volume);
  virtual void  SetDynamicRangeCompression(long drc)              {}
  virtual void  GetAudioInfo(CStdString &strAudioInfo);
  virtual void  GetVideoInfo(CStdString &strVideoInfo);
  virtual void  GetGeneralInfo(CStdString &strVideoInfo) {};
  virtual void  Update(bool bPauseDrawing);
  virtual void  GetVideoRect(CRect& SrcRect, CRect& DestRect);
  virtual void  GetVideoAspectRatio(float &fAR);
  virtual bool  CanRecord()                                       {return false;};
  virtual bool  IsRecording()                                     {return false;};
  virtual bool  Record(bool bOnOff)                               {return false;};

  virtual void  SetAVDelay(float fValue = 0.0f);
  virtual float GetAVDelay();

  virtual void  SetSubTitleDelay(float fValue);
  virtual float GetSubTitleDelay();
  virtual int   GetSubtitleCount();
  virtual int   GetSubtitle();
  virtual void  GetSubtitleName(int iStream, CStdString &strStreamName);
  virtual void  SetSubtitle(int iStream);
  virtual bool  GetSubtitleVisible();
  virtual void  SetSubtitleVisible(bool bVisible);
  virtual bool  GetSubtitleExtension(CStdString &strSubtitleExtension) { return false; }
  virtual int   AddSubtitle(const CStdString& strSubPath);

  virtual int   GetAudioStreamCount();
  virtual int   GetAudioStream();
  virtual void  GetAudioStreamName(int iStream, CStdString &strStreamName);
  virtual void  SetAudioStream(int iStream);
  virtual void  GetAudioStreamLanguage(int iStream, CStdString &strLanguage) {};

  virtual TextCacheStruct_t* GetTeletextCache()                   {return NULL;};
  virtual void  LoadPage(int p, int sp, unsigned char* buffer)    {};

  virtual int   GetChapterCount();
  virtual int   GetChapter();
  virtual void  GetChapterName(CStdString& strChapterName);
  virtual int   SeekChapter(int iChapter);

  virtual float GetActualFPS();
  virtual void  SeekTime(__int64 iTime = 0);
  virtual __int64 GetTime();
  virtual __int64 GetTotalTime();
  virtual int   GetAudioBitrate();
  virtual int   GetVideoBitrate();
  virtual int   GetSourceBitrate();
  virtual int   GetChannels();
  virtual int   GetBitsPerSample();
  virtual int   GetSampleRate();
  virtual CStdString GetAudioCodecName();
  virtual CStdString GetVideoCodecName();
  virtual int   GetPictureWidth();
  virtual int   GetPictureHeight();
  virtual bool  GetStreamDetails(CStreamDetails &details);
  virtual void  ToFFRW(int iSpeed = 0);
  // Skip to next track/item inside the current media (if supported).
  virtual bool  SkipNext()                                        {return false;}

  //Returns true if not playback (paused or stopped beeing filled)
  virtual bool  IsCaching() const                                 {return false;};
  //Cache filled in Percent
  virtual int   GetCacheLevel() const                             {return -1;};

  virtual bool  IsInMenu() const                                  {return false;};
  virtual bool  HasMenu()                                         {return false;};

  virtual void  DoAudioWork()                                     {};
  virtual bool  OnAction(const CAction &action)                   {return false;};

  virtual bool  GetCurrentSubtitle(CStdString& strSubtitle);
  //returns a state that is needed for resuming from a specific time
  virtual CStdString GetPlayerState()                             {return "";};
  virtual bool  SetPlayerState(CStdString state)                  {return false;};

  virtual CStdString GetPlayingTitle()                            {return "";};
/*
  virtual void  GetRenderFeatures(Features* renderFeatures);
  virtual void  GetDeinterlaceMethods(Features* deinterlaceMethods);
  virtual void  GetDeinterlaceModes(Features* deinterlaceModes);
  virtual void  GetScalingMethods(Features* scalingMethods);
  virtual void  GetAudioCapabilities(Features* audioCaps);
  virtual void  GetSubtitleCapabilities(Features* subCaps);
*/

  virtual void  GetRenderFeatures(std::vector<int> &renderFeatures);
  virtual void  GetDeinterlaceMethods(std::vector<int> &deinterlaceMethods);
  virtual void  GetDeinterlaceModes(std::vector<int> &deinterlaceModes);
  virtual void  GetScalingMethods(std::vector<int> &scalingMethods);
  virtual void  GetAudioCapabilities(std::vector<int> &audioCaps);
  virtual void  GetSubtitleCapabilities(std::vector<int> &subCaps);

protected:
  virtual void  OnStartup();
  virtual void  OnExit();
  virtual void  Process();

private:
  int           GetVideoStreamCount();
  void          ShowMainVideo(bool show);
  void          SetVideoZoom(float zoom);
  void          SetVideoContrast(int contrast);
  void          SetVideoBrightness(int brightness);
  void          SetVideoSaturation(int saturation);
  void          SetAudioPassThrough(int format);
  bool          WaitForPausedThumbJobs(int timeout_ms);

  int           GetPlayerSerializedState(void);
  static int    UpdatePlayerInfo(int pid, player_info_t *info);
  bool          CheckPlaying();
  bool          WaitForStopped(int timeout_ms);
  bool          WaitForSearchOK(int timeout_ms);
  bool          WaitForPlaying(int timeout_ms);
  bool          WaitForOpenMedia(int timeout_ms);
  bool          WaitForFormatValid(int timeout_ms);
  void          ClearStreamInfos();
  bool          GetStatus();

  void          FindSubtitleFiles();
  int           AddSubtitleFile(const std::string& filename, const std::string& subfilename = "");
  bool          OpenSubtitleStream(int index);
  void          SetVideoRect(const CRect &SrcRect, const CRect &DestRect);
  static void   RenderUpdateCallBack(const void *ctx, const CRect &SrcRect, const CRect &DestRect);

  DllLibAmplayer         *m_dll;
  int                     m_cpu;
  int                     m_speed;
  bool                    m_paused;
  bool                    m_bAbortRequest;
  CEvent                  m_ready;
  CFileItem               m_item;
  CPlayerOptions          m_options;
  int                     m_log_level;

  int64_t                 m_elapsed_ms;
  int64_t                 m_duration_ms;

  int                     m_audio_index;
  int                     m_audio_count;
  CStdString              m_audio_info;
  int                     m_audio_delay;
  bool                    m_audio_passthrough_ac3;
  bool                    m_audio_passthrough_dts;

  int                     m_video_index;
  int                     m_video_count;
  CStdString              m_video_info;
  int                     m_video_width;
  int                     m_video_height;
  float                   m_video_fps_numerator;
  float                   m_video_fps_denominator;

  int                     m_subtitle_index;
  int                     m_subtitle_count;
  bool                    m_subtitle_show;
  int                     m_subtitle_delay;
  CAMLSubTitleThread     *m_subtitle_thread;
  CDVDPlayerSubtitle     *m_dvdPlayerSubtitle;
  CDVDOverlayContainer   *m_dvdOverlayContainer;

  int                     m_chapter_index;
  int                     m_chapter_count;

  int                     m_show_mainvideo;
  CRect                   m_dst_rect;
  int                     m_view_mode;
  float                   m_zoom;
  int                     m_contrast;
  int                     m_brightness;

  CCriticalSection        m_aml_csection;
  CCriticalSection        m_aml_state_csection;
  std::deque<int>         m_aml_state;
  int                     m_pid;

  std::vector<AMLPlayerStreamInfo*> m_video_streams;
  std::vector<AMLPlayerStreamInfo*> m_audio_streams;
  std::vector<AMLPlayerStreamInfo*> m_subtitle_streams;
  std::vector<AMLChapterInfo*>      m_chapters;

};
