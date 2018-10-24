/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GBMUtils.h"
#include "utils/log.h"

using namespace KODI::WINDOWING::GBM;

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

bool CGBMUtils::CreateSurface(int width, int height, uint32_t format, const uint64_t *modifiers, const int modifiers_count)
{
  if (m_surface)
    CLog::Log(LOGWARNING, "CGBMUtils::%s - surface already created", __FUNCTION__);

#if defined(HAS_GBM_MODIFIERS)
  m_surface = gbm_surface_create_with_modifiers(m_device,
                                                width,
                                                height,
                                                format,
                                                modifiers,
                                                modifiers_count);
#endif
  if (!m_surface)
  {
    m_surface = gbm_surface_create(m_device,
                                   width,
                                   height,
                                   format,
                                   GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
  }

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
