/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RenderBufferGBM.h"

#include "ServiceBroker.h"
#include "utils/EGLImage.h"
#include "utils/GBMBufferObject.h"
#include "windowing/gbm/WinSystemGbmEGLContext.h"

using namespace KODI::WINDOWING::GBM;
using namespace KODI;
using namespace RETRO;

CRenderBufferGBM::CRenderBufferGBM(CRenderContext &context,
                                   int fourcc) :
  m_context(context),
  m_fourcc(fourcc),
  m_egl(new CEGLImage(static_cast<CWinSystemGbmEGLContext*>(CServiceBroker::GetWinSystem())->GetEGLDisplay())),
  m_bo(new CGBMBufferObject(fourcc))
{
}

CRenderBufferGBM::~CRenderBufferGBM()
{
  DeleteTexture();
}

bool CRenderBufferGBM::Allocate(AVPixelFormat format, unsigned int width, unsigned int height)
{
  // Initialize IRenderBuffer
  m_format = format;
  m_width = width;
  m_height = height;

  m_bo->CreateBufferObject(m_width, m_height);

  return true;
}

size_t CRenderBufferGBM::GetFrameSize() const
{
  return m_bo->GetStride() * m_height;
}

uint8_t *CRenderBufferGBM::GetMemory()
{
  return m_bo->GetMemory();
}

void CRenderBufferGBM::ReleaseMemory()
{
  m_bo->ReleaseMemory();
}

void CRenderBufferGBM::CreateTexture()
{
  glGenTextures(1, &m_textureId);

  glBindTexture(m_textureTarget, m_textureId);

  glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  glBindTexture(m_textureTarget, 0);
}

bool CRenderBufferGBM::UploadTexture()
{
  if (m_bo->GetFd() < 0)
    return false;

  if (!glIsTexture(m_textureId))
    CreateTexture();

  glBindTexture(m_textureTarget, m_textureId);

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

void CRenderBufferGBM::DeleteTexture()
{
  if (glIsTexture(m_textureId))
    glDeleteTextures(1, &m_textureId);

  m_textureId = 0;
}
