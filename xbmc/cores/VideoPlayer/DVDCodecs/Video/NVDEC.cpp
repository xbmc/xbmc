/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "NVDEC.h"

#include "ServiceBroker.h"
#include "cores/VideoPlayer/DVDCodecs/DVDFactoryCodec.h"
#include "cores/VideoPlayer/Process/ProcessInfo.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "utils/log.h"

#include <array>
#include <atomic>
#include <cstring>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
#include <libavutil/frame.h>
#include <libavutil/hwcontext.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
#include <libavutil/pixfmt.h>
}

namespace
{
struct NvdecCodecInfo
{
  AVCodecID codec;
  const char* setting;
};

constexpr auto NVDEC_CODEC_INFO = std::array{
    NvdecCodecInfo{AV_CODEC_ID_H264, CSettings::SETTING_VIDEOPLAYER_USENVDECH264},
    NvdecCodecInfo{AV_CODEC_ID_HEVC, CSettings::SETTING_VIDEOPLAYER_USENVDECHEVC},
    NvdecCodecInfo{AV_CODEC_ID_VP9, CSettings::SETTING_VIDEOPLAYER_USENVDECVP9},
    NvdecCodecInfo{AV_CODEC_ID_AV1, CSettings::SETTING_VIDEOPLAYER_USENVDECAV1},
    NvdecCodecInfo{AV_CODEC_ID_MPEG2VIDEO, CSettings::SETTING_VIDEOPLAYER_USENVDECMPEG2},
    NvdecCodecInfo{AV_CODEC_ID_MPEG4, CSettings::SETTING_VIDEOPLAYER_USENVDECMPEG4},
    NvdecCodecInfo{AV_CODEC_ID_H263, CSettings::SETTING_VIDEOPLAYER_USENVDECMPEG4},
    NvdecCodecInfo{AV_CODEC_ID_VC1, CSettings::SETTING_VIDEOPLAYER_USENVDECVC1},
    NvdecCodecInfo{AV_CODEC_ID_WMV3, CSettings::SETTING_VIDEOPLAYER_USENVDECVC1},
    NvdecCodecInfo{AV_CODEC_ID_VP8, CSettings::SETTING_VIDEOPLAYER_USENVDECVP8},
};

const NvdecCodecInfo* FindNvdecCodecInfo(AVCodecID codec)
{
  for (const auto& info : NVDEC_CODEC_INFO)
  {
    if (info.codec == codec)
      return &info;
  }
  return nullptr;
}

bool IsVisibleSettingEnabled(const std::shared_ptr<CSettings>& settings,
                             const std::string& settingId)
{
  if (!settings)
    return false;

  auto setting = settings->GetSetting(settingId);
  return setting && setting->IsVisible() && settings->GetBool(settingId);
}

} // namespace

AVPixelFormat CDVDVideoCodecNVDEC::GetFormatNVDEC(AVCodecContext* avctx, const AVPixelFormat* fmts)
{
  auto* self = static_cast<CDVDVideoCodecNVDEC*>(static_cast<ICallbackHWAccel*>(avctx->opaque));

  // Once NVDEC has failed, stop selecting CUDA so FFmpeg doesn't keep retrying.
  if (self->m_decoderState == STATE_HW_FAILED)
    return avcodec_default_get_format(avctx, fmts);

  for (const AVPixelFormat* p = fmts; *p != AV_PIX_FMT_NONE; ++p)
  {
    if (*p == AV_PIX_FMT_CUDA)
    {
      CLog::Log(LOGINFO, "NVDEC: get_format selecting AV_PIX_FMT_CUDA");
      return AV_PIX_FMT_CUDA;
    }
  }

  CLog::Log(LOGWARNING,
            "NVDEC: get_format - AV_PIX_FMT_CUDA not offered, NVDEC not available for this codec");
  self->m_decoderState = STATE_HW_FAILED;
  return avcodec_default_get_format(avctx, fmts);
}

CDVDVideoCodecNVDEC::CDVDVideoCodecNVDEC(CProcessInfo& processInfo)
  : CDVDVideoCodecFFmpeg(processInfo)
{
}

