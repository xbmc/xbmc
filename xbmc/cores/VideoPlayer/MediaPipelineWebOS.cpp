/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "MediaPipelineWebOS.h"

#include "Buffers/VideoBufferStarfish.h"
#include "CompileInfo.h"
#include "DVDCodecs/Audio/DVDAudioCodecFFmpeg.h"
#include "DVDCodecs/Audio/DVDAudioCodecPassthrough.h"
#include "DVDCodecs/DVDCodecs.h"
#include "DVDCodecs/DVDFactoryCodec.h"
#include "DVDDemuxers/DVDDemuxUtils.h"
#include "DVDOverlayContainer.h"
#include "Interface/TimingConstants.h"
#include "Process/ProcessInfo.h"
#include "VideoRenderers/RenderManager.h"
#include "cores/AudioEngine/Encoders/AEEncoderFFmpeg.h"
#include "cores/AudioEngine/Engines/ActiveAE/ActiveAEBuffer.h"
#include "cores/AudioEngine/Interfaces/AE.h"
#include "cores/AudioEngine/Utils/AEUtil.h"
#include "cores/VideoPlayer/Interface/DemuxCrypto.h"
#include "settings/SettingUtils.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "utils/Base64.h"
#include "utils/BitstreamConverter.h"
#include "utils/JSONVariantParser.h"
#include "utils/JSONVariantWriter.h"
#include "utils/SystemInfo.h"
#include "utils/log.h"
#include "windowing/wayland/WinSystemWaylandWebOS.h"

#include "platform/linux/WebOSTVPlatformConfig.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <exception>
#include <map>
#include <ratio>
#include <string_view>
#include <utility>
#include <vector>

#include <appswitching-control-block/AcbAPI.h>
#include <glib-object.h>
#include <player-factory/custompipeline.hpp>
#include <player-factory/customplayer.hpp>
#include <starfish-media-pipeline/StarfishMediaAPIs.h>

extern "C"
{
#include <libavcodec/defs.h>
#include <libavutil/opt.h>
}

using namespace std::chrono_literals;

namespace
{
constexpr unsigned int AC3_MAX_SYNC_FRAME_SIZE = 3840;
constexpr int RESAMPLED_STREAM_ID = -1000;
constexpr unsigned int MIN_AUDIO_RESAMPLE_BUFFER_SIZE = 4096;

constexpr unsigned int PRE_BUFFER_BYTES = 0;
constexpr unsigned int MAX_QUEUE_BUFFER_LEVEL = 0; // no additional queue
constexpr unsigned int MIN_BUFFER_LEVEL = 0;
constexpr unsigned int MAX_BUFFER_LEVEL = 0;
constexpr unsigned int MIN_SRC_BUFFER_LEVEL_AUDIO = 1 * 1024 * 1024; // 1 MB
constexpr unsigned int MIN_SRC_BUFFER_LEVEL_VIDEO = 1 * 1024 * 1024; // 1 MB
constexpr unsigned int MAX_SRC_BUFFER_LEVEL_AUDIO = 2 * 1024 * 1024; // 2 MB
constexpr unsigned int MAX_SRC_BUFFER_LEVEL_VIDEO = 8 * 1024 * 1024; // 8 MB

constexpr unsigned int SVP_VERSION_30 = 30;
constexpr unsigned int SVP_VERSION_40 = 40;

auto ms_codecMap = std::map<AVCodecID, std::string_view>({{AV_CODEC_ID_VP8, "VP8"},
                                                          {AV_CODEC_ID_VP9, "VP9"},
                                                          {AV_CODEC_ID_AVS, "H264"},
                                                          {AV_CODEC_ID_CAVS, "H264"},
                                                          {AV_CODEC_ID_H264, "H264"},
                                                          {AV_CODEC_ID_HEVC, "H265"},
                                                          {AV_CODEC_ID_AV1, "AV1"},
                                                          {AV_CODEC_ID_AC3, "AC3"},
                                                          {AV_CODEC_ID_EAC3, "AC3 PLUS"},
                                                          {AV_CODEC_ID_AC4, "AC4"},
                                                          {AV_CODEC_ID_OPUS, "OPUS"},
                                                          {AV_CODEC_ID_MP3, "MP3"},
                                                          {AV_CODEC_ID_AAC, "AAC"},
                                                          {AV_CODEC_ID_AAC_LATM, "AAC"}});

const auto ms_hdrInfoMap = std::map<AVColorTransferCharacteristic, std::string_view>({
    {AVCOL_TRC_SMPTE2084, "HDR10"},
    {AVCOL_TRC_ARIB_STD_B67, "HLG"},
});

unsigned int SelectTranscodingSampleRate(const unsigned int sampleRate)
{
  switch (sampleRate)
  {
    case 11025:
    case 22050:
    case 44100:
    case 88200:
    case 176400:
    case 352800:
      return 44100;
    case 32000:
      return 32000;
    default:
      return 48000;
  }
}

unsigned int ParseAACSampleRate(const uint8_t* data, const size_t size)
{
  if (size < 2)
    return 0;

  // see ff_mpeg4audio_sample_rates
  static const unsigned int sampleRates[16] = {96000, 88200, 64000, 48000, 44100, 32000,
                                               24000, 22050, 16000, 12000, 11025, 8000,
                                               7350,  0,     0,     0};

  constexpr unsigned int AAC_AOT_AAC_LC = 2;
  constexpr unsigned int AAC_AOT_SBR = 5;
  constexpr unsigned int AAC_AOT_PS = 29;
  constexpr size_t MIN_SIZE_FOR_SBR = 5;
  constexpr unsigned int MAX_SAMPLE_INDEX = 15;

  // Read the Audio Object Type (AOT)
  const unsigned int aot = (data[0] >> 3) & 0x1F;

  // Read the sampling_frequency_index
  const unsigned int srIndex = ((data[0] & 0x07) << 1) | (data[1] >> 7);
  unsigned int sampleRate = (srIndex <= MAX_SAMPLE_INDEX) ? sampleRates[srIndex] : 0;

  // HE-AAC / HE-AACv2 (PS) check
  if ((aot == AAC_AOT_AAC_LC || aot == AAC_AOT_SBR || aot == AAC_AOT_PS) &&
      size >= MIN_SIZE_FOR_SBR)
  {
    // Check if SBR or PS extension is present
    const unsigned int extAot = (data[2] >> 3) & 0x1F;
    if (extAot == AAC_AOT_SBR || extAot == AAC_AOT_PS)
    {
      // Read the extension sampling frequency index
      const unsigned int extSrIndex = ((data[2] & 0x07) << 1) | (data[3] >> 7);
      if (extSrIndex <= MAX_SAMPLE_INDEX)
        sampleRate = sampleRates[extSrIndex]; // effective HE-AAC / HE-AACv2 rate
    }
  }

  return sampleRate;
}
} // namespace

CMediaPipelineWebOS::CMediaPipelineWebOS(CProcessInfo& processInfo,
                                         CRenderManager& renderManager,
                                         CDVDClock& clock,
                                         CDVDMessageQueue& parent,
                                         CDVDOverlayContainer& overlay,
                                         const bool hasAudio)
  : CThread("MediaPipelineWebOS"),
    m_mediaAPIs(std::make_unique<StarfishMediaAPIs>()),
    m_messageQueueAudio("audio"),
    m_messageQueueVideo("video"),
    m_messageQueueParent(parent),
    m_processInfo(processInfo),
    m_renderManager(renderManager),
    m_clock(clock),
    m_overlayContainer(overlay),
    m_hasAudio(hasAudio)
{
  m_messageQueueAudio.Init();
  m_messageQueueVideo.Init();
  m_messageQueueAudio.SetMaxTimeSize(1.0);
  m_messageQueueVideo.SetMaxTimeSize(1.0);
  m_messageQueueAudio.SetMaxDataSize(MAX_SRC_BUFFER_LEVEL_AUDIO);
  m_messageQueueVideo.SetMaxDataSize(MAX_SRC_BUFFER_LEVEL_VIDEO);

  m_picture.Reset();
  m_picture.videoBuffer = new CStarfishVideoBuffer();

  m_webOSVersion = WebOSTVPlatformConfig::GetWebOSVersion();
  if (WebOSTVPlatformConfig::SupportsDTS())
    ms_codecMap.emplace(AV_CODEC_ID_DTS, "DTS");
  m_processInfo.GetVideoBufferManager().ReleasePools();
}

