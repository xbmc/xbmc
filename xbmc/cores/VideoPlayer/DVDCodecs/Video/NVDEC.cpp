/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "NVDEC.h"

#include "DVDCodecs/DVDFactoryCodec.h"
#include "ServiceBroker.h"
#include "cores/VideoPlayer/DVDStreamInfo.h"
#include "cores/VideoPlayer/Interface/TimingConstants.h"
#include "cores/VideoPlayer/Process/ProcessInfo.h"
#include "rendering/RenderSystem.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "utils/log.h"

#include <atomic>
#include <list>
#include <mutex>
#include <optional>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/hwcontext_cuda.h>
#include <libavutil/pixdesc.h>
}

using namespace NVDEC;

namespace
{
std::atomic<bool> interopFailed{false};

bool IsSupportedSwFormat(AVPixelFormat fmt)
{
  // Two-plane layouts the GL renderer handles with its NV12 shader.
  return fmt == AV_PIX_FMT_NV12 || fmt == AV_PIX_FMT_P010 || fmt == AV_PIX_FMT_P016;
}

CuvidFunctions* GetCuvidFunctions()
{
  static CuvidFunctions* functions = []() -> CuvidFunctions*
  {
    CuvidFunctions* f = nullptr;
    if (cuvid_load_functions(&f, nullptr) < 0 || !f || !f->cuvidGetDecoderCaps)
    {
      if (f)
        cuvid_free_functions(&f);
      return nullptr;
    }
    return f;
  }();
  return functions;
}

std::optional<cudaVideoCodec> ToCudaCodec(AVCodecID id)
{
  switch (id)
  {
    case AV_CODEC_ID_MPEG1VIDEO:
      return cudaVideoCodec_MPEG1;
    case AV_CODEC_ID_MPEG2VIDEO:
      return cudaVideoCodec_MPEG2;
    case AV_CODEC_ID_MPEG4:
      return cudaVideoCodec_MPEG4;
    case AV_CODEC_ID_VC1:
    case AV_CODEC_ID_WMV3:
      return cudaVideoCodec_VC1;
    case AV_CODEC_ID_H264:
      return cudaVideoCodec_H264;
    case AV_CODEC_ID_HEVC:
      return cudaVideoCodec_HEVC;
    case AV_CODEC_ID_VP8:
      return cudaVideoCodec_VP8;
    case AV_CODEC_ID_VP9:
      return cudaVideoCodec_VP9;
    case AV_CODEC_ID_AV1:
      return cudaVideoCodec_AV1;
    case AV_CODEC_ID_MJPEG:
      return cudaVideoCodec_JPEG;
    default:
      return std::nullopt;
  }
}

//! Asks the driver whether the GPU decodes this codec at this chroma format,
//! bit depth and size.
bool IsSupportedByHardware(const AVCodecContext* avctx, AVBufferRef* deviceRef)
{
  const auto codec = ToCudaCodec(avctx->codec_id);
  const AVPixFmtDescriptor* desc = av_pix_fmt_desc_get(avctx->sw_pix_fmt);
  CuvidFunctions* cuvid = GetCuvidFunctions();
  if (!codec || !desc || !cuvid)
    return false;

  CUVIDDECODECAPS caps{};
  caps.eCodecType = *codec;
  if (desc->nb_components < 3)
    caps.eChromaFormat = cudaVideoChromaFormat_Monochrome;
  else if (desc->log2_chroma_w == 1 && desc->log2_chroma_h == 1)
    caps.eChromaFormat = cudaVideoChromaFormat_420;
  else if (desc->log2_chroma_w == 1)
    caps.eChromaFormat = cudaVideoChromaFormat_422;
  else
    caps.eChromaFormat = cudaVideoChromaFormat_444;
  caps.nBitDepthMinus8 = static_cast<unsigned int>(desc->comp[0].depth - 8);

  const auto* device = reinterpret_cast<const AVHWDeviceContext*>(deviceRef->data);
  const auto* hwctx = static_cast<const AVCUDADeviceContext*>(device->hwctx);
  CudaFunctions* cu = GetCudaFunctions();
  if (cu->cuCtxPushCurrent(hwctx->cuda_ctx) != CUDA_SUCCESS)
    return false;
  const CUresult result = cuvid->cuvidGetDecoderCaps(&caps);
  CUcontext dummy;
  cu->cuCtxPopCurrent(&dummy);

  if (result != CUDA_SUCCESS || !caps.bIsSupported)
    return false;

  const auto width = static_cast<unsigned int>(avctx->coded_width);
  const auto height = static_cast<unsigned int>(avctx->coded_height);
  if (width && height &&
      (width > caps.nMaxWidth || height > caps.nMaxHeight ||
       width < static_cast<unsigned int>(caps.nMinWidth) ||
       height < static_cast<unsigned int>(caps.nMinHeight)))
    return false;

  return true;
}
} // namespace

