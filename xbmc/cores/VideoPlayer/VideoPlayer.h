/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DVDClock.h"
#include "DVDMessageQueue.h"
#include "Edl.h"
#include "FileItem.h"
#include "IVideoPlayer.h"
#include "VideoPlayerAudioID3.h"
#include "VideoPlayerRadioRDS.h"
#include "VideoPlayerSubtitle.h"
#include "VideoPlayerTeletext.h"
#include "cores/IPlayer.h"
#include "cores/MenuType.h"
#include "cores/VideoPlayer/Interface/TimingConstants.h"
#include "cores/VideoPlayer/VideoRenderers/RenderManager.h"
#include "guilib/DispResource.h"
#include "threads/SystemClock.h"
#include "threads/Thread.h"

#include <atomic>
#include <chrono>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

struct SPlayerState
{
  SPlayerState() { Clear(); }
  void Clear()
  {
    timestamp = 0;
    time = 0;
    startTime = 0;
    timeMin = 0;
    timeMax = 0;
    time_offset = 0;
    dts = DVD_NOPTS_VALUE;
    player_state  = "";
    isInMenu = false;
    menuType = MenuType::NONE;
    chapter = 0;
    chapters.clear();
    canpause = false;
    canseek = false;
    cantempo = false;
    caching = false;
    cache_bytes = 0;
    cache_level = 0.0;
    cache_offset = 0.0;
    lastSeek = 0;
    streamsReady = false;
  }

  double timestamp;         // last time of update
  double lastSeek;          // time of last seek
  double time_offset;       // difference between time and pts

  double time;              // current playback time
  double timeMax;
  double timeMin;
  time_t startTime;
  double dts;               // last known dts

  std::string player_state; // full player state
  bool isInMenu;
  MenuType menuType;
  bool streamsReady;

  int chapter;              // current chapter
  std::vector<std::pair<std::string, int64_t>> chapters; // name and position for chapters

  bool canpause;            // pvr: can pause the current playing item
  bool canseek;             // pvr: can seek in the current playing item
  bool cantempo;
  bool caching;

  int64_t cache_bytes; // number of bytes current's cached
  double cache_level; // current cache level
  double cache_offset; // percentage of file ahead of current position
  double cache_time; // estimated playback time of current cached bytes
};

class CDVDInputStream;

class CDVDDemux;
class CDemuxStreamVideo;
class CDemuxStreamAudio;
class CStreamInfo;
class CDVDDemuxCC;
class CVideoPlayer;

#define DVDSTATE_NORMAL           0x00000001 // normal dvd state
#define DVDSTATE_STILL            0x00000002 // currently displaying a still frame
#define DVDSTATE_WAIT             0x00000003 // waiting for demuxer read error
#define DVDSTATE_SEEK             0x00000004 // we are finishing a seek request

class CCurrentStream
{
public:
  int64_t demuxerId; // demuxer's id of current playing stream
  int id;     // id of current playing stream
  int source;
  double dts;    // last dts from demuxer, used to find discontinuities
  double dur;    // last frame expected duration
  int dispTime; // display time from input stream
  CDVDStreamInfo hint;   // stream hints, used to notice stream changes
  void* stream; // pointer or integer, identifying stream playing. if it changes stream changed
  int changes; // remembered counter from stream to track codec changes
  bool inited;
  unsigned int packets;
  IDVDStreamPlayer::ESyncState syncState;
  double starttime;
  double cachetime;
  double cachetotal;
  const StreamType type;
  const int player;
  // stuff to handle starting after seek
  double startpts;
  double lastdts;

  enum
  {
    AV_SYNC_NONE,
    AV_SYNC_CHECK,
    AV_SYNC_CONT,
    AV_SYNC_FORCE
  } avsync;

  CCurrentStream(StreamType t, int i)
    : type(t)
    , player(i)
  {
    Clear();
  }

