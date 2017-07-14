/*
 *      Copyright (C) 2005-2017 Team XBMC
 *      http://xbmc.org
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

#include "VideoBuffer.h"
#include "threads/SingleLock.h"
#include <string.h>

//-----------------------------------------------------------------------------
// CVideoBuffer
//-----------------------------------------------------------------------------

CVideoBuffer::CVideoBuffer(int id)
{
  m_id = id;
  m_refCount = 0;
}

void CVideoBuffer::Acquire()
{
  m_refCount++;
}

void CVideoBuffer::Acquire(std::shared_ptr<IVideoBufferPool> pool)
{
  m_refCount++;
  m_pool = pool;
}

void CVideoBuffer::Release()
{
  if (--m_refCount <= 0)
  {
    std::shared_ptr<IVideoBufferPool> pool = m_pool->GetPtr();
    m_pool = nullptr;
    pool->Return(m_id);
  }
}

AVPixelFormat CVideoBuffer::GetFormat()
{
  return m_pixFormat;
}

bool CVideoBuffer::CopyPicture(YuvImage* pDst, YuvImage *pSrc)
{
  uint8_t *s = pSrc->plane[0];
  uint8_t *d = pDst->plane[0];
  int w = pDst->width * pDst->bpp;
  int h = pDst->height;
  if ((w == pSrc->stride[0]) && (pSrc->stride[0] == pDst->stride[0]))
  {
    memcpy(d, s, w*h);
  }
  else
  {
    for (int y = 0; y < h; y++)
    {
      memcpy(d, s, w);
      s += pSrc->stride[0];
      d += pDst->stride[0];
    }
  }
  s = pSrc->plane[1];
  d = pDst->plane[1];
  w =(pDst->width  >> pDst->cshift_x) * pDst->bpp;
  h =(pDst->height >> pDst->cshift_y);
  if ((w == pSrc->stride[1]) && (pSrc->stride[1] == pDst->stride[1]))
  {
    memcpy(d, s, w*h);
  }
  else
  {
    for (int y = 0; y < h; y++)
    {
      memcpy(d, s, w);
      s += pSrc->stride[1];
      d += pDst->stride[1];
    }
  }
  s = pSrc->plane[2];
  d = pDst->plane[2];
  if ((w == pSrc->stride[2]) && (pSrc->stride[2] == pDst->stride[2]))
  {
    memcpy(d, s, w*h);
  }
  else
  {
    for (int y = 0; y < h; y++)
    {
      memcpy(d, s, w);
      s += pSrc->stride[2];
      d += pDst->stride[2];
    }
  }
  return true;
}


bool CVideoBuffer::CopyNV12Picture(YuvImage* pDst, YuvImage *pSrc)
{
  uint8_t *s = pSrc->plane[0];
  uint8_t *d = pDst->plane[0];
  int w = pDst->width;
  int h = pDst->height;
  // Copy Y
  if ((w == pSrc->stride[0]) && (pSrc->stride[0] == pDst->stride[0]))
  {
    memcpy(d, s, w*h);
  }
  else
  {
    for (int y = 0; y < h; y++)
    {
      memcpy(d, s, w);
      s += pSrc->stride[0];
      d += pDst->stride[0];
    }
  }

  s = pSrc->plane[1];
  d = pDst->plane[1];
  w = pDst->width;
  h = pDst->height >> 1;
  // Copy packed UV (width is same as for Y as it's both U and V components)
  if ((w == pSrc->stride[1]) && (pSrc->stride[1] == pDst->stride[1]))
  {
    memcpy(d, s, w*h);
  }
  else
  {
    for (int y = 0; y < h; y++)
    {
      memcpy(d, s, w);
      s += pSrc->stride[1];
      d += pDst->stride[1];
    }
  }

  return true;
}

bool CVideoBuffer::CopyYUV422PackedPicture(YuvImage* pDst, YuvImage *pSrc)
{
  uint8_t *s = pSrc->plane[0];
  uint8_t *d = pDst->plane[0];
  int w = pDst->width;
  int h = pDst->height;

  // Copy YUYV
  if ((w * 2 == pSrc->stride[0]) && (pSrc->stride[0] == pDst->stride[0]))
  {
    memcpy(d, s, w*h*2);
  }
  else
  {
    for (int y = 0; y < h; y++)
    {
      memcpy(d, s, w*2);
      s += pSrc->stride[0];
      d += pDst->stride[0];
    }
  }

  return true;
}

CVideoBufferSysMem::CVideoBufferSysMem(IVideoBufferPool &pool, int id, AVPixelFormat format, int size)
: CVideoBuffer(id)
{
  m_pixFormat = format;
  m_size = size;
  memset(&m_image, 0, sizeof(YuvImage));
}

CVideoBufferSysMem::~CVideoBufferSysMem()
{
  delete[] m_data;
}

uint8_t* CVideoBufferSysMem::GetMemPtr()
{
  return m_data;
}

void CVideoBufferSysMem::GetPlanes(uint8_t*(&planes)[YuvImage::MAX_PLANES])
{
  planes[0] = m_image.plane[0];
  planes[1] = m_image.plane[1];
  planes[2] = m_image.plane[2];
}

void CVideoBufferSysMem::GetStrides(int(&strides)[YuvImage::MAX_PLANES])
{
  strides[0] = m_image.stride[0];
  strides[1] = m_image.stride[1];
  strides[2] = m_image.stride[2];
}

void CVideoBufferSysMem::SetDimensions(int width, int height, const int (&strides)[YuvImage::MAX_PLANES])
{
  m_width = width;
  m_height = height;

  m_image.width  = m_width;
  m_image.height = m_height;
  m_image.stride[0] = strides[0];
  m_image.stride[1] = strides[1];
  m_image.stride[2] = strides[2];
  m_image.cshift_x = 1;
  m_image.cshift_y = 1;
  m_image.bpp = 1;

  if (m_pixFormat == AV_PIX_FMT_YUV420P ||
      m_pixFormat == AV_PIX_FMT_YUV420P16 ||
      m_pixFormat == AV_PIX_FMT_YUV420P14 ||
      m_pixFormat == AV_PIX_FMT_YUV420P12 ||
      m_pixFormat == AV_PIX_FMT_YUV420P10 ||
      m_pixFormat == AV_PIX_FMT_YUV420P9)
  {
    if (m_pixFormat != AV_PIX_FMT_YUV420P)
      m_image.bpp = 2;

    m_image.planesize[0] = m_image.stride[0] * m_image.height;
    m_image.planesize[1] = m_image.stride[1] * (m_image.height >> m_image.cshift_y);
    m_image.planesize[2] = m_image.stride[2] * (m_image.height >> m_image.cshift_y);
  }
  else if (m_pixFormat == AV_PIX_FMT_NV12)
  {
    // Y plane
    m_image.planesize[0] = m_image.stride[0] * m_image.height;
    // packed UV plane
    m_image.planesize[1] = m_image.stride[1] * m_image.height / 2;
    // third plane is not used
    m_image.planesize[2] = 0;
  }
  else if (m_pixFormat == AV_PIX_FMT_YUYV422 ||
           m_pixFormat == AV_PIX_FMT_UYVY422)
  {
    // packed YUYV plane
    m_image.planesize[0] = m_image.stride[0] * m_image.height;
    // second plane is not used
    m_image.planesize[1] = 0;
    // third plane is not used
    m_image.planesize[2] = 0;
  }

  m_image.plane[0] = m_data;
  m_image.plane[1] = m_data + m_image.planesize[0];
  m_image.plane[2] = m_image.plane[1] + m_image.planesize[1];
}

void CVideoBufferSysMem::SetDimensions(int width, int height, const int (&strides)[YuvImage::MAX_PLANES], const int (&planeOffsets)[YuvImage::MAX_PLANES])
{
  SetDimensions(width, height, strides);

  m_image.plane[0] = m_data + planeOffsets[0];
  m_image.plane[1] = m_data + planeOffsets[1];
  m_image.plane[2] = m_data + planeOffsets[2];
}

bool CVideoBufferSysMem::Alloc()
{
  m_data = new uint8_t[m_size];
  return true;
}


//-----------------------------------------------------------------------------
// CVideoBufferPool
//-----------------------------------------------------------------------------

CVideoBuffer* CVideoBufferPoolSysMem::Get()
{
  CSingleLock lock(m_critSection);

  CVideoBufferSysMem *buf = nullptr;
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
    buf = new CVideoBufferSysMem(*this, id, m_pixFormat, m_size);
    buf->Alloc();
    m_all.push_back(buf);
    m_used.push_back(id);
  }

  buf->Acquire(GetPtr());
  return buf;
}

void CVideoBufferPoolSysMem::Return(int id)
{
  CSingleLock lock(m_critSection);

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

void CVideoBufferPoolSysMem::Configure(AVPixelFormat format, int size)
{
  m_pixFormat = format;
  m_size = size;
  m_configured = true;
}

inline bool CVideoBufferPoolSysMem::IsConfigured()
{
  return m_configured;
}

bool CVideoBufferPoolSysMem::IsCompatible(AVPixelFormat format, int size)
{
  if (m_pixFormat == format &&
      m_size == size)
    return true;

  return false;
}

//-----------------------------------------------------------------------------
// CVideoBufferManager
//-----------------------------------------------------------------------------

CVideoBufferManager::CVideoBufferManager()
{
  CSingleLock lock(m_critSection);
  std::shared_ptr<IVideoBufferPool> pool = std::make_shared<CVideoBufferPoolSysMem>();
  RegisterPool(pool);
}

void CVideoBufferManager::RegisterPool(std::shared_ptr<IVideoBufferPool> pool)
{
  CSingleLock lock(m_critSection);
  // preferred pools are to the front
  m_pools.push_front(pool);
}

void CVideoBufferManager::ReleasePools()
{
  CSingleLock lock(m_critSection);
  std::list<std::shared_ptr<IVideoBufferPool>> pools = m_pools;
  m_pools.clear();
  std::shared_ptr<IVideoBufferPool> pool = std::make_shared<CVideoBufferPoolSysMem>();
  RegisterPool(pool);

  for (auto pool : pools)
  {
    pool->Released(*this);
  }
}

CVideoBuffer* CVideoBufferManager::Get(AVPixelFormat format, int size)
{
  CSingleLock lock(m_critSection);
  for (auto pool: m_pools)
  {
    if (!pool->IsConfigured())
    {
      pool->Configure(format, size);
    }
    if (pool->IsCompatible(format, size))
    {
      return pool->Get();
    }
  }
  return nullptr;
}