CudaFunctions* NVDEC::GetCudaFunctions()
{
  static CudaFunctions* functions = []() -> CudaFunctions*
  {
    CudaFunctions* f = nullptr;
    if (cuda_load_functions(&f, nullptr) < 0 || !f)
    {
      CLog::Log(LOGINFO, "NVDEC: CUDA driver library not available");
      return nullptr;
    }
    if (f->cuInit(0) != CUDA_SUCCESS)
    {
      CLog::Log(LOGINFO, "NVDEC: cuInit failed, no usable NVIDIA GPU");
      cuda_free_functions(&f);
      return nullptr;
    }
    return f;
  }();
  return functions;
}

void NVDEC::SetInteropFailed()
{
  if (!interopFailed.exchange(true))
    CLog::Log(LOGERROR, "NVDEC: GL interop failed, falling back to software decoding");
}

//------------------------------------------------------------------------------
// Video buffers
//------------------------------------------------------------------------------

CVideoBufferNVDEC::CVideoBufferNVDEC(int id) : CVideoBuffer(id)
{
  // Lets the render manager tell these buffers from other hwaccel buffers.
  m_pixFormat = AV_PIX_FMT_CUDA;
  m_frame = av_frame_alloc();
}

CVideoBufferNVDEC::~CVideoBufferNVDEC()
{
  av_frame_free(&m_frame);
}

bool CVideoBufferNVDEC::SetRef(AVFrame* frame)
{
  if (!m_frame)
    m_frame = av_frame_alloc();
  if (!m_frame)
    return false;

  av_frame_unref(m_frame);
  if (av_frame_ref(m_frame, frame) < 0 || !m_frame->hw_frames_ctx)
  {
    av_frame_unref(m_frame);
    return false;
  }
  const auto* frames = reinterpret_cast<AVHWFramesContext*>(m_frame->hw_frames_ctx->data);
  m_swFormat = frames->sw_format;
  return true;
}

void CVideoBufferNVDEC::Unref()
{
  if (m_frame)
    av_frame_unref(m_frame);
}

CUcontext CVideoBufferNVDEC::GetCudaContext() const
{
  if (!m_frame || !m_frame->hw_frames_ctx)
    return nullptr;
  const auto* frames = reinterpret_cast<AVHWFramesContext*>(m_frame->hw_frames_ctx->data);
  return static_cast<AVCUDADeviceContext*>(frames->device_ctx->hwctx)->cuda_ctx;
}

AVBufferRef* CVideoBufferNVDEC::GetDeviceRef() const
{
  if (!m_frame || !m_frame->hw_frames_ctx)
    return nullptr;
  return reinterpret_cast<AVHWFramesContext*>(m_frame->hw_frames_ctx->data)->device_ref;
}

CVideoBufferPoolNVDEC::~CVideoBufferPoolNVDEC()
{
  for (auto* buf : m_all)
    delete buf;
}

CVideoBuffer* CVideoBufferPoolNVDEC::Get()
{
  std::unique_lock lock(m_critSection);

  CVideoBufferNVDEC* buf = nullptr;
  if (!m_free.empty())
  {
    const int idx = m_free.front();
    m_free.pop_front();
    m_used.push_back(idx);
    buf = m_all[idx];
  }
  else
  {
    const int id = static_cast<int>(m_all.size());
    buf = new CVideoBufferNVDEC(id);
    m_all.push_back(buf);
    m_used.push_back(id);
  }

  buf->Acquire(GetPtr());
  return buf;
}

void CVideoBufferPoolNVDEC::Return(int id)
{
  std::unique_lock lock(m_critSection);

  m_all[id]->Unref();
  for (auto it = m_used.begin(); it != m_used.end(); ++it)
  {
    if (*it == id)
    {
      m_used.erase(it);
      break;
    }
  }
  m_free.push_back(id);
}

