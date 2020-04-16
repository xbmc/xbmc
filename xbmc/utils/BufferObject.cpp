/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "BufferObject.h"

#include "BufferObjectFactory.h"
#include "utils/log.h"

#if defined(HAVE_LINUX_DMA_BUF)
#include <linux/dma-buf.h>
#include <sys/ioctl.h>
#endif

std::unique_ptr<CBufferObject> CBufferObject::GetBufferObject(bool needsCreateBySize)
{
  return CBufferObjectFactory::CreateBufferObject(needsCreateBySize);
}

int CBufferObject::GetFd()
{
  return m_fd;
}

uint32_t CBufferObject::GetStride()
{
  return m_stride;
}

uint64_t CBufferObject::GetModifier()
{
  return 0; // linear
}

void CBufferObject::SyncStart()
{
#if defined(HAVE_LINUX_DMA_BUF)
  struct dma_buf_sync sync;
  sync.flags = DMA_BUF_SYNC_START | DMA_BUF_SYNC_RW;

  int ret = ioctl(m_fd, DMA_BUF_IOCTL_SYNC, &sync);
  if (ret < 0)
    CLog::LogF(LOGERROR, "ioctl DMA_BUF_IOCTL_SYNC failed, ret={} errno={}", ret, strerror(errno));
#endif
}

void CBufferObject::SyncEnd()
{
#if defined(HAVE_LINUX_DMA_BUF)
  struct dma_buf_sync sync;
  sync.flags = DMA_BUF_SYNC_END | DMA_BUF_SYNC_RW;

  int ret = ioctl(m_fd, DMA_BUF_IOCTL_SYNC, &sync);
  if (ret < 0)
    CLog::LogF(LOGERROR, "ioctl DMA_BUF_IOCTL_SYNC failed, ret={} errno={}", ret, strerror(errno));
#endif
}
