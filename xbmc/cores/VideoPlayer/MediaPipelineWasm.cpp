/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "MediaPipelineWasm.h"

#include "Buffers/VideoBufferWasmMedia.h"
#include "DVDClock.h"
#include "DVDCodecs/Audio/DVDAudioCodecFFmpeg.h"
#include "DVDCodecs/DVDCodecs.h"
#include "DVDOverlayContainer.h"
#include "Interface/DemuxPacket.h"
#include "Interface/TimingConstants.h"
#include "Process/ProcessInfo.h"
#include "ServiceBroker.h"
#include "VideoRenderers/RenderManager.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationVolumeHandling.h"
#include "cores/AudioEngine/Utils/AEUtil.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/log.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

using namespace std::chrono_literals;

namespace
{
constexpr int DECODED_STREAM_ID = -3000;

// How far the position is carried forward from the backend's last report.
constexpr double MAX_EXTRAPOLATION = 0.5;
constexpr double DEFAULT_FPS = 25.0;
constexpr double MIN_RATE = 0.5;
constexpr double MAX_RATE = 2.0;

double ToSeconds(double dvdTime)
{
  return dvdTime / DVD_TIME_BASE;
}
} // namespace

CMediaPipelineWasm::CMediaPipelineWasm(std::unique_ptr<IMediaBackendWasm> backend,
                                       CProcessInfo& processInfo,
                                       CRenderManager& renderManager,
                                       CDVDClock& clock,
                                       CDVDMessageQueue& parent,
                                       CDVDOverlayContainer& overlay,
                                       bool hasAudio,
                                       std::function<int()> streamLength)
  : CThread("MediaPipelineWasm"),
    m_backend(std::move(backend)),
    m_videoBuffer(std::make_unique<CVideoBufferWasmMedia>()),
    m_hasAudio(hasAudio),
    m_messageQueueVideo("video"),
    m_messageQueueAudio("audio"),
    m_messageQueueParent(parent),
    m_processInfo(processInfo),
    m_renderManager(renderManager),
    m_clock(clock),
    m_overlayContainer(overlay),
    m_streamLength(std::move(streamLength))
{
  m_messageQueueVideo.Init();
  m_messageQueueAudio.Init();
  const auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  const int tenthsSeconds = settings->GetInt(CSettings::SETTING_VIDEOPLAYER_QUEUETIMESIZE);
  m_messageQueueVideo.SetMaxTimeSize(tenthsSeconds / 10.0);
  m_messageQueueAudio.SetMaxTimeSize(tenthsSeconds / 10.0);
  // as in CVideoPlayerAudio: 18 Mbit/s, the TrueHD peak, during the queue time
  m_messageQueueAudio.SetMaxDataSize(18 * (tenthsSeconds / 10.0) / 8 * 1024 * 1024);
  m_messageQueueVideo.SetMaxDataSize(
      settings->GetInt(CSettings::SETTING_VIDEOPLAYER_QUEUEDATASIZE) * 1024 * 1024);

  m_picture.Reset();
  m_picture.videoBuffer = m_videoBuffer.get();
  m_processInfo.GetVideoBufferManager().ReleasePools();
}

CMediaPipelineWasm::~CMediaPipelineWasm()
{
  Unload();
}

void CMediaPipelineWasm::FlushVideoMessages()
{
  m_processInfo.SetLevelVQ(0);
  m_messageQueueVideo.Flush();
}

void CMediaPipelineWasm::FlushAudioMessages()
{
  m_messageQueueAudio.Flush();
}

bool CMediaPipelineWasm::OpenVideoStream(const CDVDStreamInfo& hint)
{
  if (hint.cryptoSession || !m_backend->SupportsVideo(hint))
  {
    CLog::LogF(LOGERROR, "Unsupported video stream: codec {} profile {}", hint.codec, hint.profile);
    return false;
  }

  // Tracks are fixed once the source is open.
  if (m_loaded)
    Unload();

  m_videoHint = hint;
  if (!m_hasAudio)
    m_audioHint = CDVDStreamInfo();
  m_videoClosed = false;
  if (m_audioHint.codec || !m_hasAudio || m_audioFailed)
    return Load();
  return true;
}