CDVDVideoCodecNVDEC::~CDVDVideoCodecNVDEC()
{
  av_frame_free(&m_swFrame);
  av_buffer_unref(&m_hwDeviceRef);
}

std::unique_ptr<CDVDVideoCodec> CDVDVideoCodecNVDEC::Create(CProcessInfo& processInfo)
{
  return std::make_unique<CDVDVideoCodecNVDEC>(processInfo);
}

void CDVDVideoCodecNVDEC::Register()
{
  static std::atomic_flag registered{};
  if (registered.test_and_set())
    return;

  CLog::Log(LOGINFO, "NVDEC: Register() called");

  // Check if FFmpeg can create a CUDA hardware device
  AVBufferRef* deviceRef = nullptr;
  int ret = av_hwdevice_ctx_create(&deviceRef, AV_HWDEVICE_TYPE_CUDA, nullptr, nullptr, 0);
  if (ret < 0)
  {
    CLog::Log(LOGINFO, "NVDEC: FFmpeg cannot create CUDA device (error {}), not registering", ret);
    registered.clear();
    return;
  }

  av_buffer_unref(&deviceRef);
  CLog::Log(LOGINFO, "NVDEC: FFmpeg CUDA device available, registering decoder");
  CDVDFactoryCodec::RegisterHWVideoCodec("nvdec", Create);

  auto settingsComponent = CServiceBroker::GetSettingsComponent();
  if (!settingsComponent)
    return;

  auto settings = settingsComponent->GetSettings();
  if (!settings)
    return;

  constexpr auto nvdecSettings = std::array{
      CSettings::SETTING_VIDEOPLAYER_USENVDEC,      CSettings::SETTING_VIDEOPLAYER_USENVDECH264,
      CSettings::SETTING_VIDEOPLAYER_USENVDECHEVC,  CSettings::SETTING_VIDEOPLAYER_USENVDECVP9,
      CSettings::SETTING_VIDEOPLAYER_USENVDECAV1,   CSettings::SETTING_VIDEOPLAYER_USENVDECMPEG2,
      CSettings::SETTING_VIDEOPLAYER_USENVDECMPEG4, CSettings::SETTING_VIDEOPLAYER_USENVDECVC1,
      CSettings::SETTING_VIDEOPLAYER_USENVDECVP8};

  for (const auto nvdecSetting : nvdecSettings)
  {
    auto setting = settings->GetSetting(nvdecSetting);
    if (!setting)
    {
      CLog::Log(LOGERROR, "NVDEC: Failed to load setting for: {}", nvdecSetting);
      continue;
    }

    setting->SetVisible(true);
  }
}

