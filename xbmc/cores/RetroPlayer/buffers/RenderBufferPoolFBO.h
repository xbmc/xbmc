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

#include "RenderBufferFBO.h"
#include "cores/RetroPlayer/buffers/BaseRenderBufferPool.h"

#include <mutex>
#include <thread>

#include "system_gl.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>

namespace KODI
{
namespace RETRO
{
class CRenderContext;

/*!
 * \brief Framebuffers for game clients that render on the GPU themselves
 *
 * The pool owns an OpenGL context shared with the one the window system draws
 * with, and hands the client a framebuffer to render each frame into. Kodi then
 * draws the texture that framebuffer is backed by, so the frame never leaves
 * the GPU.
 *
 * \note Desktop OpenGL and OpenGL ES 3.0 or newer. The copy path needs
 *       glBlitFramebuffer and fence syncs, neither of which GLES 2.0 has, so a
 *       GLES 2.0 build has no pool that reports SupportsHardwareRendering() and
 *       such clients are told during negotiation that hardware rendering is
 *       unavailable, and can fall back to software rather than failing later.
 */
class CRenderBufferPoolFBO : public CBaseRenderBufferPool
{
public:
  CRenderBufferPoolFBO(CRenderContext& context);
  ~CRenderBufferPoolFBO() override;

  // implementation of IRenderBufferPool via CRenderBufferPoolSysMem
  bool IsCompatible(const CRenderVideoSettings& renderSettings) const override;

  // implementation of CBaseRenderBufferPool via CRenderBufferPoolSysMem
  IRenderBuffer* CreateRenderBuffer(void* header = nullptr) override;
  bool ConfigureInternal() override;
  IRenderBuffer* GetBuffer(unsigned int width, unsigned int height) override;
  void Return(IRenderBuffer* buffer) override;

  bool SupportsHardwareRendering() const override;
  bool CreateContext(const HwContextProperties& properties) override;
  bool BeginClientFrame() override;
  void EndClientFrame() override;
  void DestroyContext() override;

  IRenderBuffer* CaptureClientFrame(IRenderBuffer* clientBuffer,
                                    unsigned int width,
                                    unsigned int height) override;

protected:
  // Construction parameters
  CRenderContext& m_context;

  // Configuration parameters
  HwContextProperties m_contextProperties;

  // Context operations are serialized independently of Kodi's render thread.
  std::recursive_mutex m_contextMutex;
  unsigned int m_clientFrameDepth{0};
  std::thread::id m_clientThread;
  EGLenum m_prevAPI{EGL_OPENGL_ES_API};
  EGLDisplay m_prevDisplay{EGL_NO_DISPLAY};
  EGLSurface m_prevDraw{EGL_NO_SURFACE};
  EGLSurface m_prevRead{EGL_NO_SURFACE};
  EGLContext m_prevContext{EGL_NO_CONTEXT};
  EGLDisplay m_eglDisplay{EGL_NO_DISPLAY};
  EGLConfig m_eglConfig{};
  EGLContext m_eglContext{EGL_NO_CONTEXT};

private:
  void CollectBuffers();
  std::vector<std::shared_ptr<CRenderBufferFBO::Resources>> m_resources;
  std::mutex m_captureMutex;
  unsigned int m_captureWidth{0};
  unsigned int m_captureHeight{0};
};
} // namespace RETRO
} // namespace KODI
