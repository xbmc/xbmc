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

#include "PixelConverter.h"
#include "cores/VideoPlayer/DVDCodecs/Video/DVDVideoCodec.h"
#include "cores/VideoPlayer/Process/VideoBuffer.h"
#include "cores/VideoPlayer/TimingConstants.h"
#include "threads/CriticalSection.h"
#include "threads/SingleLock.h"
#include "utils/log.h"

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "libavutil/frame.h"
}

//------------------------------------------------------------------------------
// Video Buffers
//------------------------------------------------------------------------------

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//! @todo Remove dependence on VideoPlayer
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

class CPixelBufferFFmpeg : public CVideoBuffer
{
public:
  CPixelBufferFFmpeg(IVideoBufferPool &pool, int id);
  virtual ~CPixelBufferFFmpeg();
  virtual void GetPlanes(uint8_t*(&planes)[YuvImage::MAX_PLANES]) override;
  virtual void GetStrides(int(&strides)[YuvImage::MAX_PLANES]) override;

  void SetRef(AVFrame *frame);
  void Unref();

protected:
  AVFrame* m_pFrame;
};

CPixelBufferFFmpeg::CPixelBufferFFmpeg(IVideoBufferPool &pool, int id)
: CVideoBuffer(id)
{
  m_pFrame = av_frame_alloc();
}

CPixelBufferFFmpeg::~CPixelBufferFFmpeg()
{
  av_frame_free(&m_pFrame);
}

void CPixelBufferFFmpeg::GetPlanes(uint8_t*(&planes)[YuvImage::MAX_PLANES])
{
  planes[0] = m_pFrame->data[0];
  planes[1] = m_pFrame->data[1];
  planes[2] = m_pFrame->data[2];
}

void CPixelBufferFFmpeg::GetStrides(int(&strides)[YuvImage::MAX_PLANES])
{
  strides[0] = m_pFrame->linesize[0];
  strides[1] = m_pFrame->linesize[1];
  strides[2] = m_pFrame->linesize[2];
}

void CPixelBufferFFmpeg::SetRef(AVFrame *frame)
{
  av_frame_unref(m_pFrame);
  av_frame_move_ref(m_pFrame, frame);
  m_pixFormat = static_cast<AVPixelFormat>(m_pFrame->format);
}

void CPixelBufferFFmpeg::Unref()
{
  av_frame_unref(m_pFrame);
}

//------------------------------------------------------------------------------

class CPixelBufferPoolFFmpeg : public IVideoBufferPool
{
public:
  virtual ~CPixelBufferPoolFFmpeg();
  virtual void Return(int id) override;
  virtual CVideoBuffer* Get() override;

protected:
  CCriticalSection m_critSection;
  std::vector<CPixelBufferFFmpeg*> m_all;
  std::deque<int> m_used;
  std::deque<int> m_free;
};

CPixelBufferPoolFFmpeg::~CPixelBufferPoolFFmpeg()
{
  for (auto buf : m_all)
    delete buf;
}

CVideoBuffer* CPixelBufferPoolFFmpeg::Get()
{
  CSingleLock lock(m_critSection);

  CPixelBufferFFmpeg *buf = nullptr;
  if (!m_free.empty())
  {
    int idx = m_free.front();
    m_free.pop_front();
    m_used.push_back(idx);
    buf = m_all[idx];
  }
  else
  {
    int id = m_all.size();
    buf = new CPixelBufferFFmpeg(*this, id);
    m_all.push_back(buf);
    m_used.push_back(id);
  }

  buf->Acquire(GetPtr());
  return buf;
}

void CPixelBufferPoolFFmpeg::Return(int id)
{
  CSingleLock lock(m_critSection);

  m_all[id]->Unref();
  auto it = m_used.begin();
  while (it != m_used.end())
  {
    if (*it == id)
    {
      m_used.erase(it);
      break;
    }
    else
      ++it;
  }
  m_free.push_back(id);
}

//------------------------------------------------------------------------------
// main class
//------------------------------------------------------------------------------

void CPixelConverter::delete_buffer_pool::operator()(CPixelBufferPoolFFmpeg *p) const
{
  delete p;
}

CPixelConverter::CPixelConverter() :
  m_targetFormat(AV_PIX_FMT_NONE),
  m_width(0),
  m_height(0),
  m_swsContext(nullptr),
  m_pFrame(nullptr),
  m_pixelBufferPool(new CPixelBufferPoolFFmpeg)
{
}

bool CPixelConverter::Open(AVPixelFormat pixfmt, AVPixelFormat targetfmt, unsigned int width, unsigned int height)
{
  if (pixfmt == targetfmt || width == 0 || height == 0)
    return false;

  m_targetFormat = targetfmt;
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

  m_pFrame = av_frame_alloc();
  if (m_pFrame == nullptr)
  {
    CLog::Log(LOGERROR, "%s: Failed to allocate frame", __FUNCTION__);
    Dispose();
    return false;
  }

  return true;
}

void CPixelConverter::Dispose()
{
  if (m_pFrame)
    av_frame_free(&m_pFrame);

  if (m_swsContext)
  {
    sws_freeContext(m_swsContext);
    m_swsContext = nullptr;
  }
}

bool CPixelConverter::Decode(const uint8_t* pData, unsigned int size)
{
  if (pData == nullptr || size == 0 || m_swsContext == nullptr)
    return false;

  if (!AllocateBuffers(m_pFrame))
    return false;

  uint8_t* dataMutable = const_cast<uint8_t*>(pData);

  const int stride = size / m_height;

  uint8_t* src[] =       { dataMutable,         0,                   0,                   0 };
  int      srcStride[] = { stride,              0,                   0,                   0 };
  uint8_t* dst[] =       { m_pFrame->data[0],     m_pFrame->data[1],     m_pFrame->data[2],     0 };
  int      dstStride[] = { m_pFrame->linesize[0], m_pFrame->linesize[1], m_pFrame->linesize[2], 0 };

  sws_scale(m_swsContext, src, srcStride, 0, m_height, dst, dstStride);

  return true;
}

void CPixelConverter::GetPicture(VideoPicture& dvdVideoPicture)
{
  if (m_pFrame != nullptr)
  {
    CPixelBufferFFmpeg *buffer = dynamic_cast<CPixelBufferFFmpeg*>(m_pixelBufferPool->Get());
    buffer->SetRef(m_pFrame);
    dvdVideoPicture.videoBuffer = buffer;
  }

  dvdVideoPicture.dts            = DVD_NOPTS_VALUE;
  dvdVideoPicture.pts            = DVD_NOPTS_VALUE;
  dvdVideoPicture.iFlags         = 0;
  dvdVideoPicture.color_matrix   = 4; // CONF_FLAGS_YUVCOEF_BT601
  dvdVideoPicture.color_range    = 0; // *not* CONF_FLAGS_YUV_FULLRANGE
  dvdVideoPicture.iWidth         = m_width;
  dvdVideoPicture.iHeight        = m_height;
  dvdVideoPicture.iDisplayWidth  = m_width; //! @todo: Update if aspect ratio changes
  dvdVideoPicture.iDisplayHeight = m_height;
}

bool CPixelConverter::AllocateBuffers(AVFrame *pFrame) const
{
  pFrame->format = m_targetFormat;
  pFrame->width = m_width;
  pFrame->height = m_height;
  const unsigned int align = 64; // From VAAPI
  int res = av_frame_get_buffer(pFrame, align);
  if (res < 0)
  {
    CLog::Log(LOGERROR, "%s: Failed to allocate buffers: %s", __FUNCTION__, res);
    return false;
  }

  return true;
}
