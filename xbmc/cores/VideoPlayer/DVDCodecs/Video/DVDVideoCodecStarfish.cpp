/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DVDVideoCodecStarfish.h"

#include "CompileInfo.h"
#include "DVDCodecs/DVDFactoryCodec.h"
#include "ServiceBroker.h"
#include "cores/VideoPlayer/Buffers/VideoBuffer.h"
#include "cores/VideoPlayer/Interface/DemuxCrypto.h"
#include "cores/VideoPlayer/Interface/TimingConstants.h"
#include "cores/VideoPlayer/VideoRenderers/RenderManager.h"
#include "media/decoderfilter/DecoderFilterManager.h"
#include "messaging/ApplicationMessenger.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/BitstreamConverter.h"
#include "utils/CPUInfo.h"
#include "utils/JSONVariantWriter.h"
#include "utils/log.h"
#include "windowing/wayland/WinSystemWaylandWebOS.h"

#include <memory>
#include <vector>

#include <appswitching-control-block/AcbAPI.h>
#include <player-factory/custompipeline.hpp>
#include <player-factory/customplayer.hpp>

using namespace KODI::MESSAGING;
using namespace std::chrono_literals;

namespace
{
constexpr unsigned int PRE_BUFFER_BYTES = 0;
constexpr unsigned int MAX_QUEUE_BUFFER_LEVEL = 1 * 1024 * 1024; // 1 MB
constexpr unsigned int MIN_BUFFER_LEVEL = 0;
constexpr unsigned int MAX_BUFFER_LEVEL = 0;
constexpr unsigned int MIN_SRC_BUFFER_LEVEL = 1 * 1024 * 1024; // 1 MB
constexpr unsigned int MAX_SRC_BUFFER_LEVEL = 8 * 1024 * 1024; // 8 MB
} // namespace

CDVDVideoCodecStarfish::CDVDVideoCodecStarfish(CProcessInfo& processInfo)
  : CDVDVideoCodec(processInfo), m_starfishMediaAPI(std::make_unique<StarfishMediaAPIs>())
{
  using namespace KODI::WINDOWING::WAYLAND;
  auto winSystem = static_cast<CWinSystemWaylandWebOS*>(CServiceBroker::GetWinSystem());
  if (!winSystem->SupportsExportedWindow())
  {
    m_acbId = AcbAPI_create();
    if (m_acbId)
    {
      if (!AcbAPI_initialize(m_acbId, PLAYER_TYPE_MSE, getenv("APPID"), &AcbCallback))
      {
        AcbAPI_destroy(m_acbId);
        m_acbId = 0;
      }
    }
  }
}

CDVDVideoCodecStarfish::~CDVDVideoCodecStarfish()
{
  Dispose();
}

std::unique_ptr<CDVDVideoCodec> CDVDVideoCodecStarfish::Create(CProcessInfo& processInfo)
{
  return std::make_unique<CDVDVideoCodecStarfish>(processInfo);
}

bool CDVDVideoCodecStarfish::Register()
{
  CDVDFactoryCodec::RegisterHWVideoCodec("starfish_dec", &CDVDVideoCodecStarfish::Create);
  return true;
}

std::atomic<bool> CDVDVideoCodecStarfish::ms_instanceGuard(false);

bool CDVDVideoCodecStarfish::Open(CDVDStreamInfo& hints, CDVDCodecOptions& options)
{
  // allow only 1 instance here
  if (ms_instanceGuard.exchange(true))
  {
    CLog::LogF(LOGERROR, "CDVDVideoCodecStarfish: InstanceGuard locked");
    return false;
  }

  bool ok = OpenInternal(hints, options);

  if (!ok)
    ms_instanceGuard.exchange(false);

  return ok;
}