bool CDVDVideoCodecNVDEC::Open(CDVDStreamInfo& hints, CDVDCodecOptions& options)
{
  CLog::Log(LOGINFO, "CDVDVideoCodecNVDEC::Open() - Called for codec {}", hints.codec);

  // --- Settings gates ---

  auto settingsComponent = CServiceBroker::GetSettingsComponent();
  if (!settingsComponent)
    return false;

  auto settings = settingsComponent->GetSettings();
  if (!settings)
    return false;

  if (!IsVisibleSettingEnabled(settings, CSettings::SETTING_VIDEOPLAYER_USENVDEC))
  {
    CLog::Log(LOGINFO, "CDVDVideoCodecNVDEC::Open() - NVDEC is disabled (global)");
    return false;
  }

  const NvdecCodecInfo* codecInfo = FindNvdecCodecInfo(hints.codec);
  if (!codecInfo)
  {
    CLog::Log(LOGINFO, "CDVDVideoCodecNVDEC::Open() - No NVDEC config for codec {}", hints.codec);
    return false;
  }

  if (!IsVisibleSettingEnabled(settings, codecInfo->setting))
  {
    CLog::Log(LOGINFO, "CDVDVideoCodecNVDEC::Open() - NVDEC disabled for codec {}", hints.codec);
    return false;
  }

  // --- Find an FFmpeg decoder that supports CUDA hwaccel for this codec ---

  const AVCodec* pCodec = nullptr;
  {
    // The default decoder may be a pure-software implementation (e.g. libdav1d for AV1)
    // that doesn't advertise CUDA hwaccel.  Walk all decoders for the codec ID and pick
    // the first one that supports it.
    void* iter = nullptr;
    const AVCodec* c = nullptr;
    while ((c = av_codec_iterate(&iter)))
    {
      if (!av_codec_is_decoder(c) || c->id != hints.codec)
        continue;

      for (int i = 0;; ++i)
      {
        const AVCodecHWConfig* config = avcodec_get_hw_config(c, i);
        if (!config)
          break;

        if (config->device_type == AV_HWDEVICE_TYPE_CUDA &&
            (config->methods &
             (AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX | AV_CODEC_HW_CONFIG_METHOD_HW_FRAMES_CTX)))
        {
          pCodec = c;
          break;
        }
      }
      if (pCodec)
        break;
    }
  }

  if (!pCodec)
  {
    CLog::Log(LOGINFO, "CDVDVideoCodecNVDEC::Open() - No decoder with CUDA hwaccel for codec {}",
              hints.codec);
    return false;
  }

  CLog::Log(LOGINFO, "CDVDVideoCodecNVDEC::Open() - Using decoder '{}' with nvdec hwaccel",
            pCodec->name);

  // --- Clean state ---

  Dispose();
  av_buffer_unref(&m_hwDeviceRef);

  m_hints = hints;
  m_options = options;
  m_iOrientation = hints.orientation;

  // Bypass the base-class IHardwareDecoder selection logic.
  m_decoderState = STATE_SW_SINGLE;
  m_processInfo.SetSwDeinterlacingMethods();
  m_processInfo.SetVideoInterlaced(false);

  // Populate the supported pixel format list for the filter graph need_scale check.
  m_formats = m_processInfo.GetPixFormats();
#if LIBAVFILTER_BUILD < AV_VERSION_INT(10, 6, 100)
  m_formats.push_back(AV_PIX_FMT_NONE);
#endif

  // --- Create CUDA device context ---

  int ret = av_hwdevice_ctx_create(&m_hwDeviceRef, AV_HWDEVICE_TYPE_CUDA, nullptr, nullptr, 0);
  if (ret < 0)
  {
    CLog::Log(LOGERROR, "CDVDVideoCodecNVDEC::Open() - Failed to create CUDA device (error {})",
              ret);
    return false;
  }

  // --- Configure codec context ---

  m_pCodecContext = avcodec_alloc_context3(pCodec);
  if (!m_pCodecContext)
  {
    av_buffer_unref(&m_hwDeviceRef);
    return false;
  }

  m_pCodecContext->opaque = static_cast<ICallbackHWAccel*>(this);
  m_pCodecContext->debug = 0;
  m_pCodecContext->workaround_bugs = FF_BUG_AUTODETECT;
  m_pCodecContext->get_format = CDVDVideoCodecNVDEC::GetFormatNVDEC;
  m_pCodecContext->codec_tag = hints.codec_tag;

#if LIBAVCODEC_VERSION_MAJOR >= 60
  m_pCodecContext->flags = AV_CODEC_FLAG_COPY_OPAQUE;
#endif

  m_pCodecContext->thread_count = 1;

  m_pCodecContext->coded_height = hints.height;
  m_pCodecContext->coded_width = hints.width;
  m_pCodecContext->bits_per_coded_sample = hints.bitsperpixel;
  m_pCodecContext->bits_per_raw_sample = hints.bitdepth;

  if (hints.extradata)
  {
    m_pCodecContext->extradata =
        static_cast<uint8_t*>(av_mallocz(hints.extradata.GetSize() + AV_INPUT_BUFFER_PADDING_SIZE));
    if (m_pCodecContext->extradata)
    {
      m_pCodecContext->extradata_size = hints.extradata.GetSize();
      std::memcpy(m_pCodecContext->extradata, hints.extradata.GetData(), hints.extradata.GetSize());
    }
  }

  for (const auto& key : options.m_keys)
    av_opt_set(m_pCodecContext, key.m_name.c_str(), key.m_value.c_str(), 0);

  // Tell FFmpeg to use the nvdec hwaccel via CUDA.  FFmpeg will automatically
  // create hw_frames_ctx when get_format returns AV_PIX_FMT_CUDA.
  m_pCodecContext->hw_device_ctx = av_buffer_ref(m_hwDeviceRef);
  if (!m_pCodecContext->hw_device_ctx)
  {
    CLog::Log(LOGERROR, "CDVDVideoCodecNVDEC::Open() - Failed to ref hw_device_ctx");
    Dispose();
    av_buffer_unref(&m_hwDeviceRef);
    return false;
  }

  // --- Open ---

  if (avcodec_open2(m_pCodecContext, pCodec, nullptr) < 0)
  {
    CLog::Log(LOGDEBUG, "CDVDVideoCodecNVDEC::Open() - Unable to open codec");
    Dispose();
    av_buffer_unref(&m_hwDeviceRef);
    return false;
  }

  m_pFrame = av_frame_alloc();
  m_pDecodedFrame = av_frame_alloc();
  m_pFilterFrame = av_frame_alloc();
  if (!m_pFrame || !m_pDecodedFrame || !m_pFilterFrame)
  {
    Dispose();
    av_buffer_unref(&m_hwDeviceRef);
    return false;
  }

  m_name = std::string("ff-") + pCodec->name + "-nvdec";
  m_processInfo.SetVideoDecoderName(m_name, true);
  m_processInfo.SetVideoDimensions(m_pCodecContext->coded_width, m_pCodecContext->coded_height);

  if (!m_swFrame)
    m_swFrame = av_frame_alloc();
  if (!m_swFrame)
  {
    Dispose();
    av_buffer_unref(&m_hwDeviceRef);
    return false;
  }

  m_dropCtrl.Reset(true);
  m_eof = false;

  CLog::Log(LOGINFO, "CDVDVideoCodecNVDEC::Open() - nvdec hwaccel decoder opened successfully ({})",
            m_name);
  return true;
}

