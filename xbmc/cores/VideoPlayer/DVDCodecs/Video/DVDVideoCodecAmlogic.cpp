/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include <math.h>

#include "DVDCodecs/DVDFactoryCodec.h"
#include "utils/MemUtils.h"
#include "DVDVideoCodecAmlogic.h"
#include "cores/VideoPlayer/Interface/TimingConstants.h"
#include "DVDStreamInfo.h"
#include "AMLCodec.h"
#include "ServiceBroker.h"
#include "utils/AMLUtils.h"
#include "utils/log.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "threads/Thread.h"

CAMLVideoBufferPool::~CAMLVideoBufferPool()
{
  logM(LOGDEBUG, "CAMLVideoBufferPool", "Deleting {:d} buffers", static_cast<unsigned int>(m_videoBuffers.size()));
  for (auto buffer : m_videoBuffers)
    delete buffer;
}

CVideoBuffer* CAMLVideoBufferPool::Get()
{
  std::lock_guard lock(m_criticalSection);

  if (m_freeBuffers.empty())
  {
    m_freeBuffers.push_back(m_videoBuffers.size());
    m_videoBuffers.push_back(new CAMLVideoBuffer(static_cast<int>(m_videoBuffers.size())));
  }
  int bufferIdx(m_freeBuffers.back());
  m_freeBuffers.pop_back();

  m_videoBuffers[bufferIdx]->Acquire(shared_from_this());

  return m_videoBuffers[bufferIdx];
}

void CAMLVideoBufferPool::Return(int id)
{
  std::lock_guard lock(m_criticalSection);

  if (m_videoBuffers[id]->m_amlCodec)
  {
    m_videoBuffers[id]->m_amlCodec->ReleaseFrame(m_videoBuffers[id]->m_bufferIndex, true);
    m_videoBuffers[id]->m_amlCodec = nullptr;
  }
  m_freeBuffers.push_back(id);
}

/***************************************************************************/

CDVDVideoCodecAmlogic::CDVDVideoCodecAmlogic(CProcessInfo &processInfo)
  : CDVDVideoCodec(processInfo)
  , m_pFormatName("amcodec")
  , m_opened(false)
  , m_codecControlFlags(0)
  , m_framerate(0.0)
  , m_video_rate(0)
  , m_has_keyframe(false)
{
}

CDVDVideoCodecAmlogic::~CDVDVideoCodecAmlogic()
{
  Close();
}

std::unique_ptr<CDVDVideoCodec> CDVDVideoCodecAmlogic::Create(CProcessInfo& processInfo)
{
  return std::make_unique<CDVDVideoCodecAmlogic>(processInfo);
}

bool CDVDVideoCodecAmlogic::Register()
{
  CDVDFactoryCodec::RegisterHWVideoCodec("amlogic_dec", CDVDVideoCodecAmlogic::Create);
  return true;
}