bool CDVDVideoCodecStarfish::OpenInternal(CDVDStreamInfo& hints, CDVDCodecOptions& options)
{
  m_opened = false;
  CVariant payloadArg;
  CVariant payloadArgs;

  if (hints.cryptoSession)
  {
    CLog::LogF(LOGERROR, "CDVDVideoCodecStarfish: CryptoSessions unsupported");
    return false;
  }

  if (!hints.width || !hints.height)
  {
    CLog::LogF(LOGERROR, "CDVDVideoCodecStarfish: {}", "null size, cannot handle");
    return false;
  }

  CLog::LogF(LOGDEBUG,
             "CDVDVideoCodecStarfish: hints: Width {} x Height {}, Fpsrate {} / Fpsscale {}, "
             "CodecID {}, Level {}, Profile {}, PTS_invalid {}, Tag {}, Extradata-Size: {}",
             hints.width, hints.height, hints.fpsrate, hints.fpsscale, hints.codec, hints.level,
             hints.profile, hints.ptsinvalid, hints.codec_tag, hints.extradata.GetSize());

  if (ms_codecMap.find(hints.codec) == ms_codecMap.cend() ||
      ms_formatInfoMap.find(hints.codec) == ms_formatInfoMap.cend())
  {
    CLog::LogF(LOGDEBUG, "CDVDVideoCodecStarfish: Unsupported hints.codec({})", hints.codec);
    return false;
  }

  m_codecname = ms_codecMap.at(hints.codec);
  m_formatname = ms_formatInfoMap.at(hints.codec);

  m_hints = hints;
  switch (m_hints.codec)
  {
    case AV_CODEC_ID_AVS:
    case AV_CODEC_ID_CAVS:
    case AV_CODEC_ID_H264:
      // check for h264-avcC and convert to h264-annex-b
      if (m_hints.extradata && !m_hints.cryptoSession)
      {
        m_bitstream = std::make_unique<CBitstreamConverter>();
        if (!m_bitstream->Open(m_hints.codec, m_hints.extradata.GetData(),
                               m_hints.extradata.GetSize(), true))
        {
          m_bitstream.reset();
        }
      }
      break;
    case AV_CODEC_ID_HEVC:
    {
      bool isDvhe = (m_hints.codec_tag == MKTAG('d', 'v', 'h', 'e'));
      bool isDvh1 = (m_hints.codec_tag == MKTAG('d', 'v', 'h', '1'));

      // some files don't have dvhe or dvh1 tag set up but have Dolby Vision side data
      if (!isDvhe && !isDvh1 && m_hints.hdrType == StreamHdrType::HDR_TYPE_DOLBYVISION)
      {
        // page 10, table 2 from https://professional.dolby.com/siteassets/content-creation/dolby-vision-for-content-creators/dolby-vision-streams-within-the-http-live-streaming-format-v2.0-13-november-2018.pdf
        if (m_hints.codec_tag == MKTAG('h', 'v', 'c', '1'))
          isDvh1 = true;
        else
          isDvhe = true;
      }

      if (isDvhe || isDvh1)
      {
        m_formatname = isDvhe ? "starfish-dvhe" : "starfish-dvh1";

        payloadArg["option"]["externalStreamingInfo"]["contents"]["DolbyHdrInfo"]
                  ["encryptionType"] = "clear"; //"clear", "bl", "el", "all"
        payloadArg["option"]["externalStreamingInfo"]["contents"]["DolbyHdrInfo"]["profileId"] =
            m_hints.dovi.dv_profile; // profile 0-9
        payloadArg["option"]["externalStreamingInfo"]["contents"]["DolbyHdrInfo"]["trackType"] =
            m_hints.dovi.el_present_flag ? "dual" : "single"; // "single" / "dual"
      }

      // check for hevc-hvcC and convert to h265-annex-b
      if (m_hints.extradata && !m_hints.cryptoSession)
      {
        m_bitstream = std::make_unique<CBitstreamConverter>();
        if (!m_bitstream->Open(m_hints.codec, m_hints.extradata.GetData(),
                               m_hints.extradata.GetSize(), true))
        {
          m_bitstream.reset();
        }
      }

      break;
    }
    case AV_CODEC_ID_AV1:
    {
      if (m_hints.hdrType == StreamHdrType::HDR_TYPE_DOLBYVISION && m_hints.dovi.dv_profile == 10)
      {
        m_formatname = "starfish-dav1";

        payloadArg["option"]["externalStreamingInfo"]["contents"]["DolbyHdrInfo"]
                  ["encryptionType"] = "clear"; //"clear", "bl", "el", "all"
        payloadArg["option"]["externalStreamingInfo"]["contents"]["DolbyHdrInfo"]["profileId"] =
            m_hints.dovi.dv_profile; // profile 10
        payloadArg["option"]["externalStreamingInfo"]["contents"]["DolbyHdrInfo"]["trackType"] =
            "single"; // "single" / "dual"
      }

      break;
    }
    default:
      break;
  }

  m_starfishMediaAPI->notifyForeground();

  using namespace KODI::WINDOWING::WAYLAND;
  auto winSystem = static_cast<CWinSystemWaylandWebOS*>(CServiceBroker::GetWinSystem());

  payloadArg["mediaTransportType"] = "BUFFERSTREAM";
  if (winSystem->SupportsExportedWindow())
  {
    std::string exportedWindowName = winSystem->GetExportedWindowName();
    payloadArg["option"]["windowId"] = exportedWindowName;
  }
  payloadArg["option"]["appId"] = CCompileInfo::GetPackage();
  payloadArg["option"]["externalStreamingInfo"]["contents"]["codec"]["video"] = m_codecname;
  payloadArg["option"]["externalStreamingInfo"]["contents"]["esInfo"]["pauseAtDecodeTime"] = true;
  payloadArg["option"]["externalStreamingInfo"]["contents"]["esInfo"]["seperatedPTS"] = true;
  payloadArg["option"]["externalStreamingInfo"]["contents"]["esInfo"]["ptsToDecode"] = 0;
  payloadArg["option"]["externalStreamingInfo"]["contents"]["esInfo"]["videoWidth"] = m_hints.width;
  payloadArg["option"]["externalStreamingInfo"]["contents"]["esInfo"]["videoHeight"] =
      m_hints.height;
  payloadArg["option"]["externalStreamingInfo"]["contents"]["esInfo"]["videoFpsValue"] =
      m_hints.fpsrate;
  payloadArg["option"]["externalStreamingInfo"]["contents"]["esInfo"]["videoFpsScale"] =
      m_hints.fpsscale;
  payloadArg["option"]["externalStreamingInfo"]["contents"]["format"] = "RAW";

  CVariant& bufferingCtrInfo = payloadArg["option"]["externalStreamingInfo"]["bufferingCtrInfo"];
  bufferingCtrInfo["preBufferByte"] = PRE_BUFFER_BYTES;
  bufferingCtrInfo["bufferMinLevel"] = MIN_BUFFER_LEVEL;
  bufferingCtrInfo["bufferMaxLevel"] = MAX_BUFFER_LEVEL;
  bufferingCtrInfo["qBufferLevelVideo"] = MAX_QUEUE_BUFFER_LEVEL;
  bufferingCtrInfo["srcBufferLevelVideo"]["minimum"] = MIN_SRC_BUFFER_LEVEL;
  bufferingCtrInfo["srcBufferLevelVideo"]["maximum"] = MAX_SRC_BUFFER_LEVEL;

  payloadArg["option"]["transmission"]["contentsType"] = "LIVE"; // "LIVE", "WebRTC"
  payloadArg["option"]["needAudio"] = false;
  payloadArg["option"]["seekMode"] = "late_Iframe";

  payloadArgs["args"] = CVariant(CVariant::VariantTypeArray);
  payloadArgs["args"].push_back(std::move(payloadArg));

  std::string payload;
  CJSONVariantWriter::Write(payloadArgs, payload, true);

  CLog::LogFC(LOGDEBUG, LOGVIDEO, "CDVDVideoCodecStarfish: Sending Load payload {}", payload);
  if (!m_starfishMediaAPI->Load(payload.c_str(), &CDVDVideoCodecStarfish::PlayerCallback, this))
  {
    CLog::LogF(LOGERROR, "CDVDVideoCodecStarfish: Load failed");
    return false;
  }

  SetHDR();

  m_codecControlFlags = 0;

  CLog::LogF(LOGINFO, "CDVDVideoCodecStarfish: Starfish {}", m_codecname);

  // first make sure all properties are reset.
  m_videobuffer.Reset();

  m_videobuffer.iWidth = m_hints.width;
  m_videobuffer.iHeight = m_hints.height;
  m_videobuffer.iDisplayWidth = m_hints.width;
  m_videobuffer.iDisplayHeight = m_hints.height;
  m_videobuffer.stereoMode = m_hints.stereo_mode;
  CStarfishVideoBuffer* starfishVideoBuffer = new CStarfishVideoBuffer(0);
  m_videobuffer.videoBuffer = starfishVideoBuffer;
  starfishVideoBuffer->m_acbId = m_acbId;

  m_opened = true;

  m_processInfo.SetVideoDecoderName(m_formatname, true);
  m_processInfo.SetVideoPixelFormat("Surface");
  m_processInfo.SetVideoDimensions(m_hints.width, m_hints.height);
  m_processInfo.SetVideoDeintMethod("hardware");
  m_processInfo.SetVideoDAR(m_hints.aspect);

  UpdateFpsDuration();

  return true;
}

