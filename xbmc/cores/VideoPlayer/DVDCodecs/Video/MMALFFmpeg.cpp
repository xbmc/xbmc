/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include <interface/mmal/util/mmal_default_components.h>

#include "cores/VideoPlayer/VideoRenderers/RenderManager.h"
#include "../DVDCodecUtils.h"
#include "cores/VideoPlayer/DVDCodecs/DVDFactoryCodec.h"
#include "MMALFFmpeg.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "platform/linux/RBP.h"
#include "settings/AdvancedSettings.h"

extern "C" {
#include <libavutil/imgutils.h>
}

using namespace MMAL;

//-----------------------------------------------------------------------------
// MMAL Buffers
//-----------------------------------------------------------------------------

#define CLASSNAME "CMMALYUVBuffer"

#define VERBOSE 0

CMMALYUVBuffer::CMMALYUVBuffer(int id)
  : CMMALBuffer(id)
{
}

CMMALYUVBuffer::~CMMALYUVBuffer()
{
  delete m_gmem;
}

uint8_t* CMMALYUVBuffer::GetMemPtr()
{
  if (!m_gmem)
    return nullptr;
  return static_cast<uint8_t *>(m_gmem->m_arm);
}

void CMMALYUVBuffer::GetPlanes(uint8_t*(&planes)[YuvImage::MAX_PLANES])
{
  for (int i = 0; i < YuvImage::MAX_PLANES; i++)
    planes[i] = nullptr;

  std::shared_ptr<CMMALPool> pool = std::dynamic_pointer_cast<CMMALPool>(m_pool);
  assert(pool);
  AVRpiZcFrameGeometry geo = pool->GetGeometry();

  if (VERBOSE)
    CLog::Log(LOGDEBUG, LOGVIDEO, "%s::%s %dx%d %dx%d (%dx%d %dx%d)", CLASSNAME, __FUNCTION__, geo.getStrideY(), geo.getHeightY(), geo.getStrideC(), geo.getHeightC(), Width(), Height(), AlignedWidth(), AlignedHeight());

  planes[0] = GetMemPtr();
  if (planes[0] && geo.getPlanesC() >= 1)
    planes[1] = planes[0] + geo.getSizeY();
  if (planes[1] && geo.getPlanesC() >= 2)
    planes[2] = planes[1] + geo.getSizeC();
}

void CMMALYUVBuffer::GetStrides(int(&strides)[YuvImage::MAX_PLANES])
{
  for (int i = 0; i < YuvImage::MAX_PLANES; i++)
    strides[i] = 0;
  std::shared_ptr<CMMALPool> pool = std::dynamic_pointer_cast<CMMALPool>(m_pool);
  assert(pool);
  AVRpiZcFrameGeometry geo = pool->GetGeometry();
  strides[0] = geo.getStrideY();
  strides[1] = geo.getStrideC();
  strides[2] = geo.getStrideC();
}

void CMMALYUVBuffer::SetDimensions(int width, int height, const int (&strides)[YuvImage::MAX_PLANES], const int (&planeOffsets)[YuvImage::MAX_PLANES])
{
  std::shared_ptr<CMMALPool> pool = std::dynamic_pointer_cast<CMMALPool>(m_pool);
  assert(pool);
  pool->SetDimensions(width, height, strides, planeOffsets);
}

void CMMALYUVBuffer::SetDimensions(int width, int height, const int (&strides)[YuvImage::MAX_PLANES])
{
  const int (&planeOffsets)[YuvImage::MAX_PLANES] = {};
  SetDimensions(width, height, strides, planeOffsets);
}

CGPUMEM *CMMALYUVBuffer::Allocate(int size, void *opaque)
{
  m_gmem = new CGPUMEM(size, true);
  if (m_gmem && m_gmem->m_vc)
  {
    m_gmem->m_opaque = opaque;
  }
  else
  {
    delete m_gmem;
    m_gmem = nullptr;
  }
  return m_gmem;
}


//-----------------------------------------------------------------------------
// MMAL Decoder
//-----------------------------------------------------------------------------

