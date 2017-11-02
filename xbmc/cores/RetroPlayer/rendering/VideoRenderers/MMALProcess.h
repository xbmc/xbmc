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
#pragma once

#include "threads/CriticalSection.h"
#include "threads/Thread.h"

#include <interface/mmal/mmal.h>

namespace KODI
{
namespace RETRO
{
  class CMMALRenderer;
  class CRenderBufferMMAL;
  class IRenderBufferPool;

  class CMMALProcess : public CThread
  {
  public:
    CMMALProcess(IRenderBufferPool *bufferPool, CMMALRenderer *renderer);
    ~CMMALProcess() override { Deinitialize(); }

    void Deinitialize();

    void Put(CRenderBufferMMAL *buffer);

  protected:
    // implementation of CThread
    void Process() override;

  private:
    void HandleBuffer(MMAL_BUFFER_HEADER_T *buffer);

    // Construction parameters
    IRenderBufferPool *const m_bufferPool;
    CMMALRenderer *const m_renderer;

    // MMAL properties
    MMAL_QUEUE_T *m_queue_process = nullptr;
    MMAL_BUFFER_HEADER_T m_quitpacket;

    // Synchronization parameters
    CCriticalSection m_mutex;
  };
}
}