void CDVDVideoCodecStarfish::Dispose()
{
  if (!m_opened)
    return;
  m_opened = false;

  m_starfishMediaAPI->Unload();

  if (m_acbId)
  {
    AcbAPI_finalize(m_acbId);
    AcbAPI_destroy(m_acbId);
  }

  ms_instanceGuard.exchange(false);
}

bool CDVDVideoCodecStarfish::AddData(const DemuxPacket& packet)
{

  if (!m_opened)
    return false;

  auto pts = std::chrono::duration_cast<std::chrono::nanoseconds>(
      std::chrono::duration<double, std::ratio<1, DVD_TIME_BASE>>(packet.pts));
  auto dts = std::chrono::duration_cast<std::chrono::nanoseconds>(
      std::chrono::duration<double, std::ratio<1, DVD_TIME_BASE>>(packet.dts));

  CLog::LogFC(LOGDEBUG, LOGVIDEO,
              "CDVDVideoCodecStarfish: dts:{} ns pts:{} ns sz:{} current state {}", dts.count(),
              pts.count(), packet.iSize, m_state);

  if (packet.dts == DVD_NOPTS_VALUE)
    dts = 0ns;

  if (m_hints.ptsinvalid)
    pts = dts;

  uint8_t* pData = packet.pData;
  size_t iSize = packet.iSize;

  // we have an input buffer, fill it.
  if (pData && m_bitstream)
  {
    m_bitstream->Convert(pData, iSize);

    if (m_state == StarfishState::FLUSHED && !m_bitstream->CanStartDecode())
    {
      CLog::LogF(LOGDEBUG, "CDVDVideoCodecStarfish: Waiting for keyframe (bitstream)");
      return true;
    }

    iSize = m_bitstream->GetConvertSize();
    pData = m_bitstream->GetConvertBuffer();
  }

  if (m_state == StarfishState::FLUSHED)
  {
    if (pts > 0ns)
    {
      auto player = static_cast<mediapipeline::CustomPlayer*>(m_starfishMediaAPI->player.get());
      auto pipeline = static_cast<mediapipeline::CustomPipeline*>(player->getPipeline().get());
      MEDIA_CUSTOM_CONTENT_INFO_T contentInfo;
      pipeline->loadSpi_getInfo(&contentInfo);
      contentInfo.ptsToDecode = pts.count();
      pipeline->setContentInfo(MEDIA_CUSTOM_SRC_TYPE_ES, &contentInfo);
      pipeline->sendSegmentEvent();
      m_currentPlaytime = pts;
      m_newFrame = true;
    }
    m_state = StarfishState::RUNNING;
  }

  if (pData && iSize)
  {
    CVariant payload;
    payload["bufferAddr"] = fmt::format("{:#x}", reinterpret_cast<std::uintptr_t>(pData));
    payload["bufferSize"] = iSize;
    payload["pts"] = pts.count();
    payload["esData"] = 1;

    std::string json;
    CJSONVariantWriter::Write(payload, json, true);

    std::string result = m_starfishMediaAPI->Feed(json.c_str());

    if (result.find("Ok") != std::string::npos)
      return true;

    if (result.find("BufferFull") != std::string::npos)
      return false;

    CLog::LogF(LOGWARNING, "CDVDVideoCodecStarfish: Buffer submit returned error: {}", result);
  }

  return true;
}

