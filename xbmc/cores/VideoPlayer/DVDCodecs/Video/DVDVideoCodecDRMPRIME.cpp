/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DVDVideoCodecDRMPRIME.h"

#include "ServiceBroker.h"
#include "cores/VideoPlayer/Buffers/VideoBufferDRMPRIME.h"
#include "cores/VideoPlayer/DVDCodecs/DVDCodecs.h"
#include "cores/VideoPlayer/DVDCodecs/DVDFactoryCodec.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "windowing/gbm/WinSystemGbm.h"

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavutil/error.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
}

using namespace KODI::WINDOWING::GBM;

CDVDVideoCodecDRMPRIME::CDVDVideoCodecDRMPRIME(CProcessInfo& processInfo)
  : CDVDVideoCodec(processInfo)
{
  m_pFrame = av_frame_alloc();
  m_videoBufferPool = std::make_shared<CVideoBufferPoolDRMPRIMEFFmpeg>();
}

CDVDVideoCodecDRMPRIME::~CDVDVideoCodecDRMPRIME()
{
  av_frame_free(&m_pFrame);
  avcodec_free_context(&m_pCodecContext);
}

CDVDVideoCodec* CDVDVideoCodecDRMPRIME::Create(CProcessInfo& processInfo)
{
  if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
          CSettings::SETTING_VIDEOPLAYER_USEPRIMEDECODER))
    return new CDVDVideoCodecDRMPRIME(processInfo);
  return nullptr;
}

void CDVDVideoCodecDRMPRIME::Register()
{
  CServiceBroker::GetSettingsComponent()
      ->GetSettings()
      ->GetSetting(CSettings::SETTING_VIDEOPLAYER_USEPRIMEDECODER)
      ->SetVisible(true);
  CDVDFactoryCodec::RegisterHWVideoCodec("drm_prime", CDVDVideoCodecDRMPRIME::Create);
}

static bool IsSupportedHwFormat(const enum AVPixelFormat fmt)
{
  return fmt == AV_PIX_FMT_DRM_PRIME;
}

static const AVCodecHWConfig* FindHWConfig(const AVCodec* codec)
{
  const AVCodecHWConfig* config = nullptr;
  for (int n = 0; (config = avcodec_get_hw_config(codec, n)); n++)
  {
    if (!IsSupportedHwFormat(config->pix_fmt))
      continue;

    if ((config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX) &&
        config->device_type == AV_HWDEVICE_TYPE_DRM)
      return config;

    if ((config->methods & AV_CODEC_HW_CONFIG_METHOD_INTERNAL))
      return config;
  }

  return nullptr;
}

static const AVCodec* FindDecoder(CDVDStreamInfo& hints)
{
  const AVCodec* codec = nullptr;
  void* i = 0;

  while ((codec = av_codec_iterate(&i)))
  {
    if (!av_codec_is_decoder(codec))
      continue;
    if (codec->id != hints.codec)
      continue;

    const AVCodecHWConfig* config = FindHWConfig(codec);
    if (config)
      return codec;
  }

  return nullptr;
}

enum AVPixelFormat CDVDVideoCodecDRMPRIME::GetFormat(struct AVCodecContext* avctx,
                                                     const enum AVPixelFormat* fmt)
{
  for (int n = 0; fmt[n] != AV_PIX_FMT_NONE; n++)
  {
    if (IsSupportedHwFormat(fmt[n]))
    {
      CDVDVideoCodecDRMPRIME* ctx = static_cast<CDVDVideoCodecDRMPRIME*>(avctx->opaque);
      ctx->UpdateProcessInfo(avctx, fmt[n]);
      return fmt[n];
    }
  }

  CLog::Log(LOGERROR, "CDVDVideoCodecDRMPRIME::{} - unsupported pixel format", __FUNCTION__);
  return AV_PIX_FMT_NONE;
}

