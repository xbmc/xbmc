/*
 *      Copyright (C) 2017 Team Kodi
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

#include "RenderBufferMMAL.h"
#include "linux/RBP.h"

using namespace KODI;
using namespace RETRO;

// --- CRenderBufferMMAL -------------------------------------------------------

CRenderBufferMMAL::CRenderBufferMMAL(MMAL_BUFFER_HEADER_T *buffer)
{
  SetHeader(buffer);
}

CRenderBufferMMAL::~CRenderBufferMMAL()
{
  ResetHeader();
}

void CRenderBufferMMAL::Update()
{
  if (m_mmal_buffer != nullptr)
  {
    CGPUMEM *gmem = GetGPUBuffer();
    if (gmem != nullptr)
    {
      m_mmal_buffer->alloc_size = gmem->m_numbytes;
      m_mmal_buffer->length = gmem->m_numbytes;
      m_mmal_buffer->data = reinterpret_cast<uint8_t*>(gmem->m_vc_handle);
    }
    else
    {
      m_mmal_buffer->alloc_size = 0;
      m_mmal_buffer->length = 0;
      m_mmal_buffer->data = nullptr;
    }
  }
}

size_t CRenderBufferMMAL::GetFrameSize() const
{
  const CGPUMEM *gmem = GetGPUBuffer();
  if (gmem != nullptr)
    return static_cast<size_t>(gmem->m_numbytes);

  return 0; // Unknown
}

uint8_t *CRenderBufferMMAL::GetMemory()
{
  CGPUMEM *gmem = GetGPUBuffer();
  if (gmem != nullptr)
    return static_cast<uint8_t*>(gmem->m_arm);

  return nullptr;
}

void CRenderBufferMMAL::SetHeader(void *header)
{
  MMAL_BUFFER_HEADER_T *buffer = static_cast<MMAL_BUFFER_HEADER_T*>(header);

  ResetHeader();

  m_mmal_buffer = buffer;

  mmal_buffer_header_reset(m_mmal_buffer);

  m_mmal_buffer->cmd = 0;
  m_mmal_buffer->offset = 0;
  m_mmal_buffer->flags = 0;
  m_mmal_buffer->user_data = this;
}

void CRenderBufferMMAL::ResetHeader()
{
  if (m_mmal_buffer != nullptr)
  {
    mmal_buffer_header_release(m_mmal_buffer);
    m_mmal_buffer = nullptr;
  }
}

// --- CRenderBufferMMALRGB ----------------------------------------------------

CRenderBufferMMALRGB::CRenderBufferMMALRGB(MMAL_BUFFER_HEADER_T *buffer) :
  CRenderBufferMMAL(buffer)
{
}

bool CRenderBufferMMALRGB::Allocate(AVPixelFormat format, unsigned int width, unsigned int height, unsigned int size)
{
  // Initialize IRenderBuffer
  m_format = format;
  m_width = width;
  m_height = height;

  // Allocate GPU memory
  m_gmem.reset(new CGPUMEM(size, true));
  m_gmem->m_opaque = this;

  return m_gmem->m_arm != nullptr;
}