CMediaPipelineWebOS::~CMediaPipelineWebOS()
{
  Unload(false);
}

int CMediaPipelineWebOS::GetVideoBitrate() const
{
  return static_cast<int>(m_videoStats.GetBitrate());
}

void CMediaPipelineWebOS::UpdateAudioInfo()
{
  const unsigned int level = GetQueueLevel(StreamType::AUDIO);
  const double kb = m_messageQueueAudio.GetDataSize() / 1024.0;
  const double ts = m_messageQueueAudio.GetTimeSize();
  const double kbps = m_audioStats.GetBitrate() / 1024.0;

  std::scoped_lock lock(m_audioInfoMutex);
  m_audioInfo = fmt::format("aq:{:02}% {:.3f}s {:.3f}Kb, Kb/s:{:.2f}{}", level, ts, kb, kbps,
                            m_audioEncoder ? ", transcoded ac3" : "");
}

void CMediaPipelineWebOS::UpdateVideoInfo()
{
  const int level = m_processInfo.GetLevelVQ();
  const double ts = m_messageQueueVideo.GetTimeSize();
  const double mb = m_messageQueueVideo.GetDataSize() / 1024.0 / 1024.0;
  const double mbps = static_cast<double>(GetVideoBitrate()) / (1024.0 * 1024.0);
  double fps = 0.0;

  if (m_videoHint.fpsrate && m_videoHint.fpsscale)
    fps = static_cast<double>(m_videoHint.fpsrate) / static_cast<double>(m_videoHint.fpsscale);

  std::scoped_lock lock(m_videoInfoMutex);
  m_videoInfo = fmt::format("vq:{:02}% {:.3f}s, {:.3f}Mb, Mb/s:{:.2f}, fr:{:.3f}, drop:{}", level,
                            ts, mb, mbps, fps, m_droppedFrames.load());
}

void CMediaPipelineWebOS::UpdateGUISounds(const bool playing)
{
  IAE* activeAE = CServiceBroker::GetActiveAE();
  const int guiSoundMode = CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(
      CSettings::SETTING_AUDIOOUTPUT_GUISOUNDMODE);

  if (guiSoundMode != AE_SOUND_IDLE)
    return;

  if (playing)
    activeAE->SetVolume(0.0);
  else
    activeAE->SetVolume(1.0);
}

std::string CMediaPipelineWebOS::GetAudioInfo()
{
  std::scoped_lock lock(m_audioInfoMutex);
  return m_audioInfo;
}

std::string CMediaPipelineWebOS::GetVideoInfo()
{
  std::scoped_lock lock(m_videoInfoMutex);
  return m_videoInfo;
}

bool CMediaPipelineWebOS::Supports(const AVCodecID codec, const int profile)
{
  if ((codec == AV_CODEC_ID_H264 || codec == AV_CODEC_ID_AVS || codec == AV_CODEC_ID_CAVS) &&
      profile == AV_PROFILE_H264_HIGH_10)
    return false;
  return ms_codecMap.contains(codec);
}

void CMediaPipelineWebOS::AcbCallback(
    long acbId, long taskId, long eventType, long appState, long playState, const char* reply)
{
  CLog::LogF(LOGDEBUG, "acbId={}, taskId={}, eventType={}, appState={}, playState={}, reply={}",
             acbId, taskId, eventType, appState, playState, reply);
}

void CMediaPipelineWebOS::FlushVideoMessages()
{
  m_processInfo.SetLevelVQ(0);
  m_messageQueueVideo.Flush();
}

void CMediaPipelineWebOS::FlushAudioMessages()
{
  m_messageQueueAudio.Flush();
}

bool CMediaPipelineWebOS::OpenAudioStream(CDVDStreamInfo& audioHint)
{
  m_audioHint = audioHint;

  if (m_loaded)
  {
    if (m_webOSVersion >= 6)
    {
      std::scoped_lock lock(m_audioCriticalSection);
      CVariant optInfo = CVariant::VariantTypeObject;
      const std::string codecName = SetupAudio(audioHint, optInfo);

      std::string output;
      CJSONVariantWriter::Write(optInfo, output, true);
      CLog::LogF(LOGDEBUG, "changeAudioCodec: {}", output);
      if (!m_mediaAPIs->changeAudioCodec(codecName, output))
        CLog::LogF(LOGERROR, "Failed to change audio codec to {}", codecName);
      FlushAudioMessages();

      m_processInfo.SetAudioChannels(CAEUtil::GetAEChannelLayout(audioHint.channellayout));
      m_processInfo.SetAudioSampleRate(audioHint.samplerate);
      m_processInfo.SetAudioBitsPerSample(audioHint.bitspersample);
      if (Supports(audioHint.codec, audioHint.profile))
        m_processInfo.SetAudioDecoderName("starfish-" +
                                          std::string(ms_codecMap.at(audioHint.codec).data()));
      else if (m_audioEncoder)
        m_processInfo.SetAudioDecoderName("starfish-AC3 (transcoding)");

      return true;
    }
    // API introduced in webOS 6.0, so we need to handle older versions differently
    Unload(true);

    m_mediaAPIs = std::make_unique<StarfishMediaAPIs>();
  }

  if (m_audioHint.codec && m_videoHint.codec)
    return Load(m_videoHint, m_audioHint);
  return true;
}

bool CMediaPipelineWebOS::OpenVideoStream(CDVDStreamInfo hint)
{
  if (!Supports(hint.codec, hint.profile))
  {
    CLog::LogF(LOGERROR, "Unsupported codec: {}", hint.codec);
    return false;
  }

  if (m_loaded)
  {
    if (m_videoHint.codec == hint.codec && m_videoHint.hdrType == hint.hdrType)
    {
      std::scoped_lock lock(m_videoCriticalSection);
      SetupBitstreamConverter(hint);
      m_videoHint = hint;

      m_processInfo.SetVideoInterlaced(hint.interlaced);
      m_processInfo.SetVideoDimensions(hint.width, hint.height);
      m_processInfo.SetVideoDAR(static_cast<float>(hint.aspect));
      if (m_videoHint.fpsrate && m_videoHint.fpsscale)
      {
        m_processInfo.SetVideoFps(static_cast<float>(hint.fpsrate) /
                                  static_cast<float>(hint.fpsscale));
      }
      else
      {
        m_processInfo.SetVideoFps(1.0);
      }

      return true;
    }

    // Different codec => unload the current stream
    Unload(true);

    m_mediaAPIs = std::make_unique<StarfishMediaAPIs>();
  }

  m_videoHint = hint;

  if ((m_audioHint.codec || !m_hasAudio) && m_videoHint.codec)
    return Load(m_videoHint, m_audioHint);
  return true;
}

void CMediaPipelineWebOS::CloseAudioStream(bool waitForBuffers)
{
}

void CMediaPipelineWebOS::CloseVideoStream(bool waitForBuffers)
{
}

void CMediaPipelineWebOS::Flush(bool sync)
{
  if (!m_mediaAPIs->flush())
    CLog::LogF(LOGDEBUG, "Failed to flush media APIs");
  FlushAudioMessages();
  FlushVideoMessages();
  if (m_bitstream)
    m_bitstream->ResetStartDecode();
  m_flushed = true;
}

bool CMediaPipelineWebOS::AcceptsAudioData() const
{
  return !m_messageQueueAudio.IsFull();
}

bool CMediaPipelineWebOS::AcceptsVideoData() const
{
  return !m_messageQueueVideo.IsFull();
}

bool CMediaPipelineWebOS::HasAudioData() const
{
  if (!m_pipeline)
    return false;

  return GetQueuedBytes(StreamType::AUDIO) > 0;
}

bool CMediaPipelineWebOS::HasVideoData() const
{
  if (!m_pipeline)
    return false;

  return GetQueuedBytes(StreamType::VIDEO) > 0;
}