bool CMediaPipelineWasm::OpenAudioStream(const CDVDStreamInfo& hint)
{
  const IMediaBackendWasm::AudioPath path =
      hint.cryptoSession ? IMediaBackendWasm::AudioPath::NONE : m_backend->GetAudioPath(hint);
  if (path == IMediaBackendWasm::AudioPath::NONE)
  {
    CLog::LogF(LOGINFO, "Audio stream not playable by {}: codec {} profile {}", m_backend->Name(),
               hint.codec, hint.profile);
    // VideoPlayer tries the next audio stream; play without audio if none fits.
    m_audioFailed = true;
    if (m_videoHint.codec && !m_loaded)
      Load();
    return false;
  }

  if (m_loaded)
    Unload();

  m_audioHint = hint;
  m_audioPath = path;
  m_hasAudio = true;
  m_audioClosed = false;
  if (!m_videoHint.codec || Load())
    return true;

  // The backend can still refuse the audio track; VideoPlayer will not close a stream that
  // failed to open.
  CLog::LogF(LOGWARNING, "{} could not play the audio stream; playing without it",
             m_backend->Name());
  m_audioFailed = true;
  m_audioHint = CDVDStreamInfo();
  m_audioPath = IMediaBackendWasm::AudioPath::NONE;
  m_audioClosed = true;
  Load();
  return false;
}

void CMediaPipelineWasm::CloseVideoStream()
{
  m_videoClosed = true;
  if (m_audioClosed)
  {
    Unload();
    m_videoHint = CDVDStreamInfo();
    m_audioHint = CDVDStreamInfo();
    m_audioFailed = false;
  }
}

void CMediaPipelineWasm::CloseAudioStream()
{
  m_audioClosed = true;
  if (m_videoClosed)
  {
    Unload();
    m_videoHint = CDVDStreamInfo();
    m_audioHint = CDVDStreamInfo();
    m_audioFailed = false;
  }
}

