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

#include "MMALRenderer.h"
#include "MMALProcess.h"
#include "cores/RetroPlayer/process/rbpi/RenderBufferMMAL.h"
#include "cores/RetroPlayer/process/rbpi/RenderBufferPoolMMAL.h"
#include "cores/RetroPlayer/rendering/RenderContext.h"
#include "linux/RBP.h"
#include "threads/SingleLock.h"
#include "utils/log.h"

#include <interface/mmal/util/mmal_default_components.h>
#include <interface/mmal/util/mmal_util.h>
#include <interface/mmal/util/mmal_util_params.h>

#include <assert.h>
#include <math.h>

using namespace KODI;
using namespace RETRO;

//! @todo How many buffers?
#define MMAL_NUM_OUTPUT_BUFFERS  2

namespace
{
  static void vout_input_port_cb_static(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer)
  {
    CMMALRenderer *mmal = reinterpret_cast<CMMALRenderer*>(port->userdata);
    CRenderBufferMMAL *mmalBuffer = static_cast<CRenderBufferMMAL*>(buffer->user_data);

    mmal->VoutInputPortCallback(port, mmalBuffer);
  }
}

CMMALRenderer::CMMALRenderer(CRenderContext &renderContext, IRenderBufferPool *bufferPool) :
  CThread("RPRendererMMAL"),
  m_renderContext(renderContext),
  m_bufferPool(bufferPool)
{
  assert(m_bufferPool != nullptr);
}

bool CMMALRenderer::CheckConfigurationVout(uint32_t width, uint32_t height, uint32_t aligned_width, uint32_t aligned_height, MMAL_FOURCC_T encoding)
{
  MMAL_STATUS_T status;

  bool sizeChanged = width != m_vout_width ||
                     height != m_vout_height ||
                     aligned_width != m_vout_aligned_width ||
                     aligned_height != m_vout_aligned_height;

  bool encodingChanged = m_vout_input == nullptr ||
                         m_vout_input->format == nullptr ||
                         encoding != m_vout_input->format->encoding;

  if (m_vout == nullptr)
  {
    // Create video renderer
    CLog::Log(LOGDEBUG, "Creating MMAL renderer");

    status = mmal_component_create(MMAL_COMPONENT_DEFAULT_VIDEO_RENDERER, &m_vout);
    if(status != MMAL_SUCCESS)
    {
      CLog::Log(LOGERROR, "Failed to create vout component (status=%x %s)", status, mmal_status_to_string(status));
      return false;
    }

    m_vout_input = m_vout->input[0];

    status = mmal_port_parameter_set_boolean(m_vout_input, MMAL_PARAMETER_NO_IMAGE_PADDING, MMAL_TRUE);
    if (status != MMAL_SUCCESS)
      CLog::Log(LOGERROR, "Failed to enable no image padding mode on %s (status=%x %s)", m_vout_input->name, status, mmal_status_to_string(status));

    status = mmal_port_parameter_set_boolean(m_vout_input, MMAL_PARAMETER_ZERO_COPY, MMAL_TRUE);
    if (status != MMAL_SUCCESS)
       CLog::Log(LOGERROR, "Failed to enable zero copy mode on %s (status=%x %s)", m_vout_input->name, status, mmal_status_to_string(status));

    m_vout_input->format->type = MMAL_ES_TYPE_VIDEO;
  }

  if (m_vout_input != nullptr &&
      m_vout_input->format != nullptr &&
      m_vout_input->format->es != nullptr &&
      (sizeChanged || encodingChanged))
  {
    CLog::Log(LOGDEBUG, "Changing Vout dimensions from %dx%d (%dx%d) to %dx%d (%dx%d) %.4s",
        m_vout_width, m_vout_height, m_vout_aligned_width, m_vout_aligned_height,
        width, height, aligned_width, aligned_height,
        reinterpret_cast<char*>(&encoding));

    // We need to disable port when encoding changes, but not if just resolution changes
    if (encodingChanged && m_vout_input->is_enabled)
    {
      status = mmal_port_disable(m_vout_input);
      if (status != MMAL_SUCCESS)
      {
        CLog::Log(LOGERROR, "Failed to disable vout input port (status=%x %s)", status, mmal_status_to_string(status));
        return false;
      }
    }

    m_vout_width = width;
    m_vout_height = height;
    m_vout_aligned_width = aligned_width;
    m_vout_aligned_height = aligned_height;

    m_vout_input->format->es->video.crop.width = width;
    m_vout_input->format->es->video.crop.height = height;
    m_vout_input->format->es->video.width = aligned_width;
    m_vout_input->format->es->video.height = aligned_height;
    m_vout_input->format->encoding = encoding;

    status = mmal_port_format_commit(m_vout_input);
    if (status != MMAL_SUCCESS)
    {
      CLog::Log(LOGERROR, "Failed to commit vout input format (status=%x %s)", status, mmal_status_to_string(status));
      return false;
    }

    if (!m_vout_input->is_enabled)
    {
      m_vout_input->buffer_num = MMAL_NUM_OUTPUT_BUFFERS;
      m_vout_input->buffer_size = m_vout_input->buffer_size_recommended;
      m_vout_input->userdata = (struct MMAL_PORT_USERDATA_T *)this;

      status = mmal_port_enable(m_vout_input, vout_input_port_cb_static);
      if (status != MMAL_SUCCESS)
      {
        CLog::Log(LOGERROR, "Failed to enable vout input port (status=%x %s)", status, mmal_status_to_string(status));
        return false;
      }
    }
  }

  if (m_vout != nullptr && !m_vout->is_enabled)
  {
    status = mmal_component_enable(m_vout);
    if(status != MMAL_SUCCESS)
    {
      CLog::Log(LOGERROR, "Failed to enable vout component (status=%x %s)", status, mmal_status_to_string(status));
      return false;
    }

    if (m_queue_render == nullptr)
    {
      m_queue_render = mmal_queue_create();
      if (m_queue_render != nullptr)
        Create();
    }
  }

  SetVideoRect();

  return true;
}