bool CMediaPipelineWebOS::IsAudioInited() const
{
  return m_messageQueueAudio.IsInited();
}

bool CMediaPipelineWebOS::IsVideoInited() const
{
  return m_messageQueueVideo.IsInited();
}

int CMediaPipelineWebOS::GetAudioLevel() const
{
  if (!m_pipeline)
    return 0;

  const unsigned int level = GetQueueLevel(StreamType::AUDIO);
  return std::min(99, static_cast<int>(level));
}

bool CMediaPipelineWebOS::IsStalled() const
{
  return m_stalled;
}

void CMediaPipelineWebOS::SendAudioMessage(const std::shared_ptr<CDVDMsg>& msg, const int priority)
{
  m_messageQueueAudio.Put(msg, priority);
}

void CMediaPipelineWebOS::SendVideoMessage(const std::shared_ptr<CDVDMsg>& msg, const int priority)
{
  m_messageQueueVideo.Put(msg, priority);
  if (m_pipeline)
  {
    const unsigned int level = GetQueueLevel(StreamType::VIDEO);
    m_processInfo.SetLevelVQ(static_cast<int>(level));
  }
}

void CMediaPipelineWebOS::SetSpeed(const int speed)
{
  if (!m_loaded)
    return;

  if (speed == DVD_PLAYSPEED_PAUSE)
  {
    if (!m_mediaAPIs->Pause())
      CLog::LogF(LOGERROR, "Pause failed");
    return;
  }
  if (speed == DVD_PLAYSPEED_NORMAL)
  {
    if (!m_mediaAPIs->Play())
      CLog::LogF(LOGERROR, "Play failed");
  }

  CVariant payload;
  payload["audioOutput"] = std::abs(speed) <= 2000;
  payload["playRate"] = speed / 1000.0;
  std::string output;
  CJSONVariantWriter::Write(payload, output, true);

  if (!m_mediaAPIs->SetPlayRate(output.c_str()))
    CLog::LogF(LOGERROR, "SetPlayRate failed");
}

double CMediaPipelineWebOS::GetCurrentPts() const
{
  using dvdTime = std::ratio<1, DVD_TIME_BASE>;
  return std::chrono::duration_cast<std::chrono::duration<double, dvdTime>>(m_pts.load()).count();
}

void CMediaPipelineWebOS::EnableSubtitle(const bool enable)
{
  m_subtitle = enable;
}

bool CMediaPipelineWebOS::IsSubtitleEnabled() const
{
  return m_subtitle;
}

double CMediaPipelineWebOS::GetSubtitleDelay() const
{
  return m_subtitleDelay;
}

void CMediaPipelineWebOS::SetSubtitleDelay(const double delay)
{
  m_subtitleDelay = delay;
}

