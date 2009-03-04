#pragma once

/*
 *      Copyright (C) 2005-2008 Team XBMC
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

#include "cores/IPlayer.h"
#include "utils/Thread.h"

#include "IDVDPlayer.h"

#include "DVDMessageQueue.h"
#include "DVDClock.h"
#include "DVDPlayerAudio.h"
#include "DVDPlayerVideo.h"
#include "DVDPlayerSubtitle.h"

//#include "DVDChapterReader.h"
#include "DVDSubtitles/DVDFactorySubtitle.h"
#include "utils/BitstreamStats.h"

#include "../../Edl.h"
#include "dlgcache.h"
#include "FileItem.h"


class CDVDInputStream;

class CDVDDemux;
class CDemuxStreamVideo;
class CDemuxStreamAudio;
class CStreamInfo;

#define DVDSTATE_NORMAL           0x00000001 // normal dvd state
#define DVDSTATE_STILL            0x00000002 // currently displaying a still frame
#define DVDSTATE_WAIT             0x00000003 // waiting for demuxer read error

typedef struct DVDInfo
{
  int state;                // current dvdstate
  DWORD iDVDStillTime;      // total time in ticks we should display the still before continuing
  DWORD iDVDStillStartTime; // time in ticks when we started the still
  int iSelectedSPUStream;   // mpeg stream id, or -1 if disabled
  int iSelectedAudioStream; // mpeg stream id, or -1 if disabled
}
DVDInfo;

class CCurrentStream
{
public:
  int              id;     // demuxerid of current playing stream
  int              source;
  double           dts;    // last dts from demuxer, used to find disncontinuities
  CDVDStreamInfo   hint;   // stream hints, used to notice stream changes
  void*            stream; // pointer or integer, identifying stream playing. if it changes stream changed
  bool             inited;
  const StreamType type;
  // stuff to handle starting after seek
  double                     startpts;
  CDVDMsgGeneralSynchronize* startsync;

  CCurrentStream(StreamType t)
    : type(t)
  {
    startsync = NULL;
    Clear();
  }

  void Clear()
  {
    id     = -1;
    source = STREAM_SOURCE_NONE;
    dts    = DVD_NOPTS_VALUE;
    hint.Clear();
    stream = NULL;
    inited = false;
    if(startsync) 
      startsync->Release();
    startsync = NULL;
    startpts  = DVD_NOPTS_VALUE;
  }
};

typedef struct 
{
  StreamType   type;
  std::string  filename;
  std::string  language;
  std::string  name;
  int          source;
  int          id;  
} SelectionStream;

class CSelectionStreams
{
  CCriticalSection m_section;
  SelectionStream  m_invalid;  
public:
  CSelectionStreams()
  {
    m_invalid.id = -1;
    m_invalid.source = STREAM_SOURCE_NONE;
    m_invalid.type = STREAM_NONE;
  }
  std::vector<SelectionStream> m_Streams;

  int              IndexOf (StreamType type, int source, int id);
  int              IndexOf (StreamType type, CDVDPlayer& p);
  int              Count   (StreamType type) { return IndexOf(type, STREAM_SOURCE_NONE, -1) + 1; }
  SelectionStream& Get     (StreamType type, int index);
  
  void             Clear   (StreamType type, StreamSource source);
  int              Source  (StreamSource source, std::string filename);

  void             Update  (SelectionStream& s);
  void             Update  (CDVDInputStream* input, CDVDDemux* demuxer);
};


#define DVDPLAYER_AUDIO    1
#define DVDPLAYER_VIDEO    2
#define DVDPLAYER_SUBTITLE 3

class CDVDPlayer : public IPlayer, public CThread, public IDVDPlayer
{
public:
  CDVDPlayer(IPlayerCallback& callback);
  virtual ~CDVDPlayer();
  virtual void RegisterAudioCallback(IAudioCallback* pCallback) { m_dvdPlayerAudio.RegisterAudioCallback(pCallback); }
  virtual void UnRegisterAudioCallback()                        { m_dvdPlayerAudio.UnRegisterAudioCallback(); }
  virtual bool OpenFile(const CFileItem& file, const CPlayerOptions &options);
  virtual bool CloseFile();
  virtual bool IsPlaying() const;
  virtual void Pause();
  virtual bool IsPaused() const;
  virtual bool HasVideo() const;
  virtual bool HasAudio() const;
  virtual void ToggleFrameDrop();
  virtual bool CanSeek();
  virtual void Seek(bool bPlus, bool bLargeStep);
  virtual bool SeekScene(bool bPlus = true);
  virtual void SeekPercentage(float iPercent);
  virtual float GetPercentage();
  virtual void SetVolume(long nVolume)                          { m_dvdPlayerAudio.SetVolume(nVolume); }
  virtual void SetDynamicRangeCompression(long drc)             { m_dvdPlayerAudio.SetDynamicRangeCompression(drc); }
  virtual void GetAudioInfo(CStdString& strAudioInfo);
  virtual void GetVideoInfo(CStdString& strVideoInfo);
  virtual void GetGeneralInfo( CStdString& strVideoInfo);
  virtual void Update(bool bPauseDrawing)                       { m_dvdPlayerVideo.Update(bPauseDrawing); }
  virtual void GetVideoRect(RECT& SrcRect, RECT& DestRect)      { m_dvdPlayerVideo.GetVideoRect(SrcRect, DestRect); }
  virtual void GetVideoAspectRatio(float& fAR)                  { fAR = m_dvdPlayerVideo.GetAspectRatio(); }
  virtual bool CanRecord();
  virtual bool IsRecording();
  virtual bool Record(bool bOnOff);
  virtual void SetAVDelay(float fValue = 0.0f);
  virtual float GetAVDelay();

  virtual void SetSubTitleDelay(float fValue = 0.0f);
  virtual float GetSubTitleDelay();
  virtual int GetSubtitleCount();
  virtual int GetSubtitle();
  virtual void GetSubtitleName(int iStream, CStdString &strStreamName);
  virtual void SetSubtitle(int iStream);
  virtual bool GetSubtitleVisible();
  virtual void SetSubtitleVisible(bool bVisible);
  virtual bool GetSubtitleExtension(CStdString &strSubtitleExtension) { return false; }
  virtual bool AddSubtitle(const CStdString& strSubPath);

  virtual int GetAudioStreamCount();
  virtual int GetAudioStream();
  virtual void GetAudioStreamName(int iStream, CStdString &strStreamName);
  virtual void SetAudioStream(int iStream);

  virtual int  GetChapterCount();
  virtual int  GetChapter();
  virtual void GetChapterName(CStdString& strChapterName);
  virtual int  SeekChapter(int iChapter);

  virtual void SeekTime(__int64 iTime);
  virtual __int64 GetTime();
  virtual int GetTotalTime();
  virtual void ToFFRW(int iSpeed);
  virtual bool OnAction(const CAction &action);
  virtual bool HasMenu();
  virtual int GetAudioBitrate();
  virtual int GetVideoBitrate();
  virtual int GetSourceBitrate();
  
  virtual bool GetCurrentSubtitle(CStdString& strSubtitle);
  
  virtual CStdString GetPlayerState();
  virtual bool SetPlayerState(CStdString state);

  virtual bool IsCaching() const { return m_caching; }
  virtual int GetCacheLevel() const ; 

  virtual int OnDVDNavResult(void* pData, int iMessage);    
protected:  
  friend class CSelectionStreams;
  void LockStreams()                                            { EnterCriticalSection(&m_critStreamSection); }
  void UnlockStreams()                                          { LeaveCriticalSection(&m_critStreamSection); }
  
  virtual void OnStartup();
  virtual void OnExit();
  virtual void Process();

  bool OpenAudioStream(int iStream, int source);
  bool OpenVideoStream(int iStream, int source);
  bool OpenSubtitleStream(int iStream, int source);
  bool CloseAudioStream(bool bWaitForBuffers);
  bool CloseVideoStream(bool bWaitForBuffers);
  bool CloseSubtitleStream(bool bKeepOverlays);

  void ProcessPacket(CDemuxStream* pStream, DemuxPacket* pPacket);
  void ProcessAudioData(CDemuxStream* pStream, DemuxPacket* pPacket);
  void ProcessVideoData(CDemuxStream* pStream, DemuxPacket* pPacket);
  void ProcessSubData(CDemuxStream* pStream, DemuxPacket* pPacket);
  
  bool AddSubtitleFile(const std::string& filename);
  /**
   * one of the DVD_PLAYSPEED defines
   */
  void SetPlaySpeed(int iSpeed);
  int GetPlaySpeed()                                                { return m_playSpeed; }
  void SetCaching(bool enabled);

  __int64 GetTotalTimeInMsec();
  void FlushBuffers(bool queued);

  void HandleMessages();
  void HandlePlaySpeed();
  bool IsInMenu() const;

  void SyncronizePlayers(DWORD sources, double pts = DVD_NOPTS_VALUE);
  void SyncronizeDemuxer(DWORD timeout);
  void CheckContinuity(CCurrentStream& current, DemuxPacket* pPacket);
  bool CheckSceneSkip(CCurrentStream& current);
  bool CheckPlayerInit(CCurrentStream& current, unsigned int source);
  bool CheckStartCaching(CCurrentStream& current);
  void SendPlayerMessage(CDVDMsg* pMsg, unsigned int target);

  bool ReadPacket(DemuxPacket*& packet, CDemuxStream*& stream);
  bool IsValidStream(CCurrentStream& stream);
  bool IsBetterStream(CCurrentStream& current, CDemuxStream* stream);

  bool OpenInputStream();
  bool OpenDemuxStream();

  void UpdateApplication(double timeout);
  void UpdatePlayState(double timeout);
  double m_UpdateApplication;

  bool m_bAbortRequest;

  std::string m_filename; // holds the actual filename
  std::string m_content;  // hold a hint to what content file contains (mime type)
  bool        m_caching;  // player is filling up the demux queue
  bool        m_seeking;  // player is currently trying to fullfill a seek request
  CFileItem   m_item;

  CCurrentStream m_CurrentAudio;
  CCurrentStream m_CurrentVideo;
  CCurrentStream m_CurrentSubtitle;

  CSelectionStreams m_SelectionStreams;

  int m_playSpeed;
  time_t m_tmLastSeek;
  struct SSpeedState
  {
    double lastpts;  // holds last display pts during ff/rw operations
    double lasttime; 
  } m_SpeedState;

  int m_errorCount;
  // classes
  CDVDPlayerVideo m_dvdPlayerVideo; // video part
  CDVDPlayerAudio m_dvdPlayerAudio; // audio part
  CDVDPlayerSubtitle m_dvdPlayerSubtitle; // subtitle part
  
  CDVDMessageQueue m_messenger;     // thread messenger, only the dvdplayer.cpp class itself may send message to this!
  
  CDVDClock m_clock;                // master clock  
  CDVDOverlayContainer m_overlayContainer;
  
  CDVDInputStream* m_pInputStream;  // input stream for current playing file
  CDVDDemux* m_pDemuxer;            // demuxer for current playing file
  CDVDDemux* m_pSubtitleDemuxer;
  DVDInfo m_dvd;

  CDlgCache *m_pDlgCache;  

  struct SPlayerState
  {
    void Clear()
    {
      timestamp     = 0;
      time          = 0;
      time_total    = 0;
      chapter       = 0;
      chapter_count = 0;
      canrecord     = false;
      recording     = false;
      demux_video   = "";
      demux_audio   = "";
    }

    double timestamp;         // last time of update

    double time;              // current playback time
    double time_total;        // total playback time

    int         chapter;      // current chapter
    std::string chapter_name; // name of current chapter
    int         chapter_count;// number of chapter

    bool canrecord;           // can input stream record
    bool recording;           // are we currently recording

    std::string demux_video;
    std::string demux_audio;
  } m_State;
  CCriticalSection m_StateSection;

  class CPlayerSeek
  {
  public:
    CPlayerSeek(CDVDPlayer* player);
    ~CPlayerSeek();
    CDVDPlayer& m_player;
  };

  HANDLE m_hReadyEvent;
  CRITICAL_SECTION m_critStreamSection; // need to have this lock when switching streams (audio / video)

  CEdl m_Edl;

  CPlayerOptions m_PlayerOptions;
};

