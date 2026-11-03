/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <array>
#include <atomic>
#include <cstdint>
#include <memory>

struct wl_resource;

namespace KODI
{
namespace WAYLANDHOST
{

class CWaylandHost;

struct DmaBufPlane
{
  int fd{-1};
  uint32_t offset{0};
  uint32_t stride{0};
};

/*!
 * \brief A dmabuf-backed wl_buffer a client attached to one of its surfaces.
 *
 * The host keeps the planes' file descriptors open for as long as the object
 * lives, so a consumer may hold the buffer after the client destroyed the
 * wl_buffer. wl_buffer.release is sent once the surface no longer shows the
 * buffer and no consumer holds a lock on it.
 */
class CWaylandBuffer : public std::enable_shared_from_this<CWaylandBuffer>
{
public:
  static constexpr unsigned int MAX_PLANES = 4;

  CWaylandBuffer(CWaylandHost& host,
                 uint32_t width,
                 uint32_t height,
                 uint32_t format,
                 uint64_t modifier,
                 const std::array<DmaBufPlane, MAX_PLANES>& planes,
                 unsigned int planeCount);
  ~CWaylandBuffer();

  CWaylandBuffer(const CWaylandBuffer&) = delete;
  CWaylandBuffer& operator=(const CWaylandBuffer&) = delete;

  uint32_t GetWidth() const { return m_width; }
  uint32_t GetHeight() const { return m_height; }
  uint32_t GetFormat() const { return m_format; }
  uint64_t GetModifier() const { return m_modifier; }
  unsigned int GetPlaneCount() const { return m_planeCount; }
  const DmaBufPlane& GetPlane(unsigned int index) const { return m_planes[index]; }

  /*!
   * \brief Keep the buffer from being released to the client; any thread
   */
  void Lock();

  /*!
   * \brief Drop a lock taken with Lock(); any thread
   */
  void Unlock();

  // host thread only
  void SetResource(wl_resource* resource) { m_resource = resource; }
  wl_resource* GetResource() const { return m_resource; }
  void SetCurrent(bool current) { m_current = current; }
  bool IsCurrent() const { return m_current; }
  bool IsLocked() const { return m_locks.load() > 0; }
  void SendRelease();

private:
  CWaylandHost& m_host;
  const uint32_t m_width;
  const uint32_t m_height;
  const uint32_t m_format;
  const uint64_t m_modifier;
  std::array<DmaBufPlane, MAX_PLANES> m_planes;
  const unsigned int m_planeCount;
  wl_resource* m_resource{nullptr};
  bool m_current{false};
  std::atomic<int> m_locks{0};
};

} // namespace WAYLANDHOST
} // namespace KODI