bool CMediaPipelineWebOS::Load(CDVDStreamInfo videoHint, CDVDStreamInfo audioHint)
{
  std::scoped_lock videoLock(m_videoCriticalSection);
  std::scoped_lock audioLock(m_audioCriticalSection);

  CVariant p;

  if (videoHint.cryptoSession || audioHint.cryptoSession)
  {
    const CryptoSessionSystem keySystem = videoHint.cryptoSession
                                              ? videoHint.cryptoSession->keySystem
                                              : audioHint.cryptoSession->keySystem;

    const int svpVersion = m_webOSVersion > 4 ? SVP_VERSION_40 : SVP_VERSION_30;

    switch (keySystem)
    {
      case CRYPTO_SESSION_SYSTEM_NONE:
        break;
      case CRYPTO_SESSION_SYSTEM_CLEARKEY:
        break;
      case CRYPTO_SESSION_SYSTEM_WIDEVINE:
        CLog::Log(LOGDEBUG, "Setting drm type to WIDEVINE_MODULAR with svpVersion {}", svpVersion);
        p["option"]["externalStreamingInfo"]["svpVersion"] = svpVersion;
        p["option"]["drm"]["type"] = "WIDEVINE_MODULAR";
        break;
      case CRYPTO_SESSION_SYSTEM_PLAYREADY:
        CLog::Log(LOGDEBUG, "Setting drm type to PLAYREADY with svpVersion {}", svpVersion);
        p["option"]["externalStreamingInfo"]["svpVersion"] = svpVersion;
        p["option"]["drm"]["type"] = "PLAYREADY";
        break;
      default:
        if (audioHint.cryptoSession && !videoHint.cryptoSession)
          CLog::LogF(LOGERROR, "CryptoSession (audio) unsupported: {}",
                     audioHint.cryptoSession->keySystem);
        else if (videoHint.cryptoSession)
          CLog::LogF(LOGERROR, "CryptoSession (video) unsupported: {}",
                     videoHint.cryptoSession->keySystem);
        return false;
    }
  }

  if (!videoHint.width || !videoHint.height)
  {
    CLog::LogF(LOGERROR, "{}", "null size, cannot handle");
    return false;
  }

  CLog::LogF(LOGDEBUG,
             "hints: Width {} x Height {}, Fpsrate {} / Fpsscale {}, "
             "CodecID {}, Level {}, Profile {}, PTS_invalid {}, Tag {}, Extradata-Size: {}",
             videoHint.width, videoHint.height, videoHint.fpsrate, videoHint.fpsscale,
             videoHint.codec, videoHint.level, videoHint.profile, videoHint.ptsinvalid,
             videoHint.codec_tag, videoHint.extradata.GetSize());

  if (!ms_codecMap.contains(videoHint.codec))
  {
    CLog::LogF(LOGDEBUG, "Unsupported video hints.codec({})", videoHint.codec);
    return false;
  }

  SetupBitstreamConverter(videoHint);
  CVariant& contents = p["option"]["externalStreamingInfo"]["contents"];
  if (videoHint.hdrType == StreamHdrType::HDR_TYPE_DOLBYVISION)
  {
    contents["DolbyHdrInfo"]["encryptionType"] = videoHint.cryptoSession ? "all" : "clear";
    contents["DolbyHdrInfo"]["profileId"] = videoHint.dovi.dv_profile;
    contents["DolbyHdrInfo"]["trackType"] = videoHint.dovi.el_present_flag ? "dual" : "single";
  }

  using namespace KODI::WINDOWING::WAYLAND;
  const auto winSystem = dynamic_cast<CWinSystemWaylandWebOS*>(CServiceBroker::GetWinSystem());
  if (winSystem->SupportsExportedWindow())
  {
    p["option"]["windowId"] = winSystem->GetExportedWindowName();
  }
  else
  {
    auto buffer = static_cast<CStarfishVideoBuffer*>(m_picture.videoBuffer);
    const std::unique_ptr<AcbHandle>& acb = buffer->CreateAcbHandle();
    if (acb->Id())
    {
      if (!AcbAPI_initialize(acb->Id(), PLAYER_TYPE_MSE, getenv("APPID"), &AcbCallback))
      {
        buffer->ResetAcbHandle();
      }
    }
  }

  p["option"]["appId"] = CCompileInfo::GetPackage();
  contents["codec"]["video"] = ms_codecMap.at(videoHint.codec).data();

  if (audioHint.codec == AV_CODEC_ID_NONE)
  {
    m_audioCodec = nullptr;
    m_audioEncoder = nullptr;
    m_audioResample = nullptr;
    m_encoderBuffers = nullptr;
    p["option"]["needAudio"] = false;
  }
  else
  {
    contents["codec"]["audio"] = SetupAudio(audioHint, contents);
  }

  if (audioHint.codec == AV_CODEC_ID_EAC3 && audioHint.profile == AV_PROFILE_EAC3_DDP_ATMOS)
    contents["immersive"] = "ATMOS";

  contents["format"] = "RAW";
  p["mediaTransportType"] = "BUFFERSTREAM";
  contents["provider"] = CCompileInfo::GetPackage();
  p["option"]["transmission"]["contentsType"] = "LIVE";
  p["option"]["transmission"]["trickType"] = "client-side";
  p["option"]["seekMode"] = "keep-rate";
  p["option"]["useDroppedFrameEvent"] = true;
  p["option"]["externalStreamingInfo"]["streamQualityInfo"] = true;
  p["option"]["externalStreamingInfo"]["streamQualityInfoNonFlushable"] = true;
  p["option"]["externalStreamingInfo"]["streamQualityInfoCorruptedFrame"] = true;

  CVariant& esInfo = contents["esInfo"];
  esInfo["pauseAtDecodeTime"] = true;
  esInfo["seperatedPTS"] = true;
  esInfo["ptsToDecode"] = m_pts.load().count();
  esInfo["videoWidth"] = videoHint.width;
  esInfo["videoHeight"] = videoHint.height;
  if (videoHint.fpsrate && videoHint.fpsscale)
  {
    esInfo["videoFpsValue"] = videoHint.fpsrate;
    esInfo["videoFpsScale"] = videoHint.fpsscale;
  }

  CVariant& bufferingCtrInfo = p["option"]["externalStreamingInfo"]["bufferingCtrInfo"];
  bufferingCtrInfo["preBufferByte"] = PRE_BUFFER_BYTES;
  bufferingCtrInfo["bufferMinLevel"] = MIN_BUFFER_LEVEL;
  bufferingCtrInfo["bufferMaxLevel"] = MAX_BUFFER_LEVEL;
  bufferingCtrInfo["qBufferLevelVideo"] = MAX_QUEUE_BUFFER_LEVEL;
  bufferingCtrInfo["srcBufferLevelVideo"]["minimum"] = MIN_SRC_BUFFER_LEVEL_VIDEO;
  bufferingCtrInfo["srcBufferLevelVideo"]["maximum"] = MAX_SRC_BUFFER_LEVEL_VIDEO;
  bufferingCtrInfo["qBufferLevelAudio"] = MAX_QUEUE_BUFFER_LEVEL;
  bufferingCtrInfo["srcBufferLevelAudio"]["minimum"] = MIN_SRC_BUFFER_LEVEL_AUDIO;
  bufferingCtrInfo["srcBufferLevelAudio"]["maximum"] = MAX_SRC_BUFFER_LEVEL_AUDIO;

  int32_t maxWidth = 0;
  int32_t maxHeight = 0;
  int32_t maxFramerate = 0;
  smp::util::getMaxVideoResolution(ms_codecMap.at(videoHint.codec).data(), &maxWidth, &maxHeight,
                                   &maxFramerate);
  p["option"]["adaptiveStreaming"]["adaptiveResolution"] = true;
  p["option"]["adaptiveStreaming"]["maxWidth"] = maxWidth;
  p["option"]["adaptiveStreaming"]["maxHeight"] = maxHeight;
  p["option"]["adaptiveStreaming"]["maxFrameRate"] = maxFramerate;

  CVariant payloadArgs;
  payloadArgs["args"] = CVariant(CVariant::VariantTypeArray);
  payloadArgs["args"].push_back(std::move(p));

  std::string payload;
  CJSONVariantWriter::Write(payloadArgs, payload, true);

  if (!m_mediaAPIs->notifyForeground())
    CLog::LogF(LOGERROR, "notifyForeground failed");
  CLog::LogFC(LOGDEBUG, LOGVIDEO, "Sending Load payload {}", payload);
  if (!m_mediaAPIs->Load(payload.c_str(), &CMediaPipelineWebOS::PlayerCallback, this))
  {
    CLog::LogF(LOGERROR, "Load failed");
    m_messageQueueParent.Put(std::make_shared<CDVDMsg>(CDVDMsg::PLAYER_ABORT));
    return false;
  }

  SetHDR(videoHint);

  double fps = 0.0;
  if (videoHint.fpsrate && videoHint.fpsscale)
  {
    fps = static_cast<double>(videoHint.fpsrate) / static_cast<double>(videoHint.fpsscale);
    m_clock.UpdateFramerate(fps);
    m_picture.iDuration = 1000.0 / fps;
  }
  m_picture.iWidth = videoHint.width;
  m_picture.iHeight = videoHint.height;
  m_picture.iDisplayWidth = videoHint.width;
  m_picture.iDisplayHeight = videoHint.height;
  m_picture.stereoMode = videoHint.stereo_mode;
  m_picture.hdrType = videoHint.hdrType;
  m_picture.color_transfer = videoHint.colorTransferCharacteristic;

  const int sorient = m_processInfo.GetVideoSettings().m_Orientation;
  const int orientation =
      sorient != 0 ? (sorient + videoHint.orientation) % 360 : videoHint.orientation;

  if (!m_renderManager.Configure(m_picture, static_cast<float>(fps), orientation))
  {
    CLog::LogF(LOGERROR, "RenderManager configure failed");
    m_messageQueueParent.Put(std::make_shared<CDVDMsg>(CDVDMsg::PLAYER_ABORT));
    return false;
  }

  std::unique_lock lock(m_eventMutex);
  if (!m_eventCondition.wait_for(lock, 1s, [this] { return m_loaded == true; }))
  {
    CLog::LogF(LOGERROR, "Pipeline did not load");
    m_messageQueueParent.Put(std::make_shared<CDVDMsg>(CDVDMsg::PLAYER_ABORT));
    return false;
  }

  m_droppedFrames = 0;
  std::string formatName = fmt::format(
      "starfish-{}{}", videoHint.hdrType == StreamHdrType::HDR_TYPE_DOLBYVISION ? "d" : "",
      StringUtils::ToLower(ms_codecMap.at(videoHint.codec)));
  m_processInfo.SetVideoDecoderName(formatName, true);
  m_processInfo.SetVideoPixelFormat("Surface");
  m_processInfo.SetVideoDimensions(videoHint.width, videoHint.height);
  m_processInfo.SetVideoInterlaced(videoHint.interlaced);
  m_processInfo.SetVideoDeintMethod("hardware");
  m_processInfo.SetVideoDAR(static_cast<float>(videoHint.aspect));
  m_processInfo.SetVideoFps(static_cast<float>(fps));
  if (audioHint.codec != AV_CODEC_ID_NONE)
  {
    if (audioHint.channellayout)
      m_processInfo.SetAudioChannels(CAEUtil::GetAEChannelLayout(audioHint.channellayout));
    else
      m_processInfo.SetAudioChannels(CAEUtil::GuessChLayout(audioHint.channels));
    m_processInfo.SetAudioSampleRate(audioHint.samplerate);
    m_processInfo.SetAudioBitsPerSample(audioHint.bitspersample);
    if (Supports(audioHint.codec, audioHint.profile))
      m_processInfo.SetAudioDecoderName(std::string("starfish-") +
                                        ms_codecMap.at(audioHint.codec).data());
    else if (m_audioEncoder)
      m_processInfo.SetAudioDecoderName("starfish-AC3 (transcoding)");
  }

  m_renderManager.ShowVideo(true);
  return true;
}

void CMediaPipelineWebOS::Unload(const bool sync)
{
  CThread::StopThread(true);
  if (m_audioThread.joinable())
    m_audioThread.join();

  if (!m_mediaAPIs->Unload())
    CLog::LogF(LOGERROR, "Unload failed");

  const auto buffer = static_cast<CStarfishVideoBuffer*>(m_picture.videoBuffer);
  buffer->ResetAcbHandle();

  if (sync)
  {
    // wait until m_loaded is false
    std::unique_lock lock(m_eventMutex);
    if (!m_eventCondition.wait_for(lock, 1s, [this] { return !m_loaded; }))
    {
      CLog::LogF(LOGERROR, "Timeout waiting for m_loaded to be false");
    }
  }
}