bool CDVDVideoCodecAmlogic::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  if (!CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_VIDEOPLAYER_USEAMCODEC))
    return false;
  if ((hints.stills && hints.fpsrate == 0) || hints.width == 0)
    return false;

  // close open decoder if necessary
  if (m_opened)
    Close();

  m_hints = hints;
  m_hints.pClock = hints.pClock;

  logM(LOGDEBUG, "CDVDVideoCodecAmlogic", "codec {:d} profile:{:d} extra_size:{:d} fps:{:d}/{:d}",
    m_hints.codec, m_hints.profile, m_hints.extradata.GetSize(), m_hints.fpsrate, m_hints.fpsscale);

  switch(m_hints.codec)
  {
    case AV_CODEC_ID_MJPEG:
      m_pFormatName = "am-mjpeg";
      break;
    case AV_CODEC_ID_MPEG1VIDEO:
    case AV_CODEC_ID_MPEG2VIDEO:
      if (m_hints.width <= CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_VIDEOPLAYER_USEAMCODECMPEG2))
        goto FAIL;

      switch(m_hints.profile)
      {
        case FF_PROFILE_MPEG2_422:
          logM(LOGDEBUG, "CDVDVideoCodecAmlogic", "MPEG2 unsupported hints.profile({:d})", m_hints.profile);
          goto FAIL;
      }

      // if we have SD PAL content assume it is widescreen
      // correct aspect ratio will be detected later anyway
      if ((m_hints.width == 720 || m_hints.width == 544 || m_hints.width == 480) && m_hints.height == 576 && m_hints.aspect == 0.0)
          m_hints.aspect = 16.0 / 9.0;

      m_mpeg2_sequence_pts = 0;
      m_mpeg2_sequence = std::make_unique<mpeg2_sequence>();
      m_mpeg2_sequence->width  = m_hints.width;
      m_mpeg2_sequence->height = m_hints.height;
      m_mpeg2_sequence->ratio  = m_hints.aspect;
      m_mpeg2_sequence->fps_rate  = m_hints.fpsrate;
      m_mpeg2_sequence->fps_scale  = m_hints.fpsscale;
      m_pFormatName = "am-mpeg2";
      break;
    case AV_CODEC_ID_H264:
      if (m_hints.width <= CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_VIDEOPLAYER_USEAMCODECH264))
      {
        logM(LOGDEBUG, "CDVDVideoCodecAmlogic", "h264 size check failed {:d}", CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_VIDEOPLAYER_USEAMCODECH264));
        goto FAIL;
      }
      switch(hints.profile)
      {
        case FF_PROFILE_H264_HIGH_10:
        case FF_PROFILE_H264_HIGH_10_INTRA:
        case FF_PROFILE_H264_HIGH_422:
        case FF_PROFILE_H264_HIGH_422_INTRA:
        case FF_PROFILE_H264_HIGH_444_PREDICTIVE:
        case FF_PROFILE_H264_HIGH_444_INTRA:
        case FF_PROFILE_H264_CAVLC_444:
          logM(LOGDEBUG, "CDVDVideoCodecAmlogic", "H264 unsupported hints.profile({:d})", m_hints.profile);
          goto FAIL;
      }
      if ((aml_support_h264_4k2k() == AML_NO_H264_4K2K) && ((m_hints.width > 1920) || (m_hints.height > 1088)))
      {
        logM(LOGDEBUG, "CDVDVideoCodecAmlogic", "4K H264 is supported only on Amlogic S802 and S812 chips or newer");
        goto FAIL;
      }

      if (m_hints.aspect == 0.0)
      {
        m_h264_sequence_pts = 0;
        m_h264_sequence = std::make_unique<h264_sequence>();
        m_h264_sequence->width  = m_hints.width;
        m_h264_sequence->height = m_hints.height;
        m_h264_sequence->ratio  = m_hints.aspect;
      }

      if (m_hints.codec_tag == MKTAG('M', 'V', 'C', '1'))
        m_pFormatName = "am-h264mvc";
      else
        m_pFormatName = "am-h264";

      // convert h264-avcC to h264-annex-b as h264-avcC
      // under streamers can have issues when seeking.
      if (m_hints.extradata && m_hints.extradata.GetData()[0] == 1)
      {
        m_bitstream = std::make_unique<CBitstreamConverter>(m_hints);
        m_bitstream->Open(true);
        m_bitstream->ResetStartDecode();
        // make sure we do not leak the existing m_hints.extradata
        m_hints.extradata = {};
        m_hints.extradata = FFmpegExtraData(m_bitstream->GetExtraSize());
        memcpy(m_hints.extradata.GetData(), m_bitstream->GetExtraData(), m_hints.extradata.GetSize());
      }
      else
      {
        m_bitparser = std::make_unique<CBitstreamParser>();
        m_bitparser->Open();
      }

      // if we have SD PAL content assume it is widescreen
      // correct aspect ratio will be detected later anyway
      if (m_hints.width == 720 && m_hints.height == 576 && m_hints.aspect == 0.0)
          m_hints.aspect = 16.0 / 9.0;

      // assume widescreen for "HD Lite" channels
      // correct aspect ratio will be detected later anyway
      if ((m_hints.width == 1440 || m_hints.width ==1280) && m_hints.height == 1080 && m_hints.aspect == 0.0)
          m_hints.aspect = 16.0 / 9.0;;

      break;
    case AV_CODEC_ID_MPEG4:
    case AV_CODEC_ID_MSMPEG4V2:
    case AV_CODEC_ID_MSMPEG4V3:
      if (m_hints.width <= CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_VIDEOPLAYER_USEAMCODECMPEG4))
        goto FAIL;
      m_pFormatName = "am-mpeg4";
      break;
    case AV_CODEC_ID_H263:
    case AV_CODEC_ID_H263P:
    case AV_CODEC_ID_H263I:
      // amcodec can't handle h263
      logM(LOGDEBUG, "CDVDVideoCodecAmlogic", "amcodec does not support H263");
      goto FAIL;
