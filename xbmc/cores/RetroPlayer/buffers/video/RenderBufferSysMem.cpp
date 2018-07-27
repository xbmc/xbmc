/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RenderBufferSysMem.h"
#include "cores/RetroPlayer/rendering/RenderTranslator.h"

using namespace KODI;
using namespace RETRO;

bool CRenderBufferSysMem::Allocate(AVPixelFormat format, unsigned int width, unsigned int height)
{
  // Initialize IRenderBuffer
  m_format = format;
  m_width = width;
  m_height = height;

  const size_t size = GetBufferSize(format, width, height);

  if (m_format != AV_PIX_FMT_NONE && size > 0)
  {
    // Allocate memory
    m_data.resize(size);
    return true;
  }

  return false;
}

size_t CRenderBufferSysMem::GetFrameSize() const
{
  return m_data.size();
}

uint8_t *CRenderBufferSysMem::GetMemory()
{
  return m_data.data();
}

size_t CRenderBufferSysMem::GetBufferSize(AVPixelFormat format, unsigned int width, unsigned int height)
{
  const size_t bufferStride = CRenderTranslator::TranslateWidthToBytes(width, format);
  const size_t bufferSize = bufferStride * height;

  return bufferSize;
}
