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
#pragma once

#include "threads/CriticalSection.h"
#include <atomic>
#include <deque>
#include <list>
#include <memory>
#include <vector>

extern "C" {
#include "libavutil/pixfmt.h"
}

struct YuvImage
{
  static const int MAX_PLANES = 3;

  uint8_t* plane[MAX_PLANES];
  int planesize[MAX_PLANES];
  int stride[MAX_PLANES];
  unsigned int width;
  unsigned int height;
  unsigned int cshift_x; // this is the chroma shift used
  unsigned int cshift_y;
  unsigned int bpp; // bytes per pixel
};

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

#define BUFFER_STATE_DECODER 0x01;
#define BUFFER_STATE_RENDER  0x02;

class CVideoBuffer;
class IVideoBufferPool;
class CVideoBufferManager;

class IVideoBufferPool : public std::enable_shared_from_this<IVideoBufferPool>
{
public:
  virtual ~IVideoBufferPool() = default;

  // get a free buffer from the pool, sets ref count to 1
  virtual CVideoBuffer* Get() = 0;

  // called by buffer when ref count goes to zero
  virtual void Return(int id) = 0;

  // required if pool is registered with BufferManager BM call configure
  // as soon as it knows parameters: pixFmx, size
  virtual void Configure(AVPixelFormat format, int size) {};

  // required if pool is registered with BufferManager
  virtual bool IsConfigured() { return false;};

  // required if pool is registered with BufferManager
  // called before Get() to check if buffer pool is suitable
  virtual bool IsCompatible(AVPixelFormat format, int size) { return false;};

  // callback when BM releases buffer pool. i.e. before a new codec is created
  // clients can register a new pool on this callback
  virtual void Released(CVideoBufferManager &videoBufferManager) {};

  // call on Get() before returning buffer to caller
  std::shared_ptr<IVideoBufferPool> GetPtr() { return shared_from_this(); };
};

class CVideoBuffer
{
public:
  CVideoBuffer() = delete;
  virtual ~CVideoBuffer() = default;
  void Acquire();
  void Acquire(std::shared_ptr<IVideoBufferPool> pool);
  void Release();
  int GetId() const { return m_id; };

  virtual AVPixelFormat GetFormat();
  virtual uint8_t* GetMemPtr() { return nullptr; };
  virtual void GetPlanes(uint8_t*(&planes)[YuvImage::MAX_PLANES]) {};
  virtual void GetStrides(int(&strides)[YuvImage::MAX_PLANES]) {};
  virtual void SetDimensions(int width, int height, const int (&strides)[YuvImage::MAX_PLANES]) {};
  virtual void SetDimensions(int width, int height, const int (&strides)[YuvImage::MAX_PLANES], const int (&planeOffsets)[YuvImage::MAX_PLANES]) {};

  static bool CopyPicture(YuvImage* pDst, YuvImage *pSrc);
  static bool CopyNV12Picture(YuvImage* pDst, YuvImage *pSrc);
  static bool CopyYUV422PackedPicture(YuvImage* pDst, YuvImage *pSrc);

protected:
  explicit CVideoBuffer(int id);
  AVPixelFormat m_pixFormat = AV_PIX_FMT_NONE;
  std::atomic_int m_refCount;
  int m_id;
  std::shared_ptr<IVideoBufferPool> m_pool;
};

class CVideoBufferSysMem : public CVideoBuffer
{
public:
  CVideoBufferSysMem(IVideoBufferPool &pool, int id, AVPixelFormat format, int size);
  ~CVideoBufferSysMem() override;
  uint8_t* GetMemPtr() override;
  void GetPlanes(uint8_t*(&planes)[YuvImage::MAX_PLANES]) override;
  void GetStrides(int(&strides)[YuvImage::MAX_PLANES]) override;
  void SetDimensions(int width, int height, const int (&strides)[YuvImage::MAX_PLANES]) override;
  void SetDimensions(int width, int height, const int (&strides)[YuvImage::MAX_PLANES], const int (&planeOffsets)[YuvImage::MAX_PLANES]) override;
  bool Alloc();

protected:
  int m_width = 0;
  int m_height = 0;
  int m_size = 0;
  uint8_t *m_data = nullptr;
  YuvImage m_image;
};

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

class CVideoBufferPoolSysMem : public IVideoBufferPool
{
public:
  CVideoBuffer* Get() override;
  void Return(int id) override;
  void Configure(AVPixelFormat format, int size) override;
  bool IsConfigured() override;
  bool IsCompatible(AVPixelFormat format, int size) override;

protected:
  int m_width = 0;
  int m_height = 0;
  int m_size = 0;
  AVPixelFormat m_pixFormat = AV_PIX_FMT_NONE;
  bool m_configured = false;
  CCriticalSection m_critSection;

  std::vector<CVideoBufferSysMem*> m_all;
  std::deque<int> m_used;
  std::deque<int> m_free;
};

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

class CVideoBufferManager
{
public:
  CVideoBufferManager();
  void RegisterPool(std::shared_ptr<IVideoBufferPool> pool);
  void ReleasePools();
  CVideoBuffer* Get(AVPixelFormat format, int size);

protected:
  CCriticalSection m_critSection;
  std::list<std::shared_ptr<IVideoBufferPool>> m_pools;

private:
  CVideoBufferManager (const CVideoBufferManager&) = delete;
  CVideoBufferManager& operator= (const CVideoBufferManager&) = delete;
};