void CMMALRenderer::Deinitialize()
{
  CSingleLock lock(m_mutex);

  if (m_queue_render != nullptr)
  {
    mmal_queue_put(m_queue_render, &m_quitpacket);

    {
      // Leave the lock to allow other threads to exit
      CSingleExit unlock(m_mutex);
      StopThread(true);
    }

    mmal_queue_destroy(m_queue_render);
    m_queue_render = nullptr;
  }

  if (m_vout != nullptr)
    mmal_component_disable(m_vout);

  if (m_vout_input != nullptr)
  {
    mmal_port_flush(m_vout_input);
    mmal_port_disable(m_vout_input);
    m_vout_input = nullptr;
  }

  m_bufferPool->Flush();

  if (m_vout != nullptr)
  {
    mmal_component_release(m_vout);
    m_vout = nullptr;
  }

  m_vout_width = 0;
  m_vout_height = 0;
  m_vout_aligned_width = 0;
  m_vout_aligned_height = 0;
  m_src_rect.SetRect(0, 0, 0, 0);
  m_dst_rect.SetRect(0, 0, 0, 0);
}

bool CMMALRenderer::PutRenderer(CRenderBufferMMAL *buffer)
{
  if (m_queue_render != nullptr)
  {
    mmal_queue_put(m_queue_render, buffer->GetHeader());
    return true;
  }

  return false;
}

bool CMMALRenderer::SendVout(CRenderBufferMMAL *buffer)
{
  bool bSuccess = false;

  CRenderBufferPoolMMAL *bufferPool = static_cast<CRenderBufferPoolMMAL*>(m_bufferPool);
  if (CheckConfigurationVout(bufferPool->Width(),
                             bufferPool->Height(),
                             bufferPool->AlignedWidth(),
                             bufferPool->AlignedHeight(),
                             bufferPool->Encoding()))
  {
    MMAL_STATUS_T status = mmal_port_send_buffer(m_vout_input, buffer->GetHeader());
    if (status != MMAL_SUCCESS)
      CLog::Log(LOGERROR, "Failed to send buffer to %s (status=0%x %s)", m_vout_input->name, status, mmal_status_to_string(status));
    else
      bSuccess = true;
  }

  return bSuccess;
}

void CMMALRenderer::SetDimensions(const CRect &sourceRect, const CRect &destRect)
{
  m_cachedSourceRect = sourceRect;
  m_cachedDestRect = destRect;
}

void CMMALRenderer::Process()
{
  SetPriority(THREAD_PRIORITY_ABOVE_NORMAL);

  while (!m_bStop)
  {
    g_RBP.WaitVsync();

    CSingleLock lock(m_mutex);

    // We may need to discard frames if frame rate is above display frame rate
    while (mmal_queue_length(m_queue_render) > 1)
    {
      MMAL_BUFFER_HEADER_T *buffer = mmal_queue_get(m_queue_render);
      if (buffer == &m_quitpacket)
      {
        m_bStop = true;
        break;
      }
      else if (buffer != nullptr)
      {
        CRenderBufferMMALRGB *rgbBuffer = static_cast<CRenderBufferMMALRGB*>(buffer->user_data);
        rgbBuffer->Release();
      }
    }

    if (m_bStop)
      break;

    // Display a new frame
    MMAL_BUFFER_HEADER_T *buffer = mmal_queue_get(m_queue_render);
    if (buffer == &m_quitpacket)
    {
      m_bStop = true;
      break;
    }
    else if (buffer != nullptr)
    {
      CRenderBufferMMALRGB *rgbBuffer = static_cast<CRenderBufferMMALRGB*>(buffer->user_data);
      SendVout(rgbBuffer);
    }
  }
}

