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

#include "RenderBufferSysMem.h"

using namespace KODI;
using namespace RETRO;

bool CRenderBufferSysMem::Allocate(AVPixelFormat format, unsigned int width, unsigned int height, unsigned int size)
{
  // Initialize IRenderBuffer
  m_format = format;
  m_width = width;
  m_height = height;

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
