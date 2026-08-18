/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RenderBufferDMA.h"

#include "ServiceBroker.h"
#include "utils/BufferObject.h"
#if defined(HAVE_LINUX_DMA_HEAP)
#include "utils/DMAHeapBufferObject.h"
#endif
#include "utils/EGLImage.h"
#if defined(HAVE_LINUX_MEMFD) && defined(HAVE_LINUX_UDMABUF)
#include "utils/UDMABufferObject.h"
#endif
#include "utils/log.h"
#include "windowing/WinSystem.h"
#include "windowing/linux/WinSystemEGL.h"

using namespace KODI;
using namespace RETRO;

CRenderBufferDMA::CRenderBufferDMA(int fourcc)
  : m_fourcc(fourcc),
    m_bo(CBufferObject::GetBufferObject(false))
{
  auto winSystemEGL =
      dynamic_cast<KODI::WINDOWING::LINUX::CWinSystemEGL*>(CServiceBroker::GetWinSystem());

  if (winSystemEGL == nullptr)
    throw std::runtime_error("dynamic_cast failed to cast to CWinSystemEGL. This is likely due to "
                             "a build misconfiguration as DMA can only be used with EGL and "
                             "specifically platforms that implement CWinSystemEGL");

  m_egl = std::make_unique<CEGLImage>(winSystemEGL->GetEGLDisplay());

  CLog::Log(LOGDEBUG, "CRenderBufferDMA: using BufferObject type: {}", m_bo->GetName());
}

CRenderBufferDMA::~CRenderBufferDMA()
{
  DeleteTexture();
}

bool CRenderBufferDMA::Allocate(AVPixelFormat format, unsigned int width, unsigned int height)
{
  // Initialize IRenderBuffer
  m_format = format;
  m_width = width;
  m_height = height;

  m_bo->CreateBufferObject(m_fourcc, m_width, m_height);

  return true;
}

size_t CRenderBufferDMA::GetFrameSize() const
{
  return m_bo->GetStride() * m_height;
}

uint32_t CRenderBufferDMA::GetStride() const
{
  return m_bo->GetStride();
}

uint8_t* CRenderBufferDMA::GetMemory()
{
  // Map first, then open CPU access over the mapping that will be written
  uint8_t* const memory = m_bo->GetMemory();
  if (memory == nullptr)
    return nullptr;

  m_bo->SyncStart();

  return memory;
}

void CRenderBufferDMA::ReleaseMemory()
{
  // Close CPU access while the mapping is still there, then drop it. Ending it
  // after the unmap leaves the writes outside the bracket the GPU relies on.
  m_bo->SyncEnd();
  m_bo->ReleaseMemory();
}

void CRenderBufferDMA::CreateTexture()
{
  glGenTextures(1, &m_textureId);

  glBindTexture(m_textureTarget, m_textureId);
  glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  ConfigureTexture();

  glBindTexture(m_textureTarget, 0);
}

bool CRenderBufferDMA::RequiresCoherencyWorkaround() const
{
#if defined(HAVE_LINUX_DMA_HEAP)
  if (dynamic_cast<const CDMAHeapBufferObject*>(m_bo.get()) != nullptr)
    return true;
#endif
#if defined(HAVE_LINUX_MEMFD) && defined(HAVE_LINUX_UDMABUF)
  if (dynamic_cast<const CUDMABufferObject*>(m_bo.get()) != nullptr)
    return true;
#endif

  return false;
}

bool CRenderBufferDMA::UploadTexture()
{
  if (m_bo->GetFd() < 0)
    return false;

  if (!glIsTexture(m_textureId))
    CreateTexture();

  glBindTexture(m_textureTarget, m_textureId);

  // Directly sampling CPU-written DMAHeap and UDMABuf buffers has demonstrated
  // stale/torn data on tested systems, so currently upload those two backends
  // as a coherency workaround.
  if (RequiresCoherencyWorkaround())
  {
    const bool bUploaded = UploadFromMemory();

    glBindTexture(m_textureTarget, 0);

    return bUploaded;
  }

  std::array<CEGLImage::EglPlane, CEGLImage::MAX_NUM_PLANES> planes;

  planes[0].fd = m_bo->GetFd();
  planes[0].offset = 0;
  planes[0].pitch = m_bo->GetStride();
  planes[0].modifier = m_bo->GetModifier();

  CEGLImage::EglAttrs attribs;

  attribs.width = m_width;
  attribs.height = m_height;
  attribs.format = m_fourcc;
  attribs.planes = planes;

  if (m_egl->CreateImage(attribs))
    m_egl->UploadImage(m_textureTarget);

  m_egl->DestroyImage();

  glBindTexture(m_textureTarget, 0);

  return true;
}

void CRenderBufferDMA::DeleteTexture()
{
  if (glIsTexture(m_textureId))
    glDeleteTextures(1, &m_textureId);

  m_textureId = 0;
}
