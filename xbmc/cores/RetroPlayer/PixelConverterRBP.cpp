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
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "PixelConverterRBP.h"
#include "cores/VideoPlayer/TimingConstants.h"
#include "cores/VideoPlayer/VideoRenderers/HwDecRender/MMALRenderer.h"
#include "linux/RBP.h"
#include "utils/log.h"

extern "C"
{
  #include "libavutil/imgutils.h"
  #include "libswscale/swscale.h"
}

std::vector<CPixelConverterRBP::PixelFormatTargetTable> CPixelConverterRBP::pixfmt_target_table =
{
  { AV_PIX_FMT_BGR0,      AV_PIX_FMT_BGR0 },
  { AV_PIX_FMT_RGB565LE,  AV_PIX_FMT_RGB565LE },
};

std::vector<CPixelConverterRBP::MMALEncodingTable> CPixelConverterRBP::mmal_encoding_table =
{
  { AV_PIX_FMT_YUV420P,  MMAL_ENCODING_I420 },
  { AV_PIX_FMT_ARGB,     MMAL_ENCODING_ARGB },
  { AV_PIX_FMT_RGBA,     MMAL_ENCODING_RGBA },
  { AV_PIX_FMT_ABGR,     MMAL_ENCODING_ABGR },
  { AV_PIX_FMT_BGRA,     MMAL_ENCODING_ABGR },
  { AV_PIX_FMT_BGR0,     MMAL_ENCODING_BGRA },
  { AV_PIX_FMT_RGB24,    MMAL_ENCODING_RGB24 },
  { AV_PIX_FMT_BGR24,    MMAL_ENCODING_BGR24 },
  { AV_PIX_FMT_RGB565,   MMAL_ENCODING_RGB16 },
  { AV_PIX_FMT_RGB565LE, MMAL_ENCODING_RGB16 },
  { AV_PIX_FMT_BGR565,   MMAL_ENCODING_BGR16 },
};

CPixelConverterRBP::CPixelConverterRBP() :
  m_mmal_format(MMAL_ENCODING_UNKNOWN)
{
}

bool CPixelConverterRBP::Open(AVPixelFormat pixfmt, AVPixelFormat targetfmt, unsigned int width, unsigned int height)
{
  if (width == 0 || height == 0)
    return false;

  targetfmt = TranslateTargetFormat(pixfmt);

  CLog::Log(LOGDEBUG, "CPixelConverter::%s: pixfmt:%d(%s) targetfmt:%d(%s) %dx%d", __FUNCTION__, pixfmt, av_get_pix_fmt_name(pixfmt), targetfmt, av_get_pix_fmt_name(targetfmt), width, height);

  if (targetfmt == AV_PIX_FMT_NONE)
  {
    CLog::Log(LOGERROR, "%s: Invalid target pixel format: %d", __FUNCTION__, targetfmt);
    return false;
  }

  m_renderFormat = RENDER_FMT_MMAL;
  m_width = width;
  m_height = height;
  m_swsContext = sws_getContext(width, height, pixfmt,
                                width, height, targetfmt,
                                SWS_FAST_BILINEAR, NULL, NULL, NULL);
  if (!m_swsContext)
  {
    CLog::Log(LOGERROR, "%s: Failed to create swscale context", __FUNCTION__);
    return false;
  }

  m_mmal_format = TranslateFormat(targetfmt);

  if (m_mmal_format == MMAL_ENCODING_UNKNOWN)
    return false;

  /* Create dummy component with attached pool */
  m_pool = std::make_shared<CMMALPool>(MMAL_COMPONENT_DEFAULT_VIDEO_DECODER, false, MMAL_NUM_OUTPUT_BUFFERS, 0, MMAL_ENCODING_I420, MMALStateFFDec);

  return true;
}

void CPixelConverterRBP::Dispose()
{
  m_pool->Close();
  m_pool = nullptr;

  CPixelConverter::Dispose();
}

