/*
 *      Copyright (C) 2016-2017 Team Kodi
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

#include <interface/mmal/util/mmal_default_components.h>

#include "settings/AdvancedSettings.h"
#include "PixelConverterRBP.h"
#include "cores/VideoPlayer/DVDCodecs/Video/MMALFFmpeg.h"
#include "linux/RBP.h"
#include "utils/log.h"
#include "cores/VideoPlayer/TimingConstants.h"

extern "C"
{
  #include "libavutil/imgutils.h"
  #include "libswscale/swscale.h"
}

#define CLASSNAME "CPixelConverterRBP"

#define VERBOSE 0

using namespace MMAL;

std::vector<CPixelConverterRBP::PixelFormatTargetTable> CPixelConverterRBP::pixfmt_target_table =
{
  { AV_PIX_FMT_BGR0,      AV_PIX_FMT_BGR0 },
  { AV_PIX_FMT_RGB565LE,  AV_PIX_FMT_RGB565LE },
};

AVPixelFormat CPixelConverterRBP::TranslateTargetFormat(AVPixelFormat pixfmt)
{
  for (const auto& entry : pixfmt_target_table)
  {
    if (entry.pixfmt == pixfmt)
      return entry.targetfmt;
  }
  return AV_PIX_FMT_NONE;
}


//------------------------------------------------------------------------------
// main class
//------------------------------------------------------------------------------

CPixelConverterRBP::CPixelConverterRBP() :
  /* Create dummy component with attached pool */
  m_pixelBufferPool(new CMMALPool(MMAL_COMPONENT_DEFAULT_VIDEO_DECODER, false, MMAL_NUM_OUTPUT_BUFFERS, 0, MMAL_ENCODING_I420, MMALStateFFDec))
{
}

bool CPixelConverterRBP::Open(AVPixelFormat pixfmt, AVPixelFormat targetfmt, unsigned int width, unsigned int height)
{
  if (pixfmt == targetfmt || width == 0 || height == 0)
    return false;

  targetfmt = TranslateTargetFormat(pixfmt);

  CLog::Log(LOGDEBUG, "%s::%s: pixfmt:%d(%s) targetfmt:%d(%s) %dx%d", CLASSNAME, __FUNCTION__, pixfmt, av_get_pix_fmt_name(pixfmt), targetfmt, av_get_pix_fmt_name(targetfmt), width, height);

  if (targetfmt == AV_PIX_FMT_NONE)
  {
    CLog::Log(LOGERROR, "%s::%s: Invalid target pixel format: %d", CLASSNAME, __FUNCTION__, targetfmt);
    return false;
  }

  m_targetFormat = targetfmt;
  m_width = width;
  m_height = height;

  m_swsContext = sws_getContext(width, height, pixfmt,
                                width, height, targetfmt,
                                SWS_FAST_BILINEAR, NULL, NULL, NULL);
  if (!m_swsContext)
  {
    CLog::Log(LOGERROR, "%s::%s: Failed to create swscale context", CLASSNAME, __FUNCTION__);
    return false;
  }

  m_pixelBufferPool->Configure(m_targetFormat, 0);
  m_pixelBufferPool->SetDimensions(width, height, width, height);

  return true;
}

void CPixelConverterRBP::Dispose()
{
  if (m_swsContext)
  {
    sws_freeContext(m_swsContext);
    m_swsContext = nullptr;
  }
}

bool CPixelConverterRBP::Decode(const uint8_t* pData, unsigned int size)
{
  if (pData == nullptr || size == 0 || m_swsContext == nullptr)
    return false;

  m_renderBuffer = dynamic_cast<CMMALYUVBuffer*>(m_pixelBufferPool->Get());
  if (!m_renderBuffer)
  {
    CLog::Log(LOGERROR, "%s::%s: Failed to get buffer from pool", CLASSNAME, __FUNCTION__);
    return false;
  }
  CGPUMEM *gmem = m_renderBuffer->GetMem();
  assert(m_renderBuffer && m_renderBuffer->mmal_buffer && gmem);
  if (m_renderBuffer && m_renderBuffer->mmal_buffer && gmem)
  {
    m_renderBuffer->mmal_buffer->data = (uint8_t *)gmem->m_vc_handle;
    m_renderBuffer->mmal_buffer->alloc_size = m_renderBuffer->mmal_buffer->length = gmem->m_numbytes;
  }

  uint8_t *dst[YuvImage::MAX_PLANES];
  int dstStride[YuvImage::MAX_PLANES];
  m_renderBuffer->GetPlanes(dst);
  m_renderBuffer->GetStrides(dstStride);

  uint8_t* dataMutable = const_cast<uint8_t*>(pData);

  const int stride = size / m_height;
  uint8_t* src[] =       { dataMutable,         0,                   0,                   0 };
  int      srcStride[] = { stride,              0,                   0,                   0 };

  sws_scale(m_swsContext, src, srcStride, 0, m_height, dst, dstStride);

  return true;
}

void CPixelConverterRBP::GetPicture(VideoPicture& picture)
{
  if (picture.videoBuffer)
    picture.videoBuffer->Release();

  assert(m_renderBuffer);
  assert(m_renderBuffer->Width() == static_cast<int>(m_width) && m_renderBuffer->Height() == static_cast<int>(m_height));
  uint32_t encoding = m_renderBuffer->Encoding();
  picture.videoBuffer = m_renderBuffer;
  m_renderBuffer = nullptr;

  picture.dts            = DVD_NOPTS_VALUE;
  picture.pts            = DVD_NOPTS_VALUE;
  picture.iFlags         = 0;
  picture.color_matrix   = 4; // CONF_FLAGS_YUVCOEF_BT601
  picture.color_range    = 0; // *not* CONF_FLAGS_YUV_FULLRANGE
  picture.iWidth         = m_height;
  picture.iHeight        = m_height;
  picture.iDisplayWidth  = m_width; //! @todo: Update if aspect ratio changes
  picture.iDisplayHeight = m_height;
  if (VERBOSE && g_advancedSettings.CanLogComponent(LOGVIDEO))
    CLog::Log(LOGDEBUG, "%s::%s: %dx%d enc:%.4s", CLASSNAME, __FUNCTION__, m_width, m_height, (char *)&encoding);
}
