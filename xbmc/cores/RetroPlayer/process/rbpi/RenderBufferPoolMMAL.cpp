/*
 *      Copyright (C) 2017 Team Kodi
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
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "RenderBufferPoolMMAL.h"
#include "RenderBufferMMAL.h"
#include "cores/RetroPlayer/rendering/VideoRenderers/RPRendererMMAL.h"
#include "cores/RetroPlayer/rendering/RenderVideoSettings.h"
#include "cores/VideoPlayer/VideoRenderers/HwDecRender/MMALRenderer.h"
#include "linux/RBP.h"
#include "threads/SingleLock.h"
#include "utils/log.h"

#include <interface/mmal/util/mmal_default_components.h>
#include <interface/mmal/util/mmal_util.h>
#include <interface/mmal/util/mmal_util_params.h>

#include <algorithm>
#include <cmath>

using namespace KODI;
using namespace RETRO;

#define BUFFER_TIMEOUT_MS  500

bool CRenderBufferPoolMMAL::IsCompatible(const CRenderVideoSettings &renderSettings) const
{
  if (!CRPRendererMMAL::SupportsScalingMethod(renderSettings.GetScalingMethod()))
    return false;

  return true;
}

IRenderBuffer *CRenderBufferPoolMMAL::CreateRenderBuffer(void *header /* = nullptr */)
{
  return new CRenderBufferMMALRGB(static_cast<MMAL_BUFFER_HEADER_T*>(header));
}

bool CRenderBufferPoolMMAL::ConfigureInternal()
{
  CSingleLock lock(m_mutex);

  m_mmal_format = MMAL::CMMALPool::TranslateFormat(m_format);

  if (m_mmal_format == MMAL_ENCODING_UNKNOWN)
  {
    CLog::Log(LOGERROR, "MMAL pool: Invalid pixel format (%d)", m_format);
    return false;
  }

  m_alignedWidth = m_width;
  m_alignedHeight = m_height;
  const unsigned int bpp = g_RBP.GetFrameGeometry(m_mmal_format, m_width, m_height).bytes_per_pixel;
  m_frameSize = m_width * m_height * bpp;

  if (m_frameSize == 0)
  {
    CLog::Log(LOGERROR, "Failed to configure pool of dims %ux%ux%u", m_width, m_height, bpp);
    return false;
  }

  if (!InitializeMMAL())
    return false;

  // Success
  CLog::Log(LOGDEBUG, "Pool buffer size %u (%ux%ux%u) format: %d", m_frameSize, m_width, m_height, bpp, m_format);

  return true;
}

bool CRenderBufferPoolMMAL::InitializeMMAL()
{
  // Create MMAL pool
  MMAL_STATUS_T status;

  status = mmal_component_create(MMAL_COMPONENT_DEFAULT_VIDEO_RENDERER, &m_component);
  if (status != MMAL_SUCCESS)
  {
    CLog::Log(LOGERROR, "Failed to create component %s", MMAL_COMPONENT_DEFAULT_VIDEO_RENDERER);
    return false;
  }

  MMAL_PORT_T *port = m_input ? m_component->input[0] : m_component->output[0];

  // Set up initial decoded frame format - may change from this
  port->format->encoding = m_mmal_format;

  status = mmal_port_parameter_set_boolean(port, MMAL_PARAMETER_ZERO_COPY, MMAL_TRUE);
  if (status != MMAL_SUCCESS)
  {
    CLog::Log(LOGERROR, "Failed to enable zero copy mode on %s (status=%x %s)", port->name, status, mmal_status_to_string(status));
    return false;
  }

  status = mmal_port_format_commit(port);
  if (status != MMAL_SUCCESS)
  {
    CLog::Log(LOGERROR, "Failed to commit format for %s (status=%x %s)", port->name, status, mmal_status_to_string(status));
    return false;
  }

  port->buffer_size = m_frameSize;
  unsigned int bufferCount = 1; //! @todo
  port->buffer_num = std::max(bufferCount, port->buffer_num_recommended);

  m_mmal_pool = mmal_port_pool_create(port, port->buffer_num, port->buffer_size);
  if (m_mmal_pool == nullptr)
  {
    CLog::Log(LOGERROR, "Failed to create pool for port %s", port->name);
    return false;
  }

  CLog::Log(LOGDEBUG, "Created pool of size %ux%u for port %s", bufferCount, m_frameSize, port->name);

  return true;
}

void CRenderBufferPoolMMAL::Deinitialize()
{
  Flush();

  MMAL_STATUS_T status;

  MMAL_PORT_T *port = m_input ? m_component->input[0] : m_component->output[0];

  CLog::Log(LOGDEBUG, "Destroying pool for port %s", port->name);

  if (port != nullptr && port->is_enabled)
  {
    status = mmal_port_disable(port);
    if (status != MMAL_SUCCESS)
       CLog::Log(LOGERROR, "Failed to disable port %s (status=%x %s)", port->name, status, mmal_status_to_string(status));
  }

  if (m_component != nullptr && m_component->is_enabled)
  {
    status = mmal_component_disable(m_component);
    if (status != MMAL_SUCCESS)
      CLog::Log(LOGERROR, "Failed to disable component %s (status=%x %s)", m_component->name, status, mmal_status_to_string(status));
  }

  if (m_component != nullptr)
    mmal_component_destroy(m_component);

  m_component = nullptr;

  if (m_mmal_pool != nullptr)
    mmal_port_pool_destroy(port, m_mmal_pool);

  m_mmal_pool = nullptr;
}

void *CRenderBufferPoolMMAL::GetHeader(unsigned int timeoutMs /* = 0 */)
{
  MMAL_BUFFER_HEADER_T *buffer = nullptr;

  if (m_mmal_pool != nullptr && m_mmal_pool->queue != nullptr)
    buffer = mmal_queue_timedwait(m_mmal_pool->queue, timeoutMs);

  if (timeoutMs > 0 && buffer == nullptr)
    CLog::Log(LOGERROR, "Pool failed with timeout %ums", timeoutMs);

  return buffer;
}

bool CRenderBufferPoolMMAL::GetHeaderWithTimeout(void *&header)
{
  header = GetHeader(BUFFER_TIMEOUT_MS);

  return header != nullptr;
}

bool CRenderBufferPoolMMAL::SendBuffer(IRenderBuffer *buffer)
{
  CSingleLock lock(m_mutex);

  if (m_mmal_pool == nullptr || m_component == nullptr)
    return false;

  MMAL_PORT_T *port = m_input ? m_component->input[0] : m_component->output[0];
  if (!port->is_enabled)
    return false;

  MMAL_STATUS_T status = mmal_port_send_buffer(port, static_cast<CRenderBufferMMAL*>(buffer)->GetHeader());

  if (status != MMAL_SUCCESS)
  {
    CLog::Log(LOGERROR, "Failed to send buffer from pool to %s (status=0%x %s)", port->name, status, mmal_status_to_string(status));
    return false;
  }

  return true;
}