//    case AV_CODEC_ID_FLV1:
//      m_pFormatName = "am-flv1";
//      break;
    case AV_CODEC_ID_RV10:
    case AV_CODEC_ID_RV20:
    case AV_CODEC_ID_RV30:
    case AV_CODEC_ID_RV40:
      // m_pFormatName = "am-rv";
      // rmvb is not handled well by amcodec
      logM(LOGDEBUG, "CDVDVideoCodecAmlogic", "amcodec does not support RMVB");
      goto FAIL;
    case AV_CODEC_ID_VC1:
    case AV_CODEC_ID_WMV3:
      if (m_hints.width <= CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(
        CSettings::SETTING_VIDEOPLAYER_USEAMCODECVC1))
      {
        if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(
          CSettings::SETTING_VIDEOPLAYER_USEAMCODECVC1) != 9998)
        {
          logM(LOGDEBUG, "CDVDVideoCodecAmlogic", "vc1 {:d} disabled by user",
            CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(
            CSettings::SETTING_VIDEOPLAYER_USEAMCODECVC1));
          goto FAIL;
        }
        else if (m_hints.fpsrate <= 24000)
        {
          logM(LOGDEBUG, "CDVDVideoCodecAmlogic", "vc1 {:d} disabled by user",
            CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(
            CSettings::SETTING_VIDEOPLAYER_USEAMCODECVC1));
          goto FAIL;
        }
      }

      switch(m_hints.codec)
      {
        case AV_CODEC_ID_VC1:
          m_pFormatName = "am-vc1";
          break;
        case AV_CODEC_ID_WMV3:
          m_pFormatName = "am-wmv3";
          break;
        default:
          goto FAIL;
      }
      break;
    case AV_CODEC_ID_AVS:
    case AV_CODEC_ID_CAVS:
      m_pFormatName = "am-avs";
      break;
    case AV_CODEC_ID_VP9:
      if (!aml_support_vp9())
      {
        logM(LOGDEBUG, "CDVDVideoCodecAmlogic", "VP9 hardware decoder is not supported on current platform");
        goto FAIL;
      }
      m_pFormatName = "am-vp9";
      break;
    case AV_CODEC_ID_AV1:
      if (!aml_support_av1())
      {
        logM(LOGDEBUG, "CDVDVideoCodecAmlogic", "AV1 hardware decoder is not supported on current platform");
        goto FAIL;
      }
      m_pFormatName = "am-av1";
      break;
    case AV_CODEC_ID_HEVC:
      if (aml_support_hevc()) {
        if (!aml_support_hevc_8k4k() && ((m_hints.width > 4096) || (m_hints.height > 2176)))
        {
          logM(LOGDEBUG, "CDVDVideoCodecAmlogic", "8K HEVC hardware decoder is not supported on current platform");
          goto FAIL;
        } else if (!aml_support_hevc_4k2k() && ((m_hints.width > 1920) || (m_hints.height > 1088)))
        {
          logM(LOGDEBUG, "CDVDVideoCodecAmlogic", "4K HEVC hardware decoder is not supported on current platform");
          goto FAIL;
        }
      } else {
        logM(LOGDEBUG, "CDVDVideoCodecAmlogic", "HEVC hardware decoder is not supported on current platform");
        goto FAIL;
      }
      if ((hints.profile == FF_PROFILE_HEVC_MAIN_10) && !aml_support_hevc_10bit())
      {
        logM(LOGDEBUG, "CDVDVideoCodecAmlogic", "HEVC 10-bit hardware decoder is not supported on current platform");
        goto FAIL;
      }
      m_pFormatName = "am-h265";
      m_bitstream = std::make_unique<CBitstreamConverter>(m_hints);
      m_bitstream->Open(true);

      // check for hevc-hvcC and convert to h265-annex-b - and DV is on.
      if (m_hints.extradata && !m_hints.cryptoSession && m_bitstream && (aml_dv_mode() != DV_MODE::OFF))
      {
        auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();

        bool dualPriorityHdr10Plus = (settings->GetInt(CSettings::SETTING_COREELEC_AMLOGIC_DV_DUAL_PRIORITY) == 1);

        if (m_hints.hdrType == StreamHdrType::HDR_TYPE_DOLBYVISION)
        {
          if (dualPriorityHdr10Plus)
          {
            logM(LOGINFO, "CDVDVideoCodecAmlogic", "DV HEVC bitstream - if stream also contains HDR10+, native HDR10+ has priority.");
            m_bitstream->SetDualPriorityHdr10Plus(true);
          }
          else if (settings->GetBool(CSettings::SETTING_COREELEC_AMLOGIC_DV_HDR10PLUS_CONVERT))
          {
            bool preferConvertHdr10Plus = settings->GetBool(CSettings::SETTING_COREELEC_AMLOGIC_DV_HDR10PLUS_PREFER_CONVERT);
            m_bitstream->SetPreferCovertHdr10Plus(preferConvertHdr10Plus);

            if (preferConvertHdr10Plus)
              logM(LOGINFO, "CDVDVideoCodecAmlogic", "DV HEVC bitstream - if stream also contains HDR10+, conversion will be preferred over original Dolby Vision.");
          }

          if (m_hints.dovi.dv_profile == 7)
          {
            auto convertDovi = static_cast<DOVIMode>(settings->GetInt(CSettings::SETTING_VIDEOPLAYER_CONVERTDOVI));
            if (convertDovi)
            {
              logM(LOGINFO, "CDVDVideoCodecAmlogic", "DV HEVC bitstream - user chooses to convert to mode [{:d}]", convertDovi);
              m_bitstream->SetConvertDovi(convertDovi);
            }
          }
        }

        // Potential HDR10+ (Cannot tell at this point)
        if (settings->GetBool(CSettings::SETTING_COREELEC_AMLOGIC_DV_HDR10PLUS_CONVERT))
        {
          auto peakBrightnessSource = static_cast<PeakBrightnessSource>(settings->GetInt(CSettings::SETTING_COREELEC_AMLOGIC_DV_HDR10PLUS_PEAK_BRIGHTNESS_SOURCE));
          logM(LOGINFO, "CDVDVideoCodecAmlogic", "HEVC bitstream - if also HDR10+ then will be considered for conversion to Dolby Vision P8.1 with brightness source [{:d}]",
               peakBrightnessSource);
          m_bitstream->SetConvertHdr10Plus(true);
          m_bitstream->SetConvertHdr10PlusPeakBrightnessSource(peakBrightnessSource);
        }

        // If HDR10 or Dual Priority HDR10+ and doing VS10 - remove the HDR10+ and DV if present to avoid conflict with VS10.
        if ((m_hints.hdrType == StreamHdrType::HDR_TYPE_HDR10) || dualPriorityHdr10Plus)
        {
          unsigned int mode(aml_vs10_by_setting(CSettings::SETTING_COREELEC_AMLOGIC_DV_VS10_HDR10PLUS));
          if (mode < DOLBY_VISION_OUTPUT_MODE_BYPASS)
          {
            // for VS10 conversion need to remove the HDR10plus metadata.
            logM(LOGINFO, "CDVDVideoCodecAmlogic", "HDR10 HEVC bitstream - if HDR10+ then metadata will be removed to allow correct VS10 processing");
            m_bitstream->SetRemoveHdr10Plus(true);
            m_bitstream->SetRemoveDovi(true);
          }
        }
      }

      // make sure we do not leak the existing m_hints.extradata
      m_hints.extradata = {};
      m_hints.extradata = FFmpegExtraData(m_bitstream->GetExtraSize());
      memcpy(m_hints.extradata.GetData(), m_bitstream->GetExtraData(), m_hints.extradata.GetSize());
      break;
    default:
      logM(LOGDEBUG, "CDVDVideoCodecAmlogic", "Unknown hints.codec({:d})", m_hints.codec);
      goto FAIL;
  }

  m_aspect_ratio = m_hints.aspect;

  m_Codec = std::shared_ptr<CAMLCodec>(new CAMLCodec(m_processInfo, m_hints));
  if (!m_Codec)
  {
    logM(LOGERROR, "CDVDVideoCodecAmlogic", "Failed to create Amlogic Codec");
    goto FAIL;
  }

  // allocate a dummy VideoPicture buffer.
  m_videobuffer.Reset();

  m_videobuffer.iWidth  = m_hints.width;
  m_videobuffer.iHeight = m_hints.height;

  m_videobuffer.iDisplayWidth  = m_videobuffer.iWidth;
  m_videobuffer.iDisplayHeight = m_videobuffer.iHeight;
  if (m_hints.aspect > 0.0 && !m_hints.forced_aspect)
  {
    m_videobuffer.iDisplayWidth  = ((int)lrint(m_videobuffer.iHeight * m_hints.aspect)) & ~3;
    if (m_videobuffer.iDisplayWidth > m_videobuffer.iWidth)
    {
      m_videobuffer.iDisplayWidth  = m_videobuffer.iWidth;
      m_videobuffer.iDisplayHeight = ((int)lrint(m_videobuffer.iWidth / m_hints.aspect)) & ~3;
    }
  }

  m_videobuffer.hdrType = m_hints.hdrType;
  m_videobuffer.color_space = m_hints.colorSpace;
  m_videobuffer.color_primaries = m_hints.colorPrimaries;
  m_videobuffer.color_transfer = m_hints.colorTransferCharacteristic;

  m_processInfo.SetVideoDecoderName(m_pFormatName, true);
  m_processInfo.SetVideoDimensions(m_hints.width, m_hints.height);
  m_processInfo.SetVideoDeintMethod("hardware");
  m_processInfo.SetVideoDAR(m_hints.aspect);

  m_has_keyframe = false;

  logM(LOGINFO, "CDVDVideoCodecAmlogic", "Opened Amlogic Codec");
  return true;