//------------------------------------------------------------------------------
// Decoder
//------------------------------------------------------------------------------

IHardwareDecoder* CDecoder::Create(CDVDStreamInfo& hint,
                                   CProcessInfo& processInfo,
                                   AVPixelFormat fmt)
{
  if (fmt != AV_PIX_FMT_CUDA || interopFailed || !GetCudaFunctions())
    return nullptr;

  // The renderer copies frames with CUDA-GL interop, which needs GL on the NVIDIA GPU.
  // On hybrid systems GL may run on another GPU; leave decoding to its hwaccel then.
  const CRenderSystemBase* renderSystem = CServiceBroker::GetRenderSystem();
  if (!renderSystem || renderSystem->GetRenderVendor().find("NVIDIA") == std::string::npos)
    return nullptr;

  if (!CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
          CSettings::SETTING_VIDEOPLAYER_USENVDEC))
    return nullptr;

  // Without bwdif_cuda (FFmpeg built without cuda-llvm) interlaced frames would be shown
  // as they are. Leave interlaced streams to software decoding, which deinterlaces.
  if (hint.interlaced && !avfilter_get_by_name("bwdif_cuda") &&
      processInfo.GetVideoSettings().m_InterlaceMethod != EINTERLACEMETHOD::VS_INTERLACEMETHOD_NONE)
  {
    CLog::Log(LOGINFO, "NVDEC: no bwdif_cuda, leaving the interlaced stream to software decoding");
    return nullptr;
  }

  return new CDecoder(processInfo);
}

void CDecoder::Register()
{
  if (!GetCudaFunctions())
    return;

  CDVDFactoryCodec::RegisterHWAccel("nvdec", CDecoder::Create);

  auto settingsComponent = CServiceBroker::GetSettingsComponent();
  if (!settingsComponent || !settingsComponent->GetSettings())
    return;

  auto setting =
      settingsComponent->GetSettings()->GetSetting(CSettings::SETTING_VIDEOPLAYER_USENVDEC);
  if (!setting)
    CLog::Log(LOGERROR, "Failed to load setting for: {}", CSettings::SETTING_VIDEOPLAYER_USENVDEC);
  else
    setting->SetVisible(true);
}

CDecoder::CDecoder(CProcessInfo& processInfo)
  : m_processInfo(processInfo),
    m_videoBufferPool(std::make_shared<CVideoBufferPoolNVDEC>())
{
}

CDecoder::~CDecoder()
{
  if (m_renderBuffer)
    m_renderBuffer->Release();
  CloseDeinterlacer();
  av_frame_free(&m_filterFrame);
  av_buffer_unref(&m_deviceRef);
}

bool CDecoder::Open(AVCodecContext* avctx, AVCodecContext* mainctx, const enum AVPixelFormat fmt)
{
  if (av_hwdevice_ctx_create(&m_deviceRef, AV_HWDEVICE_TYPE_CUDA, nullptr, nullptr, 0) < 0)
  {
    CLog::Log(LOGERROR, "NVDEC: failed to create CUDA device context");
    return false;
  }

  if (!IsSupportedByHardware(avctx, m_deviceRef))
  {
    CLog::Log(LOGINFO, "NVDEC: {} {} not supported by this GPU", avcodec_get_name(avctx->codec_id),
              av_get_pix_fmt_name(avctx->sw_pix_fmt));
    return false;
  }

  // Same margin FFmpeg adds when it sets up the frames context itself. Output frames
  // are copies, the renderer and the deinterlacer don't hold decoder surfaces.
  avctx->extra_hw_frames = 3;

  AVBufferRef* framesRef = nullptr;
  if (avcodec_get_hw_frames_parameters(avctx, m_deviceRef, AV_PIX_FMT_CUDA, &framesRef) < 0)
  {
    CLog::Log(LOGINFO, "NVDEC: stream not supported by the hardware decoder");
    return false;
  }

  auto* frames = reinterpret_cast<AVHWFramesContext*>(framesRef->data);
  if (!IsSupportedSwFormat(frames->sw_format))
  {
    CLog::Log(LOGINFO, "NVDEC: unsupported surface format {}",
              av_get_pix_fmt_name(frames->sw_format));
    av_buffer_unref(&framesRef);
    return false;
  }

  if (av_hwframe_ctx_init(framesRef) < 0)
  {
    CLog::Log(LOGERROR, "NVDEC: failed to initialize CUDA frames context");
    av_buffer_unref(&framesRef);
    return false;
  }

  av_buffer_unref(&avctx->hw_frames_ctx);
  avctx->hw_frames_ctx = framesRef;

  // Interlaced frames are deinterlaced on the GPU with bwdif_cuda.
  m_processInfo.SetVideoDeintMethod("none");
  std::list<EINTERLACEMETHOD> deintMethods{EINTERLACEMETHOD::VS_INTERLACEMETHOD_NONE,
                                           EINTERLACEMETHOD::VS_INTERLACEMETHOD_AUTO};
  m_processInfo.UpdateDeinterlacingMethods(deintMethods);

  CLog::Log(LOGINFO, "NVDEC: using hardware decoding ({}x{}, {})", frames->width, frames->height,
            av_get_pix_fmt_name(frames->sw_format));
  return true;
}

