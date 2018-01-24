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

#include "GBMUtils.h"
#include "utils/log.h"

bool CGBMUtils::CreateDevice(int fd)
{
  if (m_device)
    CLog::Log(LOGWARNING, "CGBMUtils::%s - device already created", __FUNCTION__);

  m_device = gbm_create_device(fd);
  if (!m_device)
  {
    CLog::Log(LOGERROR, "CGBMUtils::%s - failed to create device", __FUNCTION__);
    return false;
  }

  return true;
}

void CGBMUtils::DestroyDevice()
{
  if (!m_device)
    CLog::Log(LOGWARNING, "CGBMUtils::%s - device already destroyed", __FUNCTION__);

  if (m_device)
  {
    gbm_device_destroy(m_device);
    m_device = nullptr;
  }
}

bool CGBMUtils::CreateSurface(int width, int height)
{
  if (m_surface)
    CLog::Log(LOGWARNING, "CGBMUtils::%s - surface already created", __FUNCTION__);

  m_surface = gbm_surface_create(m_device,
                                 width,
                                 height,
                                 GBM_FORMAT_ARGB8888,
                                 GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);

  if (!m_surface)
  {
    CLog::Log(LOGERROR, "CGBMUtils::%s - failed to create surface", __FUNCTION__);
    return false;
  }

  CLog::Log(LOGDEBUG, "CGBMUtils::%s - created surface with size %dx%d", __FUNCTION__,
                                                                         width,
                                                                         height);

  return true;
}

void CGBMUtils::DestroySurface()
{
  if (!m_surface)
    CLog::Log(LOGWARNING, "CGBMUtils::%s - surface already destroyed", __FUNCTION__);

  if (m_surface)
  {
    ReleaseBuffer();

    gbm_surface_destroy(m_surface);
    m_surface = nullptr;
  }
}

struct gbm_bo *CGBMUtils::LockFrontBuffer()
{
  if (m_next_bo)
    CLog::Log(LOGWARNING, "CGBMUtils::%s - uneven surface buffer usage", __FUNCTION__);

  m_next_bo = gbm_surface_lock_front_buffer(m_surface);
  return m_next_bo;
}

void CGBMUtils::ReleaseBuffer()
{
  if (m_bo)
    gbm_surface_release_buffer(m_surface, m_bo);

  m_bo = m_next_bo;
  m_next_bo = nullptr;
}