FAIL:
  Close();
  return false;
}

void CDVDVideoCodecAmlogic::Close(void)
{
  logNoFormatM(LOGINFO, "CDVDVideoCodecAmlogic");

  m_videoBufferPool = nullptr;

  if (m_Codec)
    m_Codec->CloseDecoder(false), m_Codec = nullptr;

  m_videobuffer.iFlags = 0;

  m_opened = false;

  ClearBitstreamCommon();
}

void CDVDVideoCodecAmlogic::ClearBitstreamCommon(void)
{
  while (!m_packages.empty())
  {
    KODI::MEMORY::AlignedFree(std::get<0>(m_packages.front()));
    m_packages.pop_front();
  }

  m_last_added = true;
  m_last_pData = nullptr;
  m_last_iSize = 0;
}

bool CDVDVideoCodecAmlogic::DualLayerConvert(uint8_t *pData, uint32_t iSize, const DemuxPacket &packet)
{
  bool dual_layer_converted = false;

  logComponentM(LOGDEBUG, LOGVIDEO, "CDVDVideoCodecAmlogic", "DT-DL {} package with dts: {:.3f}, pts: {:.3f} and size {} arrived, list {} empty",
    packet.isELPackage ? "EL" : "BL", packet.dts/DVD_TIME_BASE, packet.pts/DVD_TIME_BASE, iSize, m_packages.empty() ? "is" : "is not");

  if (!m_packages.empty())
  {
    // convert bl and el package to single package
    DLDemuxPacket dual_layer_packet = m_packages.front();
    uint8_t *pDataBackup = std::get<0>(dual_layer_packet);
    uint32_t iSizeBackup = std::get<1>(dual_layer_packet);
    bool isELPackageBackup = std::get<2>(dual_layer_packet);
    double dts = std::get<3>(dual_layer_packet);

    if (isELPackageBackup != packet.isELPackage)
    {
      logComponentM(LOGDEBUG, LOGVIDEO, "CDVDVideoCodecAmlogic", "found DT-DL {} package with dts: {:.3f} in list",
        packet.isELPackage ? "BL" : "EL", dts/DVD_TIME_BASE);

      if (packet.dts < dts) // prior dts arrived - out of step - remove and attempt next.
      {
        logComponentM(LOGDEBUG, LOGVIDEO, "CDVDVideoCodecAmlogic", "discarding DT-DL {} package with dts {:.3f} as before package in list with dts: {:.3f}",
          packet.isELPackage ? "EL" : "BL", packet.dts/DVD_TIME_BASE, dts/DVD_TIME_BASE);

        return false;
      }

      if (packet.isELPackage)
        dual_layer_converted = m_bitstream->Convert(pDataBackup, iSizeBackup, pData, iSize, packet.pts);
      else
        dual_layer_converted = m_bitstream->Convert(pData, iSize, pDataBackup, iSizeBackup, packet.pts);
    }
  }

  if (!dual_layer_converted) // backup package and don't send to decoder yet
  {
    auto pDataBackup = static_cast<uint8_t*>(KODI::MEMORY::AlignedMalloc(packet.iSize + AV_INPUT_BUFFER_PADDING_SIZE, 16));
    memcpy(pDataBackup, packet.pData, packet.iSize);
    m_packages.push_back(std::make_tuple(pDataBackup, iSize, packet.isELPackage, packet.dts));

    logComponentM(LOGDEBUG, LOGVIDEO, "CDVDVideoCodecAmlogic", "did add DT-DL {} package with dts: {:.3f}, pts: {:.3f} and size {} in list",
      packet.isELPackage ? "EL" : "BL", packet.dts/DVD_TIME_BASE, packet.pts/DVD_TIME_BASE, packet.iSize);

    return false;
  }
  else
  {
    logComponentM(LOGDEBUG, LOGVIDEO, "CDVDVideoCodecAmlogic", "converted DT-DL with dts: {:.3f}, pts: {:.3f}",
      packet.dts/DVD_TIME_BASE, packet.pts/DVD_TIME_BASE);

    // All good can remove the backed up package
    KODI::MEMORY::AlignedFree(std::get<0>(m_packages.front()));
    m_packages.pop_front();

    if (!m_bitstream->CanStartDecode())
    {
      logM(LOGDEBUG, "CDVDVideoCodecAmlogic", "waiting for keyframe (bitstream)");
      return false;
    }
  }

  return true;
}

