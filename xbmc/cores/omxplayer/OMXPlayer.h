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

#if defined(HAVE_CONFIG_H) && !defined(TARGET_WINDOWS)
#include "config.h"
#define DECLARE_UNUSED(a,b) a __attribute__((unused)) b;
#endif

#include <semaphore.h>
#include <deque>

#include "FileItem.h"
#include "cores/IPlayer.h"
#include "cores/dvdplayer/IDVDPlayer.h"
#include "dialogs/GUIDialogBusy.h"
#include "threads/Thread.h"
#include "threads/SingleLock.h"

#include "OMXCore.h"
#include "OMXClock.h"
#include "OMXPlayerAudio.h"
#include "OMXPlayerVideo.h"
#include "DVDPlayerSubtitle.h"

#include "utils/BitstreamStats.h"

#include "linux/DllBCM.h"
#include "Edl.h"

#define MAX_CHAPTERS 64

#define DVDPLAYER_AUDIO    1
#define DVDPLAYER_VIDEO    2
#define DVDPLAYER_SUBTITLE 3
#define DVDPLAYER_TELETEXT 4

#define DVDSTATE_NORMAL           0x00000001 // normal dvd state
#define DVDSTATE_STILL            0x00000002 // currently displaying a still frame
#define DVDSTATE_WAIT             0x00000003 // waiting for demuxer read error
#define DVDSTATE_SEEK             0x00000004 // we are finishing a seek request

class COMXPlayer;
class OMXPlayerVideo;
class OMXPlayerAudio;

namespace PVR
{
  class CPVRChannel;
}

class COMXCurrentStream
{
public:
  int              id;     // demuxerid of current playing stream
  int              source;
  double           dts;    // last dts from demuxer, used to find disncontinuities
  double           dur;    // last frame expected duration
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


class COMXPlayer : public IPlayer, public CThread, public IDVDPlayer
{
public:

  COMXPlayer(IPlayerCallback &callback);
  virtual ~COMXPlayer();
  
  virtual void RegisterAudioCallback(IAudioCallback* pCallback) { m_player_audio.RegisterAudioCallback(pCallback); };
  virtual void UnRegisterAudioCallback()                        { m_player_audio.UnRegisterAudioCallback();        };

  virtual bool  IsValidStream(COMXCurrentStream& stream);
  virtual bool  IsBetterStream(COMXCurrentStream& current, CDemuxStream* stream);
  virtual bool  CheckDelayedChannelEntry(void);
  virtual bool  ReadPacket(DemuxPacket*& packet, CDemuxStream*& stream);
  virtual bool  CloseAudioStream(bool bWaitForBuffers);
  virtual bool  CloseVideoStream(bool bWaitForBuffers);
  virtual bool  CloseSubtitleStream(bool bKeepOverlays);
  virtual bool  OpenAudioStream(int iStream, int source);
  virtual bool  OpenVideoStream(int iStream, int source);
  virtual bool  OpenSubtitleStream(int iStream, int source); 
  virtual void  OpenDefaultStreams();
  virtual bool  OpenDemuxStream();
  virtual bool  OpenInputStream();
  virtual bool  CheckPlayerInit(COMXCurrentStream& current, unsigned int source);
  virtual void  UpdateCorrection(DemuxPacket* pkt, double correction);
  virtual void  UpdateTimestamps(COMXCurrentStream& current, DemuxPacket* pPacket);
  virtual void  UpdateLimits(double& minimum, double& maximum, double dts);
  virtual bool  CheckSceneSkip(COMXCurrentStream& current);
  virtual void  CheckAutoSceneSkip();
  virtual void  CheckContinuity(COMXCurrentStream& current, DemuxPacket* pPacket);
  virtual void  ProcessAudioData(CDemuxStream* pStream, DemuxPacket* pPacket);
  virtual void  ProcessVideoData(CDemuxStream* pStream, DemuxPacket* pPacket);
  virtual void  ProcessSubData(CDemuxStream* pStream, DemuxPacket* pPacket);
  virtual void  ProcessPacket(CDemuxStream* pStream, DemuxPacket* pPacket);
  virtual void  SynchronizeDemuxer(unsigned int timeout);
  virtual void  SynchronizePlayers(unsigned int sources);
  virtual void  SendPlayerMessage(CDVDMsg* pMsg, unsigned int target);
  virtual void  HandleMessages();

  virtual bool  OpenFile(const CFileItem &file, const CPlayerOptions &options);
  virtual bool  QueueNextFile(const CFileItem &file)             {return false;}
  virtual void  OnNothingToQueueNotify()                         {}
  virtual bool  CloseFile();
  virtual bool  IsPlaying() const;
  virtual void  SetPlaySpeed(int speed);
  int GetPlaySpeed()                                                { return m_playSpeed; }
  virtual void  Pause();
  virtual bool  IsPaused() const;
  virtual bool  HasVideo() const;
  virtual bool  HasAudio() const;
  virtual bool  IsPassthrough() const;
  virtual bool  CanSeek();
  virtual void  Seek(bool bPlus = true, bool bLargeStep = false);
  virtual bool  SeekScene(bool bPlus = true);
  virtual void  SeekPercentage(float fPercent = 0.0f);
  virtual float GetPercentage();
  virtual float GetCachePercentage();

  virtual void  SetVolume(float fVolume);
  virtual void  SetDynamicRangeCompression(long drc)              {}
  virtual void  GetAudioInfo(CStdString &strAudioInfo);
  virtual void  GetVideoInfo(CStdString &strVideoInfo);
  virtual void  GetGeneralInfo(CStdString &strVideoInfo);
  virtual void  Update(bool bPauseDrawing);
  virtual void  GetVideoRect(CRect& SrcRect, CRect& DestRect);
  virtual void  GetVideoAspectRatio(float &fAR);
  virtual void  UpdateApplication(double timeout);
  virtual bool  CanRecord();
  virtual bool  IsRecording();
  virtual bool  CanPause();
  virtual bool  Record(bool bOnOff);
  virtual void  SetAVDelay(float fValue = 0.0f);
  virtual float GetAVDelay();