void CDVDVideoCodecStarfish::Reset()
{
  if (!m_opened)
    return;

  CLog::LogF(LOGDEBUG, "CDVDVideoCodecStarfish: Reset");
  m_starfishMediaAPI->flush();

  m_state = StarfishState::FLUSHED;

  // Invalidate our local VideoPicture bits
  m_videobuffer.pts = DVD_NOPTS_VALUE;

  if (m_bitstream)
    m_bitstream->ResetStartDecode();
}

bool CDVDVideoCodecStarfish::Reconfigure(CDVDStreamInfo& hints)
{
  if (m_hints.Equal(hints, CDVDStreamInfo::COMPARE_ALL &
                               ~(CDVDStreamInfo::COMPARE_ID | CDVDStreamInfo::COMPARE_EXTRADATA)))
  {
    CLog::LogF(LOGDEBUG, "CDVDVideoCodecStarfish: true");
    m_hints = hints;
    return true;
  }
  CLog::LogF(LOGDEBUG, "CDVDVideoCodecStarfish: false");
  return false;
}

CDVDVideoCodec::VCReturn CDVDVideoCodecStarfish::GetPicture(VideoPicture* pVideoPicture)
{
  if (!m_opened)
    return VC_NONE;

  if (m_state == StarfishState::ERROR)
    return VC_ERROR;

  if (m_state == StarfishState::EOS)
    return VC_EOF;

  if (m_state == StarfishState::FLUSHED || !m_newFrame)
    return VC_BUFFER;

  pVideoPicture->videoBuffer = nullptr;
  pVideoPicture->SetParams(m_videobuffer);
  pVideoPicture->videoBuffer = m_videobuffer.videoBuffer;
  pVideoPicture->dts = 0;
  using dvdTime = std::ratio<1, DVD_TIME_BASE>;
  pVideoPicture->pts =
      std::chrono::duration_cast<std::chrono::duration<double, dvdTime>>(m_currentPlaytime).count();
  m_newFrame = false;

  CLog::LogFC(LOGDEBUG, LOGVIDEO, "CDVDVideoCodecStarfish: pts:{:0.4f}", pVideoPicture->pts);

  return VC_PICTURE;
}