bool CDVDVideoCodecAmlogic::SingleLayerConvert(uint8_t *pData, uint32_t iSize, const DemuxPacket &packet)
{
  if (!m_bitstream->Convert(pData, iSize, packet.pts))
    return false;

  if (!m_bitstream->CanStartDecode())
  {
    logM(LOGDEBUG, "CDVDVideoCodecAmlogic", "waiting for keyframe (bitstream)");
    return false;
  }

  return true;
}

bool CDVDVideoCodecAmlogic::AddData(const DemuxPacket &packet)
{
  // Handle Input, add demuxer packet to input queue, we must accept it or
  // it will be discarded as VideoPlayerVideo has no concept of "try again".

  uint8_t *pData(packet.pData);
  uint32_t iSize(packet.iSize);
  bool set_osd_max = false;

  if (pData)
  {
    if (m_bitstream)
    {
      if (!m_last_added) // Try again
      {
        pData = m_last_pData;
        iSize = m_last_iSize;
      }
      else
      {
        if (packet.isDualStream && aml_dolby_vision_enabled())
        {
          if (!DualLayerConvert(pData, iSize, packet))
            return true;
        }
        else
        {
          if (!SingleLayerConvert(pData, iSize, packet))
            return true;
        }
        m_last_pData = pData = m_bitstream->GetConvertBuffer();
        m_last_iSize = iSize = m_bitstream->GetConvertSize();
      }
    }
    else if (!m_has_keyframe && m_bitparser)
    {
      if (!m_bitparser->CanStartDecode(pData, iSize))
      {
        logM(LOGDEBUG, "CDVDVideoCodecAmlogic", "waiting for keyframe (bitparser)");
        return true;
      }
      else
        m_has_keyframe = true;
    }

    FrameRateTracking(pData, iSize, packet.dts, packet.pts);

    if (!m_opened)
    {
      if (packet.pts == DVD_NOPTS_VALUE)
        m_hints.ptsinvalid = true;

      logM(LOGINFO, "CDVDVideoCodecAmlogic", "Open decoder: fps:{:d}/{:d}", m_hints.fpsrate, m_hints.fpsscale);

      if (m_Codec && !m_Codec->OpenDecoder(false))
        logM(LOGERROR, "CDVDVideoCodecAmlogic", "Failed to open Amlogic Codec");

      m_videoBufferPool = std::shared_ptr<CAMLVideoBufferPool>(new CAMLVideoBufferPool());

      m_opened = true;
      set_osd_max = true;
    }
  }

  m_last_added = m_Codec->AddData(pData, iSize, packet.dts, m_hints.ptsinvalid ? DVD_NOPTS_VALUE : packet.pts);

  // Make change in luminance as late a possible to try and avoid starting change in luminance in menu.
  if (set_osd_max)
  {
    // if DV_MODE::ON (i.e. on in Kodi Menu), then set graphics max to 0 (OSD luminance will be handled by amlogic).
    if (aml_dv_mode() == DV_MODE::ON) aml_dv_set_osd_max(0);
  }

  return m_last_added;
}

