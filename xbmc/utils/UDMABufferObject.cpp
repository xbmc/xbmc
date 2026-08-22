/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "UDMABufferObject.h"

#include "utils/BufferObjectFactory.h"
#include "utils/log.h"

#include <drm_fourcc.h>
#include <fcntl.h>
#include <linux/udmabuf.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#include "PlatformDefs.h"

namespace
{

static constexpr uint32_t STRIDE_ALIGN = 256;

static constexpr uint64_t AlignUpU64(uint64_t value, uint64_t alignment)
{
  return (value + alignment - 1) & ~(alignment - 1);
}

int RoundUp(int num, int factor)
{
  return num + factor - 1 - (num - 1) % factor;
}

} // namespace

std::unique_ptr<CBufferObject> CUDMABufferObject::Create()
{
  return std::make_unique<CUDMABufferObject>();
}

void CUDMABufferObject::Register()
{
  int fd = open("/dev/udmabuf", O_RDWR);
  if (fd < 0)
  {
    CLog::Log(LOGDEBUG, "CUDMABufferObject::{} - unable to open /dev/udmabuf: {}", __FUNCTION__,
              strerror(errno));
    return;
  }

  close(fd);

  CBufferObjectFactory::RegisterBufferObject(CUDMABufferObject::Create);
}

CUDMABufferObject::~CUDMABufferObject()
{
  ReleaseMemory();
  DestroyBufferObject();

  int ret = close(m_udmafd);
  if (ret < 0)
    CLog::Log(LOGERROR, "CUDMABufferObject::{} - close /dev/udmabuf failed, errno={}", __FUNCTION__,
              strerror(errno));

  m_udmafd = -1;
}

bool CUDMABufferObject::CreateBufferObject(uint32_t format, uint32_t width, uint32_t height)
{
  if (m_fd >= 0)
    return true;

  uint32_t bpp{1};

  switch (format)
  {
    case DRM_FORMAT_ARGB8888:
    case DRM_FORMAT_XRGB8888:
      bpp = 4;
      break;
    case DRM_FORMAT_ARGB1555:
    case DRM_FORMAT_RGB565:
      bpp = 2;
      break;
    default:
      throw std::runtime_error("CUDMABufferObject: pixel format not implemented");
  }

  const uint64_t rawStride = static_cast<uint64_t>(width) * bpp;
  const uint64_t stride = AlignUpU64(rawStride, STRIDE_ALIGN);

  if (stride > std::numeric_limits<uint32_t>::max())
  {
    CLog::Log(LOGERROR, "CUDMABufferObject::{} - stride overflow ({}x{} fmt={})", __FUNCTION__,
              width, height, format);
    return false;
  }

  m_stride = static_cast<uint32_t>(stride);

  if (rawStride != static_cast<uint64_t>(m_stride))
  {
    CLog::Log(LOGDEBUG,
              "CUDMABufferObject::{} - aligned stride for format={} size={}x{} from {} to {}",
              __FUNCTION__, format, width, height, rawStride, m_stride);
  }

  const uint64_t bufferSize = stride * height;
  return CreateBufferObject(bufferSize);
}

bool CUDMABufferObject::CreateBufferObject(uint64_t size)
{
  // Must be rounded to the system page size
  m_size = RoundUp(size, sysconf(_SC_PAGESIZE));

  m_memfd = memfd_create("kodi", MFD_CLOEXEC | MFD_ALLOW_SEALING);
  if (m_memfd < 0)
  {
    CLog::Log(LOGERROR, "CUDMABufferObject::{} - memfd_create failed: {}", __FUNCTION__,
              strerror(errno));
    return false;
  }

  if (ftruncate(m_memfd, m_size) < 0)
  {
    CLog::Log(LOGERROR, "CUDMABufferObject::{} - ftruncate failed: {}", __FUNCTION__,
              strerror(errno));
    return false;
  }

  if (fcntl(m_memfd, F_ADD_SEALS, F_SEAL_SHRINK) < 0)
  {
    CLog::Log(LOGERROR, "CUDMABufferObject::{} - fcntl failed: {}", __FUNCTION__, strerror(errno));
    close(m_memfd);
    return false;
  }

  if (m_udmafd < 0)
  {
    m_udmafd = open("/dev/udmabuf", O_RDWR);
    if (m_udmafd < 0)
    {
      CLog::Log(LOGERROR, "CUDMABufferObject::{} - unable to open /dev/udmabuf: {}", __FUNCTION__,
                strerror(errno));
      close(m_memfd);
      return false;
    }
  }

  struct udmabuf_create_item create{};
  create.memfd = static_cast<uint32_t>(m_memfd);
  create.offset = 0;
  create.size = m_size;

  m_fd = ioctl(m_udmafd, UDMABUF_CREATE, &create);
  if (m_fd < 0)
  {
    CLog::Log(LOGERROR, "CUDMABufferObject::{} - ioctl UDMABUF_CREATE failed: {}", __FUNCTION__,
              strerror(errno));
    close(m_memfd);
    return false;
  }

  return true;
}

void CUDMABufferObject::DestroyBufferObject()
{
  if (m_fd < 0)
    return;

  int ret = close(m_fd);
  if (ret < 0)
    CLog::Log(LOGERROR, "CUDMABufferObject::{} - close fd failed, errno={}", __FUNCTION__,
              strerror(errno));

  ret = close(m_memfd);
  if (ret < 0)
    CLog::Log(LOGERROR, "CUDMABufferObject::{} - close memfd failed, errno={}", __FUNCTION__,
              strerror(errno));

  m_memfd = -1;
  m_fd = -1;
  m_stride = 0;
  m_size = 0;
}

uint8_t* CUDMABufferObject::GetMemory()
{
  if (m_fd < 0)
    return nullptr;

  if (m_map)
  {
    CLog::Log(LOGDEBUG, "CUDMABufferObject::{} - already mapped fd={} map={}", __FUNCTION__, m_fd,
              fmt::ptr(m_map));
    return m_map;
  }

  m_map = static_cast<uint8_t*>(mmap(nullptr, m_size, PROT_WRITE, MAP_SHARED, m_memfd, 0));
  if (m_map == MAP_FAILED)
  {
    CLog::Log(LOGERROR, "CUDMABufferObject::{} - mmap failed, errno={}", __FUNCTION__,
              strerror(errno));
    return nullptr;
  }

  return m_map;
}

void CUDMABufferObject::ReleaseMemory()
{
  if (!m_map)
    return;

  int ret = munmap(m_map, m_size);
  if (ret < 0)
    CLog::Log(LOGERROR, "CUDMABufferObject::{} - munmap failed, errno={}", __FUNCTION__,
              strerror(errno));

  m_map = nullptr;
}