std::string CMediaPipelineWebOS::SetupAudio(CDVDStreamInfo& audioHint, CVariant& optInfo)
{
  m_audioCodec = nullptr;
  m_audioEncoder = nullptr;
  m_audioResample = nullptr;
  m_encoderBuffers = nullptr;

  std::string codecName = "AC3";
  if (!Supports(audioHint.codec, audioHint.profile))
  {
    m_audioCodec = std::make_unique<CDVDAudioCodecFFmpeg>(m_processInfo);
    CDVDCodecOptions options;
    m_audioCodec->Open(audioHint, options);
    m_audioEncoder = std::make_unique<CAEEncoderFFmpeg>();
    return codecName;
  }

  codecName = ms_codecMap.at(audioHint.codec);
  if (audioHint.codec == AV_CODEC_ID_EAC3)
  {
    optInfo["ac3PlusInfo"]["channels"] = audioHint.channels;
    optInfo["ac3PlusInfo"]["frequency"] = audioHint.samplerate / 1000.0;

    if (audioHint.profile == AV_PROFILE_EAC3_DDP_ATMOS)
    {
      optInfo["ac3PlusInfo"]["channels"] = audioHint.channels + 2;
    }
  }
  if (audioHint.codec == AV_CODEC_ID_AC4)
  {
    optInfo["ac4Info"]["channels"] = audioHint.channels;
    optInfo["ac4Info"]["frequency"] = audioHint.samplerate / 1000.0;
  }
  else if (audioHint.codec == AV_CODEC_ID_DTS)
  {
    optInfo["dtsInfo"]["channels"] = audioHint.channels;
    optInfo["dtsInfo"]["frequency"] = audioHint.samplerate / 1000.0;

    if (audioHint.profile == AV_PROFILE_DTS_ES)
      codecName = "DTSE";
    if (audioHint.profile == AV_PROFILE_DTS_HD_MA_X ||
        audioHint.profile == AV_PROFILE_DTS_HD_MA_X_IMAX)
      codecName = "DTSX";
  }
  else if (audioHint.codec == AV_CODEC_ID_OPUS)
  {
    optInfo["opusInfo"]["channels"] = audioHint.channels;
    optInfo["opusInfo"]["frequency"] = audioHint.samplerate / 1000.0;
    optInfo["opusInfo"]["streamHeader"] =
        Base64::Encode(reinterpret_cast<const char*>(audioHint.extradata.GetData()),
                       audioHint.extradata.GetSize());
  }
  else if (audioHint.codec == AV_CODEC_ID_AAC || audioHint.codec == AV_CODEC_ID_AAC_LATM)
  {
    optInfo["aacInfo"]["channels"] = audioHint.channels;
    optInfo["aacInfo"]["profile"] = audioHint.profile + 1;
    optInfo["aacInfo"]["format"] = audioHint.extradata ? "raw" : "adts";

    const uint8_t* data = audioHint.extradata.GetData();
    const size_t size = audioHint.extradata.GetSize();

    if (data && size > 0)
    {
      // ParseAACSampleRate is used to determine the actual sample rate from extradata
      // cannot use avpriv_mpeg4audio_get_config2 as not exposed in FFmpeg public API
      unsigned int parsedRate = ParseAACSampleRate(data, size);
      if (parsedRate > 0)
        audioHint.samplerate = parsedRate;
    }

    optInfo["aacInfo"]["frequency"] = audioHint.samplerate / 1000.0;
  }

  return codecName;
}

void CMediaPipelineWebOS::SetupBitstreamConverter(CDVDStreamInfo& hint)
{
  const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  const bool convertDovi =
      hint.dovi.el_present_flag || settings->GetBool(CSettings::SETTING_VIDEOPLAYER_CONVERTDOVI);

  const std::shared_ptr allowedHdrFormatsSetting(std::dynamic_pointer_cast<CSettingList>(
      settings->GetSetting(CSettings::SETTING_VIDEOPLAYER_ALLOWEDHDRFORMATS)));
  const bool removeDovi = !CSettingUtils::FindIntInList(
      allowedHdrFormatsSetting, CSettings::VIDEOPLAYER_ALLOWED_HDR_TYPE_DOLBY_VISION);

  if (hint.codec == AV_CODEC_ID_AVS || hint.codec == AV_CODEC_ID_CAVS ||
      hint.codec == AV_CODEC_ID_H264 || hint.codec == AV_CODEC_ID_HEVC)
  {
    if (hint.extradata && !hint.cryptoSession)
    {
      m_bitstream = std::make_unique<CBitstreamConverter>();
      if (m_bitstream->Open(hint.codec, hint.extradata.GetData(),
                            static_cast<int>(hint.extradata.GetSize()), true))
      {
        if (hint.codec == AV_CODEC_ID_HEVC)
        {
          m_bitstream->SetRemoveDovi(removeDovi);

          // webOS doesn't support HDR10+ and it can cause issues
          m_bitstream->SetRemoveHdr10Plus(true);

          // Only set for profile 7, container hint allows to skip parsing unnecessarily
          // set profile 8 and single layer when converting
          if (!removeDovi && convertDovi && hint.dovi.dv_profile == 7)
          {
            m_bitstream->SetConvertDovi(true);
            hint.dovi.dv_profile = 8;
            hint.dovi.el_present_flag = false;
          }

          bool isDvhe = hint.codec_tag == MKTAG('d', 'v', 'h', 'e');
          bool isDvh1 = hint.codec_tag == MKTAG('d', 'v', 'h', '1');

          // some files don't have dvhe or dvh1 tag set up but have Dolby Vision side data
          if (!isDvhe && !isDvh1 && hint.hdrType == StreamHdrType::HDR_TYPE_DOLBYVISION)
          {
            // page 10, table 2 from https://professional.dolby.com/siteassets/content-creation/dolby-vision-for-content-creators/dolby-vision-streams-within-the-http-live-streaming-format-v2.0-13-november-2018.pdf
            if (hint.codec_tag == MKTAG('h', 'v', 'c', '1'))
              isDvh1 = true;
            else
              isDvhe = true;
          }

          if (removeDovi && (isDvhe || isDvh1))
            hint.hdrType = StreamHdrType::HDR_TYPE_HDR10;
        }
      }
      else
      {
        m_bitstream.reset();
      }
    }
  }
}

void CMediaPipelineWebOS::SetHDR(const CDVDStreamInfo& hint) const
{
  if (hint.hdrType == StreamHdrType::HDR_TYPE_NONE)
    return;

  CVariant hdrData;
  CVariant sei;

  if (ms_hdrInfoMap.contains(hint.colorTransferCharacteristic))
    hdrData["hdrType"] = std::string(ms_hdrInfoMap.at(hint.colorTransferCharacteristic));
  else
    hdrData["hdrType"] = "none";

  if (hint.masteringMetadata)
  {
    if (hint.masteringMetadata->has_primaries)
    {
      // for more information, see CTA+861.3-A standard document
      constexpr int maxChromaticity = 50000;
      // expected input is in gbr order
      sei["displayPrimariesX0"] = static_cast<int>(
          std::round(av_q2d(hint.masteringMetadata->display_primaries[1][0]) * maxChromaticity));
      sei["displayPrimariesY0"] = static_cast<int>(
          std::round(av_q2d(hint.masteringMetadata->display_primaries[1][1]) * maxChromaticity));
      sei["displayPrimariesX1"] = static_cast<int>(
          std::round(av_q2d(hint.masteringMetadata->display_primaries[2][0]) * maxChromaticity));
      sei["displayPrimariesY1"] = static_cast<int>(
          std::round(av_q2d(hint.masteringMetadata->display_primaries[2][1]) * maxChromaticity));
      sei["displayPrimariesX2"] = static_cast<int>(
          std::round(av_q2d(hint.masteringMetadata->display_primaries[0][0]) * maxChromaticity));
      sei["displayPrimariesY2"] = static_cast<int>(
          std::round(av_q2d(hint.masteringMetadata->display_primaries[0][1]) * maxChromaticity));
      sei["whitePointX"] = static_cast<int>(
          std::round(av_q2d(hint.masteringMetadata->white_point[0]) * maxChromaticity));
      sei["whitePointY"] = static_cast<int>(
          std::round(av_q2d(hint.masteringMetadata->white_point[1]) * maxChromaticity));
    }

    if (hint.masteringMetadata->has_luminance)
    {
      constexpr int maxLuminance = 10000;
      sei["minDisplayMasteringLuminance"] = static_cast<int>(
          std::round(av_q2d(hint.masteringMetadata->min_luminance) * maxLuminance));
      sei["maxDisplayMasteringLuminance"] = static_cast<int>(
          std::round(av_q2d(hint.masteringMetadata->max_luminance) * maxLuminance));
    }
  }

  // we can have HDR content that does not provide content light level metadata
  if (hint.contentLightMetadata)
  {
    sei["maxContentLightLevel"] = hint.contentLightMetadata->MaxCLL;
    sei["maxPicAverageLightLevel"] = hint.contentLightMetadata->MaxFALL;
  }

  // Some TVs crash on a hdr info message without SEI information
  // This data is often not available from ffmpeg from the stream (av_stream_get_side_data)
  // So return here early and let the TV detect the presence of HDR metadata on its own
  if (sei.empty())
    return;
  hdrData[m_webOSVersion < 5 ? "mediaSei" : "sei"] = sei;

  CVariant vui;
  vui["transferCharacteristics"] = hint.colorTransferCharacteristic;
  vui["colorPrimaries"] = hint.colorPrimaries;
  vui["matrixCoeffs"] = hint.colorSpace;
  vui["videoFullRangeFlag"] = hint.colorRange == AVCOL_RANGE_JPEG;
  hdrData[m_webOSVersion < 5 ? "mediaVui" : "vui"] = vui;

  std::string payload;
  CJSONVariantWriter::Write(hdrData, payload, true);

  CLog::LogFC(LOGDEBUG, LOGVIDEO, "Setting HDR data payload {}", payload);
  if (!m_mediaAPIs->setHdrInfo(payload.c_str()))
    CLog::LogF(LOGERROR, "setHdrInfo failed");
}