bool CDVDVideoCodecDRMPRIME::Open(CDVDStreamInfo& hints, CDVDCodecOptions& options)
{
  const AVCodec* pCodec = FindDecoder(hints);
  if (!pCodec)
  {
    CLog::Log(LOGDEBUG, "CDVDVideoCodecDRMPRIME::{} - unable to find decoder for codec {}",
              __FUNCTION__, hints.codec);
    return false;
  }

  CLog::Log(LOGINFO, "CDVDVideoCodecDRMPRIME::{} - using decoder {}", __FUNCTION__,
            pCodec->long_name ? pCodec->long_name : pCodec->name);

  m_pCodecContext = avcodec_alloc_context3(pCodec);
  if (!m_pCodecContext)
    return false;

  m_hints = hints;

  const AVCodecHWConfig* pConfig = FindHWConfig(pCodec);
  if (pConfig && (pConfig->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX) &&
      pConfig->device_type == AV_HWDEVICE_TYPE_DRM)
  {
    CWinSystemGbm* winSystem = dynamic_cast<CWinSystemGbm*>(CServiceBroker::GetWinSystem());
    if (av_hwdevice_ctx_create(&m_pCodecContext->hw_device_ctx, AV_HWDEVICE_TYPE_DRM,
                               drmGetDeviceNameFromFd2(winSystem->GetDrm()->GetFileDescriptor()),
                               nullptr, 0) < 0)
    {
      CLog::Log(LOGINFO, "CDVDVideoCodecDRMPRIME::{} - unable to create hwdevice context",
                __FUNCTION__);
      avcodec_free_context(&m_pCodecContext);
      return false;
    }
  }

  m_pCodecContext->pix_fmt = AV_PIX_FMT_DRM_PRIME;
  m_pCodecContext->opaque = static_cast<void*>(this);
  m_pCodecContext->get_format = GetFormat;
  m_pCodecContext->codec_tag = hints.codec_tag;
  m_pCodecContext->coded_width = hints.width;
  m_pCodecContext->coded_height = hints.height;
  m_pCodecContext->bits_per_coded_sample = hints.bitsperpixel;
  m_pCodecContext->time_base.num = 1;
  m_pCodecContext->time_base.den = DVD_TIME_BASE;

  if (hints.extradata && hints.extrasize > 0)
  {
    m_pCodecContext->extradata_size = hints.extrasize;
    m_pCodecContext->extradata =
        static_cast<uint8_t*>(av_mallocz(hints.extrasize + AV_INPUT_BUFFER_PADDING_SIZE));
    memcpy(m_pCodecContext->extradata, hints.extradata, hints.extrasize);
  }

  for (auto&& option : options.m_keys)
    av_opt_set(m_pCodecContext, option.m_name.c_str(), option.m_value.c_str(), 0);

  if (avcodec_open2(m_pCodecContext, pCodec, nullptr) < 0)
  {
    CLog::Log(LOGINFO, "CDVDVideoCodecDRMPRIME::{} - unable to open codec", __FUNCTION__);
    avcodec_free_context(&m_pCodecContext);
    return false;
  }

  UpdateProcessInfo(m_pCodecContext, m_pCodecContext->pix_fmt);
  m_processInfo.SetVideoDeintMethod("none");
  m_processInfo.SetVideoDAR(hints.aspect);

  return true;
}

void CDVDVideoCodecDRMPRIME::UpdateProcessInfo(struct AVCodecContext* avctx,
                                               const enum AVPixelFormat pix_fmt)
{
  const char* pixFmtName = av_get_pix_fmt_name(pix_fmt);
  m_processInfo.SetVideoPixelFormat(pixFmtName ? pixFmtName : "");
  m_processInfo.SetVideoDimensions(avctx->coded_width, avctx->coded_height);

  if (avctx->codec && avctx->codec->name)
    m_name = std::string("ff-") + avctx->codec->name;
  else
    m_name = "ffmpeg";

  m_processInfo.SetVideoDecoderName(m_name + "-drm_prime", IsSupportedHwFormat(pix_fmt));
}

bool CDVDVideoCodecDRMPRIME::AddData(const DemuxPacket& packet)
{
  if (!m_pCodecContext)
    return true;

  if (!packet.pData)
    return true;

  AVPacket avpkt;
  av_init_packet(&avpkt);
  avpkt.data = packet.pData;
  avpkt.size = packet.iSize;
  avpkt.dts = (packet.dts == DVD_NOPTS_VALUE)
                  ? AV_NOPTS_VALUE
                  : static_cast<int64_t>(packet.dts / DVD_TIME_BASE * AV_TIME_BASE);
  avpkt.pts = (packet.pts == DVD_NOPTS_VALUE)
                  ? AV_NOPTS_VALUE
                  : static_cast<int64_t>(packet.pts / DVD_TIME_BASE * AV_TIME_BASE);
  avpkt.side_data = static_cast<AVPacketSideData*>(packet.pSideData);
  avpkt.side_data_elems = packet.iSideDataElems;

  int ret = avcodec_send_packet(m_pCodecContext, &avpkt);
  if (ret == AVERROR(EAGAIN))
    return false;
  else if (ret)
  {
    char err[AV_ERROR_MAX_STRING_SIZE] = {};
    av_strerror(ret, err, AV_ERROR_MAX_STRING_SIZE);
    CLog::Log(LOGERROR, "CDVDVideoCodecDRMPRIME::{} - send packet failed: {} ({})", __FUNCTION__,
              err, ret);
    if (ret != AVERROR_EOF && ret != AVERROR_INVALIDDATA)
      return false;
  }

  return true;
}