bool CMediaPipelineWasm::Load()
{
  const bool withAudio = m_audioHint.codec != AV_CODEC_ID_NONE;
  const bool converted = withAudio && m_audioPath == IMediaBackendWasm::AudioPath::CONVERTED;
  if (converted)
  {
    m_audioCodec = std::make_unique<CDVDAudioCodecFFmpeg>(m_processInfo);
    CDVDStreamInfo hint = m_audioHint;
    CDVDCodecOptions options;
    if (!m_audioCodec->Open(hint, options))
    {
      CLog::LogF(LOGERROR, "Could not open an audio decoder for codec {}", m_audioHint.codec);
      m_audioCodec.reset();
      return false;
    }
  }

  // Past the last packet, so that none is refused; a stream of unknown length plays as live.
  const int lengthMs = m_streamLength ? m_streamLength() : 0;
  const double duration =
      lengthMs > 0 ? lengthMs / 1000.0 + 1.0 : std::numeric_limits<double>::infinity();
  const std::string tracks =
      m_backend->Open(m_videoHint, withAudio ? &m_audioHint : nullptr, m_audioPath, duration);
  if (tracks.empty())
  {
    Unload();
    return false;
  }

  const double fps = m_videoHint.fpsrate > 0 && m_videoHint.fpsscale > 0
                         ? static_cast<double>(m_videoHint.fpsrate) / m_videoHint.fpsscale
                         : 0.0;
  if (fps > 0.0)
    m_clock.UpdateFramerate(fps);
  m_picture.iDuration = DVD_TIME_BASE / (fps > 0.0 ? fps : DEFAULT_FPS);
  m_picture.iWidth = m_videoHint.width;
  m_picture.iHeight = m_videoHint.height;
  m_picture.iDisplayWidth = m_videoHint.width;
  m_picture.iDisplayHeight = m_videoHint.height;
  if (m_videoHint.aspect > 0.0)
  {
    m_picture.iDisplayWidth = std::lround(m_picture.iDisplayHeight * m_videoHint.aspect);
    if (m_picture.iDisplayWidth > m_picture.iWidth)
    {
      m_picture.iDisplayWidth = m_picture.iWidth;
      m_picture.iDisplayHeight = std::lround(m_picture.iDisplayWidth / m_videoHint.aspect);
    }
  }
  m_picture.stereoMode = m_videoHint.stereo_mode;
  m_picture.hdrType = m_videoHint.hdrType;
  // Backends show the base layer of Dolby Vision.
  if (m_picture.hdrType == StreamHdrType::HDR_TYPE_DOLBYVISION)
  {
    if (m_videoHint.colorTransferCharacteristic == AVCOL_TRC_SMPTE2084)
      m_picture.hdrType = StreamHdrType::HDR_TYPE_HDR10;
    else if (m_videoHint.colorTransferCharacteristic == AVCOL_TRC_ARIB_STD_B67)
      m_picture.hdrType = StreamHdrType::HDR_TYPE_HLG;
    else
      m_picture.hdrType = StreamHdrType::HDR_TYPE_NONE;
  }
  m_picture.color_transfer = m_videoHint.colorTransferCharacteristic;

  const int settingsOrientation = m_processInfo.GetVideoSettings().m_Orientation;
  const int orientation = settingsOrientation != 0
                              ? (settingsOrientation + m_videoHint.orientation) % 360
                              : m_videoHint.orientation;
  if (!m_renderManager.Configure(m_picture, static_cast<float>(fps), orientation))
  {
    CLog::LogF(LOGERROR, "RenderManager configure failed");
    Unload();
    return false;
  }

  m_processInfo.SetVideoDecoderName(m_backend->VideoCodecName(),
                                    m_backend->IsHardwareVideoDecoder());
  m_processInfo.SetVideoPixelFormat("Surface");
  m_processInfo.SetVideoDimensions(m_videoHint.width, m_videoHint.height);
  m_processInfo.SetVideoInterlaced(m_videoHint.interlaced);
  m_processInfo.SetVideoDeintMethod("hardware");
  m_processInfo.SetVideoDAR(static_cast<float>(m_videoHint.aspect));
  m_processInfo.SetVideoFps(static_cast<float>(fps));
  if (withAudio)
  {
    m_processInfo.SetAudioDecoderName(
        converted ? fmt::format("{} to {}", m_audioCodec->GetName(), m_backend->AudioCodecName())
                  : m_backend->AudioCodecName());
    m_processInfo.SetAudioChannels(m_audioHint.channellayout
                                       ? CAEUtil::GetAEChannelLayout(m_audioHint.channellayout)
                                       : CAEUtil::GuessChLayout(m_audioHint.channels));
    m_processInfo.SetAudioSampleRate(m_audioHint.samplerate);
    m_processInfo.SetAudioBitsPerSample(m_audioHint.bitspersample);
  }

  CLog::LogF(LOGINFO, "Playing {} {}x{} through {} for {} ms", tracks, m_videoHint.width,
             m_videoHint.height, m_backend->Name(), lengthMs);

  m_flushed = true;
  m_positioned = false;
  m_needKeyFrame = true;
  m_lastVideoDts = -1.0;
  m_lastAudioDts = -1.0;
  m_fedVideoPts = -1.0;
  m_fedAudioPts = -1.0;
  m_drain = false;
  m_audioEnded = false;
  m_audioFinished = false;
  m_appliedSpeed = DVD_PLAYSPEED_PAUSE;
  m_playing = false;
  m_errorReported = false;
  m_volume = -1.0f;
  m_refusedVideo = false;
  m_refusedAudio = false;
  m_reportedPosition = 0.0;
  m_reportedAt = std::chrono::steady_clock::now();
  m_picturePosition = -1.0;
  m_maxAudioAhead = m_backend->MaxAudioAhead();
  m_videoStats.Start();
  m_audioStats.Start();
  m_loaded = true;

  Create();
  if (withAudio)
    m_audioThread = std::thread([this] { ProcessAudio(); });
  m_renderManager.ShowVideo(true);
  return true;
}

void CMediaPipelineWasm::Unload()
{
  StopThread(true);
  if (m_audioThread.joinable())
    m_audioThread.join();

  m_backend->Close();
  m_loaded = false;
  m_audioCodec.reset();
}

void CMediaPipelineWasm::Flush()
{
  std::scoped_lock lock(m_videoMutex, m_audioMutex);
  ++m_flushes;
  FlushVideoMessages();
  FlushAudioMessages();
  m_backend->Flush();
  m_flushed = true;
  m_needKeyFrame = true;
  m_lastVideoDts = -1.0;
  m_lastAudioDts = -1.0;
  m_fedVideoPts = -1.0;
  m_fedAudioPts = -1.0;
  m_drain = false;
  m_audioEnded = false;
  m_audioFinished = false;
  if (m_audioCodec)
    m_audioCodec->Reset();
}

bool CMediaPipelineWasm::AcceptsVideoData() const
{
  return !m_messageQueueVideo.IsFull();
}