void CDVDVideoCodecAmlogic::Reset(void)
{
  if (m_Codec->IsStreamTypeStream())
  {
    if (m_dataCacheCore.GetSpeed() == 1.0f)
    {
      // Cannot just do reset, needs fuller open and close to have correct pts.
      m_Codec->CloseDecoder(true);
      m_Codec->OpenDecoder(true);
    }
  }
  else
    m_Codec->Reset();

  ClearBitstreamCommon();

  m_mpeg2_sequence_pts = 0;
  m_has_keyframe = false;

  if (m_bitstream)
    m_bitstream->ResetStartDecode();
}

CDVDVideoCodec::VCReturn CDVDVideoCodecAmlogic::GetPicture(VideoPicture* pVideoPicture)
{
  if (!m_Codec)
    return VC_ERROR;

  VCReturn retVal = m_Codec->GetPicture(m_videobuffer);

  if (retVal == VC_PICTURE)
  {
    pVideoPicture->videoBuffer = nullptr;
    pVideoPicture->SetParams(m_videobuffer);

    pVideoPicture->videoBuffer = m_videoBufferPool->Get();
    static_cast<CAMLVideoBuffer*>(pVideoPicture->videoBuffer)->Set(this, m_Codec,
     m_Codec->GetOMXPts(), m_Codec->GetAmlDuration(), m_Codec->GetBufferIndex());;
  }

  // check for mpeg2 aspect ratio changes
  if (m_mpeg2_sequence && pVideoPicture->pts >= m_mpeg2_sequence_pts)
    m_aspect_ratio = m_mpeg2_sequence->ratio;

  // check for h264 aspect ratio changes
  if (m_h264_sequence && pVideoPicture->pts >= m_h264_sequence_pts)
    m_aspect_ratio = m_h264_sequence->ratio;

  pVideoPicture->iDisplayWidth  = pVideoPicture->iWidth;
  pVideoPicture->iDisplayHeight = pVideoPicture->iHeight;
  if (m_aspect_ratio > 1.0f && !m_hints.forced_aspect)
  {
    pVideoPicture->iDisplayWidth  = ((int)lrint(pVideoPicture->iHeight * m_aspect_ratio)) & ~3;
    if (pVideoPicture->iDisplayWidth > pVideoPicture->iWidth)
    {
      pVideoPicture->iDisplayWidth  = pVideoPicture->iWidth;
      pVideoPicture->iDisplayHeight = ((int)lrint(pVideoPicture->iWidth / m_aspect_ratio)) & ~3;
    }
  }

  return retVal;
}

