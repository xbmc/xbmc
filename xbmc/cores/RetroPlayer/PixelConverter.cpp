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

#include "PixelConverter.h"
#include "cores/VideoPlayer/TimingConstants.h"
#include "cores/VideoPlayer/DVDCodecs/DVDCodecUtils.h"
#include "utils/log.h"

extern "C"
{
  #include "libswscale/swscale.h"
}

CPixelConverter::CPixelConverter() :
  m_renderFormat(RENDER_FMT_NONE),
  m_width(0),
  m_height(0),
  m_swsContext(nullptr),
  m_buf(nullptr)
{
}

bool CPixelConverter::Open(AVPixelFormat pixfmt, AVPixelFormat targetfmt, unsigned int width, unsigned int height)
{
  if (pixfmt == targetfmt || width == 0 || height == 0)
    return false;

  m_renderFormat = CDVDCodecUtils::EFormatFromPixfmt(targetfmt);
  if (m_renderFormat == RENDER_FMT_NONE)
  {
    CLog::Log(LOGERROR, "%s: Invalid target pixel format: %d", __FUNCTION__, targetfmt);
    return false;
  }

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

  m_buf = CDVDCodecUtils::AllocatePicture(width, height);
  if (!m_buf)
  {
    CLog::Log(LOGERROR, "%s: Failed to allocate picture of dimensions %dx%d", __FUNCTION__, width, height);
    return false;
  }

  return true;
}

void CPixelConverter::Dispose()
{
  if (m_swsContext)
  {
    sws_freeContext(m_swsContext);
    m_swsContext = nullptr;
  }

  if (m_buf)
  {
    CDVDCodecUtils::FreePicture(m_buf);
    m_buf = nullptr;
  }
}

bool CPixelConverter::Decode(const uint8_t* pData, unsigned int size)
{
  if (pData == nullptr || size == 0 || m_swsContext == nullptr)
    return false;

  uint8_t* dataMutable = const_cast<uint8_t*>(pData);

  const int stride = size / m_height;

  uint8_t* src[] =       { dataMutable,         0,                   0,                   0 };
  int      srcStride[] = { stride,              0,                   0,                   0 };
  uint8_t* dst[] =       { m_buf->data[0],      m_buf->data[1],      m_buf->data[2],      0 };
  int      dstStride[] = { m_buf->iLineSize[0], m_buf->iLineSize[1], m_buf->iLineSize[2], 0 };

  sws_scale(m_swsContext, src, srcStride, 0, m_height, dst, dstStride);

  return true;
}

void CPixelConverter::GetPicture(DVDVideoPicture& dvdVideoPicture)
{
  dvdVideoPicture.dts            = DVD_NOPTS_VALUE;
  dvdVideoPicture.pts            = DVD_NOPTS_VALUE;

  for (int i = 0; i < 4; i++)
  {
    dvdVideoPicture.data[i]      = m_buf->data[i];
    dvdVideoPicture.iLineSize[i] = m_buf->iLineSize[i];
  }

  dvdVideoPicture.iFlags         = 0; // *not* DVP_FLAG_ALLOCATED
  dvdVideoPicture.color_matrix   = 4; // CONF_FLAGS_YUVCOEF_BT601
  dvdVideoPicture.color_range    = 0; // *not* CONF_FLAGS_YUV_FULLRANGE
  dvdVideoPicture.iWidth         = m_width;
  dvdVideoPicture.iHeight        = m_height;
  dvdVideoPicture.iDisplayWidth  = m_width; //! @todo: Update if aspect ratio changes
  dvdVideoPicture.iDisplayHeight = m_height;
  dvdVideoPicture.format         = m_renderFormat;
}