CDVDVideoCodec::VCReturn CDecoder::Decode(AVCodecContext* avctx, AVFrame* frame)
{
  // Makes VideoPlayer reopen the codec, Create() then declines.
  if (interopFailed)
    return CDVDVideoCodec::VC_FATAL;

  if (!frame)
  {
    // The deinterlacer outputs two frames per interlaced input frame.
    if (!m_filterGraph)
      return CDVDVideoCodec::VC_BUFFER;

    // At the end of the stream flush the frame the deinterlacer holds back.
    if ((m_codecControl & DVD_CODEC_CTRL_DRAIN) && !m_filterEof)
    {
      if (av_buffersrc_add_frame(m_bufferSrc, nullptr) < 0)
        CLog::Log(LOGERROR, "NVDEC: failed to flush the deinterlacer");
      m_filterEof = true;
    }
    return PullDeinterlacedFrame() ? CDVDVideoCodec::VC_PICTURE : CDVDVideoCodec::VC_BUFFER;
  }

  if (frame->format != AV_PIX_FMT_CUDA || !frame->hw_frames_ctx)
    return CDVDVideoCodec::VC_FATAL;

  if (m_deintFailed || m_processInfo.GetVideoSettings().m_InterlaceMethod ==
                           EINTERLACEMETHOD::VS_INTERLACEMETHOD_NONE)
    CloseDeinterlacer();
  else if (!m_filterGraph && (frame->flags & AV_FRAME_FLAG_INTERLACED))
    InitDeinterlacer(frame);

  // Once running, the deinterlacer gets every frame so the output stays in order.
  // It passes progressive frames through unchanged.
  if (m_filterGraph)
  {
    AVFrame* input = av_frame_clone(frame);
    if (!input)
      return CDVDVideoCodec::VC_ERROR;
    input->pts = frame->best_effort_timestamp;
    const int ret = av_buffersrc_add_frame(m_bufferSrc, input);
    av_frame_free(&input);
    if (ret < 0)
    {
      CLog::Log(LOGERROR, "NVDEC: failed to feed the deinterlacer");
      return CDVDVideoCodec::VC_ERROR;
    }
    return PullDeinterlacedFrame() ? CDVDVideoCodec::VC_PICTURE : CDVDVideoCodec::VC_BUFFER;
  }

  m_filtered = false;
  return SetRenderBuffer(frame) ? CDVDVideoCodec::VC_PICTURE : CDVDVideoCodec::VC_ERROR;
}

bool CDecoder::GetPicture(AVCodecContext* avctx, VideoPicture* picture)
{
  static_cast<ICallbackHWAccel*>(avctx->opaque)->GetPictureCommon(picture);

  if (m_filtered)
  {
    // The deinterlacer delays its output, take the timestamp from the filtered frame.
    // Each field has its own timestamp, so no repeat-picture shift (as with VDPAU).
    picture->pts = m_deintPts;
    picture->dts = DVD_NOPTS_VALUE;
    picture->iRepeatPicture = 0.0;
  }

  // The renderer draws whole frames only. Without the deinterlacer it would
  // otherwise be asked for single fields.
  picture->iFlags &= ~DVP_FLAG_INTERLACED;

  if (picture->videoBuffer)
    picture->videoBuffer->Release();

  picture->videoBuffer = m_renderBuffer;
  picture->videoBuffer->Acquire();
  return true;
}