void CDVDVideoCodecAmlogic::SetCodecControl(int flags)
{
  if (m_codecControlFlags != flags)
  {
    logComponentM(LOGDEBUG, LOGVIDEO, "CDVDVideoCodecAmlogic", "{:x}->{:x}", m_codecControlFlags, flags);
    m_codecControlFlags = flags;

    if (flags & DVD_CODEC_CTRL_DROP)
      m_videobuffer.iFlags |= DVP_FLAG_DROPPED;
    else
      m_videobuffer.iFlags &= ~DVP_FLAG_DROPPED;

    if (m_Codec)
      m_Codec->SetDrain((flags & DVD_CODEC_CTRL_DRAIN) != 0);
  }
}

void CDVDVideoCodecAmlogic::SetSpeed(int iSpeed)
{
  if (m_Codec)
  {
    m_Codec->SetSpeed(iSpeed);
    if (m_hints.codec == AV_CODEC_ID_H264) m_Codec->Reset();
  }
}

void CDVDVideoCodecAmlogic::FrameRateTracking(uint8_t *pData, int iSize, double dts, double pts)
{
  // mpeg2 handling
  if (m_mpeg2_sequence)
  {
    // probe demux for sequence_header_code NAL and
    // decode aspect ratio and frame rate.
    if (CBitstreamConverter::mpeg2_sequence_header(pData, iSize, m_mpeg2_sequence.get()) &&
       (m_mpeg2_sequence->fps_rate > 0) && (m_mpeg2_sequence->fps_scale > 0))
    {
      if (!m_mpeg2_sequence->fps_scale || !m_mpeg2_sequence->fps_scale)
        return;

      m_mpeg2_sequence_pts = pts;
      if (m_mpeg2_sequence_pts == DVD_NOPTS_VALUE)
        m_mpeg2_sequence_pts = dts;

      logM(LOGDEBUG, "CDVDVideoCodecAmlogic", "fps:{:d}/{:d} mpeg2_fps:{:d}/{:d} options:0x{:2x}",
              m_hints.fpsrate, m_hints.fpsscale, m_mpeg2_sequence->fps_rate, m_mpeg2_sequence->fps_scale, m_hints.codecOptions);
      if  (!(m_hints.codecOptions & CODEC_INTERLACED))
      {
        m_hints.fpsrate = m_mpeg2_sequence->fps_rate;
        m_hints.fpsscale = m_mpeg2_sequence->fps_scale;
      }
      if (m_hints.fpsrate && m_hints.fpsscale)
      {
        m_framerate = static_cast<float>(m_hints.fpsrate) / m_hints.fpsscale;
        if (m_hints.codecOptions & CODEC_UNKNOWN_I_P)
          if (std::abs(m_framerate - 25.0) < 0.02 || std::abs(m_framerate - 29.97) < 0.02)
          {
            m_framerate += m_framerate;
            m_hints.fpsrate += m_hints.fpsrate;
          }
        m_video_rate = (int)(0.5 + (96000.0 / m_framerate));
      }
      m_hints.width    = m_mpeg2_sequence->width;
      m_hints.height   = m_mpeg2_sequence->height;
      m_hints.aspect   = m_mpeg2_sequence->ratio;

      m_processInfo.SetVideoFps(m_framerate);
//      m_processInfo.SetVideoDAR(m_hints.aspect);
    }
    return;
  }

  // h264 aspect ratio handling
  if (m_h264_sequence)
  {
    // probe demux for SPS NAL and decode aspect ratio
    if (CBitstreamConverter::h264_sequence_header(pData, iSize, m_h264_sequence.get()))
    {
      m_h264_sequence_pts = pts;
      if (m_h264_sequence_pts == DVD_NOPTS_VALUE)
          m_h264_sequence_pts = dts;

      logM(LOGDEBUG, "CDVDVideoCodecAmlogic", "detected h264 aspect ratio({:f})", m_h264_sequence->ratio);
      m_hints.width    = m_h264_sequence->width;
      m_hints.height   = m_h264_sequence->height;
      m_hints.aspect   = m_h264_sequence->ratio;
    }
  }
}