void CDVDVideoCodecDRMPRIME::Reset()
{
  if (!m_pCodecContext)
    return;

  Drain();

  do
  {
    int ret = avcodec_receive_frame(m_pCodecContext, m_pFrame);
    if (ret == AVERROR_EOF)
      break;
    else if (ret)
    {
      char err[AV_ERROR_MAX_STRING_SIZE] = {};
      av_strerror(ret, err, AV_ERROR_MAX_STRING_SIZE);
      CLog::Log(LOGERROR, "CDVDVideoCodecDRMPRIME::{} - receive frame failed: {} ({})",
                __FUNCTION__, err, ret);
      break;
    }
    else
      av_frame_unref(m_pFrame);
  } while (true);

  CLog::Log(LOGDEBUG, "CDVDVideoCodecDRMPRIME::{} - flush buffers", __FUNCTION__);
  avcodec_flush_buffers(m_pCodecContext);
}

void CDVDVideoCodecDRMPRIME::Drain()
{
  AVPacket avpkt;
  av_init_packet(&avpkt);
  avpkt.data = nullptr;
  avpkt.size = 0;
  int ret = avcodec_send_packet(m_pCodecContext, &avpkt);
  if (ret && ret != AVERROR_EOF)
  {
    char err[AV_ERROR_MAX_STRING_SIZE] = {};
    av_strerror(ret, err, AV_ERROR_MAX_STRING_SIZE);
    CLog::Log(LOGERROR, "CDVDVideoCodecDRMPRIME::{} - send packet failed: {} ({})", __FUNCTION__,
              err, ret);
  }
}

void CDVDVideoCodecDRMPRIME::SetPictureParams(VideoPicture* pVideoPicture)
{
  pVideoPicture->iWidth = m_pFrame->width;
  pVideoPicture->iHeight = m_pFrame->height;

  double aspect_ratio = 0;
  AVRational pixel_aspect = m_pFrame->sample_aspect_ratio;
  if (pixel_aspect.num)
    aspect_ratio = av_q2d(pixel_aspect) * pVideoPicture->iWidth / pVideoPicture->iHeight;

  if (aspect_ratio <= 0.0)
    aspect_ratio =
        static_cast<float>(pVideoPicture->iWidth) / static_cast<float>(pVideoPicture->iHeight);

  pVideoPicture->iDisplayWidth =
      (static_cast<int>(lrint(pVideoPicture->iHeight * aspect_ratio))) & -3;
  pVideoPicture->iDisplayHeight = pVideoPicture->iHeight;
  if (pVideoPicture->iDisplayWidth > pVideoPicture->iWidth)
  {
    pVideoPicture->iDisplayWidth = pVideoPicture->iWidth;
    pVideoPicture->iDisplayHeight =
        (static_cast<int>(lrint(pVideoPicture->iWidth / aspect_ratio))) & -3;
  }

  pVideoPicture->color_range =
      m_pFrame->color_range == AVCOL_RANGE_JPEG || m_hints.colorRange == AVCOL_RANGE_JPEG;
  pVideoPicture->color_primaries = m_pFrame->color_primaries == AVCOL_PRI_UNSPECIFIED
                                       ? m_hints.colorPrimaries
                                       : m_pFrame->color_primaries;
  pVideoPicture->color_transfer = m_pFrame->color_trc == AVCOL_TRC_UNSPECIFIED
                                      ? m_hints.colorTransferCharacteristic
                                      : m_pFrame->color_trc;
  pVideoPicture->color_space =
      m_pFrame->colorspace == AVCOL_SPC_UNSPECIFIED ? m_hints.colorSpace : m_pFrame->colorspace;
  pVideoPicture->chroma_position = m_pFrame->chroma_location;

  pVideoPicture->colorBits = 8;
  if (m_pCodecContext->codec_id == AV_CODEC_ID_HEVC &&
      m_pCodecContext->profile == FF_PROFILE_HEVC_MAIN_10)
    pVideoPicture->colorBits = 10;
  else if (m_pCodecContext->codec_id == AV_CODEC_ID_H264 &&
           (m_pCodecContext->profile == FF_PROFILE_H264_HIGH_10 ||
            m_pCodecContext->profile == FF_PROFILE_H264_HIGH_10_INTRA))
    pVideoPicture->colorBits = 10;

  pVideoPicture->hasDisplayMetadata = false;
  AVFrameSideData* sd = av_frame_get_side_data(m_pFrame, AV_FRAME_DATA_MASTERING_DISPLAY_METADATA);
  if (sd)
  {
    pVideoPicture->displayMetadata = *reinterpret_cast<AVMasteringDisplayMetadata*>(sd->data);
    pVideoPicture->hasDisplayMetadata = true;
  }
  else if (m_hints.masteringMetadata)
  {
    pVideoPicture->displayMetadata = *m_hints.masteringMetadata.get();
    pVideoPicture->hasDisplayMetadata = true;
  }

  pVideoPicture->hasLightMetadata = false;
  sd = av_frame_get_side_data(m_pFrame, AV_FRAME_DATA_CONTENT_LIGHT_LEVEL);
  if (sd)
  {
    pVideoPicture->lightMetadata = *reinterpret_cast<AVContentLightMetadata*>(sd->data);
    pVideoPicture->hasLightMetadata = true;
  }
  else if (m_hints.contentLightMetadata)
  {
    pVideoPicture->lightMetadata = *m_hints.contentLightMetadata.get();
    pVideoPicture->hasLightMetadata = true;
  }

  pVideoPicture->iRepeatPicture = 0;
  pVideoPicture->iFlags = 0;
  pVideoPicture->iFlags |= m_pFrame->interlaced_frame ? DVP_FLAG_INTERLACED : 0;
  pVideoPicture->iFlags |= m_pFrame->top_field_first ? DVP_FLAG_TOP_FIELD_FIRST : 0;

  int64_t pts = m_pFrame->best_effort_timestamp;
  pVideoPicture->pts = (pts == AV_NOPTS_VALUE)
                           ? DVD_NOPTS_VALUE
                           : static_cast<double>(pts) * DVD_TIME_BASE / AV_TIME_BASE;
  pVideoPicture->dts = DVD_NOPTS_VALUE;
}