#undef CLASSNAME
#define CLASSNAME "CDecoder"

void CDecoder::AlignedSize(AVCodecContext *avctx, int &width, int &height)
{
  if (!avctx)
    return;
  int w = width, h = height;
  AVFrame picture;
  int unaligned;
  int stride_align[AV_NUM_DATA_POINTERS];

  avcodec_align_dimensions2(avctx, &w, &h, stride_align);

  do {
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

CDecoder::CDecoder(CProcessInfo &processInfo, CDVDStreamInfo &hints) : m_processInfo(processInfo), m_hints(hints)
{
  CLog::Log(LOGDEBUG, LOGVIDEO, "%s::%s - create %p", CLASSNAME, __FUNCTION__, static_cast<void*>(this));
  m_avctx = nullptr;
  m_pool = nullptr;
}

CDecoder::~CDecoder()
{
  if (m_renderBuffer)
    m_renderBuffer->Release();
  CLog::Log(LOGDEBUG, LOGVIDEO, "%s::%s - destroy %p", CLASSNAME, __FUNCTION__, static_cast<void*>(this));
}

long CDecoder::Release()
{
  CLog::Log(LOGDEBUG, LOGVIDEO, "%s::%s - m_refs:%ld", CLASSNAME, __FUNCTION__, m_refs.load());
  return IHardwareDecoder::Release();
}

void CDecoder::FFReleaseBuffer(void *opaque, uint8_t *data)
{
  CGPUMEM *gmem = (CGPUMEM *)opaque;
  CMMALYUVBuffer *YUVBuffer = (CMMALYUVBuffer *)gmem->m_opaque;
  CLog::Log(LOGDEBUG, LOGVIDEO, "%s::%s buf:%p gmem:%p", CLASSNAME, __FUNCTION__,
            static_cast<void*>(YUVBuffer), static_cast<void*>(gmem));

  YUVBuffer->Release();
}

int CDecoder::FFGetBuffer(AVCodecContext *avctx, AVFrame *frame, int flags)
{
  ICallbackHWAccel* cb = static_cast<ICallbackHWAccel*>(avctx->opaque);
  CDecoder* dec = static_cast<CDecoder*>(cb->GetHWAccel());
  CLog::Log(LOGDEBUG, LOGVIDEO, "%s::%s %dx%d format:%x:%x flags:%x", CLASSNAME, __FUNCTION__, frame->width, frame->height, frame->format, dec->m_fmt, flags);

  if ((avctx->codec && (avctx->codec->capabilities & AV_CODEC_CAP_DR1) == 0) || frame->format != dec->m_fmt)
  {
    assert(0);
    return avcodec_default_get_buffer2(avctx, frame, flags);
  }

  std::shared_ptr<CMMALPool> pool = std::dynamic_pointer_cast<CMMALPool>(dec->m_pool);
  if (!pool->IsConfigured())
  {
    int aligned_width = frame->width;
    int aligned_height = frame->height;
    if (pool->Encoding() != MMAL_ENCODING_YUVUV128 && pool->Encoding() != MMAL_ENCODING_YUVUV64_16)
    {
      // ffmpeg requirements
      AlignedSize(dec->m_avctx, aligned_width, aligned_height);
      // GPU requirements
      aligned_width = ALIGN_UP(aligned_width, 32);
      aligned_height = ALIGN_UP(aligned_height, 16);
    }
    pool->Configure(dec->m_fmt, frame->width, frame->height, aligned_width, aligned_height, 0);
  }
  CMMALYUVBuffer *YUVBuffer = dynamic_cast<CMMALYUVBuffer *>(pool->Get());
  if (!YUVBuffer)
  {
    CLog::Log(LOGERROR,"%s::%s Failed to allocated buffer in time", CLASSNAME, __FUNCTION__);
    return -1;
  }
  assert(YUVBuffer->mmal_buffer);

  CGPUMEM *gmem = YUVBuffer->GetMem();
  assert(gmem);

  AVBufferRef *buf = av_buffer_create((uint8_t *)gmem->m_arm, gmem->m_numbytes, CDecoder::FFReleaseBuffer, gmem, AV_BUFFER_FLAG_READONLY);
  if (!buf)
  {
    CLog::Log(LOGERROR, "%s::%s av_buffer_create() failed", CLASSNAME, __FUNCTION__);
    YUVBuffer->Release();
    return -1;
  }

  uint8_t *planes[YuvImage::MAX_PLANES];
  int strides[YuvImage::MAX_PLANES];
  YUVBuffer->GetPlanes(planes);
  YUVBuffer->GetStrides(strides);

  for (int i = 0; i < AV_NUM_DATA_POINTERS; i++)
  {
    frame->data[i] = i < YuvImage::MAX_PLANES ? planes[i] : nullptr;
    frame->linesize[i] = i < YuvImage::MAX_PLANES ? strides[i] : 0;
    frame->buf[i] = i == 0 ? buf : nullptr;
  }

  frame->extended_data = frame->data;
  // Leave extended buf alone

  CLog::Log(LOGDEBUG, LOGVIDEO, "%s::%s buf:%p mmal:%p gmem:%p avbuf:%p:%p:%p", CLASSNAME,
            __FUNCTION__, static_cast<void*>(YUVBuffer), static_cast<void*>(YUVBuffer->mmal_buffer),
            static_cast<void*>(gmem), static_cast<void*>(frame->data[0]),
            static_cast<void*>(frame->data[1]), static_cast<void*>(frame->data[2]));

  return 0;
}


bool CDecoder::Open(AVCodecContext *avctx, AVCodecContext* mainctx, enum AVPixelFormat fmt)
{
  CSingleLock lock(m_section);

  CLog::Log(LOGINFO, "%s::%s - fmt:%d", CLASSNAME, __FUNCTION__, fmt);

  CLog::Log(LOGDEBUG, "%s::%s MMAL - source requires %d references", CLASSNAME, __FUNCTION__, avctx->refs);

  avctx->get_buffer2 = CDecoder::FFGetBuffer;
  mainctx->get_buffer2 = CDecoder::FFGetBuffer;

  m_avctx = mainctx;
  m_fmt = fmt;

  /* Create dummy component with attached pool */
  m_pool = std::make_shared<CMMALPool>(MMAL_COMPONENT_DEFAULT_VIDEO_DECODER, false, MMAL_NUM_OUTPUT_BUFFERS, 0, MMAL_ENCODING_UNKNOWN, MMALStateFFDec);
  if (!m_pool)
  {
    CLog::Log(LOGERROR, "%s::%s Failed to create pool for decoder output", CLASSNAME, __func__);
    return false;
  }

  std::shared_ptr<CMMALPool> pool = std::dynamic_pointer_cast<CMMALPool>(m_pool);
  pool->SetProcessInfo(&m_processInfo);

  std::list<EINTERLACEMETHOD> deintMethods;
  deintMethods.push_back(EINTERLACEMETHOD::VS_INTERLACEMETHOD_AUTO);
  deintMethods.push_back(EINTERLACEMETHOD::VS_INTERLACEMETHOD_MMAL_ADVANCED);
  deintMethods.push_back(EINTERLACEMETHOD::VS_INTERLACEMETHOD_MMAL_ADVANCED_HALF);
  deintMethods.push_back(EINTERLACEMETHOD::VS_INTERLACEMETHOD_MMAL_BOB);
  deintMethods.push_back(EINTERLACEMETHOD::VS_INTERLACEMETHOD_MMAL_BOB_HALF);
  m_processInfo.UpdateDeinterlacingMethods(deintMethods);

  return true;
}

CDVDVideoCodec::VCReturn CDecoder::Decode(AVCodecContext* avctx, AVFrame* frame)
{
  CSingleLock lock(m_section);

  if (frame)
  {
    if ((frame->format != AV_PIX_FMT_YUV420P && frame->format != AV_PIX_FMT_YUV420P10 && frame->format != AV_PIX_FMT_YUV420P12 && frame->format != AV_PIX_FMT_YUV420P14 && frame->format != AV_PIX_FMT_YUV420P16 &&
        frame->format != AV_PIX_FMT_BGR0 && frame->format != AV_PIX_FMT_RGB565LE) ||
        frame->buf[1] != nullptr || frame->buf[0] == nullptr)
    {
      CLog::Log(LOGERROR, "%s::%s frame format invalid format:%d buf:%p,%p", CLASSNAME, __func__,
                frame->format, static_cast<void*>(frame->buf[0]),
                static_cast<void*>(frame->buf[1]));
      return CDVDVideoCodec::VC_ERROR;
    }
    CVideoBuffer *old = m_renderBuffer;
    if (m_renderBuffer)
      m_renderBuffer->Release();

    CGPUMEM *m_gmem = (CGPUMEM *)av_buffer_get_opaque(frame->buf[0]);
    assert(m_gmem);
    // need to flush ARM cache so GPU can see it
    m_gmem->Flush();
    m_renderBuffer = static_cast<CMMALYUVBuffer*>(m_gmem->m_opaque);
    assert(m_renderBuffer && m_renderBuffer->mmal_buffer);
    if (m_renderBuffer)
    {
      m_renderBuffer->m_stills = m_hints.stills;
      CLog::Log(LOGDEBUG, LOGVIDEO, "%s::%s - mmal:%p buf:%p old:%p gpu:%p %dx%d (%dx%d)",
                CLASSNAME, __FUNCTION__, static_cast<void*>(m_renderBuffer->mmal_buffer),
                static_cast<void*>(m_renderBuffer), static_cast<void*>(old),
                static_cast<void*>(m_renderBuffer->GetMem()), m_renderBuffer->Width(),
                m_renderBuffer->Height(), m_renderBuffer->AlignedWidth(),
                m_renderBuffer->AlignedHeight());
      m_renderBuffer->Acquire();
    }
  }

  CDVDVideoCodec::VCReturn status = Check(avctx);
  if (status != CDVDVideoCodec::VC_NONE)
    return status;

  if (frame)
    return CDVDVideoCodec::VC_PICTURE;
  else
    return CDVDVideoCodec::VC_BUFFER;
}

bool CDecoder::GetPicture(AVCodecContext* avctx, VideoPicture* picture)
{
  CSingleLock lock(m_section);

  bool ret = ((ICallbackHWAccel*)avctx->opaque)->GetPictureCommon(picture);
  if (!ret || !m_renderBuffer)
    return false;

  CVideoBuffer *old = picture->videoBuffer;
  if (picture->videoBuffer)
    picture->videoBuffer->Release();

  picture->videoBuffer = m_renderBuffer;
  CLog::Log(
      LOGDEBUG, LOGVIDEO, "%s::%s - mmal:%p dts:%.3f pts:%.3f buf:%p old:%p gpu:%p %dx%d (%dx%d)",
      CLASSNAME, __FUNCTION__, static_cast<void*>(m_renderBuffer->mmal_buffer), 1e-6 * picture->dts,
      1e-6 * picture->pts, static_cast<void*>(m_renderBuffer), static_cast<void*>(old),
      static_cast<void*>(m_renderBuffer->GetMem()), m_renderBuffer->Width(),
      m_renderBuffer->Height(), m_renderBuffer->AlignedWidth(), m_renderBuffer->AlignedHeight());
  picture->videoBuffer->Acquire();

  return true;
}

CDVDVideoCodec::VCReturn CDecoder::Check(AVCodecContext* avctx)
{
  CSingleLock lock(m_section);
  return CDVDVideoCodec::VC_NONE;
}

unsigned CDecoder::GetAllowedReferences()
{
  return 6;
}

IHardwareDecoder* CDecoder::Create(CDVDStreamInfo &hint, CProcessInfo &processInfo, AVPixelFormat fmt)
 {
   return new CDecoder(processInfo, hint);
 }

void CDecoder::Register()
{
  CDVDFactoryCodec::RegisterHWAccel("mmalffmpeg", CDecoder::Create);
}