void CMediaPipelineWebOS::FeedAudioData(const std::shared_ptr<CDVDMsg>& msg)
{
  DemuxPacket* packet = std::static_pointer_cast<CDVDMsgDemuxerPacket>(msg)->GetPacket();

  const auto pts = std::chrono::duration_cast<std::chrono::nanoseconds>(
      std::chrono::duration<double, std::ratio<1, DVD_TIME_BASE>>(packet->pts));

  if (pts < 0ns)
    return;

  CVariant payload;
  payload["bufferAddr"] = fmt::format("{:#x}", reinterpret_cast<std::uintptr_t>(packet->pData));
  payload["bufferSize"] = packet->iSize;
  payload["pts"] = pts.count();
  payload["esData"] = 2;

  std::string json;
  CJSONVariantWriter::Write(payload, json, true);
  CLog::LogFC(LOGDEBUG, LOGVIDEO, "{}", json);

  const std::string result = m_mediaAPIs->Feed(json.c_str());

  if (result.find("Ok") != std::string::npos)
  {
    m_audioStats.AddSampleBytes(packet->iSize);
    UpdateAudioInfo();
    return;
  }

  if (result.find("BufferFull") != std::string::npos)
  {
    m_messageQueueAudio.PutBack(msg);
    std::this_thread::sleep_for(100ms);
    return;
  }

  CLog::LogF(LOGWARNING, "Buffer submit returned error: {}", result);
}

void CMediaPipelineWebOS::FeedVideoData(const std::shared_ptr<CDVDMsg>& msg)
{
  DemuxPacket* packet = std::static_pointer_cast<CDVDMsgDemuxerPacket>(msg)->GetPacket();

  auto pts = std::chrono::duration_cast<std::chrono::nanoseconds>(
      std::chrono::duration<double, std::ratio<1, DVD_TIME_BASE>>(packet->pts));
  auto dts = std::chrono::duration_cast<std::chrono::nanoseconds>(
      std::chrono::duration<double, std::ratio<1, DVD_TIME_BASE>>(packet->dts));

  if (packet->dts == DVD_NOPTS_VALUE)
    dts = 0ns;

  if (m_videoHint.ptsinvalid)
    pts = dts;

  if (pts < 0ns)
    return;

  uint8_t* data = packet->pData;
  size_t size = packet->iSize;

  // we have an input buffer, fill it.
  if (data && m_bitstream)
  {
    m_bitstream->Convert(data, static_cast<int>(size));

    if (!m_bitstream->CanStartDecode())
    {
      CLog::LogF(LOGDEBUG, "Waiting for keyframe (bitstream)");
      return;
    }

    size = m_bitstream->GetConvertSize();
    data = m_bitstream->GetConvertBuffer();
  }

  if (m_flushed)
  {
    CVariant time;
    time["position"] = pts.count();
    std::string payload;
    CJSONVariantWriter::Write(time, payload, true);

    auto player = static_cast<mediapipeline::CustomPlayer*>(m_mediaAPIs->player.get());
    auto pipeline = static_cast<mediapipeline::CustomPipeline*>(player->getPipeline().get());
    if (!m_mediaAPIs->setTimeToDecode(payload.c_str()))
    {
      CLog::LogF(LOGERROR, "setTimeToDecode failed");
      MEDIA_CUSTOM_CONTENT_INFO_T contentInfo;
      pipeline->loadSpi_getInfo(&contentInfo);
      contentInfo.ptsToDecode = pts.count();
      pipeline->setContentInfo(MEDIA_CUSTOM_SRC_TYPE_ES, &contentInfo);
    }

    pipeline->sendSegmentEvent();

    m_pts = pts;

    SStartMsg startMsg{.timestamp = GetCurrentPts(),
                       .player = VideoPlayer_VIDEO,
                       .cachetime = DVD_MSEC_TO_TIME(50),
                       .cachetotal = DVD_MSEC_TO_TIME(100)};
    m_messageQueueParent.Put(
        std::make_shared<CDVDMsgType<SStartMsg>>(CDVDMsg::PLAYER_STARTED, startMsg));
    startMsg.player = VideoPlayer_AUDIO;
    m_messageQueueParent.Put(
        std::make_shared<CDVDMsgType<SStartMsg>>(CDVDMsg::PLAYER_STARTED, startMsg));
    m_flushed = false;
  }

  if (data && size)
  {
    CVariant payload;
    payload["bufferAddr"] = fmt::format("{:#x}", reinterpret_cast<std::uintptr_t>(data));
    payload["bufferSize"] = size;
    payload["pts"] = (pts - std::chrono::milliseconds(m_renderManager.GetDelay())).count();
    payload["esData"] = 1;

    std::string json;
    CJSONVariantWriter::Write(payload, json, true);
    CLog::LogFC(LOGDEBUG, LOGVIDEO, "{}", json);

    const std::string result = m_mediaAPIs->Feed(json.c_str());

    if (result.find("Ok") != std::string::npos)
    {
      m_videoStats.AddSampleBytes(packet->iSize);
      UpdateVideoInfo();
      const unsigned int level = GetQueueLevel(StreamType::VIDEO);
      m_processInfo.SetLevelVQ(static_cast<int>(level));
      return;
    }

    if (result.find("BufferFull") != std::string::npos)
    {
      m_messageQueueVideo.PutBack(msg);
      std::this_thread::sleep_for(100ms);
      return;
    }

    CLog::LogF(LOGWARNING, "Buffer submit returned error: {}", result);
  }
}

void CMediaPipelineWebOS::ProcessOverlays(const double pts) const
{
  // remove any overlays that are out of time
  m_overlayContainer.CleanUp(pts - m_subtitleDelay);

  std::vector<std::shared_ptr<CDVDOverlay>> overlays;
  std::scoped_lock<CCriticalSection> lock(m_overlayContainer);

  // Check all overlays and render those that should be rendered, based on time and forced
  // Both forced and subs should check timing
  for (const std::shared_ptr<CDVDOverlay>& overlay : *m_overlayContainer.GetOverlays())
  {
    if (!overlay->bForced && !m_subtitle)
      continue;

    const double pts2 = overlay->bForced ? pts : pts - m_subtitleDelay;

    if (overlay->iPTSStartTime <= pts2 &&
        (overlay->iPTSStopTime > pts2 || overlay->iPTSStopTime == 0LL))
    {
      if (overlay->IsOverlayType(DVDOVERLAY_TYPE_GROUP))
        overlays.insert(overlays.end(), static_cast<CDVDOverlayGroup&>(*overlay).m_overlays.begin(),
                        static_cast<CDVDOverlayGroup&>(*overlay).m_overlays.end());
      else
        overlays.push_back(overlay);
    }
  }

  for (const std::shared_ptr<CDVDOverlay>& overlay : overlays)
  {
    const double pts2 = overlay->bForced ? pts : pts - m_subtitleDelay;
    m_renderManager.AddOverlay(overlay, pts2);
  }
}

