/*
 *      Copyright (C) 2005-2013 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "GBMBufferObject.h"

#include "ServiceBroker.h"
#include "windowing/gbm/WinSystemGbmGLESContext.h"

#include <gbm.h>

CGBMBufferObject::CGBMBufferObject(int format) :
  m_format(format)
{
  m_device = static_cast<CWinSystemGbmGLESContext*>(CServiceBroker::GetWinSystem())->GetGBMDevice();
}

CGBMBufferObject::~CGBMBufferObject()
{
  ReleaseMemory();
  DestroyBufferObject();
}

bool CGBMBufferObject::CreateBufferObject(int width, int height)
{
  m_width = width;
  m_height = height;

  m_bo = gbm_bo_create(m_device, m_width, m_height, m_format, GBM_BO_USE_LINEAR);

  if (!m_bo)
    return false;

  m_fd = gbm_bo_get_fd(m_bo);

  return true;
}

void CGBMBufferObject::DestroyBufferObject()
{
  if (m_bo)
    gbm_bo_destroy(m_bo);
}

uint8_t* CGBMBufferObject::GetMemory()
{
  if (m_bo)
  {
    m_map = static_cast<uint8_t*>(gbm_bo_map(m_bo, 0, 0, m_width, m_height, GBM_BO_TRANSFER_WRITE, &m_stride, &m_map_data));
    if (m_map)
      return m_map;
  }

  return nullptr;
}

void CGBMBufferObject::ReleaseMemory()
{
  if (m_bo && m_map)
  {
    gbm_bo_unmap(m_bo, m_map_data);
    m_map_data = nullptr;
    m_map = nullptr;
  }
}

int CGBMBufferObject::GetFd()
{
  return m_fd;
}

int CGBMBufferObject::GetStride()
{
  return m_stride;
}

uint64_t CGBMBufferObject::GetModifier()
{
  return gbm_bo_get_modifier(m_bo);
}
