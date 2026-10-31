/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DVDCodecs/Audio/DVDAudioCodec.h"
#include "DVDCodecs/Video/DVDVideoCodec.h"
#include "DVDMessageQueue.h"
#include "DVDStreamInfo.h"
#include "IVideoPlayer.h"
#include "Interface/TimingConstants.h"
#include "MediaBackendWasm.h"
#include "threads/Thread.h"
#include "utils/BitstreamStats.h"

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

class CDVDClock;
class CDVDOverlayContainer;
class CProcessInfo;
class CRenderManager;
class CVideoBufferWasmMedia;
struct DemuxPacket;

/*!
 * \brief Plays audio and video through a media player of the browser or the device.
 *
 * The backend decodes both streams, keeps them in sync and shows the video in a <video>
 * element under the GUI. This thread appends video packets and drives playback,
 * m_audioThread appends audio; VideoPlayer's clock and RenderManager follow the
 * backend's position.
 */
class CMediaPipelineWasm final : public CThread
{
public:
  /*!
   * \param streamLength Length of the stream in milliseconds, 0 when unknown.
   */
  CMediaPipelineWasm(std::unique_ptr<IMediaBackendWasm> backend,
                     CProcessInfo& processInfo,
                     CRenderManager& renderManager,
                     CDVDClock& clock,
                     CDVDMessageQueue& parent,
                     CDVDOverlayContainer& overlay,
                     bool hasAudio,
                     std::function<int()> streamLength);
  ~CMediaPipelineWasm() override;

  IMediaBackendWasm& Backend() const { return *m_backend; }
  void SetHasAudio(bool hasAudio) { m_hasAudio = hasAudio; }
  void FlushVideoMessages();
  void FlushAudioMessages();
  bool OpenVideoStream(const CDVDStreamInfo& hint);
  bool OpenAudioStream(const CDVDStreamInfo& hint);
  void CloseVideoStream();
  void CloseAudioStream();
  void Flush();
  bool AcceptsVideoData() const;
  bool AcceptsAudioData() const;
  bool HasVideoData() const;
  bool HasAudioData() const;
  bool IsVideoInited() const;
  bool IsAudioInited() const;
  int GetAudioLevel() const;
  //! The backend buffers on its own.
  bool IsStalled() const { return false; }
  void SendVideoMessage(const std::shared_ptr<CDVDMsg>& msg, int priority);
  void SendAudioMessage(const std::shared_ptr<CDVDMsg>& msg, int priority);
  void SetSpeed(int speed);
  double GetCurrentPts() const;
  int GetAudioChannels() const { return m_audioHint.channels; }
  void EnableSubtitle(bool enable) { m_subtitle = enable; }
  bool IsSubtitleEnabled() const { return m_subtitle; }
  double GetSubtitleDelay() const { return m_subtitleDelay; }
  void SetSubtitleDelay(double delay) { m_subtitleDelay = delay; }
  int GetVideoBitrate() const;
  std::string GetVideoInfo() const;
  std::string GetAudioInfo() const;
  void GetVideoResolution(unsigned int& width, unsigned int& height) const;

protected:
  void Process() override;

private:
  bool Load();
  void Unload();
  void ProcessAudio();

  //! Starts playback, applies speed and volume changes and reports the backend's position
  //! to VideoPlayer.
  void UpdatePlayback();
  void ProcessOverlays(double pts) const;

  //! \return false when the packet has to be offered again later.
  bool FeedVideoData(DemuxPacket& packet);
  bool FeedAudioData(const DemuxPacket& packet);
  bool FeedConvertedAudio(DemuxPacket& packet);
  bool AppendAudio(const IMediaBackendWasm::Packet& packet);
  //! Appends the converted audio that is ready, and ends the audio track once it is all in.
  void DrainAudio();

  std::unique_ptr<IMediaBackendWasm> m_backend;

  CDVDStreamInfo m_videoHint;
  CDVDStreamInfo m_audioHint;
  IMediaBackendWasm::AudioPath m_audioPath{IMediaBackendWasm::AudioPath::NONE};
  std::unique_ptr<CVideoBufferWasmMedia> m_videoBuffer;
  VideoPicture m_picture{};

  std::atomic<bool> m_loaded{false};
  std::atomic<bool> m_hasAudio{true};
  std::atomic<bool> m_audioFailed{false};
  std::atomic<bool> m_videoClosed{true};
  std::atomic<bool> m_audioClosed{true};

  // Feeding: m_flushed until the first video packet after a load or flush has placed the
  // backend, which the audio thread waits for. m_flushes drops packets taken out of a
  // queue before a flush.
  std::mutex m_videoMutex;
  std::mutex m_audioMutex;
  std::atomic<bool> m_flushed{true};
  bool m_positioned{false};
  std::atomic<unsigned int> m_flushes{0};
  std::atomic<double> m_startPosition{0.0};
  bool m_needKeyFrame{true};
  double m_lastVideoDts{-1.0};
  std::atomic<double> m_fedVideoPts{-1.0};
  std::atomic<double> m_fedAudioPts{-1.0};
  std::atomic<bool> m_drain{false};
  bool m_audioEnded{false};
  bool m_audioFinished{false};
  double m_lastAudioDts{-1.0};
  double m_maxAudioAhead{0.0};

  std::unique_ptr<CDVDAudioCodec> m_audioCodec;
  bool m_refusedVideo{false};
  bool m_refusedAudio{false};

  // Playback, applied by this thread.
  std::atomic<int> m_speed{DVD_PLAYSPEED_PAUSE};
  int m_appliedSpeed{DVD_PLAYSPEED_PAUSE};
  bool m_playing{false};
  bool m_errorReported{false};
  float m_volume{-1.0f};
  double m_reportedPosition{0.0};
  std::chrono::steady_clock::time_point m_reportedAt;
  double m_picturePosition{-1.0};
  std::atomic<double> m_position{0.0};

  std::atomic<bool> m_subtitle{false};
  std::atomic<double> m_subtitleDelay{0.0};
  BitstreamStats m_videoStats;
  BitstreamStats m_audioStats;

  CDVDMessageQueue m_messageQueueVideo;
  CDVDMessageQueue m_messageQueueAudio;
  CDVDMessageQueue& m_messageQueueParent;
  CProcessInfo& m_processInfo;
  CRenderManager& m_renderManager;
  CDVDClock& m_clock;
  CDVDOverlayContainer& m_overlayContainer;
  std::function<int()> m_streamLength;

  std::thread m_audioThread;
};
