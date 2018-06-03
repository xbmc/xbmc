/*
 *      Copyright (C) 2017-present Team Kodi
 *      This file is part of Kodi - https://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "RenderBufferSysMem.h"

using namespace KODI;
using namespace RETRO;

bool CRenderBufferSysMem::Allocate(AVPixelFormat format, unsigned int width, unsigned int height, size_t size)
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
