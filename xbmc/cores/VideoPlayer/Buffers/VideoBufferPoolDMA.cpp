/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoBufferPoolDMA.h"

#include "cores/VideoPlayer/Buffers/VideoBufferDMA.h"
#include "utils/BufferObjectFactory.h"

#include <mutex>

#include <drm_fourcc.h>

extern "C"
{
#include <libavutil/pixdesc.h>
#include <libavutil/pixfmt.h>
}

CVideoBufferPoolDMA::~CVideoBufferPoolDMA()
{
  std::unique_lock lock(m_critSection);

  for (auto buf : m_all)
    delete buf;
}

CVideoBuffer* CVideoBufferPoolDMA::Get()
{
  std::unique_lock lock(m_critSection);

  CVideoBufferDMA* buf = nullptr;
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
    buf = new CVideoBufferDMA(*this, id, m_fourcc, m_planes, m_size);

    if (!buf->Alloc())
    {
      delete buf;
      return nullptr;
    }

    m_all.push_back(buf);
    m_used.push_back(id);
  }

  buf->Acquire(GetPtr());
  return buf;
}

void CVideoBufferPoolDMA::Return(int id)
{
  std::unique_lock lock(m_critSection);

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

void CVideoBufferPoolDMA::Configure(AVPixelFormat format, int size)
{
  std::unique_lock lock(m_critSection);

  m_fourcc = TranslateFormat(format);
  const int planeCount = av_pix_fmt_count_planes(format);
  m_planes = planeCount > 0 ? static_cast<uint32_t>(planeCount) : 0;
  m_size = static_cast<uint64_t>(size);
}

bool CVideoBufferPoolDMA::IsConfigured()
{
  std::unique_lock lock(m_critSection);

  return (m_fourcc != 0 && m_size != 0);
}

bool CVideoBufferPoolDMA::IsCompatible(AVPixelFormat format, int size)
{
  std::unique_lock lock(m_critSection);

  if (m_fourcc != TranslateFormat(format) || m_size != static_cast<uint64_t>(size))
    return false;

  return true;
}

void CVideoBufferPoolDMA::Released(CVideoBufferManager& videoBufferManager)
{
  if (!CBufferObjectFactory::CreateBufferObject(true))
    return;

  videoBufferManager.RegisterPool(std::make_shared<CVideoBufferPoolDMA>());
}

std::shared_ptr<IVideoBufferPool> CVideoBufferPoolDMA::CreatePool()
{
  return std::make_shared<CVideoBufferPoolDMA>();
}

uint32_t CVideoBufferPoolDMA::TranslateFormat(AVPixelFormat format)
{
  switch (format)
  {
    case AV_PIX_FMT_NV12:
      return DRM_FORMAT_NV12;
    case AV_PIX_FMT_P010:
      return DRM_FORMAT_P010;
    case AV_PIX_FMT_YUV420P:
    case AV_PIX_FMT_YUVJ420P:
      return DRM_FORMAT_YUV420;
    case AV_PIX_FMT_YUV422P:
    case AV_PIX_FMT_YUVJ422P:
      return DRM_FORMAT_YUV422;
    case AV_PIX_FMT_YUYV422:
      return DRM_FORMAT_YUYV;
    case AV_PIX_FMT_UYVY422:
      return DRM_FORMAT_UYVY;
    case AV_PIX_FMT_YUV444P:
    case AV_PIX_FMT_YUVJ444P:
      return DRM_FORMAT_YUV444;
      //! @todo remove #ifdef guards when libdrm >= 2.4.125 is the minimum
#if defined(DRM_FORMAT_S010)
    case AV_PIX_FMT_YUV420P10:
      return DRM_FORMAT_S010;
#endif
#if defined(DRM_FORMAT_S012)
    case AV_PIX_FMT_YUV420P12:
      return DRM_FORMAT_S012;
#endif
#if defined(DRM_FORMAT_S210)
    case AV_PIX_FMT_YUV422P10:
      return DRM_FORMAT_S210;
#endif
#if defined(DRM_FORMAT_S212)
    case AV_PIX_FMT_YUV422P12:
      return DRM_FORMAT_S212;
#endif
#if defined(DRM_FORMAT_S410)
    case AV_PIX_FMT_YUV444P10:
      return DRM_FORMAT_S410;
#endif
#if defined(DRM_FORMAT_S412)
    case AV_PIX_FMT_YUV444P12:
      return DRM_FORMAT_S412;
#endif
    // Packed RGB formats. AV_PIX_FMT_BGR0 = bytes B,G,R,X = DRM_FORMAT_XRGB8888.
    case AV_PIX_FMT_BGR0:
      return DRM_FORMAT_XRGB8888;
    case AV_PIX_FMT_X2RGB10LE:
      return DRM_FORMAT_XRGB2101010;
      // ffmpeg has no X variant of 16-bit RGB; we use RGBA64/RGBAF16 with the
      // alpha component treated as undefined padding (X). DRM 16-bit RGB
      // formats need libdrm >= 2.4.110.
#if defined(DRM_FORMAT_XRGB16161616)
    case AV_PIX_FMT_RGBA64LE:
      return DRM_FORMAT_XRGB16161616;
#endif
#if defined(DRM_FORMAT_XRGB16161616F)
    case AV_PIX_FMT_RGBAF16LE:
      return DRM_FORMAT_XRGB16161616F;
#endif
    default:
      return 0;
  }
}