void CDecoder::Reset()
{
  CloseDeinterlacer();
}

bool CDecoder::SetRenderBuffer(AVFrame* frame)
{
  if (m_renderBuffer)
    m_renderBuffer->Release();

  m_renderBuffer = static_cast<CVideoBufferNVDEC*>(m_videoBufferPool->Get());
  if (m_renderBuffer->SetRef(frame))
    return true;

  CLog::Log(LOGERROR, "NVDEC: failed to reference a decoded frame");
  m_renderBuffer->Release();
  m_renderBuffer = nullptr;
  return false;
}

bool CDecoder::InitDeinterlacer(const AVFrame* frame)
{
  m_filterGraph = avfilter_graph_alloc();
  if (!m_filterFrame)
    m_filterFrame = av_frame_alloc();

  const AVFilter* bwdif = avfilter_get_by_name("bwdif_cuda");
  AVBufferSrcParameters* params = av_buffersrc_parameters_alloc();
  bool ok = m_filterGraph && m_filterFrame && bwdif && params;

  if (ok)
  {
    m_bufferSrc = avfilter_graph_alloc_filter(m_filterGraph, avfilter_get_by_name("buffer"), "in");
    params->format = AV_PIX_FMT_CUDA;
    params->width = frame->width;
    params->height = frame->height;
    params->time_base = AV_TIME_BASE_Q;
    params->sample_aspect_ratio = frame->sample_aspect_ratio;
    params->hw_frames_ctx = frame->hw_frames_ctx;
    ok = m_bufferSrc && av_buffersrc_parameters_set(m_bufferSrc, params) >= 0 &&
         avfilter_init_str(m_bufferSrc, nullptr) >= 0;
  }
  av_free(params);

  AVFilterContext* deint = nullptr;
  ok = ok &&
       avfilter_graph_create_filter(&deint, bwdif, "deint", "mode=send_field:deint=interlaced",
                                    nullptr, m_filterGraph) >= 0 &&
       avfilter_graph_create_filter(&m_bufferSink, avfilter_get_by_name("buffersink"), "out",
                                    nullptr, nullptr, m_filterGraph) >= 0 &&
       avfilter_link(m_bufferSrc, 0, deint, 0) >= 0 &&
       avfilter_link(deint, 0, m_bufferSink, 0) >= 0 &&
       avfilter_graph_config(m_filterGraph, nullptr) >= 0;

  if (!ok)
  {
    CLog::Log(LOGERROR, "NVDEC: bwdif_cuda deinterlacer not available, showing interlaced frames");
    CloseDeinterlacer();
    m_deintFailed = true;
    return false;
  }

  m_processInfo.SetVideoDeintMethod("bwdif_cuda");
  CLog::Log(LOGINFO, "NVDEC: deinterlacing with bwdif_cuda");
  return true;
}

void CDecoder::CloseDeinterlacer()
{
  avfilter_graph_free(&m_filterGraph);
  m_bufferSrc = nullptr;
  m_bufferSink = nullptr;
  m_filterEof = false;
  m_filtered = false;
}

bool CDecoder::PullDeinterlacedFrame()
{
  av_frame_unref(m_filterFrame);
  const int ret = av_buffersink_get_frame(m_bufferSink, m_filterFrame);
  if (ret < 0)
  {
    // EAGAIN: needs more input, EOF: drained. Anything else is a filter failure.
    if (ret != AVERROR(EAGAIN) && ret != AVERROR_EOF)
    {
      CLog::Log(LOGERROR, "NVDEC: deinterlacer failed, showing interlaced frames");
      CloseDeinterlacer();
      m_deintFailed = true;
    }
    return false;
  }

  const AVRational timeBase = av_buffersink_get_time_base(m_bufferSink);
  m_deintPts = m_filterFrame->pts == AV_NOPTS_VALUE
                   ? DVD_NOPTS_VALUE
                   : m_filterFrame->pts * av_q2d(timeBase) * DVD_TIME_BASE;
  m_filtered = true;
  return SetRenderBuffer(m_filterFrame);
}

CDVDVideoCodec::VCReturn CDecoder::Check(AVCodecContext* avctx)
{
  return CDVDVideoCodec::VC_NONE;
}

unsigned CDecoder::GetAllowedReferences()
{
  return 5;
}