void CDVDVideoCodecStarfish::SetCodecControl(int flags)
{
  if (m_codecControlFlags != flags)
  {
    CLog::LogFC(LOGDEBUG, LOGVIDEO, "CDVDVideoCodecStarfish: {:x}->{:x}", m_codecControlFlags,
                flags);
    m_codecControlFlags = flags;
  }
}

void CDVDVideoCodecStarfish::SetSpeed(int iSpeed)
{
  switch (iSpeed)
  {
    case DVD_PLAYSPEED_NORMAL:
      m_starfishMediaAPI->Play();
      break;
    case DVD_PLAYSPEED_PAUSE:
      m_starfishMediaAPI->Pause();
      break;
    default:
      CLog::LogF(LOGWARNING, "CDVDVideoCodecStarfish: Unknown playback speed");
      break;
  }
}

void CDVDVideoCodecStarfish::SetHDR()
{
  if (m_hints.masteringMetadata)
  {
    CVariant hdrData;

    if (ms_hdrInfoMap.find(m_hints.colorTransferCharacteristic) != ms_hdrInfoMap.cend())
      hdrData["hdrType"] = ms_hdrInfoMap.at(m_hints.colorTransferCharacteristic).data();
    else
      hdrData["hdrType"] = "none";

    CVariant sei;
    // for more information, see CTA+861.3-A standard document
    constexpr double maxChromaticity = 50000;
    constexpr double maxLuminance = 10000;
    sei["displayPrimariesX0"] = static_cast<unsigned short>(
        av_q2d(m_hints.masteringMetadata->display_primaries[0][0]) * maxChromaticity + 0.5);
    sei["displayPrimariesY0"] = static_cast<unsigned short>(
        av_q2d(m_hints.masteringMetadata->display_primaries[0][1]) * maxChromaticity + 0.5);
    sei["displayPrimariesX1"] = static_cast<unsigned short>(
        av_q2d(m_hints.masteringMetadata->display_primaries[1][0]) * maxChromaticity + 0.5);
    sei["displayPrimariesY1"] = static_cast<unsigned short>(
        av_q2d(m_hints.masteringMetadata->display_primaries[1][1]) * maxChromaticity + 0.5);
    sei["displayPrimariesX2"] = static_cast<unsigned short>(
        av_q2d(m_hints.masteringMetadata->display_primaries[2][0]) * maxChromaticity + 0.5);
    sei["displayPrimariesY2"] = static_cast<unsigned short>(
        av_q2d(m_hints.masteringMetadata->display_primaries[2][1]) * maxChromaticity + 0.5);
    sei["whitePointX"] = static_cast<unsigned short>(
        av_q2d(m_hints.masteringMetadata->white_point[0]) * maxChromaticity + 0.5);
    sei["whitePointY"] = static_cast<unsigned short>(
        av_q2d(m_hints.masteringMetadata->white_point[1]) * maxChromaticity + 0.5);
    sei["minDisplayMasteringLuminance"] =
        static_cast<unsigned short>(av_q2d(m_hints.masteringMetadata->min_luminance) + 0.5);
    sei["maxDisplayMasteringLuminance"] = static_cast<unsigned short>(
        av_q2d(m_hints.masteringMetadata->max_luminance) * maxLuminance + 0.5);
    // we can have HDR content that does not provide content light level metadata
    if (m_hints.contentLightMetadata)
    {
      sei["maxContentLightLevel"] =
          static_cast<unsigned short>(m_hints.contentLightMetadata->MaxCLL);
      sei["maxPicAverageLightLevel"] =
          static_cast<unsigned short>(m_hints.contentLightMetadata->MaxFALL);
    }
    hdrData["sei"] = sei;

    CVariant vui;
    vui["transferCharacteristics"] = static_cast<int>(m_hints.colorTransferCharacteristic);
    vui["colorPrimaries"] = static_cast<int>(m_hints.colorPrimaries);
    vui["matrixCoeffs"] = static_cast<int>(m_hints.colorSpace);
    vui["videoFullRangeFlag"] = m_hints.colorRange == AVCOL_RANGE_JPEG;
    hdrData["vui"] = vui;

    std::string payload;
    CJSONVariantWriter::Write(hdrData, payload, true);

    CLog::LogFC(LOGDEBUG, LOGVIDEO, "CDVDVideoCodecStarfish: Setting HDR data payload {}", payload);
    m_starfishMediaAPI->setHdrInfo(payload.c_str());
  }
}