void CMMALRenderer::SetVideoRect()
{
  CRect srcRect = m_cachedSourceRect;
  CRect destRect = m_cachedDestRect;

  const unsigned int orientation = 0; //! @todo Get from base renderer

  if (m_vout_input == nullptr)
    return;

  // Fix up transposed video
  if (orientation == 90 || orientation == 270)
  {
    float newWidth;
    float newHeight;
    float aspectRatio = srcRect.Width() / srcRect.Height();

    // Clamp width if too wide
    if (destRect.Height() > destRect.Width())
    {
      newWidth = destRect.Width(); // Clamp to the width of the old dest rect
      newHeight = newWidth * aspectRatio;
    }
    else // Else clamp to height
    {
      newHeight = destRect.Height(); // Clamp to the height of the old dest rect
      newWidth = newHeight / aspectRatio;
    }

    // Calculate the center point of the view and offsets
    float centerX = destRect.x1 + destRect.Width() * 0.5f;
    float centerY = destRect.y1 + destRect.Height() * 0.5f;
    float diffX = newWidth * 0.5f;
    float diffY = newHeight * 0.5f;

    destRect.x1 = centerX - diffX;
    destRect.x2 = centerX + diffX;
    destRect.y1 = centerY - diffY;
    destRect.y2 = centerY + diffY;
  }

  // Check if destination rect has changed
  if (m_dst_rect == destRect && m_src_rect == srcRect)
    return;

  m_src_rect = srcRect;
  m_dst_rect = destRect;

  // Might need to scale up m_dst_rect to display size as video decodes to
  // separate video plane that is at display size
  RESOLUTION res = m_renderContext.GetVideoResolution();
  CRect gui(0, 0, m_renderContext.GetResolutionInfo(res).iWidth, m_renderContext.GetResolutionInfo(res).iHeight);
  CRect display(0, 0, m_renderContext.GetResolutionInfo(res).iScreenWidth, m_renderContext.GetResolutionInfo(res).iScreenHeight);

  if (gui != display)
  {
    float xscale = display.Width()  / gui.Width();
    float yscale = display.Height() / gui.Height();
    destRect.x1 *= xscale;
    destRect.x2 *= xscale;
    destRect.y1 *= yscale;
    destRect.y2 *= yscale;
  }

  MMAL_DISPLAYREGION_T region = { };

  region.set                 = MMAL_DISPLAY_SET_DEST_RECT|MMAL_DISPLAY_SET_SRC_RECT|MMAL_DISPLAY_SET_FULLSCREEN|MMAL_DISPLAY_SET_NOASPECT|MMAL_DISPLAY_SET_MODE|MMAL_DISPLAY_SET_TRANSFORM;
  region.dest_rect.x         = lrintf(destRect.x1);
  region.dest_rect.y         = lrintf(destRect.y1);
  region.dest_rect.width     = lrintf(destRect.Width());
  region.dest_rect.height    = lrintf(destRect.Height());

  region.src_rect.x          = lrintf(srcRect.x1);
  region.src_rect.y          = lrintf(srcRect.y1);
  region.src_rect.width      = lrintf(srcRect.Width());
  region.src_rect.height     = lrintf(srcRect.Height());

  region.fullscreen = MMAL_FALSE;
  region.noaspect = MMAL_TRUE;
  region.mode = MMAL_DISPLAY_MODE_LETTERBOX;

  if (orientation == 90)
    region.transform = MMAL_DISPLAY_ROT90;
  else if (orientation == 180)
    region.transform = MMAL_DISPLAY_ROT180;
  else if (orientation == 270)
    region.transform = MMAL_DISPLAY_ROT270;
  else
    region.transform = MMAL_DISPLAY_ROT0;

  MMAL_STATUS_T status = mmal_util_set_display_region(m_vout_input, &region);
  if (status != MMAL_SUCCESS)
    CLog::Log(LOGERROR, "Failed to set display region (status=%x %s)", status, mmal_status_to_string(status));
}

void CMMALRenderer::Flush()
{
  if (m_vout_input != nullptr)
    mmal_port_flush(m_vout_input);
}

void CMMALRenderer::VoutInputPortCallback(MMAL_PORT_T *port, CRenderBufferMMAL *buffer)
{
  if (m_process != nullptr)
  {
    buffer->GetHeader()->length = 0;
    m_process->Put(buffer);
  }
}
