#pragma once
/*
 *      Copyright (C) 2011-2013 Team XBMC
 *      http://xbmc.org
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

#include "cores/IPlayer.h"
#include "threads/Thread.h"

#include "cores/dvdplayer/IDVDPlayer.h"

#include "DVDMessageQueue.h"
#include "OMXCore.h"
#include "OMXClock.h"
#include "OMXPlayerAudio.h"
#include "OMXPlayerVideo.h"
#include "DVDPlayerSubtitle.h"
#include "DVDPlayerTeletext.h"

//#include "DVDChapterReader.h"
#include "DVDSubtitles/DVDFactorySubtitle.h"
#include "utils/BitstreamStats.h"

#include "linux/DllBCM.h"
#include "Edl.h"
#include "FileItem.h"
#include "threads/SingleLock.h"

class COMXPlayer;
class OMXPlayerVideo;
class OMXPlayerAudio;

namespace PVR
{
  class CPVRChannel;
}

#define DVDSTATE_NORMAL           0x00000001 // normal dvd state
#define DVDSTATE_STILL            0x00000002 // currently displaying a still frame
#define DVDSTATE_WAIT             0x00000003 // waiting for demuxer read error
#define DVDSTATE_SEEK             0x00000004 // we are finishing a seek request

class COMXCurrentStream
{
public:
  int              id;     // demuxerid of current playing stream
  int              source;
  double           dts;    // last dts from demuxer, used to find disncontinuities
  double           dur;    // last frame expected duration
  double           dts_state; // when did we last send a playback state update
  CDVDStreamInfo   hint;   // stream hints, used to notice stream changes
  void*            stream; // pointer or integer, identifying stream playing. if it changes stream changed
  int              changes; // remembered counter from stream to track codec changes
  bool             inited;
  bool             started; // has the player started
  const StreamType type;
  const int        player;
  // stuff to handle starting after seek
  double   startpts;

  COMXCurrentStream(StreamType t, int i)
    : type(t)
    , player(i)
  {
    Clear();
  }

  void Clear()
  {
    id     = -1;
    source = STREAM_SOURCE_NONE;
    dts    = DVD_NOPTS_VALUE;
    dts_state = DVD_NOPTS_VALUE;
    dur    = DVD_NOPTS_VALUE;
    hint.Clear();
    stream = NULL;
    changes = 0;
    inited = false;
    started = false;
    startpts  = DVD_NOPTS_VALUE;
  }
  double dts_end()
  {
    if(dts == DVD_NOPTS_VALUE)
      return DVD_NOPTS_VALUE;
    if(dur == DVD_NOPTS_VALUE)
      return dts;
    return dts + dur;
  }
};

typedef struct
{
  StreamType   type;
  int          type_index;
  std::string  filename;
  std::string  filename2;  // for vobsub subtitles, 2 files are necessary (idx/sub) 
  std::string  language;
  std::string  name;
  CDemuxStream::EFlags flags;
  int          source;
  int          id;
  std::string  codec;
  int          channels;
} OMXSelectionStream;

typedef std::vector<OMXSelectionStream> OMXSelectionStreams;

class COMXSelectionStreams
{
  CCriticalSection m_section;
  OMXSelectionStream  m_invalid;
public:
  COMXSelectionStreams()
  {
    m_invalid.id = -1;
    m_invalid.source = STREAM_SOURCE_NONE;
    m_invalid.type = STREAM_NONE;
  }
  std::vector<OMXSelectionStream> m_Streams;

  int              IndexOf (StreamType type, int source, int id) const;
  int              IndexOf (StreamType type, COMXPlayer& p) const;
  int              Count   (StreamType type) const { return IndexOf(type, STREAM_SOURCE_NONE, -1) + 1; }
  OMXSelectionStream& Get     (StreamType type, int index);
  bool             Get     (StreamType type, CDemuxStream::EFlags flag, OMXSelectionStream& out);

  OMXSelectionStreams Get(StreamType type);
  template<typename Compare> OMXSelectionStreams Get(StreamType type, Compare compare)
  {
    OMXSelectionStreams streams = Get(type);
    std::stable_sort(streams.begin(), streams.end(), compare);
    return streams;
  }

  void             Clear   (StreamType type, StreamSource source);
  int              Source  (StreamSource source, std::string filename);

  void             Update  (OMXSelectionStream& s);
  void             Update  (CDVDInputStream* input, CDVDDemux* demuxer);
};


#define DVDPLAYER_AUDIO    1
#define DVDPLAYER_VIDEO    2
#define DVDPLAYER_SUBTITLE 3
#define DVDPLAYER_TELETEXT 4

class COMXPlayer : public IPlayer, public CThread, public IDVDPlayer
{
public:

  COMXPlayer(IPlayerCallback &callback);
  virtual ~COMXPlayer();
  
  virtual bool  OpenFile(const CFileItem &file, const CPlayerOptions &options);
  virtual bool  CloseFile(bool reopen = false);
  virtual bool  IsPlaying() const;
  virtual void  Pause();
  virtual bool  IsPaused() const;
  virtual bool  HasVideo() const;
  virtual bool  HasAudio() const;
  virtual bool  IsPassthrough() const;
  virtual bool  CanSeek();
  virtual void Seek(bool bPlus, bool bLargeStep, bool bChapterOverride);
  virtual bool  SeekScene(bool bPlus = true);
  virtual void SeekPercentage(float iPercent);
  virtual float GetPercentage();
  virtual float GetCachePercentage();

  virtual void RegisterAudioCallback(IAudioCallback* pCallback) { m_omxPlayerAudio.RegisterAudioCallback(pCallback); }
  virtual void UnRegisterAudioCallback()                        { m_omxPlayerAudio.UnRegisterAudioCallback(); }
  virtual void SetVolume(float nVolume)                         { m_omxPlayerAudio.SetVolume(nVolume); }
  virtual void SetMute(bool bOnOff)                             { m_omxPlayerAudio.SetMute(bOnOff); }
  virtual void SetDynamicRangeCompression(long drc)             { m_omxPlayerAudio.SetDynamicRangeCompression(drc); }
  virtual bool ControlsVolume() {return true;}
  virtual void GetAudioInfo(CStdString &strAudioInfo);
  virtual void GetVideoInfo(CStdString &strVideoInfo);
  virtual void GetGeneralInfo(CStdString &strVideoInfo);
  virtual bool CanRecord();
  virtual bool IsRecording();
  virtual bool CanPause();
  virtual bool Record(bool bOnOff);
  virtual void SetAVDelay(float fValue = 0.0f);
  virtual float GetAVDelay();

  virtual void  SetSubTitleDelay(float fValue = 0.0f);
  virtual float GetSubTitleDelay();
  virtual int   GetSubtitleCount();
  virtual int   GetSubtitle();
  virtual void  GetSubtitleStreamInfo(int index, SPlayerSubtitleStreamInfo &info);
  virtual void  SetSubtitle(int iStream);
  virtual bool  GetSubtitleVisible();
  virtual void  SetSubtitleVisible(bool bVisible);
  virtual int   AddSubtitle(const CStdString& strSubPath);

  virtual int   GetAudioStreamCount();
  virtual int   GetAudioStream();
  virtual void  SetAudioStream(int iStream);

  virtual TextCacheStruct_t* GetTeletextCache();
  virtual void  LoadPage(int p, int sp, unsigned char* buffer);

  virtual int   GetChapterCount();
  virtual int   GetChapter();
  virtual void  GetChapterName(CStdString& strChapterName);
  virtual int   SeekChapter(int iChapter);

  virtual void SeekTime(int64_t iTime);
  virtual int64_t GetTime();
  virtual int64_t GetTotalTime();
  virtual void ToFFRW(int iSpeed );
  virtual bool OnAction(const CAction &action);
  virtual bool HasMenu();
  virtual int GetSourceBitrate();
  virtual void GetVideoStreamInfo(SPlayerVideoStreamInfo &info);

  virtual bool GetStreamDetails(CStreamDetails &details);
  virtual void GetAudioStreamInfo(int index, SPlayerAudioStreamInfo &info);

  virtual CStdString GetPlayerState();
  virtual bool SetPlayerState(CStdString state);
  
  virtual CStdString GetPlayingTitle();

  virtual bool SwitchChannel(const PVR::CPVRChannel &channel);
  virtual bool CachePVRStream(void) const;
  
  enum ECacheState
  { CACHESTATE_DONE = 0
  , CACHESTATE_FULL     // player is filling up the demux queue
  , CACHESTATE_PVR      // player is waiting for some data in each buffer
  , CACHESTATE_INIT     // player is waiting for first packet of each stream
  , CACHESTATE_PLAY     // player is waiting for players to not be stalled
  , CACHESTATE_FLUSH    // temporary state player will choose startup between init or full
  };

  virtual bool  IsCaching() const                                 { return m_caching == CACHESTATE_FULL || m_caching == CACHESTATE_PVR; }
  virtual int   GetCacheLevel() const;

  virtual int  OnDVDNavResult(void* pData, int iMessage);

  virtual void  GetRenderFeatures(std::vector<int> &renderFeatures);
  virtual void  GetDeinterlaceMethods(std::vector<int> &deinterlaceMethods);
  virtual void  GetDeinterlaceModes(std::vector<int> &deinterlaceModes);
  virtual void  GetScalingMethods(std::vector<int> &scalingMethods);
  virtual void  GetAudioCapabilities(std::vector<int> &audioCaps);
  virtual void  GetSubtitleCapabilities(std::vector<int> &subCaps);
protected:
  friend class COMXSelectionStreams;

  class OMXStreamLock : public CSingleLock
  {
  public:
    inline OMXStreamLock(COMXPlayer* comxplayer) : CSingleLock(comxplayer->m_critStreamSection) {}
  };

  virtual void  OnStartup();
  virtual void  OnExit();
  virtual void  Process();

  bool OpenAudioStream(int iStream, int source, bool reset = true);
  bool OpenVideoStream(int iStream, int source, bool reset = true);
  bool OpenSubtitleStream(int iStream, int source);

  /** \brief Switches forced subtitles to forced subtitles matching the language of the current audio track.
  *          If these are not available, subtitles are disabled.
  *   \return true if the subtitles were changed, false otherwise.
  */
  bool AdaptForcedSubtitles();
  bool OpenTeletextStream(int iStream, int source);
  bool CloseAudioStream(bool bWaitForBuffers);
  bool CloseVideoStream(bool bWaitForBuffers);
  bool CloseSubtitleStream(bool bKeepOverlays);
  bool CloseTeletextStream(bool bWaitForBuffers);

  void ProcessPacket(CDemuxStream* pStream, DemuxPacket* pPacket);
  void ProcessAudioData(CDemuxStream* pStream, DemuxPacket* pPacket);
  void ProcessVideoData(CDemuxStream* pStream, DemuxPacket* pPacket);
  void ProcessSubData(CDemuxStream* pStream, DemuxPacket* pPacket);
  void ProcessTeletextData(CDemuxStream* pStream, DemuxPacket* pPacket);

  bool ShowPVRChannelInfo();

  int  AddSubtitleFile(const std::string& filename, const std::string& subfilename = "", CDemuxStream::EFlags flags = CDemuxStream::FLAG_NONE);
  void SetSubtitleVisibleInternal(bool bVisible);

  /**
   * one of the DVD_PLAYSPEED defines
   */
  void SetPlaySpeed(int iSpeed);
  int GetPlaySpeed()                                                { return m_playSpeed; }
  void    SetCaching(ECacheState state);

  int64_t GetTotalTimeInMsec();

  double  GetQueueTime();
  bool    GetCachingTimes(double& play_left, double& cache_left, double& file_offset);


  void FlushBuffers(bool queued, double pts = DVD_NOPTS_VALUE, bool accurate = true);


  void  HandleMessages();
  void    HandlePlaySpeed();
  bool  IsInMenu() const;

  void SynchronizePlayers(unsigned int sources);
  void SynchronizeDemuxer(unsigned int timeout);
  void CheckAutoSceneSkip();
  void CheckContinuity(COMXCurrentStream& current, DemuxPacket* pPacket);
  bool CheckSceneSkip(COMXCurrentStream& current);
  bool CheckPlayerInit(COMXCurrentStream& current, unsigned int source);
  bool CheckStartCaching(COMXCurrentStream& current);
  void UpdateCorrection(DemuxPacket* pkt, double correction);
  void UpdateTimestamps(COMXCurrentStream& current, DemuxPacket* pPacket);
  void SendPlayerMessage(CDVDMsg* pMsg, unsigned int target);
  bool ReadPacket(DemuxPacket*& packet, CDemuxStream*& stream);
  bool IsValidStream(COMXCurrentStream& stream);
  bool IsBetterStream(COMXCurrentStream& current, CDemuxStream* stream);
  bool CheckDelayedChannelEntry(void);
  bool OpenInputStream();
  bool OpenDemuxStream();
  void OpenDefaultStreams(bool reset = true);

  void UpdateApplication(double timeout);
  void UpdatePlayState(double timeout);
  double m_UpdateApplication;

  bool m_bAbortRequest;

  std::string           m_filename; // holds the actual filename
  std::string  m_mimetype;  // hold a hint to what content file contains (mime type)
  ECacheState  m_caching;
  CFileItem    m_item;
  XbmcThreads::EndTime m_ChannelEntryTimeOut;


  COMXCurrentStream m_CurrentAudio;
  COMXCurrentStream m_CurrentVideo;
  COMXCurrentStream m_CurrentSubtitle;
  COMXCurrentStream m_CurrentTeletext;

  COMXSelectionStreams m_SelectionStreams;

  int m_playSpeed;
  struct SSpeedState
  {
    double lastpts;  // holds last display pts during ff/rw operations
    double lasttime;
  } m_SpeedState;

  int m_errorCount;
  double m_offset_pts;

  CDVDMessageQueue m_messenger;     // thread messenger

  OMXPlayerVideo m_omxPlayerVideo; // video part
  OMXPlayerAudio m_omxPlayerAudio; // audio part
  CDVDPlayerSubtitle m_dvdPlayerSubtitle; // subtitle part
  CDVDTeletextData m_dvdPlayerTeletext; // teletext part

  CDVDClock m_clock;                // master clock
  OMXClock m_av_clock;

  bool m_stepped;
  int m_video_fifo;
  int m_audio_fifo;
  double m_last_check_time;         // we periodically check for gpu underrun
  double m_stamp;                   // last media stamp

  CDVDOverlayContainer m_overlayContainer;

  CDVDInputStream* m_pInputStream;  // input stream for current playing file
  CDVDDemux* m_pDemuxer;            // demuxer for current playing file
  CDVDDemux*            m_pSubtitleDemuxer;

  CStdString m_lastSub;

  struct SDVDInfo
  {
    void Clear()
    {
      state                =  DVDSTATE_NORMAL;
      iSelectedSPUStream   = -1;
      iSelectedAudioStream = -1;
      iDVDStillTime        =  0;
      iDVDStillStartTime   =  0;
    }

    int state;                // current dvdstate
    unsigned int iDVDStillTime;      // total time in ticks we should display the still before continuing
    unsigned int iDVDStillStartTime; // time in ticks when we started the still
    int iSelectedSPUStream;   // mpeg stream id, or -1 if disabled
    int iSelectedAudioStream; // mpeg stream id, or -1 if disabled
  } m_dvd;

  enum ETimeSource
  {
    ETIMESOURCE_CLOCK,
    ETIMESOURCE_INPUT,
    ETIMESOURCE_MENU,
  };

  friend class OMXPlayerVideo;
  friend class OMXPlayerAudio;

  struct SPlayerState
  {
    SPlayerState() { Clear(); }
    void Clear()
    {
      player        = 0;
      timestamp     = 0;
      time          = 0;
      time_total    = 0;
      time_offset   = 0;
      time_src      = ETIMESOURCE_CLOCK;
      dts           = DVD_NOPTS_VALUE;
      player_state  = "";
      chapter       = 0;
      chapter_name  = "";
      chapter_count = 0;
      canrecord     = false;
      recording     = false;
      canpause      = false;
      canseek       = false;
      demux_video   = "";
      demux_audio   = "";
      cache_bytes   = 0;
      cache_level   = 0.0;
      cache_delay   = 0.0;
      cache_offset  = 0.0;
    }

    int    player;            // source of this data

    double timestamp;         // last time of update
    double time_offset;       // difference between time and pts

    double time;              // current playback time
    double time_total;        // total playback time
    ETimeSource time_src;     // current time source
    double dts;               // last known dts

    std::string player_state;  // full player state

    int         chapter;      // current chapter
    std::string chapter_name; // name of current chapter
    int         chapter_count;// number of chapter

    bool canrecord;           // can input stream record
    bool recording;           // are we currently recording

    bool canpause;            // pvr: can pause the current playing item
    bool canseek;             // pvr: can seek in the current playing item

    std::string demux_video;
    std::string demux_audio;

    int64_t cache_bytes;   // number of bytes current's cached
    double  cache_level;   // current estimated required cache level
    double  cache_delay;   // time until cache is expected to reach estimated level
    double  cache_offset;  // percentage of file ahead of current position
  } m_State, m_StateInput;
  CCriticalSection m_StateSection;

  CEvent m_ready;
  CCriticalSection m_critStreamSection; // need to have this lock when switching streams (audio / video)

  CEdl m_Edl;

  struct SEdlAutoSkipMarkers {

    void Clear()
    {
      cut = -1;
      commbreak_start = -1;
      commbreak_end = -1;
      seek_to_start = false;
      mute = false;
    }

    int cut;              // last automatically skipped EDL cut seek position
    int commbreak_start;  // start time of the last commercial break automatically skipped
    int commbreak_end;    // end time of the last commercial break automatically skipped
    bool seek_to_start;   // whether seeking can go back to the start of a previously skipped break
    bool mute;            // whether EDL mute is on

  } m_EdlAutoSkipMarkers;

  CPlayerOptions          m_PlayerOptions;

  bool m_HasVideo;
  bool m_HasAudio;

  bool m_DemuxerPausePending;
};