void CDVDVideoCodecStarfish::UpdateFpsDuration()
{
  m_processInfo.SetVideoFps(static_cast<float>(m_hints.fpsrate) / m_hints.fpsscale);

  CLog::LogF(LOGDEBUG, "CDVDVideoCodecStarfish: fpsRate:{} fpsscale:{}", m_hints.fpsrate,
             m_hints.fpsscale);
}

void CDVDVideoCodecStarfish::PlayerCallback(const int32_t type,
                                            const int64_t numValue,
                                            const char* strValue)
{
  std::string logstr = strValue != nullptr ? strValue : "";
  CLog::LogF(LOGDEBUG, "CDVDVideoCodecStarfish: type: {}, numValue: {}, strValue: {}", type,
             numValue, logstr);

  switch (type)
  {
    case PF_EVENT_TYPE_FRAMEREADY:
      m_currentPlaytime = std::chrono::nanoseconds(numValue);
      m_newFrame = true;
      break;
    case PF_EVENT_TYPE_STR_RESOURCE_INFO:
      m_newFrame = true;
      break;
    case PF_EVENT_TYPE_STR_VIDEO_INFO:
      if (m_acbId)
        AcbAPI_setMediaVideoData(m_acbId, logstr.c_str());
      break;
    case PF_EVENT_TYPE_STR_STATE_UPDATE__LOADCOMPLETED:
      if (m_acbId)
      {
        AcbAPI_setSinkType(m_acbId, SINK_TYPE_MAIN);
        AcbAPI_setMediaId(m_acbId, m_starfishMediaAPI->getMediaID());
        AcbAPI_setState(m_acbId, APPSTATE_FOREGROUND, PLAYSTATE_LOADED, nullptr);
      }
      m_starfishMediaAPI->Play();
      m_state = StarfishState::FLUSHED;
      break;
    case PF_EVENT_TYPE_STR_STATE_UPDATE__PLAYING:
      if (m_acbId)
        AcbAPI_setState(m_acbId, APPSTATE_FOREGROUND, PLAYSTATE_PLAYING, nullptr);
      break;
    case PF_EVENT_TYPE_STR_STATE_UPDATE__PAUSED:
      if (m_acbId)
        AcbAPI_setState(m_acbId, APPSTATE_FOREGROUND, PLAYSTATE_PAUSED, nullptr);
      break;
    case PF_EVENT_TYPE_STR_STATE_UPDATE__ENDOFSTREAM:
      m_state = StarfishState::EOS;
      break;
    case PF_EVENT_TYPE_STR_STATE_UPDATE__UNLOADCOMPLETED:
      if (m_acbId)
        AcbAPI_setState(m_acbId, APPSTATE_FOREGROUND, PLAYSTATE_UNLOADED, nullptr);
      break;
    case PF_EVENT_TYPE_INT_ERROR:
      CLog::LogF(LOGERROR, "CDVDVideoCodecStarfish: Pipeline INT_ERROR numValue: {}, strValue: {}",
                 numValue, logstr);
      m_state = StarfishState::ERROR;
      break;
    case PF_EVENT_TYPE_STR_ERROR:
      CLog::LogF(LOGERROR, "CDVDVideoCodecStarfish: Pipeline STR_ERROR numValue: {}, strValue: {}",
                 numValue, logstr);
      m_state = StarfishState::ERROR;
      break;
  }
}

void CDVDVideoCodecStarfish::PlayerCallback(const int32_t type,
                                            const int64_t numValue,
                                            const char* strValue,
                                            void* data)
{
  static_cast<CDVDVideoCodecStarfish*>(data)->PlayerCallback(type, numValue, strValue);
}

void CDVDVideoCodecStarfish::AcbCallback(
    long acbId, long taskId, long eventType, long appState, long playState, const char* reply)
{
  CLog::LogF(LOGDEBUG,
             "ACB callback: acbId={}, taskId={}, eventType={}, appState={}, playState={}, reply={}",
             acbId, taskId, eventType, appState, playState, reply);
}