bool CMediaPipelineWasm::AcceptsAudioData() const
{
  return !m_messageQueueAudio.IsFull();
}

bool CMediaPipelineWasm::HasVideoData() const
{
  if (!m_loaded)
    return false;
  if (m_messageQueueVideo.GetDataSize() > 0)
    return true;
  return !m_backend->Ended() && m_fedVideoPts > m_position;
}

bool CMediaPipelineWasm::HasAudioData() const
{
  if (!m_loaded || !m_audioHint.codec)
    return false;
  if (m_messageQueueAudio.GetDataSize() > 0)
    return true;
  return !m_backend->Ended() && m_fedAudioPts > m_position;
}

bool CMediaPipelineWasm::IsVideoInited() const
{
  return m_messageQueueVideo.IsInited();
}

bool CMediaPipelineWasm::IsAudioInited() const
{
  return m_messageQueueAudio.IsInited();
}

int CMediaPipelineWasm::GetAudioLevel() const
{
  return m_audioHint.codec ? std::min(99, m_messageQueueAudio.GetLevel()) : 0;
}

void CMediaPipelineWasm::SendVideoMessage(const std::shared_ptr<CDVDMsg>& msg, int priority)
{
  m_messageQueueVideo.Put(msg, priority);
  m_processInfo.SetLevelVQ(std::min(99, m_messageQueueVideo.GetLevel()));
}

void CMediaPipelineWasm::SendAudioMessage(const std::shared_ptr<CDVDMsg>& msg, int priority)
{
  m_messageQueueAudio.Put(msg, priority);
}

void CMediaPipelineWasm::SetSpeed(int speed)
{
  m_speed = speed;
}

double CMediaPipelineWasm::GetCurrentPts() const
{
  return m_position * DVD_TIME_BASE;
}

int CMediaPipelineWasm::GetVideoBitrate() const
{
  return static_cast<int>(m_videoStats.GetBitrate());
}

std::string CMediaPipelineWasm::GetVideoInfo() const
{
  double fps = 0.0;
  if (m_videoHint.fpsrate != 0 && m_videoHint.fpsscale != 0)
    fps = static_cast<double>(m_videoHint.fpsrate) / static_cast<double>(m_videoHint.fpsscale);

  return fmt::format("vq:{:02}% {:.3f}s, {:.3f}Mb, Mb/s:{:.2f}, fr:{:.3f}, ahead:{:.2f}s",
                     m_processInfo.GetLevelVQ(), m_messageQueueVideo.GetTimeSize(),
                     m_messageQueueVideo.GetDataSize() / 1024.0 / 1024.0,
                     GetVideoBitrate() / (1024.0 * 1024.0), fps,
                     std::max(0.0, m_fedVideoPts - m_position));
}

std::string CMediaPipelineWasm::GetAudioInfo() const
{
  return fmt::format("aq:{:02}% {:.3f}s {:.3f}Kb, Kb/s:{:.2f}, ahead:{:.2f}s", GetAudioLevel(),
                     m_messageQueueAudio.GetTimeSize(), m_messageQueueAudio.GetDataSize() / 1024.0,
                     m_audioStats.GetBitrate() / 1024.0, std::max(0.0, m_fedAudioPts - m_position));
}

void CMediaPipelineWasm::GetVideoResolution(unsigned int& width, unsigned int& height) const
{
  if (m_videoHint.codec)
  {
    width = m_videoHint.width;
    height = m_videoHint.height;
  }
}