unsigned int CMediaPipelineWebOS::GetQueueCapacity(const StreamType type) const
{
  if (type == StreamType::VIDEO)
    return MAX_SRC_BUFFER_LEVEL_VIDEO + m_messageQueueVideo.GetMaxDataSize();
  if (type == StreamType::AUDIO)
  {
    if (m_audioHint.codec == AV_CODEC_ID_NONE)
      return 0;
    return MAX_SRC_BUFFER_LEVEL_AUDIO + m_messageQueueAudio.GetMaxDataSize();
  }
  return 0;
}

unsigned int CMediaPipelineWebOS::GetQueuedBytes(const StreamType type) const
{
  if (type == StreamType::AUDIO && m_audioHint.codec == AV_CODEC_ID_NONE)
    return 0;

  const CDVDMessageQueue* queue = nullptr;
  uint64_t queuedSrcBytes = 0;
  uint32_t queuedBytes = 0;
  uint32_t queuedSinkBytes = 0;

  if (type == StreamType::AUDIO)
  {
    queue = &m_messageQueueAudio;
    if (m_pipeline)
    {
      if (m_pipeline->audioSrc)
        g_object_get(m_pipeline->audioSrc, "current-level-bytes", &queuedSrcBytes, nullptr);
      if (m_pipeline->audioQueue)
        g_object_get(m_pipeline->audioQueue, "current-level-bytes", &queuedBytes, nullptr);
      if (m_pipeline->audioSinkQueue)
        g_object_get(m_pipeline->audioSinkQueue, "current-level-bytes", &queuedSinkBytes, nullptr);
    }
  }
  else if (type == StreamType::VIDEO)
  {
    queue = &m_messageQueueVideo;
    if (m_pipeline)
    {
      if (m_pipeline->videoSrc)
        g_object_get(m_pipeline->videoSrc, "current-level-bytes", &queuedSrcBytes, nullptr);
      if (m_pipeline->videoQueue)
        g_object_get(m_pipeline->videoQueue, "current-level-bytes", &queuedBytes, nullptr);
      if (m_pipeline->videoSinkQueue)
        g_object_get(m_pipeline->videoSinkQueue, "current-level-bytes", &queuedSinkBytes, nullptr);
    }
  }

  if (!queue)
    return 0;

  int bytes = queue->GetDataSize();
  // If the message queue is not data-based, we calculate hypothetical bytes based on the time size
  if (!queue->IsDataBased())
  {
    bytes = std::lround(queue->GetTimeSize() / queue->GetMaxTimeSize() * queue->GetMaxDataSize());
  }

  return bytes + queuedSrcBytes + queuedSinkBytes + queuedBytes;
}

unsigned int CMediaPipelineWebOS::GetQueueLevel(const StreamType type) const
{
  if (type == StreamType::AUDIO && m_audioHint.codec == AV_CODEC_ID_NONE)
    return 0;

  const unsigned int bytes = GetQueuedBytes(type);
  const unsigned int capacity = GetQueueCapacity(type);

  return std::min(99L, std::lround(100.0 * bytes / capacity));
}

void CMediaPipelineWebOS::Process()
{
  while (!m_bStop)
  {
    std::scoped_lock videoLock(m_videoCriticalSection);
    std::shared_ptr<CDVDMsg> msg = nullptr;
    int priority = 0;
    m_messageQueueVideo.Get(msg, 10ms, priority);

    if (msg)
    {
      if (msg->IsType(CDVDMsg::DEMUXER_PACKET))
      {
        FeedVideoData(msg);
      }
      else if (msg->IsType(CDVDMsg::PLAYER_REQUEST_STATE))
      {
        constexpr SStateMsg stateMsg{.syncState = IDVDStreamPlayer::SYNC_INSYNC,
                                     .player = VideoPlayer_VIDEO};
        m_messageQueueParent.Put(
            std::make_shared<CDVDMsgType<SStateMsg>>(CDVDMsg::PLAYER_REPORT_STATE, stateMsg));
      }
      else if (msg->IsType(CDVDMsg::VIDEO_DRAIN))
      {
        CLog::LogF(LOGDEBUG, "Received video message: {}", msg->GetMessageType());
        if (!m_mediaAPIs->pushEOS())
          CLog::LogF(LOGERROR, "Failed to push EOS message");
      }
      else
      {
        CLog::LogF(LOGDEBUG, "Received video message: {}", msg->GetMessageType());
      }
    }
  }
}

void CMediaPipelineWebOS::ProcessAudio()
{
  m_audioStats.Start();
  while (!m_bStop)
  {
    std::scoped_lock lock(m_audioCriticalSection);
    std::shared_ptr<CDVDMsg> msg = nullptr;
    int priority = 0;
    m_messageQueueAudio.Get(msg, 10ms, priority);
    if (msg)
    {
      if (msg->IsType(CDVDMsg::DEMUXER_PACKET))
      {
        const DemuxPacket* packet =
            std::static_pointer_cast<CDVDMsgDemuxerPacket>(msg)->GetPacket();
        if (m_audioCodec && packet->iStreamId != RESAMPLED_STREAM_ID)
        {
          if (!m_audioCodec->AddData(*packet))
            m_messageQueueAudio.PutBack(msg);

          DVDAudioFrame frame;
          m_audioCodec->GetData(frame);
          if (frame.nb_frames > 0)
          {
            if (!m_audioResample)
            {
              AEAudioFormat dstFormat = m_audioCodec->GetFormat();
              dstFormat.m_sampleRate = SelectTranscodingSampleRate(dstFormat.m_sampleRate);
              dstFormat.m_dataFormat = AE_FMT_FLOATP;
              m_audioEncoder->Initialize(dstFormat, true);
              const std::shared_ptr<CSettings> settings =
                  CServiceBroker::GetSettingsComponent()->GetSettings();
              auto quality = static_cast<AEQuality>(
                  settings->GetInt(CSettings::SETTING_AUDIOOUTPUT_PROCESSQUALITY));
              m_audioResample = std::make_unique<ActiveAE::CActiveAEBufferPoolResample>(
                  m_audioCodec->GetFormat(), dstFormat, quality);
              const double sublevel =
                  settings->GetNumber(CSettings::SETTING_AUDIOOUTPUT_MIXSUBLEVEL) / 100.0;
              m_audioResample->Create(
                  0, false, settings->GetBool(CSettings::SETTING_AUDIOOUTPUT_STEREOUPMIX),
                  !settings->GetBool(CSettings::SETTING_AUDIOOUTPUT_MAINTAINORIGINALVOLUME),
                  static_cast<float>(sublevel));
              m_audioResample->FillBuffer();

              AEAudioFormat input = m_audioCodec->GetFormat();
              input.m_frames = std::max(frame.nb_frames, MIN_AUDIO_RESAMPLE_BUFFER_SIZE);
              m_encoderBuffers = std::make_unique<ActiveAE::CActiveAEBufferPool>(input);
              m_encoderBuffers->Create(0);

              // Update process info with audio details
              m_processInfo.SetAudioChannels(frame.format.m_channelLayout);
              m_processInfo.SetAudioSampleRate(frame.format.m_sampleRate);
              m_processInfo.SetAudioBitsPerSample(frame.bits_per_sample);
            }

            using dvdTime = std::ratio<1, DVD_TIME_BASE>;
            if (frame.pts == DVD_NOPTS_VALUE)
            {
              frame.pts = m_audioClock.count();
              // guess next pts
              m_audioClock += std::chrono::duration<double, dvdTime>(frame.duration);
            }
            else
            {
              m_audioClock = std::chrono::duration<double, dvdTime>(frame.pts);
            }

            ActiveAE::CSampleBuffer* buffer = m_encoderBuffers->GetFreeBuffer();
            buffer->timestamp = static_cast<int64_t>(frame.pts);
            buffer->pkt->nb_samples = static_cast<int>(frame.nb_frames);

            const unsigned int bytes = frame.nb_frames * frame.framesize / frame.planes;
            for (unsigned int i = 0; i < frame.planes; i++)
            {
              std::copy_n(frame.data[i], bytes, buffer->pkt->data[i]);
            }

            m_audioResample->m_inputSamples.emplace_back(buffer);

            while (m_audioResample->ResampleBuffers())
            {
              for (const auto& buf : m_audioResample->m_outputSamples)
              {
                auto p = std::make_shared<CDVDMsgDemuxerPacket>(
                    CDVDDemuxUtils::AllocateDemuxPacket(AC3_MAX_SYNC_FRAME_SIZE));
                p->m_packet->pts = static_cast<double>(buf->timestamp);
                p->m_packet->iSize = AC3_MAX_SYNC_FRAME_SIZE;
                p->m_packet->iStreamId = RESAMPLED_STREAM_ID;
                p->m_packet->iSize =
                    m_audioEncoder->Encode(buf->pkt->data[0], buf->pkt->planes * buf->pkt->linesize,
                                           p->m_packet->pData, p->m_packet->iSize);
                buf->Return();
                FeedAudioData(p);
              }
              m_audioResample->m_outputSamples.clear();
            }
          }
        }
        else
          FeedAudioData(msg);
      }
      else if (msg->IsType(CDVDMsg::PLAYER_REQUEST_STATE))
      {
        constexpr SStateMsg stateMsg{
            .syncState = IDVDStreamPlayer::SYNC_INSYNC,
            .player = VideoPlayer_AUDIO,
        };
        m_messageQueueParent.Put(
            std::make_shared<CDVDMsgType<SStateMsg>>(CDVDMsg::PLAYER_REPORT_STATE, stateMsg));
      }
      else
      {
        CLog::LogF(LOGDEBUG, "Received audio message: {}", msg->GetMessageType());
      }
    }
  }
}