  void Clear()
  {
    id = -1;
    demuxerId = -1;
    source = STREAM_SOURCE_NONE;
    dts = DVD_NOPTS_VALUE;
    dur = DVD_NOPTS_VALUE;
    hint.Clear();
    stream = NULL;
    changes = 0;
    inited = false;
    packets = 0;
    syncState = IDVDStreamPlayer::SYNC_STARTING;
    starttime = DVD_NOPTS_VALUE;
    startpts = DVD_NOPTS_VALUE;
    lastdts = DVD_NOPTS_VALUE;
    avsync = AV_SYNC_FORCE;
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

//------------------------------------------------------------------------------
// selection streams
//------------------------------------------------------------------------------
struct SelectionStream
{
  StreamType type = STREAM_NONE;
  int type_index = 0;
  std::string filename;
  std::string filename2;  // for vobsub subtitles, 2 files are necessary (idx/sub)
  std::string language;
  std::string name;
  StreamFlags flags = StreamFlags::FLAG_NONE;
  int source = 0;
  int id = 0;
  int64_t demuxerId = -1;
  std::string codec;
  int channels = 0;
  int bitrate = 0;
  int width = 0;
  int height = 0;
  CRect SrcRect;
  CRect DestRect;
  CRect VideoRect;
  std::string stereo_mode;
  float aspect_ratio = 0.0f;
  StreamHdrType hdrType = StreamHdrType::HDR_TYPE_NONE;
};

class CSelectionStreams
{
public:
  CSelectionStreams() = default;

  int TypeIndexOf(StreamType type, int source, int64_t demuxerId, int id) const;
  int CountTypeOfSource(StreamType type, StreamSource source) const;
  int CountType(StreamType type) const;
  SelectionStream& Get(StreamType type, int index);
  const SelectionStream& Get(StreamType type, int index) const;
  bool Get(StreamType type, StreamFlags flag, SelectionStream& out);
  void Clear(StreamType type, StreamSource source);
  int Source(StreamSource source, const std::string& filename);
  void Update(SelectionStream& s);
  void Update(const std::shared_ptr<CDVDInputStream>& input, CDVDDemux* demuxer);
  void Update(const std::shared_ptr<CDVDInputStream>& input,
              CDVDDemux* demuxer,
              const std::string& filename2);

  std::vector<SelectionStream> Get(StreamType type);
  template<typename Compare> std::vector<SelectionStream> Get(StreamType type, Compare compare)
  {
    std::vector<SelectionStream> streams = Get(type);
    std::stable_sort(streams.begin(), streams.end(), compare);
    return streams;
  }

  std::vector<SelectionStream> m_Streams;

protected:
  SelectionStream m_invalid;
};

//------------------------------------------------------------------------------
// main class
//------------------------------------------------------------------------------

struct CacheInfo
{
  double level; // current cache level
  double offset; // percentage of file ahead of current position
  double time; // estimated playback time of current cached bytes
  bool valid;
};

class CProcessInfo;
class CJobQueue;

class CVideoPlayer : public IPlayer, public CThread, public IVideoPlayer,
                     public IDispResource, public IRenderLoop, public IRenderMsg
{
public:
  explicit CVideoPlayer(IPlayerCallback& callback);
  ~CVideoPlayer() override;
  bool OpenFile(const CFileItem& file, const CPlayerOptions &options) override;
  bool CloseFile(bool reopen = false) override;
  bool IsPlaying() const override;
  void Pause() override;
  bool HasVideo() const override;
  bool HasAudio() const override;
  bool HasRDS() const override;
  bool HasID3() const override;
  bool IsPassthrough() const override;
  bool CanSeek() const override;
  void Seek(bool bPlus, bool bLargeStep, bool bChapterOverride) override;
  bool SeekScene(bool bPlus = true) override;
  void SeekPercentage(float iPercent) override;
  float GetCachePercentage() const override;

  void SetDynamicRangeCompression(long drc) override;
  bool CanPause() const override;
  void SetAVDelay(float fValue = 0.0f) override;
  float GetAVDelay() override;
  bool IsInMenu() const override;

  /*!
   * \brief Get the supported menu type
   * \return The supported menu type
  */
  MenuType GetSupportedMenuType() const override;

  void SetSubTitleDelay(float fValue = 0.0f) override;
  float GetSubTitleDelay() override;
  int GetSubtitleCount() const override;
  int GetSubtitle() override;
  void GetSubtitleStreamInfo(int index, SubtitleStreamInfo& info) const override;
  void SetSubtitle(int iStream) override;
  bool GetSubtitleVisible() const override;
  void SetSubtitleVisible(bool bVisible) override;

  /*!
   * \brief Set the subtitle vertical position,
   * it depends on current screen resolution
   * \param value The subtitle position in pixels
   * \param save If true, the value will be saved to resolution info
   */
  void SetSubtitleVerticalPosition(const int value, bool save) override;

  void AddSubtitle(const std::string& strSubPath) override;

  int GetAudioStreamCount() const override;
  int GetAudioStream() override;
  void SetAudioStream(int iStream) override;

  int GetVideoStream() const override;
  int GetVideoStreamCount() const override;
  void GetVideoStreamInfo(int streamId, VideoStreamInfo& info) const override;
  void SetVideoStream(int iStream) override;

  int GetPrograms(std::vector<ProgramInfo>& programs) override;
  void SetProgram(int progId) override;
  int GetProgramsCount() const override;

  std::shared_ptr<TextCacheStruct_t> GetTeletextCache() override;
  bool HasTeletextCache() const override;
  void LoadPage(int p, int sp, unsigned char* buffer) override;

  int GetChapterCount() const override;
  int GetChapter() const override;
  void GetChapterName(std::string& strChapterName, int chapterIdx = -1) const override;
  int64_t GetChapterPos(int chapterIdx = -1) const override;
  int  SeekChapter(int iChapter) override;

  void SeekTime(int64_t iTime) override;
  bool SeekTimeRelative(int64_t iTime) override;
  void SetSpeed(float speed) override;
  void SetTempo(float tempo) override;
  bool SupportsTempo() const override;
  void FrameAdvance(int frames) override;
  bool OnAction(const CAction &action) override;

  void GetAudioStreamInfo(int index, AudioStreamInfo& info) const override;

  std::string GetPlayerState() override;
  bool SetPlayerState(const std::string& state) override;

  void FrameMove() override;
  void Render(bool clear, uint32_t alpha = 255, bool gui = true) override;
  void FlushRenderer() override;
  void SetRenderViewMode(int mode, float zoom, float par, float shift, bool stretch) override;
  float GetRenderAspectRatio() const override;
  void GetRects(CRect& source, CRect& dest, CRect& view) const override;
  unsigned int GetOrientation() const override;
  void TriggerUpdateResolution() override;
  bool IsRenderingVideo() const override;
  bool Supports(EINTERLACEMETHOD method) const override;
  EINTERLACEMETHOD GetDeinterlacingMethodDefault() const override;
  bool Supports(ESCALINGMETHOD method) const override;
  bool Supports(ERENDERFEATURE feature) const override;

  unsigned int RenderCaptureAlloc() override;
  void RenderCapture(unsigned int captureId, unsigned int width, unsigned int height, int flags) override;
  void RenderCaptureRelease(unsigned int captureId) override;
  bool RenderCaptureGetPixels(unsigned int captureId, unsigned int millis, uint8_t *buffer, unsigned int size) override;

  // IDispResource interface
  void OnLostDisplay() override;
  void OnResetDisplay() override;

  bool IsCaching() const override;
  int GetCacheLevel() const override;

  int OnDiscNavResult(void* pData, int iMessage) override;
  void GetVideoResolution(unsigned int &width, unsigned int &height) override;

  CVideoSettings GetVideoSettings() const override;
  void SetVideoSettings(CVideoSettings& settings) override;

  void SetUpdateStreamDetails();

protected:
  friend class CSelectionStreams;

  void OnStartup() override;
  void OnExit() override;
  void Process() override;
  void VideoParamsChange() override;
  void GetDebugInfo(std::string &audio, std::string &video, std::string &general) override;
  void UpdateClockSync(bool enabled) override;
  void UpdateRenderInfo(CRenderInfo &info) override;
  void UpdateRenderBuffers(int queued, int discard, int free) override;
  void UpdateGuiRender(bool gui) override;
  void UpdateVideoRender(bool video) override;

  void CreatePlayers();
  void DestroyPlayers();

  void Prepare();
  bool OpenStream(CCurrentStream& current, int64_t demuxerId, int iStream, int source, bool reset = true);
  bool OpenAudioStream(CDVDStreamInfo& hint, bool reset = true);
  bool OpenVideoStream(CDVDStreamInfo& hint, bool reset = true);
  bool OpenSubtitleStream(const CDVDStreamInfo& hint);
  bool OpenTeletextStream(CDVDStreamInfo& hint);
  bool OpenRadioRDSStream(CDVDStreamInfo& hint);
  bool OpenAudioID3Stream(CDVDStreamInfo& hint);

  /** \brief Switches forced subtitles to forced subtitles matching the language of the current audio track.
  *          If these are not available, subtitles are disabled.
  */
  void AdaptForcedSubtitles();
  bool CloseStream(CCurrentStream& current, bool bWaitForBuffers);

  bool CheckIsCurrent(const CCurrentStream& current, CDemuxStream* stream, DemuxPacket* pkg);
  void ProcessPacket(CDemuxStream* pStream, DemuxPacket* pPacket);
  void ProcessAudioData(CDemuxStream* pStream, DemuxPacket* pPacket);
  void ProcessVideoData(CDemuxStream* pStream, DemuxPacket* pPacket);
  void ProcessSubData(CDemuxStream* pStream, DemuxPacket* pPacket);
  void ProcessTeletextData(CDemuxStream* pStream, DemuxPacket* pPacket);
  void ProcessRadioRDSData(CDemuxStream* pStream, DemuxPacket* pPacket);
  void ProcessAudioID3Data(CDemuxStream* pStream, DemuxPacket* pPacket);

  int  AddSubtitleFile(const std::string& filename, const std::string& subfilename = "");

  /*!
   * \brief Propagate enable stream callbacks to demuxers.
   * \param current The current stream
   * \param isEnabled Set to true to enable the stream, otherwise false
   */
  void SetEnableStream(CCurrentStream& current, bool isEnabled);

  void SetSubtitleVisibleInternal(bool bVisible);

  /**
   * one of the DVD_PLAYSPEED defines
   */
  void SetPlaySpeed(int iSpeed);

  enum ECacheState
  {
    CACHESTATE_DONE = 0,
    CACHESTATE_FULL,     // player is filling up the demux queue
    CACHESTATE_INIT,     // player is waiting for first packet of each stream
    CACHESTATE_PLAY,     // player is waiting for players to not be stalled
    CACHESTATE_FLUSH,    // temporary state player will choose startup between init or full
  };

  void SetCaching(ECacheState state);

  double GetQueueTime();
  CacheInfo GetCachingTimes();

  void FlushBuffers(double pts, bool accurate, bool sync);

  void HandleMessages();
  void HandlePlaySpeed();
  bool IsInMenuInternal() const;
  void SynchronizeDemuxer();
  void CheckAutoSceneSkip();
  bool CheckContinuity(CCurrentStream& current, DemuxPacket* pPacket);
  bool CheckSceneSkip(const CCurrentStream& current);
  bool CheckPlayerInit(CCurrentStream& current);
  void UpdateCorrection(DemuxPacket* pkt, double correction);
  void UpdateTimestamps(CCurrentStream& current, DemuxPacket* pPacket);
  IDVDStreamPlayer* GetStreamPlayer(unsigned int player);
  void SendPlayerMessage(std::shared_ptr<CDVDMsg> pMsg, unsigned int target);

  bool ReadPacket(DemuxPacket*& packet, CDemuxStream*& stream);
  bool IsValidStream(const CCurrentStream& stream);
  bool IsBetterStream(const CCurrentStream& current, CDemuxStream* stream);
  void CheckBetterStream(CCurrentStream& current, CDemuxStream* stream);
  void CheckStreamChanges(CCurrentStream& current, CDemuxStream* stream);

  bool OpenInputStream();
  bool OpenDemuxStream();
  void CloseDemuxer();
  void OpenDefaultStreams(bool reset = true);

  void UpdatePlayState(double timeout);
  void GetGeneralInfo(std::string& strVideoInfo);
  int64_t GetUpdatedTime();
  int64_t GetTime();
  float GetPercentage();

  void UpdateContent();
  void UpdateContentState();

  void UpdateFileItemStreamDetails(CFileItem& item);
  int GetPreviousChapter();

  bool m_players_created;

  CFileItem m_item;
  CPlayerOptions m_playerOptions;
  bool m_bAbortRequest;
  bool m_error;
  bool m_bCloseRequest;

  ECacheState  m_caching;
  XbmcThreads::EndTime<> m_cachingTimer;

  std::unique_ptr<CProcessInfo> m_processInfo;

  CCurrentStream m_CurrentAudio;
  CCurrentStream m_CurrentVideo;
  CCurrentStream m_CurrentSubtitle;
  CCurrentStream m_CurrentTeletext;
  CCurrentStream m_CurrentRadioRDS;
  CCurrentStream m_CurrentAudioID3;

  CSelectionStreams m_SelectionStreams;
  std::vector<ProgramInfo> m_programs;

  struct SContent
  {
    mutable CCriticalSection m_section;
    CSelectionStreams m_selectionStreams;
    std::vector<ProgramInfo> m_programs;
    int m_videoIndex{-1};
    int m_audioIndex{-1};
    int m_subtitleIndex{-1};
  } m_content;

  int m_playSpeed;
  int m_streamPlayerSpeed;
  int m_demuxerSpeed = DVD_PLAYSPEED_NORMAL;
  struct SSpeedState
  {
    double lastpts{0.0}; // holds last display pts during ff/rw operations
    int64_t lasttime{0};
    double lastseekpts{0.0};
    double lastabstime{0.0};

    void Reset(double pts)
    {
      *this = {};
      if (pts != DVD_NOPTS_VALUE)
      {
        lastseekpts = pts;
      }
    }
  } m_SpeedState;

  double m_offset_pts;

  CDVDMessageQueue m_messenger;
  std::unique_ptr<CJobQueue> m_outboundEvents;

  IDVDStreamPlayerVideo *m_VideoPlayerVideo;
  IDVDStreamPlayerAudio *m_VideoPlayerAudio;
  CVideoPlayerSubtitle *m_VideoPlayerSubtitle;
  CDVDTeletextData *m_VideoPlayerTeletext;
  CDVDRadioRDSData *m_VideoPlayerRadioRDS;
  std::unique_ptr<CVideoPlayerAudioID3> m_VideoPlayerAudioID3;

  CDVDClock m_clock;
  CDVDOverlayContainer m_overlayContainer;

  std::shared_ptr<CDVDInputStream> m_pInputStream;
  std::unique_ptr<CDVDDemux> m_pDemuxer;
  std::shared_ptr<CDVDDemux> m_pSubtitleDemuxer;
  std::unordered_map<int64_t, std::shared_ptr<CDVDDemux>> m_subtitleDemuxerMap;
  std::unique_ptr<CDVDDemuxCC> m_pCCDemuxer;

  CRenderManager m_renderManager;

  struct SDVDInfo
  {
    void Clear()
    {
      state                =  DVDSTATE_NORMAL;
      iSelectedSPUStream   = -1;
      iSelectedAudioStream = -1;
      iSelectedVideoStream = -1;
      iDVDStillTime = std::chrono::milliseconds::zero();
      iDVDStillStartTime = {};
      syncClock = false;
    }

    int state;                // current dvdstate
    bool syncClock;
    std::chrono::milliseconds
        iDVDStillTime; // total time in ticks we should display the still before continuing
    std::chrono::time_point<std::chrono::steady_clock>
        iDVDStillStartTime; // time in ticks when we started the still
    int iSelectedSPUStream;   // mpeg stream id, or -1 if disabled
    int iSelectedAudioStream; // mpeg stream id, or -1 if disabled
    int iSelectedVideoStream; // mpeg stream id or angle, -1 if disabled
  } m_dvd;

  SPlayerState m_State;
  mutable CCriticalSection m_StateSection;
  XbmcThreads::EndTime<> m_syncTimer;

  CEdl m_Edl;
  bool m_SkipCommercials;

  bool m_HasVideo;
  bool m_HasAudio;

  bool m_UpdateStreamDetails;

  std::atomic<bool> m_displayLost;
};
