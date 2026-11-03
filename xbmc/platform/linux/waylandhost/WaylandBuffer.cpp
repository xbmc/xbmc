/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WaylandBuffer.h"

#include "WaylandHost.h"

#include <unistd.h>
#include <wayland-server-core.h>
#include <wayland-server-protocol.h>

using namespace KODI::WAYLANDHOST;

CWaylandBuffer::CWaylandBuffer(CWaylandHost& host,
                               uint32_t width,
                               uint32_t height,
                               uint32_t format,
                               uint64_t modifier,
                               const std::array<DmaBufPlane, MAX_PLANES>& planes,
                               unsigned int planeCount)
  : m_host(host),
    m_width(width),
    m_height(height),
    m_format(format),
    m_modifier(modifier),
    m_planes(planes),
    m_planeCount(planeCount)
{
}

CWaylandBuffer::~CWaylandBuffer()
{
  for (unsigned int i = 0; i < m_planeCount; ++i)
  {
    if (m_planes[i].fd >= 0)
      close(m_planes[i].fd);
  }
}

void CWaylandBuffer::Lock()
{
  ++m_locks;
}

void CWaylandBuffer::Unlock()
{
  if (--m_locks > 0)
    return;

  m_host.Post([buffer = shared_from_this()]() { buffer->SendRelease(); });
}

void CWaylandBuffer::SendRelease()
{
  if (m_current || IsLocked() || !m_resource)
    return;

  wl_buffer_send_release(m_resource);
}