void CMediaPipelineWasm::UpdatePlayback()
{
  const int error = m_backend->Error();
  if (error != 0 && !m_errorReported)
  {
    CLog::LogF(LOGERROR, "{} stopped playback, error {}", m_backend->Name(), error);
    m_errorReported = true;
    m_messageQueueParent.Put(std::make_shared<CDVDMsg>(CDVDMsg::PLAYER_ABORT));
  }

  // The backend starts once it has buffered enough.
  const int speed = m_speed;
  if (speed != m_appliedSpeed && (speed == DVD_PLAYSPEED_PAUSE || m_backend->CanPlay()))
  {
    // The backend plays tempo changes; VideoPlayer seeks through fast forward and rewind.
    const double rate = speed / static_cast<double>(DVD_PLAYSPEED_NORMAL);
    const bool play = rate >= MIN_RATE && rate <= MAX_RATE;
    if (play)
    {
      m_backend->SetRate(rate);
      if (!m_playing)
        m_backend->Play();
    }
    else if (m_playing)
      m_backend->Pause();
    m_playing = play;
    m_appliedSpeed = speed;
  }

  const auto volumeHandling =
      CServiceBroker::GetAppComponents().GetComponent<CApplicationVolumeHandling>();
  const float volume = volumeHandling->IsMuted() ? 0.0f : volumeHandling->GetVolumeRatio();
  if (volume != m_volume)
  {
    m_backend->SetVolume(volume);
    m_volume = volume;
  }

  const auto now = std::chrono::steady_clock::now();
  const double reported = m_backend->Position();
  if (reported != m_reportedPosition)
  {
    m_reportedPosition = reported;
    m_reportedAt = now;
  }
  double position = m_reportedPosition;
  if (m_playing)
  {
    const double elapsed = std::chrono::duration<double>(now - m_reportedAt).count();
    position += std::min(MAX_EXTRAPOLATION, elapsed * m_appliedSpeed / DVD_PLAYSPEED_NORMAL);
  }
  m_position = position;
  // VideoPlayer shows its clock as the play time; it must stop when the backend stalls.
  m_clock.Discontinuity(position * DVD_TIME_BASE);

  // One picture per frame keeps RenderManager drawing the video area and the subtitles.
  if (std::abs(position - m_picturePosition) * DVD_TIME_BASE < m_picture.iDuration)
    return;
  m_picturePosition = position;
  const double pts = position * DVD_TIME_BASE;
  ProcessOverlays(pts);
  m_picture.pts = pts;
  m_picture.dts = pts;
  std::atomic_bool stop(false);
  m_renderManager.AddVideoPicture(m_picture, stop, VS_INTERLACEMETHOD_NONE, false);
}

void CMediaPipelineWasm::ProcessOverlays(double pts) const
{
  m_overlayContainer.CleanUp(pts - m_subtitleDelay);

  std::vector<std::shared_ptr<CDVDOverlay>> overlays;
  std::scoped_lock<CCriticalSection> lock(m_overlayContainer);
  for (const std::shared_ptr<CDVDOverlay>& overlay : *m_overlayContainer.GetOverlays())
  {
    if (!overlay->bForced && !m_subtitle)
      continue;

    const double overlayPts = overlay->bForced ? pts : pts - m_subtitleDelay;
    if (overlay->iPTSStartTime <= overlayPts &&
        (overlay->iPTSStopTime > overlayPts || overlay->iPTSStopTime == 0LL))
    {
      if (overlay->IsOverlayType(DVDOVERLAY_TYPE_GROUP))
        overlays.insert(overlays.end(), static_cast<CDVDOverlayGroup&>(*overlay).m_overlays.begin(),
                        static_cast<CDVDOverlayGroup&>(*overlay).m_overlays.end());
      else
        overlays.push_back(overlay);
    }
  }

  for (const std::shared_ptr<CDVDOverlay>& overlay : overlays)
    m_renderManager.AddOverlay(overlay, overlay->bForced ? pts : pts - m_subtitleDelay);
}