void CMediaPipelineWebOS::GetVideoResolution(unsigned int& width, unsigned int& height) const
{
  if (m_videoHint.codec)
  {
    width = m_videoHint.width;
    height = m_videoHint.height;
  }
}

void CMediaPipelineWebOS::PlayerCallback(int32_t type, const int64_t numValue, const char* strValue)
{
  const std::string logStr = strValue != nullptr ? strValue : "";
  CLog::LogF(LOGDEBUG, "type: {}, numValue: {}, strValue: {}", type, numValue, logStr);

  const auto buffer = static_cast<CStarfishVideoBuffer*>(m_picture.videoBuffer);

  // Adjust event type for older webOS versions
  if (m_webOSVersion < 5 && type > PF_EVENT_TYPE_STR_STATE_UPDATE__ENDOFSTREAM)
    type += 2;

  const std::unique_ptr<AcbHandle>& acb = buffer->GetAcbHandle();
  switch (type)
  {
    case PF_EVENT_TYPE_FRAMEREADY:
    {
      m_pts = std::chrono::nanoseconds(numValue);
      const double pts = GetCurrentPts();
      ProcessOverlays(pts);
      m_picture.dts = pts;
      m_picture.pts = pts;
      std::atomic<bool> stop(false);
      m_renderManager.AddVideoPicture(m_picture, stop, VS_INTERLACEMETHOD_AUTO, false);
      m_clock.Discontinuity(pts);
      UpdateVideoInfo();
      UpdateAudioInfo();
      break;
    }
    case PF_EVENT_TYPE_STR_AUDIO_INFO:
      if (acb)
        AcbAPI_setMediaAudioData(acb->Id(), logStr.c_str(), &acb->TaskId());
      break;
    case PF_EVENT_TYPE_STR_VIDEO_INFO:
      if (acb)
        AcbAPI_setMediaVideoData(acb->Id(), logStr.c_str(), &acb->TaskId());
      break;
    case PF_EVENT_TYPE_STR_STATE_UPDATE__LOADCOMPLETED:
    {
      const auto player = static_cast<mediapipeline::CustomPlayer*>(m_mediaAPIs->player.get());
      const auto pipeline =
          static_cast<mediapipeline::CustomPipeline*>(player->getPipeline().get());
      m_pipeline = pipeline->GetGStreamerElements(
          {0, MIN_SRC_BUFFER_LEVEL_VIDEO, MAX_SRC_BUFFER_LEVEL_VIDEO, MAX_BUFFER_LEVEL});

      if (acb)
      {
        AcbAPI_setSinkType(acb->Id(), SINK_TYPE_MAIN);
        AcbAPI_setMediaId(acb->Id(), m_mediaAPIs->getMediaID());
        AcbAPI_setState(acb->Id(), APPSTATE_FOREGROUND, PLAYSTATE_LOADED, &acb->TaskId());
      }
      m_renderManager.ShowVideo(true);
      if (!m_mediaAPIs->Play())
        CLog::LogF(LOGERROR, "Failed to play");
      m_loaded = true;
      m_flushed = true;
      Create();
      m_audioThread = std::thread([this] { ProcessAudio(); });
      break;
    }
    case PF_EVENT_TYPE_STR_STATE_UPDATE__UNLOADCOMPLETED:
      if (acb)
        AcbAPI_setState(acb->Id(), APPSTATE_FOREGROUND, PLAYSTATE_UNLOADED, &acb->TaskId());
      StopThread(true);
      if (m_audioThread.joinable())
        m_audioThread.join();
      m_loaded = false;
      m_pipeline = nullptr;
      UpdateGUISounds(false);
      break;
    case PF_EVENT_TYPE_STR_STATE_UPDATE__PAUSED:
      if (acb)
        AcbAPI_setState(acb->Id(), APPSTATE_FOREGROUND, PLAYSTATE_PAUSED, &acb->TaskId());
      UpdateGUISounds(false);
      break;
    case PF_EVENT_TYPE_STR_STATE_UPDATE__PLAYING:
    {
      SStartMsg msg{.timestamp = GetCurrentPts(),
                    .player = VideoPlayer_VIDEO,
                    .cachetime = DVD_MSEC_TO_TIME(50),
                    .cachetotal = DVD_MSEC_TO_TIME(100)};
      m_messageQueueParent.Put(
          std::make_shared<CDVDMsgType<SStartMsg>>(CDVDMsg::PLAYER_STARTED, msg));
      msg.player = VideoPlayer_AUDIO;
      m_messageQueueParent.Put(
          std::make_shared<CDVDMsgType<SStartMsg>>(CDVDMsg::PLAYER_STARTED, msg));
      if (acb)
        AcbAPI_setState(acb->Id(), APPSTATE_FOREGROUND, PLAYSTATE_PLAYING, &acb->TaskId());
      UpdateGUISounds(true);
      break;
    }
    case PF_EVENT_TYPE_STR_BUFFERFULL:
    {
      SStateMsg msg{.syncState = IDVDStreamPlayer::SYNC_INSYNC, .player = VideoPlayer_AUDIO};
      m_messageQueueParent.Put(
          std::make_shared<CDVDMsgType<SStateMsg>>(CDVDMsg::PLAYER_REPORT_STATE, msg));
      msg.player = VideoPlayer_VIDEO;
      m_messageQueueParent.Put(
          std::make_shared<CDVDMsgType<SStateMsg>>(CDVDMsg::PLAYER_REPORT_STATE, msg));
      break;
    }
    case PF_EVENT_TYPE_DROPPED_FRAME:
      m_droppedFrames += static_cast<unsigned long>(numValue);
      break;
    case PF_EVENT_TYPE_INT_ERROR:
      CLog::LogF(LOGERROR, "Pipeline INT_ERROR numValue: {}, strValue: {}", numValue, logStr);
      break;
    case PF_EVENT_TYPE_STR_ERROR:
      CLog::LogF(LOGERROR, "Pipeline STR_ERROR numValue: {}, strValue: {}", numValue, logStr);
      break;
    default:
      break;
  }

  m_eventCondition.notify_all();
}

void CMediaPipelineWebOS::PlayerCallback(const int32_t type,
                                         const int64_t numValue,
                                         const char* strValue,
                                         void* data)
{
  static_cast<CMediaPipelineWebOS*>(data)->PlayerCallback(type, numValue, strValue);
}
