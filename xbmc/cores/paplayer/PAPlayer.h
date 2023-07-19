/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "AudioDecoder.h"
#include "cores/AudioEngine/Interfaces/AE.h"
#include "cores/AudioEngine/Interfaces/IAudioCallback.h"
#include "cores/IPlayer.h"
#include "threads/CriticalSection.h"
#include "threads/Thread.h"
#include "utils/Job.h"

#include <atomic>
#include <list>
#include <vector>

class IAEStream;
class CFileItem;
class CProcessInfo;

class PAPlayer : public IPlayer, public CThread, public IJobCallback
{
friend class CQueueNextFileJob;
public:
  explicit PAPlayer(IPlayerCallback& callback);
  ~PAPlayer() override;

  bool OpenFile(const CFileItem& file, const CPlayerOptions &options) override;
  bool QueueNextFile(const CFileItem &file) override;
  void OnNothingToQueueNotify() override;
  bool CloseFile(bool reopen = false) override;
  bool IsPlaying() const override;
  void Pause() override;
  bool HasVideo() const override { return false; }
  bool HasAudio() const override { return true; }
  bool CanSeek() const override;
  void Seek(bool bPlus = true, bool bLargeStep = false, bool bChapterOverride = false) override;
  void SeekPercentage(float fPercent = 0.0f) override;
  void SetVolume(float volume) override;
  void SetDynamicRangeCompression(long drc) override;
  void SetSpeed(float speed = 0) override;
  int GetCacheLevel() const override;
  void SetTotalTime(int64_t time) override;
  void GetAudioStreamInfo(int index, AudioStreamInfo& info) const override;
  void SetTime(int64_t time) override;
  void SeekTime(int64_t iTime = 0) override;
  void GetAudioCapabilities(std::vector<int>& audioCaps) const override {}

  int GetAudioStreamCount() const override { return 1; }
  int GetAudioStream() override { return 0; }

  // implementation of IJobCallback
  void OnJobComplete(unsigned int jobID, bool success, CJob *job) override;

  struct
  {
    char         m_codec[21];
    int64_t      m_time;
    int64_t      m_totalTime;
    int          m_channelCount;
    int          m_bitsPerSample;
    int          m_sampleRate;
    int          m_audioBitrate;
    int          m_cacheLevel;
    bool         m_canSeek;
  } m_playerGUIData;

protected:
  // implementation of CThread
  void OnStartup() override {}
  void Process() override;
  void OnExit() override;
  float GetPercentage();

private:
  struct StreamInfo
  {
    std::unique_ptr<CFileItem> m_fileItem;
    std::unique_ptr<CFileItem> m_nextFileItem;
    CAudioDecoder m_decoder;             /* the stream decoder */
    int64_t m_startOffset;               /* the stream start offset */
    int64_t m_endOffset;                 /* the stream end offset */
    int64_t m_decoderTotal = 0;
    AEAudioFormat m_audioFormat;
    unsigned int m_bytesPerSample;       /* number of bytes per audio sample */
    unsigned int m_bytesPerFrame;        /* number of bytes per audio frame */

    bool m_started;                      /* if playback of this stream has been started */
    bool m_finishing;                    /* if this stream is finishing */
    int m_framesSent;                    /* number of frames sent to the stream */
    int m_prepareNextAtFrame;            /* when to prepare the next stream */
    bool m_prepareTriggered;             /* if the next stream has been prepared */
    int m_playNextAtFrame;               /* when to start playing the next stream */
    bool m_playNextTriggered;            /* if this stream has started the next one */
    bool m_fadeOutTriggered;             /* if the stream has been told to fade out */
    int m_seekNextAtFrame;               /* the FF/RR sample to seek at */
    int m_seekFrame;                     /* the exact position to seek too, -1 for none */

    IAE::StreamPtr m_stream; /* the playback stream */
    float m_volume;                      /* the initial volume level to set the stream to on creation */

    bool m_isSlaved;                     /* true if the stream has been slaved to another */
    bool m_waitOnDrain;                  /* wait for stream being drained in AE */
  };

  typedef std::list<StreamInfo*> StreamList;

  bool m_signalSpeedChange = false; /* true if OnPlaybackSpeedChange needs to be called */
  bool m_signalStarted = true;
  std::atomic_int m_playbackSpeed;           /* the playback speed (1 = normal) */
  bool m_isPlaying = false;
  bool m_isPaused = false;
  bool m_isFinished = false; /* if there are no more songs in the queue */
  bool m_fullScreen;
  unsigned int m_defaultCrossfadeMS = 0; /* how long the default crossfade is in ms */
  unsigned int m_upcomingCrossfadeMS = 0; /* how long the upcoming crossfade is in ms */
  CEvent              m_startEvent;          /* event for playback start */
  StreamInfo* m_currentStream = nullptr;
  IAudioCallback*     m_audioCallback;       /* the viz audio callback */

  CCriticalSection    m_streamsLock;         /* lock for the stream list */
  StreamList          m_streams;             /* playing streams */
  StreamList          m_finishing;           /* finishing streams */
  int m_jobCounter = 0;
  CEvent              m_jobEvent;
  int64_t m_newForcedPlayerTime = -1;
  int64_t m_newForcedTotalTime = -1;
  std::unique_ptr<CProcessInfo> m_processInfo;

  bool QueueNextFileEx(const CFileItem &file, bool fadeIn);
  void SoftStart(bool wait = false);
  void SoftStop(bool wait = false, bool close = true);
  void CloseAllStreams(bool fade = true);
  void ProcessStreams(double &freeBufferTime);
  bool PrepareStream(StreamInfo *si);
  bool ProcessStream(StreamInfo *si, double &freeBufferTime);
  bool QueueData(StreamInfo *si);
  int64_t GetTotalTime64();
  void UpdateCrossfadeTime(const CFileItem& file);
  void UpdateStreamInfoPlayNextAtFrame(StreamInfo *si, unsigned int crossFadingTime);
  void UpdateGUIData(StreamInfo *si);
  int64_t GetTimeInternal();
  bool SetTimeInternal(int64_t time);
  bool SetTotalTimeInternal(int64_t time);
  void CloseFileCB(StreamInfo &si);
  void AdvancePlaylistOnError(CFileItem &fileItem);
};