CDVDVideoCodec::VCReturn CDVDVideoCodecDRMPRIME::GetPicture(VideoPicture* pVideoPicture)
{
  if (m_codecControlFlags & DVD_CODEC_CTRL_DRAIN)
    Drain();

  if (pVideoPicture->videoBuffer)
  {
    pVideoPicture->videoBuffer->Release();
    pVideoPicture->videoBuffer = nullptr;
  }

  int ret = avcodec_receive_frame(m_pCodecContext, m_pFrame);
  if (ret == AVERROR(EAGAIN))
    return VC_BUFFER;
  else if (ret == AVERROR_EOF)
  {
    if (m_codecControlFlags & DVD_CODEC_CTRL_DRAIN)
    {
      CLog::Log(LOGDEBUG, "CDVDVideoCodecDRMPRIME::{} - flush buffers", __FUNCTION__);
      avcodec_flush_buffers(m_pCodecContext);
      SetCodecControl(m_codecControlFlags & ~DVD_CODEC_CTRL_DRAIN);
    }
    return VC_EOF;
  }
  else if (ret)
  {
    char err[AV_ERROR_MAX_STRING_SIZE] = {};
    av_strerror(ret, err, AV_ERROR_MAX_STRING_SIZE);
    CLog::Log(LOGERROR, "CDVDVideoCodecDRMPRIME::{} - receive frame failed: {} ({})", __FUNCTION__,
              err, ret);
    return VC_ERROR;
  }

  SetPictureParams(pVideoPicture);

  if (IsSupportedHwFormat(static_cast<AVPixelFormat>(m_pFrame->format)))
  {
    CVideoBufferDRMPRIMEFFmpeg* buffer =
        dynamic_cast<CVideoBufferDRMPRIMEFFmpeg*>(m_videoBufferPool->Get());
    buffer->SetPictureParams(*pVideoPicture);
    buffer->SetRef(m_pFrame);
    pVideoPicture->videoBuffer = buffer;
  }

  if (!pVideoPicture->videoBuffer)
  {
    CLog::Log(LOGERROR, "CDVDVideoCodecDRMPRIME::{} - videoBuffer:nullptr format:{}", __FUNCTION__,
              av_get_pix_fmt_name(static_cast<AVPixelFormat>(m_pFrame->format)));
    av_frame_unref(m_pFrame);
    return VC_ERROR;
  }

  return VC_PICTURE;
}