bool CMediaPipelineWasm::FeedVideoData(DemuxPacket& packet)
{
  double pts = packet.pts;
  if (m_videoHint.ptsinvalid || pts == DVD_NOPTS_VALUE)
    pts = packet.dts;
  if (pts == DVD_NOPTS_VALUE || pts < 0.0 || !packet.pData || packet.iSize <= 0)
    return true;

  IMediaBackendWasm::Packet video;
  if (!m_backend->PrepareVideo(packet, video))
    return true;
  if (m_needKeyFrame && !video.keyFrame)
    return true;

  const double ptsSeconds = ToSeconds(pts);
  if (m_flushed)
  {
    // A fresh source plays from 0 already.
    if (m_positioned || ptsSeconds > 1.0)
      m_backend->Seek(ptsSeconds);
    else
      m_backend->SetStartPosition(ptsSeconds);
    m_positioned = true;
    m_startPosition = ptsSeconds;
    m_position = ptsSeconds;
    m_flushed = false;

    SStartMsg start{.timestamp = pts,
                    .player = VideoPlayer_VIDEO,
                    .cachetime = DVD_MSEC_TO_TIME(50),
                    .cachetotal = DVD_MSEC_TO_TIME(100)};
    m_messageQueueParent.Put(
        std::make_shared<CDVDMsgType<SStartMsg>>(CDVDMsg::PLAYER_STARTED, start));
    start.player = VideoPlayer_AUDIO;
    m_messageQueueParent.Put(
        std::make_shared<CDVDMsgType<SStartMsg>>(CDVDMsg::PLAYER_STARTED, start));
  }

  if (m_fedVideoPts >= 0.0 && m_fedVideoPts - m_position > WASM_MEDIA::MAX_FEED_AHEAD)
    return false;

  // Decode times have to rise.
  double dts =
      packet.dts != DVD_NOPTS_VALUE && !m_videoHint.ptsinvalid ? ToSeconds(packet.dts) : ptsSeconds;
  if (m_lastVideoDts >= 0.0 && dts <= m_lastVideoDts)
    dts = m_lastVideoDts + 1e-6;
  video.pts = ptsSeconds;
  video.dts = dts;
  video.duration =
      packet.duration > 0.0 ? ToSeconds(packet.duration) : m_picture.iDuration / DVD_TIME_BASE;

  switch (m_backend->AppendVideo(video))
  {
    case IMediaBackendWasm::AppendResult::OK:
      m_needKeyFrame = false;
      m_lastVideoDts = dts;
      m_fedVideoPts = ptsSeconds;
      m_videoStats.AddSampleBytes(packet.iSize);
      return true;
    case IMediaBackendWasm::AppendResult::FULL:
      return false;
    case IMediaBackendWasm::AppendResult::STALE:
    case IMediaBackendWasm::AppendResult::NEEDS_KEY_FRAME:
      // The backend started over on its own.
      m_needKeyFrame = true;
      return true;
    default:
      if (!m_refusedVideo)
        CLog::LogF(LOGERROR, "{} refused video at {:.3f}s", m_backend->Name(), ptsSeconds);
      m_refusedVideo = true;
      return true;
  }
}

bool CMediaPipelineWasm::FeedAudioData(const DemuxPacket& packet)
{
  if (packet.pts == DVD_NOPTS_VALUE || packet.pts < 0.0 || !packet.pData || packet.iSize <= 0)
    return true;

  // The video thread places the backend first.
  if (m_flushed)
    return false;

  IMediaBackendWasm::Packet audio;
  audio.data = packet.pData;
  audio.size = static_cast<size_t>(packet.iSize);
  audio.pts = ToSeconds(packet.pts);
  audio.duration = packet.duration > 0.0 ? ToSeconds(packet.duration) : 0.0;
  audio.keyFrame = true;
  return AppendAudio(audio);
}

bool CMediaPipelineWasm::FeedConvertedAudio(DemuxPacket& packet)
{
  if (m_flushed)
    return false;

  // A packet offered again was decoded already; its frames go first.
  if (packet.iStreamId != DECODED_STREAM_ID)
  {
    if (!m_audioCodec->AddData(packet))
      return true;
    packet.iStreamId = DECODED_STREAM_ID;
  }

  while (true)
  {
    IMediaBackendWasm::Packet converted;
    if (m_backend->PeekConvertedAudio(converted))
    {
      if (!AppendAudio(converted))
        return false;
      m_backend->PopConvertedAudio();
      continue;
    }
    DVDAudioFrame frame{};
    m_audioCodec->GetData(frame);
    if (frame.nb_frames == 0)
      return true;
    m_backend->ConvertAudio(frame);
  }
}

bool CMediaPipelineWasm::AppendAudio(const IMediaBackendWasm::Packet& packet)
{
  if (packet.pts + packet.duration < m_startPosition)
    return true;
  if (m_fedAudioPts >= 0.0 && packet.pts + packet.duration - m_position > m_maxAudioAhead)
    return false;

  IMediaBackendWasm::Packet audio = packet;
  audio.dts = packet.pts;
  if (m_lastAudioDts >= 0.0 && audio.dts <= m_lastAudioDts)
    audio.dts = m_lastAudioDts + 1e-6;

  switch (m_backend->AppendAudio(audio))
  {
    case IMediaBackendWasm::AppendResult::OK:
      m_lastAudioDts = audio.dts;
      m_fedAudioPts = audio.pts;
      m_audioStats.AddSampleBytes(static_cast<int>(audio.size));
      return true;
    case IMediaBackendWasm::AppendResult::FULL:
      return false;
    case IMediaBackendWasm::AppendResult::STALE:
      return true;
    default:
      if (!m_refusedAudio)
        CLog::LogF(LOGERROR, "{} refused audio at {:.3f}s", m_backend->Name(), audio.pts);
      m_refusedAudio = true;
      return true;
  }
}