void CDVDVideoCodecNVDEC::Reopen()
{
  if (m_decoderState == STATE_HW_FAILED)
  {
    Dispose();
    av_frame_free(&m_swFrame);

    if (m_hwDeviceRef)
    {
      // First fallback: NVDEC failed.  Clean up the CUDA device and give the
      // base-class Open() a fresh start so it can try its own hardware path
      // (e.g. VAAPI) before resorting to software.
      CLog::Log(LOGINFO, "CDVDVideoCodecNVDEC::Reopen() - NVDEC failed, trying base-class decoder");
      av_buffer_unref(&m_hwDeviceRef);
      m_decoderState = STATE_NONE;
    }
    else
    {
      // Second fallback: the base-class hardware path also failed.
      // m_decoderState stays STATE_HW_FAILED so Open() skips hardware
      // and uses multi-threaded software (and libdav1d for AV1).
      CLog::Log(LOGINFO,
                "CDVDVideoCodecNVDEC::Reopen() - base-class HW failed, falling back to software");
    }

    if (!CDVDVideoCodecFFmpeg::Open(m_hints, m_options))
      Dispose();

    return;
  }

  CDVDVideoCodecFFmpeg::Reopen();
}

CDVDVideoCodec::VCReturn CDVDVideoCodecNVDEC::GetPicture(VideoPicture* pVideoPicture)
{
  // After Reopen() fell back to software, delegate entirely to the base class.
  // m_hwDeviceRef is null once Reopen() has cleaned up the CUDA device.
  if (!m_hwDeviceRef)
    return CDVDVideoCodecFFmpeg::GetPicture(pVideoPicture);

  // NVDEC hardware accel failed during decode — request a reopen so we can
  // fall back to software in Reopen().
  if (m_decoderState == STATE_HW_FAILED)
    return VC_REOPEN;

  if (!m_startedInput)
    return VC_BUFFER;

  if (m_eof)
    return VC_EOF;

  // Drain pending frames from the filter graph first.
  if (m_pFilterGraph && !m_filterEof)
  {
    CDVDVideoCodec::VCReturn ret = FilterProcess(nullptr);
    if (ret == VC_PICTURE)
    {
      if (!SetPictureParams(pVideoPicture))
        return VC_ERROR;
      return VC_PICTURE;
    }
    else if (ret != VC_BUFFER)
      return ret;
  }

  // Handle drain — match the base-class pattern of sending an empty packet each call.
  if (m_codecControlFlags & DVD_CODEC_CTRL_DRAIN)
  {
    AVPacket* avpkt = av_packet_alloc();
    if (!avpkt)
    {
      CLog::LogF(LOGERROR, "av_packet_alloc failed");
      return VC_ERROR;
    }
    avpkt->data = nullptr;
    avpkt->size = 0;
    avpkt->dts = AV_NOPTS_VALUE;
    avpkt->pts = AV_NOPTS_VALUE;
    avcodec_send_packet(m_pCodecContext, avpkt);

    av_packet_free(&avpkt);
  }

  // --- Receive decoded frame ---

  int ret = avcodec_receive_frame(m_pCodecContext, m_pDecodedFrame);

  if (m_decoderState == STATE_HW_FAILED)
    return VC_REOPEN;

  if (m_iLastKeyframe < m_pCodecContext->has_b_frames + 2)
    m_iLastKeyframe = m_pCodecContext->has_b_frames + 2;

  if (ret == AVERROR_EOF)
  {
    m_eof = true;
    CLog::Log(LOGDEBUG, "CDVDVideoCodecNVDEC::GetPicture - eof");
    return VC_EOF;
  }
  else if (ret == AVERROR(EAGAIN))
  {
    return VC_BUFFER;
  }
  else if (ret)
  {
    CLog::Log(LOGERROR, "CDVDVideoCodecNVDEC::GetPicture - avcodec_receive_frame failed ({})", ret);
    return VC_ERROR;
  }

  // --- GPU → CPU transfer ---

  av_frame_unref(m_swFrame);

  // av_hwframe_transfer_data auto-allocates the destination buffer and chooses
  // the hw_frames_ctx sw_format (NV12 for 8-bit, P010 for 10-bit).
  int xfer = av_hwframe_transfer_data(m_swFrame, m_pDecodedFrame, 0);
  if (xfer < 0)
  {
    CLog::Log(LOGERROR, "CDVDVideoCodecNVDEC::GetPicture - av_hwframe_transfer_data failed ({})",
              xfer);
    av_frame_unref(m_pDecodedFrame);
    m_decoderState = STATE_HW_FAILED;
    return VC_REOPEN;
  }

  // Pixel data was transferred but metadata was not — copy it.
  int cpRet = av_frame_copy_props(m_swFrame, m_pDecodedFrame);
  if (cpRet < 0)
  {
    CLog::Log(LOGERROR, "CDVDVideoCodecNVDEC::GetPicture - av_frame_copy_props failed ({})", cpRet);
    av_frame_unref(m_swFrame);
    av_frame_unref(m_pDecodedFrame);
    m_decoderState = STATE_HW_FAILED;
    return VC_REOPEN;
  }

  av_frame_unref(m_pDecodedFrame);
  av_frame_move_ref(m_pDecodedFrame, m_swFrame);

  // After the transfer, m_pDecodedFrame holds a CPU frame (NV12, P010, etc.).
  // We must NOT permanently change m_pCodecContext->pix_fmt from AV_PIX_FMT_CUDA,
  // because the H.264 decoder compares it against the expected format on each frame
  // and would re-call get_format (re-creating hw_frames_ctx) every time.
  const AVPixelFormat cpuFmt = static_cast<AVPixelFormat>(m_pDecodedFrame->format);
  const char* pixFmtName = av_get_pix_fmt_name(cpuFmt);
  m_processInfo.SetVideoPixelFormat(pixFmtName ? pixFmtName : "");

  {
    CDVDVideoCodec::VCReturn ret = ProcessDecodedFrame();
    if (ret != VC_NONE)
      return ret;
  }

  return ProcessFilterGraph(cpuFmt, pVideoPicture);
}