bool CPixelConverterRBP::Decode(const uint8_t* pData, unsigned int size)
{
  if (pData == nullptr || size == 0 || m_swsContext == nullptr)
    return false;

  if (m_buf)
    FreePicture(m_buf);

  m_buf = AllocatePicture(m_width, m_height);
  if (!m_buf)
  {
    CLog::Log(LOGERROR, "%s: Failed to allocate picture of dimensions %dx%d", __FUNCTION__, m_width, m_height);
    return false;
  }

  uint8_t* dataMutable = const_cast<uint8_t*>(pData);

  int bpp;
  if (m_mmal_format == MMAL_ENCODING_ARGB ||
      m_mmal_format == MMAL_ENCODING_RGBA ||
      m_mmal_format == MMAL_ENCODING_ABGR ||
      m_mmal_format == MMAL_ENCODING_BGRA)
    bpp = 4;
  else if (m_mmal_format == MMAL_ENCODING_RGB24 ||
           m_mmal_format == MMAL_ENCODING_BGR24)
    bpp = 4;
  else if (m_mmal_format == MMAL_ENCODING_RGB16 ||
           m_mmal_format == MMAL_ENCODING_BGR16)
    bpp = 2;
  else
  {
    CLog::Log(LOGERROR, "CPixelConverter::AllocatePicture, unknown format:%.4s", (char *)&m_mmal_format);
    return false;
  }

  MMAL::CMMALYUVBuffer *omvb = (MMAL::CMMALYUVBuffer *)m_buf->MMALBuffer;

  const int stride = size / m_height;

  uint8_t* src[] =       { dataMutable,                       0, 0, 0 };
  int      srcStride[] = { stride,                            0, 0, 0 };
  uint8_t* dst[] =       { (uint8_t *)omvb->gmem->m_arm,      0, 0, 0 };
  int      dstStride[] = { (int)omvb->m_aligned_width * bpp,  0, 0, 0 };

  sws_scale(m_swsContext, src, srcStride, 0, m_height, dst, dstStride);

  return true;
}

void CPixelConverterRBP::GetPicture(DVDVideoPicture& dvdVideoPicture)
{
  CPixelConverter::GetPicture(dvdVideoPicture);

  dvdVideoPicture.MMALBuffer = m_buf->MMALBuffer;

  MMAL::CMMALYUVBuffer *omvb = (MMAL::CMMALYUVBuffer *)m_buf->MMALBuffer;

  // need to flush ARM cache so GPU can see it
  omvb->gmem->Flush();
}

DVDVideoPicture* CPixelConverterRBP::AllocatePicture(int iWidth, int iHeight)
{
  MMAL::CMMALYUVBuffer *omvb = nullptr;
  DVDVideoPicture* pPicture = new DVDVideoPicture;

  // gpu requirements
  int w = (iWidth + 31) & ~31;
  int h = (iHeight + 15) & ~15;
  if (pPicture && m_pool)
  {
    m_pool->SetFormat(m_mmal_format, iWidth, iHeight, w, h, 0, nullptr);

    omvb = dynamic_cast<MMAL::CMMALYUVBuffer *>(m_pool->GetBuffer(500));
    if (!omvb ||
        !omvb->mmal_buffer ||
        !omvb->gmem ||
        !omvb->gmem->m_arm)
    {
      CLog::Log(LOGERROR, "CPixelConverterRBP::AllocatePicture, unable to allocate new video picture, out of memory.");
      delete pPicture;
      pPicture = nullptr;
    }

    CGPUMEM *gmem = omvb->gmem;
    omvb->mmal_buffer->data = (uint8_t *)gmem->m_vc_handle;
    omvb->mmal_buffer->alloc_size = omvb->mmal_buffer->length = gmem->m_numbytes;
  }
  else
    CLog::Log(LOGERROR, "CPixelConverterRBP::AllocatePicture invalid picture:%p pool:%p", pPicture, m_pool.get());

  if (pPicture)
  {
    pPicture->MMALBuffer = omvb;
    pPicture->iWidth = iWidth;
    pPicture->iHeight = iHeight;
  }

  return pPicture;
}

void CPixelConverterRBP::FreePicture(DVDVideoPicture* pPicture)
{
  if (pPicture)
  {
    if (pPicture->MMALBuffer)
      pPicture->MMALBuffer->Release();

    delete pPicture;
  }
  else
    CLog::Log(LOGERROR, "CPixelConverterRBP::FreePicture invalid picture:%p", pPicture);
}

AVPixelFormat CPixelConverterRBP::TranslateTargetFormat(AVPixelFormat pixfmt)
{
  for (const auto& entry : pixfmt_target_table)
  {
    if (entry.pixfmt == pixfmt)
      return entry.targetfmt;
  }
  return AV_PIX_FMT_NONE;
}

uint32_t CPixelConverterRBP::TranslateFormat(AVPixelFormat pixfmt)
{
  for (const auto& entry : mmal_encoding_table)
  {
    if (entry.pixfmt == pixfmt)
      return entry.encoding;
  }
  return MMAL_ENCODING_UNKNOWN;
}