void CMediaPipelineWasm::DrainAudio()
{
  if (m_audioPath == IMediaBackendWasm::AudioPath::CONVERTED)
  {
    if (!m_audioFinished)
    {
      m_backend->FinishConvertedAudio();
      m_audioFinished = true;
    }
    IMediaBackendWasm::Packet converted;
    while (m_backend->PeekConvertedAudio(converted))
    {
      if (!AppendAudio(converted))
        return;
      m_backend->PopConvertedAudio();
    }
  }
  m_backend->EndAudio();
  m_audioEnded = true;
}

void CMediaPipelineWasm::Process()
{
  while (!m_bStop)
  {
    UpdatePlayback();

    const unsigned int flushes = m_flushes;
    std::shared_ptr<CDVDMsg> msg;
    int priority = 0;
    m_messageQueueVideo.Get(msg, 10ms, priority);
    if (!msg)
      continue;

    if (msg->IsType(CDVDMsg::DEMUXER_PACKET))
    {
      std::unique_lock lock(m_videoMutex);
      if (flushes != m_flushes)
        continue;
      if (!FeedVideoData(*std::static_pointer_cast<CDVDMsgDemuxerPacket>(msg)->GetPacket()))
      {
        m_messageQueueVideo.PutBack(msg);
        lock.unlock();
        std::this_thread::sleep_for(10ms);
      }
    }
    else if (msg->IsType(CDVDMsg::VIDEO_DRAIN))
    {
      std::unique_lock lock(m_videoMutex);
      m_backend->EndVideo();
      m_drain = true;
    }
    else if (msg->IsType(CDVDMsg::PLAYER_REQUEST_STATE))
    {
      constexpr SStateMsg state{.syncState = IDVDStreamPlayer::SYNC_INSYNC,
                                .player = VideoPlayer_VIDEO};
      m_messageQueueParent.Put(
          std::make_shared<CDVDMsgType<SStateMsg>>(CDVDMsg::PLAYER_REPORT_STATE, state));
    }
    else if (msg->IsType(CDVDMsg::GENERAL_SYNCHRONIZE))
    {
      if (!std::static_pointer_cast<CDVDMsgGeneralSynchronize>(msg)->Wait(100ms, SYNCSOURCE_VIDEO))
        m_messageQueueVideo.Put(msg, 1);
    }
  }
}

void CMediaPipelineWasm::ProcessAudio()
{
  while (!m_bStop)
  {
    const unsigned int flushes = m_flushes;
    std::shared_ptr<CDVDMsg> msg;
    int priority = 0;
    m_messageQueueAudio.Get(msg, 10ms, priority);
    if (!msg)
    {
      // VideoPlayer only tells the video stream that the demuxer is done.
      std::unique_lock lock(m_audioMutex);
      if (m_drain && !m_audioEnded && m_messageQueueAudio.GetDataSize() == 0)
        DrainAudio();
      continue;
    }

    if (msg->IsType(CDVDMsg::DEMUXER_PACKET))
    {
      std::unique_lock lock(m_audioMutex);
      if (flushes != m_flushes)
        continue;
      DemuxPacket& packet = *std::static_pointer_cast<CDVDMsgDemuxerPacket>(msg)->GetPacket();
      const bool converted = m_audioPath == IMediaBackendWasm::AudioPath::CONVERTED;
      if (!(converted ? FeedConvertedAudio(packet) : FeedAudioData(packet)))
      {
        m_messageQueueAudio.PutBack(msg);
        lock.unlock();
        std::this_thread::sleep_for(10ms);
      }
    }
    else if (msg->IsType(CDVDMsg::PLAYER_REQUEST_STATE))
    {
      constexpr SStateMsg state{.syncState = IDVDStreamPlayer::SYNC_INSYNC,
                                .player = VideoPlayer_AUDIO};
      m_messageQueueParent.Put(
          std::make_shared<CDVDMsgType<SStateMsg>>(CDVDMsg::PLAYER_REPORT_STATE, state));
    }
    else if (msg->IsType(CDVDMsg::GENERAL_SYNCHRONIZE))
    {
      if (!std::static_pointer_cast<CDVDMsgGeneralSynchronize>(msg)->Wait(100ms, SYNCSOURCE_AUDIO))
        m_messageQueueAudio.Put(msg, 1);
    }
  }
}
