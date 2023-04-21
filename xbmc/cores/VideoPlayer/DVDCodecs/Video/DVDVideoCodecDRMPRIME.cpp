/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DVDVideoCodecDRMPRIME.h"

#include "ServiceBroker.h"
#include "cores/VideoPlayer/Buffers/VideoBufferDMA.h"
#include "cores/VideoPlayer/Buffers/VideoBufferDRMPRIME.h"
#include "cores/VideoPlayer/DVDCodecs/DVDCodecs.h"
#include "cores/VideoPlayer/DVDCodecs/DVDFactoryCodec.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "threads/SingleLock.h"
#include "utils/CPUInfo.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#if defined(HAVE_GBM)
#include "windowing/gbm/WinSystemGbm.h"
#endif

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavutil/error.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
}

namespace
{

constexpr const char* SETTING_VIDEOPLAYER_USEPRIMEDECODERFORHW{"videoplayer.useprimedecoderforhw"};

static void ReleaseBuffer(void* opaque, uint8_t* data)
{
  CVideoBufferDMA* buffer = static_cast<CVideoBufferDMA*>(opaque);
  buffer->Release();
}

static void AlignedSize(AVCodecContext* avctx, int& width, int& height)
{
  int w = width;
  int h = height;
  AVFrame picture;
  int unaligned;
  int stride_align[AV_NUM_DATA_POINTERS];

  avcodec_align_dimensions2(avctx, &w, &h, stride_align);

  do
  {
    // NOTE: do not align linesizes individually, this breaks e.g. assumptions
    // that linesize[0] == 2*linesize[1] in the MPEG-encoder for 4:2:2
    av_image_fill_linesizes(picture.linesize, avctx->pix_fmt, w);
    // increase alignment of w for next try (rhs gives the lowest bit set in w)
    w += w & ~(w - 1);

    unaligned = 0;
    for (int i = 0; i < 4; i++)
      unaligned |= picture.linesize[i] % stride_align[i];
  } while (unaligned);

  width = w;
  height = h;
}

bool SupportsDRM()
{
  AVHWDeviceType type = AV_HWDEVICE_TYPE_NONE;
  while ((type = av_hwdevice_iterate_types(type)) != AV_HWDEVICE_TYPE_NONE)
  {
    if (type == AV_HWDEVICE_TYPE_DRM)
      return true;
  }

  return false;
}

} // namespace

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

std::unique_ptr<CDVDVideoCodec> CDVDVideoCodecDRMPRIME::Create(CProcessInfo& processInfo)
{
  if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
          CSettings::SETTING_VIDEOPLAYER_USEPRIMEDECODER))
    return std::make_unique<CDVDVideoCodecDRMPRIME>(processInfo);

  return nullptr;
}

void CDVDVideoCodecDRMPRIME::Register()
{
  if (!SupportsDRM())
  {
    CLog::Log(LOGINFO, "CDVDVideoCodecDRMPRIME: FFMPEG has no libdrm support");
    return;
  }

  auto settingsComponent = CServiceBroker::GetSettingsComponent();
  if (!settingsComponent)
    return;

  auto settings = settingsComponent->GetSettings();
  if (!settings)
    return;

  auto setting = settings->GetSetting(CSettings::SETTING_VIDEOPLAYER_USEPRIMEDECODER);
  if (!setting)
  {
    CLog::Log(LOGERROR, "Failed to load setting for: {}",
              CSettings::SETTING_VIDEOPLAYER_USEPRIMEDECODER);
    return;
  }

  setting->SetVisible(true);

  setting = settings->GetSetting(SETTING_VIDEOPLAYER_USEPRIMEDECODERFORHW);
  if (!setting)
  {
    CLog::Log(LOGERROR, "Failed to load setting for: {}", SETTING_VIDEOPLAYER_USEPRIMEDECODERFORHW);
    return;
  }

  setting->SetVisible(true);

  CDVDFactoryCodec::RegisterHWVideoCodec("drm_prime", CDVDVideoCodecDRMPRIME::Create);
}

