/*
*  Copyright (C) 2005-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DVDCodecs/Video/DVDVideoCodec.h"
#include "DVDMessageQueue.h"
#include "DVDStreamInfo.h"
#include "IVideoPlayer.h"
#include "threads/Thread.h"
#include "utils/BitstreamStats.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

namespace ActiveAE
{
class CActiveAEBufferPool;
class CActiveAEBufferPoolResample;
} // namespace ActiveAE

class CAEEncoderFFmpeg;
class CBitstreamConverter;
class CRenderManager;
class CDVDOverlayContainer;
class CDVDAudioCodec;
class StarfishMediaAPIs;

/**
 * @class CMediaPipelineWebOS
 * @brief WebOS media pipeline for audio/video playback.
 */
class CMediaPipelineWebOS final : public CThread
{
public:
  /**
 * @brief Construct the WebOS media pipeline.
 * @param processInfo Reference to process information.
 * @param renderManager Reference to the render manager.
 * @param clock Reference to the clock for timing.
 * @param parent Parent message queue for control messages.
 * @param overlay Overlay container for subtitle rendering.
 * @param hasAudio True if audio stream is present, false otherwise.
 */
  explicit CMediaPipelineWebOS(CProcessInfo& processInfo,
                               CRenderManager& renderManager,
                               CDVDClock& clock,
                               CDVDMessageQueue& parent,
                               CDVDOverlayContainer& overlay,
                               bool hasAudio);

  /**
 * @brief Destructor, cleans up and unloads streams.
 */
  ~CMediaPipelineWebOS() override;

  /**
 * @brief Check if a codec is supported by the pipeline.
 * @param codec AVCodecID to check.
 * @return True if supported, false otherwise.
 */
  static bool Supports(AVCodecID codec);

  /**
 * @brief Flush all pending video messages.
 */
  void FlushVideoMessages();

  /**
 * @brief Flush all pending audio messages.
 */
  void FlushAudioMessages();

  /**
 * @brief Open an audio stream using provided hints.
 * @param audioHint Audio stream information.
 * @return True on success.
 */
  bool OpenAudioStream(CDVDStreamInfo& audioHint);

  /**
 * @brief Open a video stream using provided hints.
 * @param hint Video stream information.
 * @return True on success.
 */
  bool OpenVideoStream(const CDVDStreamInfo& hint);

  /**
 * @brief Close the audio stream.
 * @param waitForBuffers If true, wait until buffers are processed.
 */
  void CloseAudioStream(bool waitForBuffers);

  /**
 * @brief Close the video stream.
 * @param waitForBuffers If true, wait until buffers are processed.
 */
  void CloseVideoStream(bool waitForBuffers);

  /**
 * @brief Flush both audio and video pipelines.
 * @param sync If true, flush synchronously.
 */
  void Flush(bool sync);

  /**
 * @brief Check if pipeline can accept more audio data.
 * @return True if accepting audio data.
 */
  bool AcceptsAudioData() const;

  /**
 * @brief Check if pipeline can accept more video data.
 * @return True if accepting video data.
 */
  bool AcceptsVideoData() const;

  /**
 * @brief Check if there is buffered audio data.
 * @return True if audio data buffered.
 */
  bool HasAudioData() const;

  /**
 * @brief Check if there is buffered video data.
 * @return True if video data buffered.
 */
  bool HasVideoData() const;

  /**
 * @brief Check if audio subsystem is initialized.
 * @return True if initialized.
 */
  bool IsAudioInited() const;

  /**
 * @brief Check if video subsystem is initialized.
 * @return True if initialized.
 */
  bool IsVideoInited() const;

  /**
 * @brief Get current audio buffer level.
 * @return Buffer level percentage.
 */
  int GetAudioLevel() const;

  /**
 * @brief Check if playback is stalled.
 * @return True if stalled.
 */
  bool IsStalled() const;

  /**
 * @brief Send a message to the audio queue.
 * @param msg Message to send.
 * @param priority Message priority.
 */
  void SendAudioMessage(const std::shared_ptr<CDVDMsg>& msg, int priority);

  /**
 * @brief Send a message to the video queue.
 * @param msg Message to send.
 * @param priority Message priority.
 */
  void SendVideoMessage(const std::shared_ptr<CDVDMsg>& msg, int priority);

  /**
 * @brief Set playback speed.
 * @param speed Playback rate (per-mille units).
 */
  void SetSpeed(int speed);

  /**
 * @brief Get current presentation timestamp in seconds.
 * @return Current PTS (seconds).
 */
  double GetCurrentPts() const;

  /**
 * @brief Get number of audio channels configured.
 * @return Channel count.
 */
  int GetAudioChannels() const { return m_audioHint.channels; }

  /**
 * @brief Enable or disable subtitle rendering.
 * @param enable True to enable subtitles.
 */
  void EnableSubtitle(bool enable);

  /**
 * @brief Check if subtitles are enabled.
 * @return True if enabled.
 */
  bool IsSubtitleEnabled() const;

