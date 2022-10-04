/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DumbBufferObject.h"

#include "ServiceBroker.h"
#include "utils/BufferObjectFactory.h"
#include "utils/log.h"
#include "windowing/gbm/WinSystemGbm.h"
#include "windowing/gbm/WinSystemGbmEGLContext.h"

#include <drm_fourcc.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include "PlatformDefs.h"

using namespace KODI::WINDOWING::GBM;

std::unique_ptr<CBufferObject> CDumbBufferObject::Create()
{
  return std::make_unique<CDumbBufferObject>();
}

void CDumbBufferObject::Register()
{
  CBufferObjectFactory::RegisterBufferObject(CDumbBufferObject::Create);
}

CDumbBufferObject::CDumbBufferObject()
{
  auto winSystem = static_cast<CWinSystemGbmEGLContext*>(CServiceBroker::GetWinSystem());

  m_device = winSystem->GetDrm()->GetFileDescriptor();
}

CDumbBufferObject::~CDumbBufferObject()
{
  ReleaseMemory();
  DestroyBufferObject();
}

bool CDumbBufferObject::CreateBufferObject(uint32_t format, uint32_t width, uint32_t height)
{
  if (m_fd >= 0)
    return true;

  uint32_t bpp;

  switch (format)
  {
    case DRM_FORMAT_ARGB1555:
    case DRM_FORMAT_RGB565:
      bpp = 16;
      break;
    case DRM_FORMAT_ARGB8888:
      bpp = 32;
      break;
    default:
      throw std::runtime_error("CDumbBufferObject: pixel format not implemented");
  }

  struct drm_mode_create_dumb create_dumb{};
  create_dumb.height = height;
  create_dumb.width = width;
  create_dumb.bpp = bpp;

  int ret = drmIoctl(m_device, DRM_IOCTL_MODE_CREATE_DUMB, &create_dumb);
  if (ret < 0)
  {
    CLog::Log(LOGERROR, "CDumbBufferObject::{} - ioctl DRM_IOCTL_MODE_CREATE_DUMB failed, errno={}",
              __FUNCTION__, strerror(errno));
    return false;
  }

  m_size = create_dumb.size;
  m_stride = create_dumb.pitch;

  ret = drmPrimeHandleToFD(m_device, create_dumb.handle, 0, &m_fd);
  if (ret < 0)
  {
    CLog::Log(LOGERROR, "CDumbBufferObject::{} - failed to get fd from prime handle, errno={}",
              __FUNCTION__, strerror(errno));
    return false;
  }

  return true;
}

void CDumbBufferObject::DestroyBufferObject()
{
  if (m_fd < 0)
    return;

  int ret = close(m_fd);
  if (ret < 0)
    CLog::Log(LOGERROR, "CDumbBufferObject::{} - close failed, errno={}", __FUNCTION__,
              strerror(errno));

  m_fd = -1;
  m_stride = 0;
  m_size = 0;
}

uint8_t* CDumbBufferObject::GetMemory()
{
  if (m_fd < 0)
    return nullptr;

  if (m_map)
  {
    CLog::Log(LOGDEBUG, "CDumbBufferObject::{} - already mapped fd={} map={}", __FUNCTION__, m_fd,
              fmt::ptr(m_map));
    return m_map;
  }

  uint32_t handle;
  int ret = drmPrimeFDToHandle(m_device, m_fd, &handle);
  if (ret < 0)
  {
    CLog::Log(LOGERROR, "CDumbBufferObject::{} - failed to get handle from prime fd, errno={}",
              __FUNCTION__, strerror(errno));
    return nullptr;
  }

  struct drm_mode_map_dumb map_dumb{};
  map_dumb.handle = handle;

  ret = drmIoctl(m_device, DRM_IOCTL_MODE_MAP_DUMB, &map_dumb);
  if (ret < 0)
  {
    CLog::Log(LOGERROR, "CDumbBufferObject::{} - ioctl DRM_IOCTL_MODE_MAP_DUMB failed, errno={}",
              __FUNCTION__, strerror(errno));
    return nullptr;
  }

  m_offset = map_dumb.offset;

  m_map = static_cast<uint8_t*>(mmap(nullptr, m_size, PROT_WRITE, MAP_SHARED, m_device, m_offset));
  if (m_map == MAP_FAILED)
  {
    CLog::Log(LOGERROR, "CDumbBufferObject::{} - mmap failed, errno={}", __FUNCTION__,
              strerror(errno));
    return nullptr;
  }

  return m_map;
}

void CDumbBufferObject::ReleaseMemory()
{
  if (!m_map)
    return;

  int ret = munmap(m_map, m_size);
  if (ret < 0)
    CLog::Log(LOGERROR, "CDumbBufferObject::{} - munmap failed, errno={}", __FUNCTION__,
              strerror(errno));

  m_map = nullptr;
  m_offset = 0;
}
