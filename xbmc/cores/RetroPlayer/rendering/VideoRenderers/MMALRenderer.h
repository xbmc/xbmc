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

#include "guilib/Geometry.h"
#include "threads/CriticalSection.h"
#include "threads/Thread.h"

#include <interface/mmal/mmal.h>

namespace KODI
{
namespace RETRO
{
  class CMMALProcess;
  class CRenderBufferMMAL;
  class CRenderContext;
  class IRenderBufferPool;

  class CMMALRenderer : public CThread
  {
  public:
    CMMALRenderer(CRenderContext &renderContext, IRenderBufferPool *bufferPool);
    ~CMMALRenderer() override { Deinitialize(); }

    void RegisterProcess(CMMALProcess *process) { m_process = process; }
    void UnregisterProcess() { m_process = nullptr; }

    bool CheckConfigurationVout(uint32_t width, uint32_t height, uint32_t aligned_width, uint32_t aligned_height, MMAL_FOURCC_T encoding);

    void Deinitialize();

    bool PutRenderer(CRenderBufferMMAL *buffer);

    bool SendVout(CRenderBufferMMAL *buffer);

    void SetDimensions(const CRect &sourceRect, const CRect &destRect);

    void SetVideoRect();

    void Flush();

    // MMAL callbacks
    void VoutInputPortCallback(MMAL_PORT_T *port, CRenderBufferMMAL *buffer);

  protected:
    // implementation of CThread
    void Process() override;

  private:
    // Construction parameters
    CRenderContext &m_renderContext;
    IRenderBufferPool *const m_bufferPool;

    // MMAL properties
    CMMALProcess *m_process = nullptr;
    MMAL_QUEUE_T *m_queue_render = nullptr;
    MMAL_COMPONENT_T *m_vout = nullptr;
    MMAL_PORT_T *m_vout_input = nullptr;
    uint32_t m_vout_width = 0;
    uint32_t m_vout_height = 0;
    uint32_t m_vout_aligned_width = 0;
    uint32_t m_vout_aligned_height = 0;
    MMAL_BUFFER_HEADER_T m_quitpacket;

    // Dimensions
    CRect m_cachedSourceRect;
    CRect m_cachedDestRect;
    CRect m_src_rect;
    CRect m_dst_rect;

    // Synchronization parameters
    CCriticalSection m_mutex;
  };
}
}
