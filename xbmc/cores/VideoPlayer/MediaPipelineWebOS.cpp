/*
*  Copyright (C) 2005-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "MediaPipelineWebOS.h"

#include "CompileInfo.h"
#include "DVDCodecs/Audio/DVDAudioCodecFFmpeg.h"
#include "DVDCodecs/Audio/DVDAudioCodecPassthrough.h"
#include "DVDCodecs/DVDCodecs.h"
#include "DVDCodecs/DVDFactoryCodec.h"
#include "DVDOverlayContainer.h"
#include "Interface/TimingConstants.h"
#include "Process/ProcessInfo.h"
#include "VideoRenderers/HwDecRender/RendererStarfish.h"
#include "VideoRenderers/RenderManager.h"
#include "cores/AudioEngine/Encoders/AEEncoderFFmpeg.h"
#include "cores/AudioEngine/Engines/ActiveAE/ActiveAEBuffer.h"
#include "cores/AudioEngine/Utils/AEUtil.h"
#include "settings/SettingUtils.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "utils/BitstreamConverter.h"
#include "utils/JSONVariantParser.h"
#include "utils/JSONVariantWriter.h"
#include "utils/SystemInfo.h"
#include "utils/log.h"
#include "windowing/wayland/WinSystemWaylandWebOS.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <exception>
#include <iomanip>
#include <map>
#include <ratio>
#include <sstream>
#include <string_view>
#include <utility>
#include <vector>

#include <appswitching-control-block/AcbAPI.h>
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

constexpr unsigned int PRE_BUFFER_BYTES = 0;
constexpr unsigned int MAX_QUEUE_BUFFER_LEVEL = 1 * 1024 * 1024; // 1 MB
constexpr unsigned int MIN_BUFFER_LEVEL = 90;
constexpr unsigned int MAX_BUFFER_LEVEL = 100;
constexpr unsigned int MIN_SRC_BUFFER_LEVEL_AUDIO = 1 * 1024 * 1024; // 1 MB
constexpr unsigned int MIN_SRC_BUFFER_LEVEL_VIDEO = 1 * 1024 * 1024; // 1 MB
constexpr unsigned int MAX_SRC_BUFFER_LEVEL_AUDIO = 2 * 1024 * 1024; // 2 MB
constexpr unsigned int MAX_SRC_BUFFER_LEVEL_VIDEO = 8 * 1024 * 1024; // 8 MB

constexpr auto LUNA_GET_CONFIG = "luna://com.webos.service.config/getConfigs";

auto ms_codecMap = std::map<AVCodecID, std::string_view>({
    {AV_CODEC_ID_VP8, "VP8"},
    {AV_CODEC_ID_VP9, "VP9"},
    {AV_CODEC_ID_AVS, "H264"},
    {AV_CODEC_ID_CAVS, "H264"},
    {AV_CODEC_ID_H264, "H264"},
    {AV_CODEC_ID_HEVC, "H265"},
    {AV_CODEC_ID_AV1, "AV1"},
    {AV_CODEC_ID_AC3, "AC3"},
    {AV_CODEC_ID_EAC3, "AC3 PLUS"},
});

const auto ms_hdrInfoMap = std::map<AVColorTransferCharacteristic, std::string_view>({
    {AVCOL_TRC_SMPTE2084, "HDR10"},
    {AVCOL_TRC_ARIB_STD_B67, "HLG"},
});

int GetWebOSVersion()
{
  std::string version = CSysInfo::GetOsVersion();
  int majorVersion = 0;
  try
  {
    const size_t pos = version.find('.');
    const std::string majorStr = (pos != std::string::npos) ? version.substr(0, pos) : version;
    majorVersion = std::stoi(majorStr);
  }
  catch (const std::exception& e)
  {
    CLog::LogF(LOGERROR, "Failed to parse WebOS version '{}': {}", version, e.what());
  }
  return majorVersion;
}

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

  m_picture.Reset();
  m_picture.videoBuffer = new CStarfishVideoBuffer();

  m_webOSVersion = GetWebOSVersion();

  CVariant request;
  request["configNames"] = std::vector<std::string>{"tv.model.edidType"};
  std::string payload;
  CJSONVariantWriter::Write(request, payload, true);

  HContext requestContext;
  requestContext.pub = true;
  requestContext.multiple = false;
  requestContext.callback = [](LSHandle* sh, LSMessage* msg, void* ctx)
  {
    CVariant config;
    CJSONVariantParser::Parse(HLunaServiceMessage(msg), config);
    if (config["configs"]["tv.model.edidType"].asString().find("dts") != std::string::npos)
      ms_codecMap.emplace(AV_CODEC_ID_DTS, "DTS");

    return false;
  };
  if (HLunaServiceCall(LUNA_GET_CONFIG, payload.c_str(), &requestContext))
  {
    CLog::LogF(LOGERROR, "Luna request call failed");
  }
}

CMediaPipelineWebOS::~CMediaPipelineWebOS()
{
  CloseVideoStream(true);
  CloseAudioStream(true);
}

int CMediaPipelineWebOS::GetVideoBitrate() const
{
  return static_cast<int>(m_videoStats.GetBitrate());
}

void CMediaPipelineWebOS::UpdateAudioInfo()
{
  std::ostringstream s;
  s << "aq:" << std::setw(2) << std::min(99, m_messageQueueAudio.GetLevel());
  s << "% " << std::fixed << std::setprecision(3) << m_messageQueueAudio.GetTimeSize();
  s << "s, Kb/s:" << std::fixed << std::setprecision(2) << m_audioStats.GetBitrate() / 1024.0;
  if (m_audioEncoder)
  {
    s << ", transcoded ac3";
  }

  m_audioInfo = s.str();
}

void CMediaPipelineWebOS::UpdateVideoInfo()
{
  std::ostringstream s;
  s << "vq:" << std::setw(2) << std::min(99, m_processInfo.GetLevelVQ());
  s << "% " << std::fixed << std::setprecision(3) << m_messageQueueVideo.GetTimeSize();
  s << "s, Mb/s:" << std::fixed << std::setprecision(2)
    << static_cast<double>(GetVideoBitrate()) / (1024.0 * 1024.0);
  s << ", fr:" << std::fixed << std::setprecision(3)
    << static_cast<double>(m_videoHint.fpsrate) / static_cast<double>(m_videoHint.fpsscale);
  s << ", drop:" << m_droppedFrames;

  m_videoInfo = s.str();
}

std::string CMediaPipelineWebOS::GetAudioInfo() const
{
  return m_audioInfo;
}

std::string CMediaPipelineWebOS::GetVideoInfo() const
{
  return m_videoInfo;
}

bool CMediaPipelineWebOS::Supports(const AVCodecID codec)
{
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
  if (m_loaded)
  {
    if (m_webOSVersion >= 6)
    {
      std::unique_lock lock(m_audioCriticalSection);
      std::string codecName = "AC3";
      m_audioCodec = nullptr;
      m_audioEncoder = nullptr;
      m_audioResample = nullptr;
      if (!ms_codecMap.contains(audioHint.codec))
      {
        m_audioCodec = std::make_unique<CDVDAudioCodecFFmpeg>(m_processInfo);
        CDVDCodecOptions options;
        m_audioCodec->Open(audioHint, options);
        m_audioEncoder = std::make_unique<CAEEncoderFFmpeg>();
      }
      else
      {
        codecName = ms_codecMap.at(audioHint.codec);
      }

      CVariant optInfo = {};
      if (audioHint.codec == AV_CODEC_ID_EAC3)
      {
        optInfo["ac3PlusInfo"]["channels"] = audioHint.channels;
        optInfo["ac3PlusInfo"]["frequency"] = audioHint.samplerate / 1000.0;
      }
      else if (audioHint.codec == AV_CODEC_ID_DTS && ms_codecMap.contains(AV_CODEC_ID_DTS))
      {
        optInfo["dtsInfo"]["channels"] = audioHint.channels;
        optInfo["dtsInfo"]["frequency"] = audioHint.samplerate / 1000.0;

        if (audioHint.profile == AV_PROFILE_DTS_ES)
          codecName = "DTSE";
        if (audioHint.profile == AV_PROFILE_DTS_HD_MA_X ||
            audioHint.profile == AV_PROFILE_DTS_HD_MA_X_IMAX)
          codecName = "DTSX";
      }

      std::string output;
      CJSONVariantWriter::Write(optInfo, output, true);
      CLog::LogF(LOGDEBUG, "changeAudioCodec: {}", output);
      m_mediaAPIs->changeAudioCodec(codecName, output);
      FlushAudioMessages();
      m_audioHint = audioHint;
      return true;
    }
    else
    {
      m_messageQueueParent.Put(std::make_shared<CDVDMsg>(CDVDMsg::PLAYER_ABORT));
    }
  }

  m_audioHint = audioHint;

  if (m_audioHint.codec && m_videoHint.codec)
    return Load(m_videoHint, m_audioHint);
  return true;
}

bool CMediaPipelineWebOS::OpenVideoStream(const CDVDStreamInfo& hint)
{
  if (!Supports(hint.codec))
  {
    CLog::LogF(LOGERROR, "Unsupported codec: {}", hint.codec);
    return false;
  }

  if (m_loaded)
  {
    StopThread(true);
    if (m_audioThread.joinable())
      m_audioThread.join();

    const CDVDStreamInfo audioHint = m_audioHint;
    CloseAudioStream(true);
    CloseVideoStream(true);

    // wait until m_loaded is false
    std::unique_lock lock(m_eventMutex);
    if (!m_eventCondition.wait_for(lock, 1s, [this] { return !m_loaded; }))
    {
      CLog::LogF(LOGERROR, "Timeout waiting for m_loaded to be false");
      return false;
    }

    m_audioHint = audioHint;
    m_mediaAPIs = std::make_unique<StarfishMediaAPIs>();
  }

  m_videoHint = hint;

  if ((m_audioHint.codec || !m_hasAudio) && m_videoHint.codec)
    return Load(m_videoHint, m_audioHint);
  return true;
}

void CMediaPipelineWebOS::CloseAudioStream(bool waitForBuffers)
{
  m_audioHint = CDVDStreamInfo{};
}

void CMediaPipelineWebOS::CloseVideoStream(bool waitForBuffers)
{
  Flush(false);
  m_mediaAPIs->Unload();
  m_videoHint = CDVDStreamInfo{};

  const auto buffer = static_cast<CStarfishVideoBuffer*>(m_picture.videoBuffer);
  if (buffer->m_acbId)
  {
    AcbAPI_finalize(buffer->m_acbId);
    AcbAPI_destroy(buffer->m_acbId);
    buffer->m_acbId = 0;
  }
}

void CMediaPipelineWebOS::Flush(bool sync)
{
  m_mediaAPIs->flush();
  FlushAudioMessages();
  FlushVideoMessages();
  if (m_bitstream)
    m_bitstream->ResetStartDecode();
  m_flushed = true;
  m_bufferLevel = 0;
}

bool CMediaPipelineWebOS::AcceptsAudioData() const
{
  return m_bufferLevel == 0 || !m_audioFull || !m_messageQueueAudio.IsFull();
}

bool CMediaPipelineWebOS::AcceptsVideoData() const
{
  return m_bufferLevel == 0 || !m_videoFull || !m_messageQueueVideo.IsFull();
}

bool CMediaPipelineWebOS::HasAudioData() const
{
  return m_bufferLevel > 0;
}

bool CMediaPipelineWebOS::HasVideoData() const
{
  return m_bufferLevel > 0;
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
  return m_bufferLevel;
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
  m_processInfo.SetLevelVQ(m_messageQueueVideo.GetLevel());
}

void CMediaPipelineWebOS::SetSpeed(const int speed)
{
  if (!m_loaded)
    return;

  switch (speed)
  {
    case DVD_PLAYSPEED_PAUSE:
      m_mediaAPIs->Pause();
      return;
    case DVD_PLAYSPEED_NORMAL:
      m_mediaAPIs->Play();
      break;
    default:
    {
      CVariant payload;
      payload["audioOutput"] = std::abs(speed) <= 2000;
      payload["playRate"] = speed / 1000.0;
      std::string output;
      CJSONVariantWriter::Write(payload, output, true);

      if (!m_mediaAPIs->SetPlayRate(output.c_str()))
        CLog::LogF(LOGERROR, "SetPlayRate failed");
    }
  }
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
  CVariant p;
  CVariant payloadArgs;

  if (videoHint.cryptoSession || audioHint.cryptoSession)
  {
    CLog::LogF(LOGERROR, "CryptoSessions unsupported");
    return false;
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

  std::string formatName = "starfish-" + StringUtils::ToLower(ms_codecMap.at(videoHint.codec));

  CVariant& contents = p["option"]["externalStreamingInfo"]["contents"];
  switch (videoHint.codec)
  {
    case AV_CODEC_ID_AVS:
    case AV_CODEC_ID_CAVS:
    case AV_CODEC_ID_H264:
      // check for h264-avcC and convert to h264-annex-b
      if (videoHint.extradata && !videoHint.cryptoSession)
      {
        m_bitstream = std::make_unique<CBitstreamConverter>();
        if (!m_bitstream->Open(videoHint.codec, videoHint.extradata.GetData(),
                               static_cast<int>(videoHint.extradata.GetSize()), true))
        {
          m_bitstream.reset();
        }
      }
      break;
    case AV_CODEC_ID_HEVC:
    {
      const auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();
      bool convertDovi{false};
      bool removeDovi{false};

      if (settings)
      {
        convertDovi = settings->GetBool(CSettings::SETTING_VIDEOPLAYER_CONVERTDOVI);

        const std::shared_ptr allowedHdrFormatsSetting(std::dynamic_pointer_cast<CSettingList>(
            settings->GetSetting(CSettings::SETTING_VIDEOPLAYER_ALLOWEDHDRFORMATS)));
        removeDovi = !CSettingUtils::FindIntInList(
            allowedHdrFormatsSetting, CSettings::VIDEOPLAYER_ALLOWED_HDR_TYPE_DOLBY_VISION);
      }

      bool isDvhe = videoHint.codec_tag == MKTAG('d', 'v', 'h', 'e');
      bool isDvh1 = videoHint.codec_tag == MKTAG('d', 'v', 'h', '1');

      // some files don't have dvhe or dvh1 tag set up but have Dolby Vision side data
      if (!isDvhe && !isDvh1 && videoHint.hdrType == StreamHdrType::HDR_TYPE_DOLBYVISION)
      {
        // page 10, table 2 from https://professional.dolby.com/siteassets/content-creation/dolby-vision-for-content-creators/dolby-vision-streams-within-the-http-live-streaming-format-v2.0-13-november-2018.pdf
        if (videoHint.codec_tag == MKTAG('h', 'v', 'c', '1'))
          isDvh1 = true;
        else
          isDvhe = true;
      }

      // check for hevc-hvcC and convert to h265-annex-b
      if (videoHint.extradata && !videoHint.cryptoSession)
      {
        m_bitstream = std::make_unique<CBitstreamConverter>();
        if (!m_bitstream->Open(videoHint.codec, videoHint.extradata.GetData(),
                               static_cast<int>(videoHint.extradata.GetSize()), true))
        {
          m_bitstream.reset();
        }

        if (m_bitstream)
        {
          m_bitstream->SetRemoveDovi(removeDovi);

          // webOS doesn't support HDR10+ and it can cause issues
          m_bitstream->SetRemoveHdr10Plus(true);

          // Only set for profile 7, container hint allows to skip parsing unnecessarily
          // set profile 8 and single layer when converting
          if (!removeDovi && convertDovi && videoHint.dovi.dv_profile == 7)
          {
            videoHint.dovi.dv_profile = 8;
            videoHint.dovi.el_present_flag = false;
            m_bitstream->SetConvertDovi(true);
          }
        }
      }

      if (!removeDovi && (isDvhe || isDvh1))
      {
        formatName = isDvhe ? "starfish-dvhe" : "starfish-dvh1";

        contents["DolbyHdrInfo"]["encryptionType"] = "clear"; //"clear", "bl", "el", "all"
        contents["DolbyHdrInfo"]["profileId"] = videoHint.dovi.dv_profile; // profile 0-9
        contents["DolbyHdrInfo"]["trackType"] = videoHint.dovi.el_present_flag ? "dual" : "single";
      }

      if (removeDovi && (isDvhe || isDvh1))
        videoHint.hdrType = StreamHdrType::HDR_TYPE_HDR10;

      break;
    }
    case AV_CODEC_ID_AV1:
    {
      if (videoHint.hdrType == StreamHdrType::HDR_TYPE_DOLBYVISION &&
          videoHint.dovi.dv_profile == 10)
      {
        formatName = "starfish-dav1";

        contents["DolbyHdrInfo"]["encryptionType"] = "clear"; //"clear", "bl", "el", "all"
        contents["DolbyHdrInfo"]["profileId"] = videoHint.dovi.dv_profile; // profile 10
        contents["DolbyHdrInfo"]["trackType"] = "single"; // "single" / "dual"
      }

      break;
    }
    default:
      break;
  }

  using namespace KODI::WINDOWING::WAYLAND;
  const auto winSystem = dynamic_cast<CWinSystemWaylandWebOS*>(CServiceBroker::GetWinSystem());
  if (winSystem->SupportsExportedWindow())
  {
    std::string exportedWindowName = winSystem->GetExportedWindowName();
    p["option"]["windowId"] = exportedWindowName;
  }
  else
  {
    auto buffer = static_cast<CStarfishVideoBuffer*>(m_picture.videoBuffer);
    if (buffer->m_acbId)
    {
      AcbAPI_finalize(buffer->m_acbId);
      AcbAPI_destroy(buffer->m_acbId);
      buffer->m_taskId = 0;
    }
    buffer->m_acbId = AcbAPI_create();
    if (buffer->m_acbId)
    {
      if (!AcbAPI_initialize(buffer->m_acbId, PLAYER_TYPE_MSE, getenv("APPID"), &AcbCallback))
      {
        AcbAPI_destroy(buffer->m_acbId);
        buffer->m_acbId = 0;
        buffer->m_taskId = 0;
      }
    }
  }

  p["option"]["appId"] = CCompileInfo::GetPackage();
  contents["codec"]["video"] = ms_codecMap.at(videoHint.codec).data();

  if (audioHint.codec == AV_CODEC_ID_NONE)
    p["option"]["needAudio"] = false;
  else if (!ms_codecMap.contains(audioHint.codec))
  {
    std::unique_lock lock(m_audioCriticalSection);
    m_audioCodec = std::make_unique<CDVDAudioCodecFFmpeg>(m_processInfo);
    CDVDCodecOptions options;
    m_audioCodec->Open(audioHint, options);
    m_audioEncoder = std::make_unique<CAEEncoderFFmpeg>();
    contents["codec"]["audio"] = "AC3";
  }
  else
  {
    contents["codec"]["audio"] = ms_codecMap.at(audioHint.codec).data();
  }
  contents["format"] = "RAW";
  p["mediaTransportType"] = "BUFFERSTREAM";
  contents["provider"] = CCompileInfo::GetPackage();
  p["option"]["transmission"]["contentsType"] = "LIVE";
  p["option"]["transmission"]["trickType"] = "client-side";
  p["option"]["seekMode"] = "late_Iframe";
  p["option"]["useDroppedFrameEvent"] = true;
  p["option"]["externalStreamingInfo"]["streamQualityInfo"] = true;
  p["option"]["externalStreamingInfo"]["streamQualityInfoNonFlushable"] = true;
  p["option"]["externalStreamingInfo"]["streamQualityInfoCorruptedFrame"] = true;

  CVariant& esInfo = contents["esInfo"];
  esInfo["pauseAtDecodeTime"] = true;
  esInfo["seperatedPTS"] = true;
  esInfo["ptsToDecode"] = 0;
  esInfo["videoWidth"] = videoHint.width;
  esInfo["videoHeight"] = videoHint.height;
  esInfo["videoFpsValue"] = videoHint.fpsrate;
  esInfo["videoFpsScale"] = videoHint.fpsscale;

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

  if (audioHint.codec == AV_CODEC_ID_EAC3)
  {
    CVariant& ac3PlusInfo = contents["ac3PlusInfo"];
    ac3PlusInfo["channels"] = audioHint.channels;
    ac3PlusInfo["frequency"] = audioHint.samplerate / 1000.0;

    if (audioHint.profile == AV_PROFILE_EAC3_DDP_ATMOS)
    {
      ac3PlusInfo["channels"] = audioHint.channels - 2;
      contents["immersive"] = "ATMOS";
    }
  }
  else if (audioHint.codec == AV_CODEC_ID_DTS && ms_codecMap.contains(AV_CODEC_ID_DTS))
  {
    CVariant& dtsInfo = contents["dtsInfo"];
    dtsInfo["channels"] = audioHint.channels;
    dtsInfo["frequency"] = audioHint.samplerate / 1000.0;

    if (audioHint.profile == AV_PROFILE_DTS_ES)
      contents["codec"]["audio"] = "DTSE";
    if (audioHint.profile == AV_PROFILE_DTS_HD_MA_X ||
        audioHint.profile == AV_PROFILE_DTS_HD_MA_X_IMAX)
      contents["codec"]["audio"] = "DTSX";
  }
  payloadArgs["args"] = CVariant(CVariant::VariantTypeArray);
  payloadArgs["args"].push_back(std::move(p));

  std::string payload;
  CJSONVariantWriter::Write(payloadArgs, payload, true);

  m_mediaAPIs->notifyForeground();
  CLog::LogFC(LOGDEBUG, LOGVIDEO, "Sending Load payload {}", payload);
  if (!m_mediaAPIs->Load(payload.c_str(), &CMediaPipelineWebOS::PlayerCallback, this))
  {
    CLog::LogF(LOGERROR, "Load failed");
    m_messageQueueParent.Put(std::make_shared<CDVDMsg>(CDVDMsg::PLAYER_ABORT));
    return false;
  }

  SetHDR(videoHint);

  double fps = static_cast<double>(videoHint.fpsrate) / static_cast<double>(videoHint.fpsscale);
  m_clock.UpdateFramerate(fps);
  m_picture.iWidth = videoHint.width;
  m_picture.iHeight = videoHint.height;
  m_picture.iDisplayWidth = videoHint.width;
  m_picture.iDisplayHeight = videoHint.height;
  m_picture.stereoMode = videoHint.stereo_mode;
  m_picture.iDuration = 1000.0 / fps;

  const int sorient = m_processInfo.GetVideoSettings().m_Orientation;
  const int orientation =
      sorient != 0 ? (sorient + videoHint.orientation) % 360 : videoHint.orientation;

  if (!m_renderManager.Configure(m_picture, static_cast<float>(fps), orientation))
  {
    CLog::LogF(LOGERROR, "RenderManager configure failed");
    m_messageQueueParent.Put(std::make_shared<CDVDMsg>(CDVDMsg::PLAYER_ABORT));
    return false;
  }

  m_droppedFrames = 0;
  m_processInfo.SetVideoDecoderName(formatName, true);
  m_processInfo.SetVideoPixelFormat("Surface");
  m_processInfo.SetVideoDimensions(videoHint.width, videoHint.height);
  m_processInfo.SetVideoDeintMethod("hardware");
  m_processInfo.SetVideoDAR(static_cast<float>(videoHint.aspect));
  m_processInfo.SetVideoFps(static_cast<float>(fps));
  if (audioHint.codec != AV_CODEC_ID_NONE)
  {
    m_processInfo.SetAudioChannels(CAEUtil::GetAEChannelLayout(audioHint.channellayout));
    m_processInfo.SetAudioSampleRate(audioHint.samplerate);
    m_processInfo.SetAudioBitsPerSample(audioHint.bitspersample);
    if (ms_codecMap.contains(audioHint.codec))
      m_processInfo.SetAudioDecoderName(std::string("starfish-") +
                                        ms_codecMap.at(audioHint.codec).data());
    else if (m_audioEncoder)
      m_processInfo.SetAudioDecoderName("starfish-AC3 (transcoding)");
  }

  m_renderManager.ShowVideo(true);
  return true;
}

void CMediaPipelineWebOS::SetHDR(const CDVDStreamInfo& hint) const
{
  if (hint.hdrType == StreamHdrType::HDR_TYPE_NONE)
    return;

  CVariant hdrData;
  CVariant sei;

  if (ms_hdrInfoMap.contains(hint.colorTransferCharacteristic))
    hdrData["hdrType"] = ms_hdrInfoMap.at(hint.colorTransferCharacteristic).data();
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
  hdrData["sei"] = sei;

  CVariant vui;
  vui["transferCharacteristics"] = hint.colorTransferCharacteristic;
  vui["colorPrimaries"] = hint.colorPrimaries;
  vui["matrixCoeffs"] = hint.colorSpace;
  vui["videoFullRangeFlag"] = hint.colorRange == AVCOL_RANGE_JPEG;
  hdrData["vui"] = vui;

  std::string payload;
  CJSONVariantWriter::Write(hdrData, payload, true);

  CLog::LogFC(LOGDEBUG, LOGVIDEO, "Setting HDR data payload {}", payload);
  m_mediaAPIs->setHdrInfo(payload.c_str());
}

void CMediaPipelineWebOS::FeedAudioData(const std::shared_ptr<CDVDMsg>& msg)
{
  DemuxPacket* packet = std::static_pointer_cast<CDVDMsgDemuxerPacket>(msg)->GetPacket();

  const auto pts = std::chrono::duration_cast<std::chrono::nanoseconds>(
      std::chrono::duration<double, std::ratio<1, DVD_TIME_BASE>>(packet->pts));

  CVariant payload;
  payload["bufferAddr"] = fmt::format("{:#x}", reinterpret_cast<std::uintptr_t>(packet->pData));
  payload["bufferSize"] = packet->iSize;
  payload["pts"] = pts.count();
  payload["esData"] = 2;

  std::string json;
  CJSONVariantWriter::Write(payload, json, true);
  CLog::LogFC(LOGDEBUG, LOGVIDEO, "{}", json);

  std::string result = m_mediaAPIs->Feed(json.c_str());

  if (result.find("Ok") != std::string::npos)
  {
    m_audioStats.AddSampleBytes(packet->iSize);
    UpdateAudioInfo();
    m_audioFull = false;
    return;
  }

  if (result.find("BufferFull") != std::string::npos)
  {
    m_audioFull = true;
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
    if (!m_mediaAPIs->setTimeToDecode(payload.c_str()))
    {
      CLog::LogF(LOGERROR, "setTimeToDecode failed");
      auto player = static_cast<mediapipeline::CustomPlayer*>(m_mediaAPIs->player.get());
      auto pipeline = static_cast<mediapipeline::CustomPipeline*>(player->getPipeline().get());
      MEDIA_CUSTOM_CONTENT_INFO_T contentInfo;
      pipeline->loadSpi_getInfo(&contentInfo);
      contentInfo.ptsToDecode = pts.count();
      pipeline->setContentInfo(MEDIA_CUSTOM_SRC_TYPE_ES, &contentInfo);
    }

    auto player = static_cast<mediapipeline::CustomPlayer*>(m_mediaAPIs->player.get());
    auto pipeline = static_cast<mediapipeline::CustomPipeline*>(player->getPipeline().get());
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

    std::string result = m_mediaAPIs->Feed(json.c_str());

    if (result.find("Ok") != std::string::npos)
    {
      m_videoStats.AddSampleBytes(packet->iSize);
      UpdateVideoInfo();
      m_videoFull = false;
      return;
    }

    if (result.find("BufferFull") != std::string::npos)
    {
      m_videoFull = true;
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
  std::unique_lock<CCriticalSection> lock(m_overlayContainer);

  //Check all overlays and render those that should be rendered, based on time and forced
  //Both forced and subs should check timing
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

void CMediaPipelineWebOS::Process()
{
  while (!m_bStop)
  {
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
      else
      {
        CLog::LogF(LOGINFO, "Received video message: {}", msg->GetMessageType());
      }
    }
  }
}

void CMediaPipelineWebOS::ProcessAudio()
{
  m_audioStats.Start();
  while (!m_bStop)
  {
    std::shared_ptr<CDVDMsg> msg = nullptr;
    int priority = 0;
    m_messageQueueAudio.Get(msg, 10ms, priority);
    if (msg)
    {
      if (msg->IsType(CDVDMsg::DEMUXER_PACKET))
      {
        std::unique_lock lock(m_audioCriticalSection);
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
                  0, true, settings->GetBool(CSettings::SETTING_AUDIOOUTPUT_STEREOUPMIX),
                  !settings->GetBool(CSettings::SETTING_AUDIOOUTPUT_MAINTAINORIGINALVOLUME),
                  static_cast<float>(sublevel));
              m_audioResample->FillBuffer();

              AEAudioFormat input = m_audioCodec->GetFormat();
              input.m_frames = frame.nb_frames;
              m_encoderBuffers = std::make_unique<ActiveAE::CActiveAEBufferPool>(input);
              m_encoderBuffers->Create(0);
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
                auto p = std::make_shared<CDVDMsgDemuxerPacket>(new DemuxPacket{});
                p->m_packet->pts = static_cast<double>(buf->timestamp);
                p->m_packet->iSize = AC3_MAX_SYNC_FRAME_SIZE;
                p->m_packet->pData = new uint8_t[p->m_packet->iSize];
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
        CLog::LogF(LOGINFO, "Received audio message: {}", msg->GetMessageType());
      }
    }
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
      break;
    }
    case PF_EVENT_TYPE_STR_AUDIO_INFO:
      if (buffer->m_acbId)
        AcbAPI_setMediaAudioData(buffer->m_acbId, logStr.c_str(), &buffer->m_taskId);
      break;
    case PF_EVENT_TYPE_STR_VIDEO_INFO:
      if (buffer->m_acbId)
        AcbAPI_setMediaVideoData(buffer->m_acbId, logStr.c_str(), &buffer->m_taskId);
      break;
    case PF_EVENT_TYPE_STR_STATE_UPDATE__LOADCOMPLETED:
      if (buffer->m_acbId)
      {
        AcbAPI_setSinkType(buffer->m_acbId, SINK_TYPE_MAIN);
        AcbAPI_setMediaId(buffer->m_acbId, m_mediaAPIs->getMediaID());
        AcbAPI_setState(buffer->m_acbId, APPSTATE_FOREGROUND, PLAYSTATE_LOADED, &buffer->m_taskId);
      }
      m_renderManager.ShowVideo(true);
      m_mediaAPIs->Play();
      m_loaded = true;
      m_flushed = true;
      Create();
      m_audioThread = std::thread([this] { ProcessAudio(); });
      break;
    case PF_EVENT_TYPE_STR_STATE_UPDATE__UNLOADCOMPLETED:
      if (buffer->m_acbId)
        AcbAPI_setState(buffer->m_acbId, APPSTATE_FOREGROUND, PLAYSTATE_UNLOADED,
                        &buffer->m_taskId);
      StopThread(true);
      if (m_audioThread.joinable())
        m_audioThread.join();
      m_loaded = false;
      break;
    case PF_EVENT_TYPE_STR_STATE_UPDATE__PAUSED:
      if (buffer->m_acbId)
        AcbAPI_setState(buffer->m_acbId, APPSTATE_FOREGROUND, PLAYSTATE_PAUSED, &buffer->m_taskId);
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
      if (buffer->m_acbId)
        AcbAPI_setState(buffer->m_acbId, APPSTATE_FOREGROUND, PLAYSTATE_PLAYING, &buffer->m_taskId);
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
      m_bufferLevel = 100;
      break;
    }
    case PF_EVENT_TYPE_STR_BUFFERLOW:
    {
      m_bufferLevel = 0;
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