  /**
 * @brief Get current subtitle display delay.
 * @return Delay in seconds.
 */
  double GetSubtitleDelay() const;

  /**
 * @brief Set subtitle display delay.
 * @param delay Delay in seconds.
 */
  void SetSubtitleDelay(double delay);

  /**
 * @return Video bitrate in bits per second.
 */
  int GetVideoBitrate() const;

  /**
 * @return Audio stream debug info
 */
  std::string GetAudioInfo() const;

  /**
 *
 * @return Video stream debug info
 */
  std::string GetVideoInfo() const;

protected:
  /**
 * @brief Video processing thread loop.
 *
 * Continuously reads messages and feeds video data to the pipeline.
 */
  void Process() override;

  /**
 * @brief Audio processing thread loop.
 *
 * Continuously reads messages and feeds audio data to the pipeline.
 */
  void ProcessAudio();

private:
  /**
 * @brief Send HDR metadata to media pipeline.
 * @param hint Stream information containing HDR details.
 */
  void SetHDR(const CDVDStreamInfo& hint) const;

  /**
 * @brief Feed a video packet to the media API.
 * @param msg Demux packet wrapped in a CDVDMsg.
 */
  void FeedVideoData(const std::shared_ptr<CDVDMsg>& msg);

  /**
 * @brief Render subtitle and overlay graphics at given timestamp.
 * @param pts Presentation timestamp in seconds.
 */
  void ProcessOverlays(double pts) const;

  /**
 * @brief Feed an audio packet to the media API.
 * @param msg Demux packet wrapped in a CDVDMsg.
 */
  void FeedAudioData(const std::shared_ptr<CDVDMsg>& msg);

  /**
 * @brief Configure and load media streams into the pipeline.
 * @param videoHint Video stream information.
 * @param audioHint Audio stream information.
 * @return True if loading succeeded.
 */
  bool Load(CDVDStreamInfo videoHint, CDVDStreamInfo audioHint);

  /**
 * @brief Callback for media events.
 * @param type Event type identifier.
 * @param numValue Numeric associated value.
 * @param strValue String associated value.
 */
  void PlayerCallback(int32_t type, int64_t numValue, const char* strValue);

  /**
 * @brief Static trampoline for PlayerCallback.
 * @param type Event type identifier.
 * @param numValue Numeric associated value.
 * @param strValue String associated value.
 * @param data Pointer to CMediaPipelineWebOS instance.
 */
  static void PlayerCallback(int32_t type, int64_t numValue, const char* strValue, void* data);

  /**
 * @brief ACB app-switching callback.
 * @param acbId ACB context ID.
 * @param taskId Task identifier.
 * @param eventType Event category.
 * @param appState Application state value.
 * @param playState Playback state value.
 * @param reply Debug or info string.
 */
  static void AcbCallback(
      long acbId, long taskId, long eventType, long appState, long playState, const char* reply);

  void UpdateVideoInfo();
  void UpdateAudioInfo();

  std::condition_variable m_eventCondition;
  std::mutex m_eventMutex;

  unsigned int m_webOSVersion{4};
  std::atomic<bool> m_stalled{false};
  std::atomic<bool> m_loaded{false};
  std::atomic<bool> m_flushed{false};
  std::atomic<bool> m_subtitle{false};
  std::atomic<double> m_subtitleDelay{0.0};
  std::atomic<bool> m_needsTranscode{false};
  std::atomic<std::chrono::nanoseconds> m_pts{std::chrono::nanoseconds(0)};
  VideoPicture m_picture{};
  std::unique_ptr<StarfishMediaAPIs> m_mediaAPIs;
  CDVDStreamInfo m_audioHint;
  CDVDStreamInfo m_videoHint;
  std::unique_ptr<CBitstreamConverter> m_bitstream{nullptr};
  std::unique_ptr<CDVDAudioCodec> m_audioCodec{nullptr};
  std::unique_ptr<ActiveAE::CActiveAEBufferPool> m_encoderBuffers{nullptr};
  std::unique_ptr<ActiveAE::CActiveAEBufferPoolResample> m_audioResample{nullptr};
  std::unique_ptr<CAEEncoderFFmpeg> m_audioEncoder{nullptr};
  std::atomic<int> m_bufferLevel{0};
  std::atomic<bool> m_audioFull{false};
  std::atomic<bool> m_videoFull{false};
  std::atomic<unsigned long> m_droppedFrames{0};

  std::mutex m_audioCriticalSection;

  CDVDMessageQueue m_messageQueueAudio;
  CDVDMessageQueue m_messageQueueVideo;
  CDVDMessageQueue& m_messageQueueParent;
  CProcessInfo& m_processInfo;
  CRenderManager& m_renderManager;
  CDVDClock& m_clock;
  CDVDOverlayContainer& m_overlayContainer;
  bool m_hasAudio{true};

