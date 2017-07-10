/*
 *      Copyright (C) 2016 Team Kodi
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "system.h"
#ifdef HAS_MMAL

#include <interface/mmal/util/mmal_default_components.h>

#include "cores/VideoPlayer/VideoRenderers/RenderManager.h"
#include "../DVDCodecUtils.h"
#include "cores/VideoPlayer/DVDCodecs/DVDFactoryCodec.h"
#include "MMALFFmpeg.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "linux/RBP.h"
#include "settings/AdvancedSettings.h"

extern "C" {
#include "libavutil/imgutils.h"
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

void CMMALYUVBuffer::GetPlanes(uint8_t*(&planes)[YuvImage::MAX_PLANES])
{
  for (int i = 0; i < YuvImage::MAX_PLANES; i++)
    planes[i] = nullptr;
  if (!m_gmem)
    return;

  std::shared_ptr<CMMALPool> pool = std::dynamic_pointer_cast<CMMALPool>(m_pool);
  assert(pool);
  AVRpiZcFrameGeometry geo = pool->GetGeometry();
  const int size_y = geo.stride_y * geo.height_y;
  const int size_c = geo.stride_c * geo.height_c;

  if (VERBOSE && g_advancedSettings.CanLogComponent(LOGVIDEO))
    CLog::Log(LOGDEBUG, "%s::%s %dx%d %dx%d (%dx%d %dx%d)", CLASSNAME, __FUNCTION__, geo.stride_y, geo.height_y, geo.stride_c, geo.height_c, Width(), Height(), AlignedWidth(), AlignedHeight());

  planes[0] = static_cast<uint8_t *>(m_gmem->m_arm);
  if (geo.planes_c >= 1)
    planes[1] = planes[0] + size_y;
  if (geo.planes_c >= 2)
    planes[2] = planes[1] + size_c;
}

void CMMALYUVBuffer::GetStrides(int(&strides)[YuvImage::MAX_PLANES])
{
  for (int i = 0; i < YuvImage::MAX_PLANES; i++)
    strides[i] = 0;
  std::shared_ptr<CMMALPool> pool = std::dynamic_pointer_cast<CMMALPool>(m_pool);
  assert(pool);
  AVRpiZcFrameGeometry geo = pool->GetGeometry();
  strides[0] = geo.stride_y;
  strides[1] = geo.stride_c;
  strides[2] = geo.stride_c;
}

void CMMALYUVBuffer::SetDimensions(int width, int height, const int (&strides)[YuvImage::MAX_PLANES])
{
  std::shared_ptr<CMMALPool> pool = std::dynamic_pointer_cast<CMMALPool>(m_pool);
  assert(pool);
  pool->SetDimensions(width, height, strides[0], height);
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
  if (g_advancedSettings.CanLogComponent(LOGVIDEO))
    CLog::Log(LOGDEBUG, "%s::%s - create %p", CLASSNAME, __FUNCTION__, this);
  m_avctx = nullptr;
  m_pool = nullptr;
}

CDecoder::~CDecoder()
{
  if (m_renderBuffer)
    m_renderBuffer->Release();
  if (g_advancedSettings.CanLogComponent(LOGVIDEO))
    CLog::Log(LOGDEBUG, "%s::%s - destroy %p", CLASSNAME, __FUNCTION__, this);
}

long CDecoder::Release()
{
  if (g_advancedSettings.CanLogComponent(LOGVIDEO))
    CLog::Log(LOGDEBUG, "%s::%s - m_refs:%ld", CLASSNAME, __FUNCTION__, m_refs.load());
  return IHardwareDecoder::Release();
}

void CDecoder::FFReleaseBuffer(void *opaque, uint8_t *data)
{
  CGPUMEM *gmem = (CGPUMEM *)opaque;
  CMMALYUVBuffer *YUVBuffer = (CMMALYUVBuffer *)gmem->m_opaque;
  if (g_advancedSettings.CanLogComponent(LOGVIDEO))
    CLog::Log(LOGDEBUG,"%s::%s buf:%p gmem:%p", CLASSNAME, __FUNCTION__, YUVBuffer, gmem);

  YUVBuffer->Release();
}

int CDecoder::FFGetBuffer(AVCodecContext *avctx, AVFrame *frame, int flags)
{
  ICallbackHWAccel* cb = static_cast<ICallbackHWAccel*>(avctx->opaque);
  CDecoder* dec = static_cast<CDecoder*>(cb->GetHWAccel());
  if (g_advancedSettings.CanLogComponent(LOGVIDEO))
    CLog::Log(LOGDEBUG,"%s::%s %dx%d format:%x:%x flags:%x", CLASSNAME, __FUNCTION__, frame->width, frame->height, frame->format, dec->m_fmt, flags);

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
    // ffmpeg requirements
    AlignedSize(dec->m_avctx, aligned_width, aligned_height);
    pool->Configure(dec->m_fmt, frame->width, frame->height, aligned_width, aligned_height, 0);
  }
  CMMALYUVBuffer *YUVBuffer = dynamic_cast<CMMALYUVBuffer *>(pool->Get());
  if (!YUVBuffer || !YUVBuffer->mmal_buffer || !YUVBuffer->GetMem())
  {
    CLog::Log(LOGERROR,"%s::%s Failed to allocated buffer in time", CLASSNAME, __FUNCTION__);
    return -1;
  }

  CGPUMEM *gmem = YUVBuffer->GetMem();
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

  if (g_advancedSettings.CanLogComponent(LOGVIDEO))
    CLog::Log(LOGDEBUG,"%s::%s buf:%p mmal:%p gmem:%p avbuf:%p:%p:%p", CLASSNAME, __FUNCTION__, YUVBuffer, YUVBuffer->mmal_buffer, gmem, frame->data[0], frame->data[1], frame->data[2]);

  return 0;
}


bool CDecoder::Open(AVCodecContext *avctx, AVCodecContext* mainctx, enum AVPixelFormat fmt)
{
  CSingleLock lock(m_section);

  CLog::Log(LOGNOTICE, "%s::%s - fmt:%d", CLASSNAME, __FUNCTION__, fmt);

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
    if ((frame->format != AV_PIX_FMT_YUV420P && frame->format != AV_PIX_FMT_BGR0 && frame->format != AV_PIX_FMT_RGB565LE) ||
        frame->buf[1] != nullptr || frame->buf[0] == nullptr)
    {
      CLog::Log(LOGERROR, "%s::%s frame format invalid format:%d buf:%p,%p", CLASSNAME, __func__, frame->format, frame->buf[0], frame->buf[1]);
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
      if (g_advancedSettings.CanLogComponent(LOGVIDEO))
        CLog::Log(LOGDEBUG, "%s::%s - mmal:%p buf:%p old:%p gpu:%p %dx%d (%dx%d)", CLASSNAME, __FUNCTION__, m_renderBuffer->mmal_buffer, m_renderBuffer, old, m_renderBuffer->GetMem(),  m_renderBuffer->Width(), m_renderBuffer->Height(), m_renderBuffer->AlignedWidth(), m_renderBuffer->AlignedHeight());
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
  if (g_advancedSettings.CanLogComponent(LOGVIDEO))
    CLog::Log(LOGDEBUG, "%s::%s - mmal:%p dts:%.3f pts:%.3f buf:%p old:%p gpu:%p %dx%d (%dx%d)", CLASSNAME, __FUNCTION__, m_renderBuffer->mmal_buffer, 1e-6*picture->dts, 1e-6*picture->pts, m_renderBuffer, old, m_renderBuffer->GetMem(),  m_renderBuffer->Width(), m_renderBuffer->Height(), m_renderBuffer->AlignedWidth(), m_renderBuffer->AlignedHeight());
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

#endif
