/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DMAHeapBufferObject.h"

#include "ServiceBroker.h"
#include "utils/BufferObjectFactory.h"
#include "utils/log.h"

#include <array>

#include <drm/drm_fourcc.h>
#include <linux/dma-heap.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

namespace
{

std::array<const char*, 2> DMA_HEAP_PATHS = {
    "/dev/dma_heap/reserved",
    "/dev/dma_heap/system",
};

static const char* DMA_HEAP_PATH;

} // namespace

std::unique_ptr<CBufferObject> CDMAHeapBufferObject::Create()
{
  return std::make_unique<CDMAHeapBufferObject>();
}

void CDMAHeapBufferObject::Register()
{
  for (auto path : DMA_HEAP_PATHS)
  {
    int fd = open(path, O_RDWR);
    if (fd < 0)
    {
      CLog::LogF(LOGERROR, "unable to open {}: {}", path, strerror(errno));
      continue;
    }

    close(fd);
    DMA_HEAP_PATH = path;
    break;
  }

  if (!DMA_HEAP_PATH)
    return;

  CLog::LogF(LOGDEBUG, "using {}: for dma-heap", DMA_HEAP_PATH);

  CBufferObjectFactory::RegisterBufferObject(CDMAHeapBufferObject::Create);
}

CDMAHeapBufferObject::~CDMAHeapBufferObject()
{
  ReleaseMemory();
  DestroyBufferObject();

  close(m_dmaheapfd);
  m_dmaheapfd = -1;
}

bool CDMAHeapBufferObject::CreateBufferObject(uint32_t format, uint32_t width, uint32_t height)
{
  if (m_fd >= 0)
    return true;

  uint32_t bpp{1};

  switch (format)
  {
    case DRM_FORMAT_ARGB8888:
      bpp = 4;
      break;
    case DRM_FORMAT_ARGB1555:
    case DRM_FORMAT_RGB565:
      bpp = 2;
      break;
    default:
      throw std::system_error(errno, std::generic_category(), "pixel format not implemented");
  }

  m_size = width * height * bpp;
  m_stride = width * bpp;

  if (m_dmaheapfd < 0)
  {
    m_dmaheapfd = open(DMA_HEAP_PATH, O_RDWR);
    if (m_dmaheapfd < 0)
    {
      CLog::LogF(LOGERROR, "failed to open {}:", DMA_HEAP_PATH, strerror(errno));
      return false;
    }
  }

  struct dma_heap_allocation_data allocData = {
      .len = m_size, .fd_flags = (O_CLOEXEC | O_RDWR), .heap_flags = 0};

  int ret = ioctl(m_dmaheapfd, DMA_HEAP_IOCTL_ALLOC, &allocData);
  if (ret < 0)
  {
    CLog::LogF(LOGERROR, "ioctl DMA_HEAP_IOCTL_ALLOC failed, ret={} errno={}", ret,
               strerror(errno));
    return false;
  }

  m_fd = allocData.fd;
  m_size = allocData.len;

  if (m_fd < 0 || m_size <= 0)
  {
    CLog::LogF(LOGERROR, "invalid allocation data: fd={} len={}", m_fd, m_size);
    return false;
  }

  return true;
}

void CDMAHeapBufferObject::DestroyBufferObject()
{
  if (m_fd < 0)
    return;

  int ret = close(m_fd);
  if (ret < 0)
    CLog::LogF(LOGERROR, "close failed, errno={}", __FUNCTION__, strerror(errno));

  m_fd = -1;
  m_stride = 0;
  m_size = 0;
}

uint8_t* CDMAHeapBufferObject::GetMemory()
{
  if (m_fd < 0)
    return nullptr;

  if (m_map)
  {
    CLog::LogF(LOGDEBUG, "already mapped fd={} map={}", m_fd, fmt::ptr(m_map));
    return m_map;
  }

  m_map = static_cast<uint8_t*>(mmap(nullptr, m_size, PROT_WRITE, MAP_SHARED, m_fd, 0));
  if (m_map == MAP_FAILED)
  {
    CLog::LogF(LOGERROR, "mmap failed, errno={}", strerror(errno));
    return nullptr;
  }

  return m_map;
}

void CDMAHeapBufferObject::ReleaseMemory()
{
  if (!m_map)
    return;

  int ret = munmap(m_map, m_size);
  if (ret < 0)
    CLog::LogF(LOGERROR, "munmap failed, errno={}", strerror(errno));

  m_map = nullptr;
}