  std::string m_audioInfo;
  std::string m_videoInfo;
  BitstreamStats m_audioStats{};
  BitstreamStats m_videoStats{};

  std::thread m_audioThread;
};

/**
 * @class CVideoPlayerVideoWebOS
 * @brief Video stream player adapter forwarding to CMediaPipelineWebOS.
 */
class CVideoPlayerVideoWebOS final : public IDVDStreamPlayerVideo
{
public:
  CVideoPlayerVideoWebOS(CMediaPipelineWebOS& mediaPipeline, CProcessInfo& processInfo)
    : IDVDStreamPlayerVideo(processInfo), m_mediaPipeline(mediaPipeline)
  {
  }
  void FlushMessages() override { m_mediaPipeline.FlushVideoMessages(); }
  bool OpenStream(const CDVDStreamInfo hint) override
  {
    return m_mediaPipeline.OpenVideoStream(hint);
  }
  void CloseStream(const bool waitForBuffers) override
  {
    m_mediaPipeline.CloseVideoStream(waitForBuffers);
  }
  void Flush(const bool sync) override { m_mediaPipeline.Flush(sync); }
  [[nodiscard]] bool AcceptsData() const override { return m_mediaPipeline.AcceptsVideoData(); }
  [[nodiscard]] bool HasData() const override { return m_mediaPipeline.HasVideoData(); }
  [[nodiscard]] bool IsInited() const override { return m_mediaPipeline.IsVideoInited(); }
  void SendMessage(const std::shared_ptr<CDVDMsg> msg, const int priority) override
  {
    m_mediaPipeline.SendVideoMessage(msg, priority);
  }
  void EnableSubtitle(const bool enable) override { m_mediaPipeline.EnableSubtitle(enable); }
  bool IsSubtitleEnabled() override { return m_mediaPipeline.IsSubtitleEnabled(); }
  double GetSubtitleDelay() override { return m_mediaPipeline.GetSubtitleDelay(); }
  void SetSubtitleDelay(const double delay) override { m_mediaPipeline.SetSubtitleDelay(delay); }
  [[nodiscard]] bool IsStalled() const override { return m_mediaPipeline.IsStalled(); }
  double GetCurrentPts() override { return m_mediaPipeline.GetCurrentPts(); }
  double GetOutputDelay() override { return 0.0; }
  std::string GetPlayerInfo() override { return m_mediaPipeline.GetVideoInfo(); }
  int GetVideoBitrate() override { return m_mediaPipeline.GetVideoBitrate(); }
  void SetSpeed(const int speed) override { m_mediaPipeline.SetSpeed(speed); }

private:
  CMediaPipelineWebOS& m_mediaPipeline;
};

/**
 * @class CVideoPlayerAudioWebOS
 * @brief Audio stream player adapter forwarding to CMediaPipelineWebOS.
 */
class CVideoPlayerAudioWebOS final : public IDVDStreamPlayerAudio
{
public:
  CVideoPlayerAudioWebOS(CMediaPipelineWebOS& mediaPipeline, CProcessInfo& processInfo)
    : IDVDStreamPlayerAudio(processInfo), m_mediaPipeline(mediaPipeline)
  {
  }
  void FlushMessages() override { m_mediaPipeline.FlushAudioMessages(); };
  bool OpenStream(CDVDStreamInfo hints) override { return m_mediaPipeline.OpenAudioStream(hints); }
  void CloseStream(const bool waitForBuffers) override
  {
    m_mediaPipeline.CloseAudioStream(waitForBuffers);
  }
  void SetSpeed(const int speed) override { m_mediaPipeline.SetSpeed(speed); }
  void Flush(const bool sync) override { m_mediaPipeline.Flush(sync); }
  [[nodiscard]] bool AcceptsData() const override { return m_mediaPipeline.AcceptsAudioData(); }
  [[nodiscard]] bool HasData() const override { return m_mediaPipeline.HasAudioData(); }
  [[nodiscard]] int GetLevel() const override { return m_mediaPipeline.GetAudioLevel(); }
  [[nodiscard]] bool IsInited() const override { return m_mediaPipeline.IsAudioInited(); }
  void SendMessage(const std::shared_ptr<CDVDMsg> msg, const int priority) override
  {
    m_mediaPipeline.SendAudioMessage(msg, priority);
  }
  void SetDynamicRangeCompression(long drc) override {}
  std::string GetPlayerInfo() override { return m_mediaPipeline.GetAudioInfo(); }
  int GetAudioChannels() override { return m_mediaPipeline.GetAudioChannels(); }
  double GetCurrentPts() override { return m_mediaPipeline.GetCurrentPts(); }
  [[nodiscard]] bool IsStalled() const override { return m_mediaPipeline.IsStalled(); }
  [[nodiscard]] bool IsPassthrough() const override { return true; }
  [[nodiscard]] float GetDynamicRangeAmplification() const override { return 0.0f; }

private:
  CMediaPipelineWebOS& m_mediaPipeline;
};