static bool IsSupportedHwFormat(const enum AVPixelFormat fmt)
{
  bool hw = CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
      SETTING_VIDEOPLAYER_USEPRIMEDECODERFORHW);

  return fmt == AV_PIX_FMT_DRM_PRIME && hw;
}

static bool IsSupportedSwFormat(const enum AVPixelFormat fmt)
{
  return fmt == AV_PIX_FMT_YUV420P || fmt == AV_PIX_FMT_YUVJ420P || fmt == AV_PIX_FMT_YUV422P ||
         fmt == AV_PIX_FMT_YUVJ422P || fmt == AV_PIX_FMT_YUV444P || fmt == AV_PIX_FMT_YUVJ444P;
}

static const AVCodecHWConfig* FindHWConfig(const AVCodec* codec)
{
  if (!CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
          SETTING_VIDEOPLAYER_USEPRIMEDECODERFORHW))
    return nullptr;

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

  if (!(hints.codecOptions & CODEC_FORCE_SOFTWARE))
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

  codec = avcodec_find_decoder(hints.codec);
  if (codec && (codec->capabilities & AV_CODEC_CAP_DR1) == AV_CODEC_CAP_DR1)
    return codec;

  return nullptr;
}

enum AVPixelFormat CDVDVideoCodecDRMPRIME::GetFormat(struct AVCodecContext* avctx,
                                                     const enum AVPixelFormat* fmt)
{
  for (int n = 0; fmt[n] != AV_PIX_FMT_NONE; n++)
  {
    if (IsSupportedHwFormat(fmt[n]) || IsSupportedSwFormat(fmt[n]))
    {
      CDVDVideoCodecDRMPRIME* ctx = static_cast<CDVDVideoCodecDRMPRIME*>(avctx->opaque);
      ctx->UpdateProcessInfo(avctx, fmt[n]);
      return fmt[n];
    }
  }

  std::vector<std::string> formats;
  for (int n = 0; fmt[n] != AV_PIX_FMT_NONE; n++)
  {
    formats.emplace_back(av_get_pix_fmt_name(fmt[n]));
  }
  CLog::Log(LOGERROR, "CDVDVideoCodecDRMPRIME::{} - no supported pixel formats: {}", __FUNCTION__,
            StringUtils::Join(formats, ", "));

  return AV_PIX_FMT_NONE;
}

