/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WebVideoBuffer.h"

#include "platform/linux/waylandhost/WaylandBuffer.h"

#include <mutex>

using namespace KODI::WEB;
using KODI::WAYLANDHOST::CWaylandBuffer;

CWebVideoBuffer::CWebVideoBuffer(int id) : CVideoBufferDRMPRIME(id)
{
}

CWebVideoBuffer::~CWebVideoBuffer()
{
  ClearBuffer();
}

void CWebVideoBuffer::SetBuffer(std::shared_ptr<CWaylandBuffer> buffer)
{
  ClearBuffer();
  m_buffer = std::move(buffer);

  m_descriptor = {};
  AVDRMLayerDescriptor& layer = m_descriptor.layers[0];
  m_descriptor.nb_layers = 1;
  layer.format = m_buffer->GetFormat();
  layer.nb_planes = static_cast<int>(m_buffer->GetPlaneCount());

  for (unsigned int i = 0; i < m_buffer->GetPlaneCount(); ++i)
  {
    const auto& plane = m_buffer->GetPlane(i);
    int object = 0;
    while (object < m_descriptor.nb_objects && m_descriptor.objects[object].fd != plane.fd)
      ++object;
    if (object == m_descriptor.nb_objects)
    {
      m_descriptor.objects[object].fd = plane.fd;
      m_descriptor.objects[object].format_modifier = m_buffer->GetModifier();
      ++m_descriptor.nb_objects;
    }
    layer.planes[i].object_index = object;
    layer.planes[i].offset = plane.offset;
    layer.planes[i].pitch = plane.stride;
  }
}

void CWebVideoBuffer::ClearBuffer()
{
  if (!m_buffer)
    return;

  m_buffer->Unlock();
  m_buffer.reset();
  m_descriptor = {};
}

uint32_t CWebVideoBuffer::GetWidth() const
{
  return m_buffer ? m_buffer->GetWidth() : 0;
}

uint32_t CWebVideoBuffer::GetHeight() const
{
  return m_buffer ? m_buffer->GetHeight() : 0;
}

AVDRMFrameDescriptor* CWebVideoBuffer::GetDescriptor() const
{
  return const_cast<AVDRMFrameDescriptor*>(&m_descriptor);
}

CWebVideoBufferPool::~CWebVideoBufferPool()
{
  for (CWebVideoBuffer* buffer : m_all)
    delete buffer;
}

CVideoBuffer* CWebVideoBufferPool::Get()
{
  std::unique_lock lock(m_critSection);

  CWebVideoBuffer* buffer;
  if (!m_free.empty())
  {
    buffer = m_all[m_free.front()];
    m_free.pop_front();
  }
  else
  {
    buffer = new CWebVideoBuffer(static_cast<int>(m_all.size()));
    m_all.push_back(buffer);
  }

  buffer->Acquire(GetPtr());
  return buffer;
}

void CWebVideoBufferPool::Return(int id)
{
  std::unique_lock lock(m_critSection);
  m_all[id]->ClearBuffer();
  m_free.push_back(id);
}