  virtual void  SetSubTitleDelay(float fValue = 0.0f);
  virtual float GetSubTitleDelay();
  virtual int   GetSubtitleCount();
  virtual int   GetSubtitle();
  virtual void  GetSubtitleName(int iStream, CStdString &strStreamName);
  virtual void  GetSubtitleLanguage(int iStream, CStdString &strStreamLang);
  virtual void  SetSubtitle(int iStream);
  virtual bool  GetSubtitleVisible();
  virtual void  SetSubtitleVisible(bool bVisible);
  virtual bool  GetSubtitleExtension(CStdString &strSubtitleExtension) { return false; }
  virtual int   AddSubtitle(const CStdString& strSubPath);

  virtual int   GetAudioStreamCount();
  virtual int   GetAudioStream();
  virtual void  GetAudioStreamName(int iStream, CStdString &strStreamName);
  virtual void  SetAudioStream(int iStream);
  virtual void  GetAudioStreamLanguage(int iStream, CStdString &strLanguage);

  virtual TextCacheStruct_t* GetTeletextCache()                   {return NULL;};
  virtual void  LoadPage(int p, int sp, unsigned char* buffer)    {};

  virtual int   GetChapterCount();
  virtual int   GetChapter();
  virtual void  GetChapterName(CStdString& strChapterName);
  virtual int   SeekChapter(int iChapter);

  virtual void  SeekTime(int64_t iTime = 0);
  virtual int64_t GetTotalTimeInMsec();
  virtual int64_t GetTime();
  virtual int64_t GetTotalTime();
  virtual void  ToFFRW(int iSpeed = 0);
  virtual int   GetAudioBitrate();
  virtual int   GetVideoBitrate();
  virtual int   GetSourceBitrate();
  virtual int   GetChannels();
  virtual CStdString GetAudioCodecName();
  virtual CStdString GetVideoCodecName();
  virtual int   GetPictureWidth();
  virtual int   GetPictureHeight();
  virtual bool  GetStreamDetails(CStreamDetails &details);

  virtual bool  IsInMenu() const;
  virtual bool  HasMenu();

  virtual bool  GetCurrentSubtitle(CStdString& strSubtitle);
  //returns a state that is needed for resuming from a specific time
  virtual CStdString GetPlayerState();
  virtual bool  SetPlayerState(CStdString state);
  
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

  int m_playSpeed;
  struct SSpeedState
  {
    double lastpts;  // holds last display pts during ff/rw operations
    double lasttime;
  } m_SpeedState;

  void    HandlePlaySpeed();
  bool    GetCachingTimes(double& play_left, double& cache_left, double& file_offset);
  bool    CheckStartCaching(COMXCurrentStream& current);
  void    SetCaching(ECacheState state);
  double  GetQueueTime();
  virtual bool  IsCaching() const                                 { return m_caching == CACHESTATE_FULL; }
  virtual int   GetCacheLevel() const;

  virtual int  OnDVDNavResult(void* pData, int iMessage);
  virtual bool OnAction(const CAction &action);

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
  bool WaitForPausedThumbJobs(int timeout_ms);
  virtual void  Process();

  CEvent                m_ready;
  std::string           m_filename; // holds the actual filename
  CDVDInputStream       *m_pInputStream;
  CDVDDemux             *m_pDemuxer;
  CDVDDemux*            m_pSubtitleDemuxer;
  COMXSelectionStreams  m_SelectionStreams;
  std::string           m_mimetype;
  COMXCurrentStream     m_CurrentAudio;
  COMXCurrentStream     m_CurrentVideo;
  COMXCurrentStream     m_CurrentSubtitle;
  COMXCurrentStream     m_CurrentTeletext;

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

  struct SPlayerState
  {
    SPlayerState() { Clear(); }
    void Clear()
    {
      timestamp     = 0;
      time          = 0;
      time_total    = 0;
      time_offset   = 0;
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

    double timestamp;         // last time of update
    double time_offset;       // difference between time and pts

    double time;              // current playback time
    double time_total;        // total playback time
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
  } m_State;
  CCriticalSection m_StateSection;

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

  bool ShowPVRChannelInfo();

  int  AddSubtitleFile(const std::string& filename, const std::string& subfilename = "", CDemuxStream::EFlags flags = CDemuxStream::FLAG_NONE);
  virtual void UpdatePlayState(double timeout);

  double m_UpdateApplication;

  void RenderUpdateCallBack(const void *ctx, const CRect &SrcRect, const CRect &DestRect);

private:
  void FlushBuffers(bool queued, double pts = DVD_NOPTS_VALUE, bool accurate = true);

  CCriticalSection        m_critStreamSection;

  bool                    m_paused;
  bool                    m_bAbortRequest;
  CFileItem               m_item;
  CPlayerOptions          m_PlayerOptions;
  unsigned int            m_iChannelEntryTimeOut;

  std::string             m_lastSub;

  double                  m_offset_pts;

  OMXClock                m_av_clock;
  OMXPlayerVideo          m_player_video;
  OMXPlayerAudio          m_player_audio;
  CDVDPlayerSubtitle      m_player_subtitle;

  CDVDMessageQueue        m_messenger;

  float                   m_current_volume;
  bool                    m_change_volume;
  bool                    m_stats;
  CDVDOverlayContainer    m_overlayContainer;
  ECacheState             m_caching;
};