int CDVDVideoCodecDRMPRIME::GetBuffer(struct AVCodecContext* avctx, AVFrame* frame, int flags)
{
  if (IsSupportedSwFormat(static_cast<AVPixelFormat>(frame->format)))
  {
    int width = frame->width;
    int height = frame->height;

    AlignedSize(avctx, width, height);

    int size;
    switch (avctx->pix_fmt)
    {
      case AV_PIX_FMT_YUV420P:
      case AV_PIX_FMT_YUVJ420P:
        size = width * height * 3 / 2;
        break;
      case AV_PIX_FMT_YUV422P:
      case AV_PIX_FMT_YUVJ422P:
        size = width * height * 2;
        break;
      case AV_PIX_FMT_YUV444P:
      case AV_PIX_FMT_YUVJ444P:
        size = width * height * 3;
        break;
      default:
        return -1;
    }

    CDVDVideoCodecDRMPRIME* ctx = static_cast<CDVDVideoCodecDRMPRIME*>(avctx->opaque);
    auto buffer = dynamic_cast<CVideoBufferDMA*>(
        ctx->m_processInfo.GetVideoBufferManager().Get(avctx->pix_fmt, size, nullptr));
    if (!buffer)
      return -1;

    frame->opaque = static_cast<void*>(buffer);
    frame->opaque_ref =
        av_buffer_create(nullptr, 0, ReleaseBuffer, frame->opaque, AV_BUFFER_FLAG_READONLY);

    buffer->Export(frame, width, height);
    buffer->SyncStart();

    return 0;
  }

  return avcodec_default_get_buffer2(avctx, frame, flags);
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
    const char* device = nullptr;

    if (getenv("KODI_RENDER_NODE"))
      device = getenv("KODI_RENDER_NODE");

#if defined(HAVE_GBM)
    auto winSystem = dynamic_cast<KODI::WINDOWING::GBM::CWinSystemGbm*>(CServiceBroker::GetWinSystem());

    if (winSystem)
    {
      auto drm = winSystem->GetDrm();

      if (!drm)
        return false;

      if (!device)
        device = drm->GetRenderDevicePath();
    }
#endif

    //! @todo: fix with proper device when dma-hints wayland protocol works
    if (!device)
      device = "/dev/dri/renderD128";

    CLog::Log(LOGDEBUG, "CDVDVideoCodecDRMPRIME::{} - using drm device for av_hwdevice_ctx: {}", __FUNCTION__, device);

    if (av_hwdevice_ctx_create(&m_pCodecContext->hw_device_ctx, pConfig->device_type,
                               device, nullptr, 0) < 0)
    {
      CLog::Log(LOGERROR,
                "CDVDVideoCodecDRMPRIME::{} - unable to create hwdevice context using device: {}",
                __FUNCTION__, device);
      avcodec_free_context(&m_pCodecContext);
      return false;
    }
  }

  m_pCodecContext->pix_fmt = AV_PIX_FMT_DRM_PRIME;
  m_pCodecContext->opaque = static_cast<void*>(this);
  m_pCodecContext->get_format = GetFormat;
  m_pCodecContext->get_buffer2 = GetBuffer;
  m_pCodecContext->codec_tag = hints.codec_tag;
  m_pCodecContext->coded_width = hints.width;
  m_pCodecContext->coded_height = hints.height;
  m_pCodecContext->bits_per_coded_sample = hints.bitsperpixel;
  m_pCodecContext->time_base.num = 1;
  m_pCodecContext->time_base.den = DVD_TIME_BASE;
  m_pCodecContext->thread_count = CServiceBroker::GetCPUInfo()->GetCPUCount();

  if (hints.extradata)
  {
    m_pCodecContext->extradata =
        static_cast<uint8_t*>(av_mallocz(hints.extradata.GetSize() + AV_INPUT_BUFFER_PADDING_SIZE));
    if (m_pCodecContext->extradata)
    {
      m_pCodecContext->extradata_size = hints.extradata.GetSize();
      memcpy(m_pCodecContext->extradata, hints.extradata.GetData(), hints.extradata.GetSize());
    }
  }

  for (auto&& option : options.m_keys)
    av_opt_set(m_pCodecContext, option.m_name.c_str(), option.m_value.c_str(), 0);

  if (avcodec_open2(m_pCodecContext, pCodec, nullptr) < 0)
  {
    CLog::Log(LOGINFO, "CDVDVideoCodecDRMPRIME::{} - unable to open codec", __FUNCTION__);
    avcodec_free_context(&m_pCodecContext);
    if (hints.codecOptions & CODEC_FORCE_SOFTWARE)
      return false;

    hints.codecOptions |= CODEC_FORCE_SOFTWARE;
    return Open(hints, options);
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

  AVPacket* avpkt = av_packet_alloc();
  if (!avpkt)
  {
    CLog::Log(LOGERROR, "CDVDVideoCodecDRMPRIME::{} - av_packet_alloc failed: {}", __FUNCTION__,
              strerror(errno));
    return false;
  }

  avpkt->data = packet.pData;
  avpkt->size = packet.iSize;
  avpkt->dts = (packet.dts == DVD_NOPTS_VALUE)
                   ? AV_NOPTS_VALUE
                   : static_cast<int64_t>(packet.dts / DVD_TIME_BASE * AV_TIME_BASE);
  avpkt->pts = (packet.pts == DVD_NOPTS_VALUE)
                   ? AV_NOPTS_VALUE
                   : static_cast<int64_t>(packet.pts / DVD_TIME_BASE * AV_TIME_BASE);
  avpkt->side_data = static_cast<AVPacketSideData*>(packet.pSideData);
  avpkt->side_data_elems = packet.iSideDataElems;

  int ret = avcodec_send_packet(m_pCodecContext, avpkt);

  //! @todo: properly handle avpkt side_data. this works around our improper use of the side_data
  // as we pass pointers to ffmpeg allocated memory for the side_data. we should really be allocating
  // and storing our own AVPacket. This will require some extensive changes.
  av_buffer_unref(&avpkt->buf);
  av_free(avpkt);

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
  AVPacket* avpkt = av_packet_alloc();
  if (!avpkt)
  {
    CLog::Log(LOGERROR, "CDVDVideoCodecDRMPRIME::{} - av_packet_alloc failed: {}", __FUNCTION__,
              strerror(errno));
    return;
  }

  avpkt->data = nullptr;
  avpkt->size = 0;

  int ret = avcodec_send_packet(m_pCodecContext, avpkt);
  if (ret && ret != AVERROR_EOF)
  {
    char err[AV_ERROR_MAX_STRING_SIZE] = {};
    av_strerror(ret, err, AV_ERROR_MAX_STRING_SIZE);
    CLog::Log(LOGERROR, "CDVDVideoCodecDRMPRIME::{} - send packet failed: {} ({})", __FUNCTION__,
              err, ret);
  }

  av_packet_free(&avpkt);
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
        static_cast<double>(pVideoPicture->iWidth) / static_cast<double>(pVideoPicture->iHeight);

  if (m_DAR != aspect_ratio)
  {
    m_DAR = aspect_ratio;
    m_processInfo.SetVideoDAR(static_cast<float>(m_DAR));
  }

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
      m_pFrame->color_range == AVCOL_RANGE_JPEG || m_pFrame->format == AV_PIX_FMT_YUVJ420P ||
      m_pFrame->format == AV_PIX_FMT_YUVJ422P || m_pFrame->format == AV_PIX_FMT_YUVJ444P ||
      m_hints.colorRange == AVCOL_RANGE_JPEG;
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
  pVideoPicture->iFlags |= m_pFrame->data[0] ? 0 : DVP_FLAG_DROPPED;

  if (m_codecControlFlags & DVD_CODEC_CTRL_DROP)
  {
    pVideoPicture->iFlags |= DVP_FLAG_DROPPED;
  }

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

  if (pVideoPicture->videoBuffer)
  {
    pVideoPicture->videoBuffer->Release();
    pVideoPicture->videoBuffer = nullptr;
  }

  if (IsSupportedHwFormat(static_cast<AVPixelFormat>(m_pFrame->format)))
  {
    CVideoBufferDRMPRIMEFFmpeg* buffer =
        dynamic_cast<CVideoBufferDRMPRIMEFFmpeg*>(m_videoBufferPool->Get());
    buffer->SetPictureParams(*pVideoPicture);
    buffer->SetRef(m_pFrame);
    pVideoPicture->videoBuffer = buffer;
  }
  else if (m_pFrame->opaque)
  {
    CVideoBufferDMA* buffer = static_cast<CVideoBufferDMA*>(m_pFrame->opaque);
    buffer->SetPictureParams(*pVideoPicture);
    buffer->Acquire();
    buffer->SyncEnd();
    buffer->SetDimensions(m_pFrame->width, m_pFrame->height);

    pVideoPicture->videoBuffer = buffer;
    av_frame_unref(m_pFrame);
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

void CDVDVideoCodecDRMPRIME::SetCodecControl(int flags)
{
  m_codecControlFlags = flags;

  if (m_pCodecContext)
  {
    if ((flags & DVD_CODEC_CTRL_DROP_ANY) != 0)
    {
      m_pCodecContext->skip_frame = AVDISCARD_NONREF;
      m_pCodecContext->skip_idct = AVDISCARD_NONREF;
      m_pCodecContext->skip_loop_filter = AVDISCARD_NONREF;
    }
    else
    {
      m_pCodecContext->skip_frame = AVDISCARD_DEFAULT;
      m_pCodecContext->skip_idct = AVDISCARD_DEFAULT;
      m_pCodecContext->skip_loop_filter = AVDISCARD_DEFAULT;
    }
  }
}
