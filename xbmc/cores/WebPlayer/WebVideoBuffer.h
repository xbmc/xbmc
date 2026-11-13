/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/VideoPlayer/Buffers/VideoBufferDRMPRIME.h"
#include "threads/CriticalSection.h"

#include <deque>
#include <memory>
#include <vector>

namespace KODI
{
namespace WAYLANDHOST
{
class CWaylandBuffer;
}

namespace WEB
{

/*!
 * \brief A Wayland client's dmabuf presented to the video renderers as a
 * DRM PRIME picture. The wl_buffer stays locked for the client until the
 * renderers return it to the pool.
 */
class CWebVideoBuffer : public CVideoBufferDRMPRIME
{
public:
  explicit CWebVideoBuffer(int id);
  ~CWebVideoBuffer() override;

  void SetBuffer(std::shared_ptr<WAYLANDHOST::CWaylandBuffer> buffer);
  void ClearBuffer();

  // implementation of CVideoBufferDRMPRIME
  uint32_t GetWidth() const override;
  uint32_t GetHeight() const override;
  AVDRMFrameDescriptor* GetDescriptor() const override;

private:
  std::shared_ptr<WAYLANDHOST::CWaylandBuffer> m_buffer;
  AVDRMFrameDescriptor m_descriptor{};
};

class CWebVideoBufferPool : public IVideoBufferPool
{
public:
  ~CWebVideoBufferPool() override;

  // implementation of IVideoBufferPool
  CVideoBuffer* Get() override;
  void Return(int id) override;

private:
  CCriticalSection m_critSection;
  std::vector<CWebVideoBuffer*> m_all;
  std::deque<int> m_free;
};

} // namespace WEB
} // namespace KODI
