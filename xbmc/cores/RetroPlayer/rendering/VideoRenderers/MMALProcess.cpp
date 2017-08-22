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

#include "MMALProcess.h"
#include "MMALRenderer.h"
#include "cores/RetroPlayer/process/rbpi/RenderBufferMMAL.h"
#include "cores/RetroPlayer/process/IRenderBufferPool.h"
#include "threads/SingleLock.h"
#include "utils/log.h"

using namespace KODI;
using namespace RETRO;

CMMALProcess::CMMALProcess(IRenderBufferPool *bufferPool, CMMALRenderer *renderer) :
  CThread("RPProcessMMAL"),
  m_bufferPool(bufferPool),
  m_renderer(renderer)
{
}

void CMMALProcess::Deinitialize()
{
  CSingleLock lock(m_mutex);

  if (m_queue_process != nullptr)
  {
    mmal_queue_put(m_queue_process, &m_quitpacket);

    {
      CSingleExit unlock(m_mutex);
      StopThread(true);
    }

    mmal_queue_destroy(m_queue_process);
    m_queue_process = nullptr;
  }
}

void CMMALProcess::Put(CRenderBufferMMAL *buffer)
{
  CSingleLock lock(m_mutex);

  mmal_queue_put(m_queue_process, buffer->GetHeader());
}

void CMMALProcess::Process()
{
  CLog::Log(LOGDEBUG, "MMAL process starting");

  while (true)
  {
    MMAL_BUFFER_HEADER_T *buffer = mmal_queue_wait(m_queue_process);
    if (buffer == nullptr)
      break;

    if (buffer == &m_quitpacket)
      break;

    HandleBuffer(buffer);
  }

  CLog::Log(LOGDEBUG, "MMAL process stopping");
}

void CMMALProcess::HandleBuffer(MMAL_BUFFER_HEADER_T *buffer)
{
  CSingleLock lock(m_mutex);

  CRenderBufferMMALRGB *rgbBuffer = static_cast<CRenderBufferMMALRGB*>(buffer->user_data);
  if (rgbBuffer == nullptr)
    return;

  bool bKept = false;

  if (buffer->cmd == 0 &&
      !(buffer->flags & (MMAL_BUFFER_HEADER_FLAG_EOS | MMAL_BUFFER_HEADER_FLAG_TRANSMISSION_FAILED)) &&
      buffer->length > 0)
  {
    if (m_renderer->PutRenderer(rgbBuffer))
    {
      bKept = true;
    }
    else
    {
      if (m_renderer->SendVout(rgbBuffer))
        bKept = true;
    }
  }

  if (!bKept)
    rgbBuffer->Release();
}
