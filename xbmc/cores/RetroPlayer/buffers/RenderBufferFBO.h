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

#include "BaseRenderBuffer.h"

#include <memory>
#include <mutex>

#include "system_gl.h"

namespace KODI
{
namespace RETRO
{
class CRenderBufferPoolFBO;
class CRenderContext;

class CRenderBufferFBO : public CBaseRenderBuffer
{
public:
  enum class Type
  {
    CLIENT,
    CAPTURE,
  };

  CRenderBufferFBO(CRenderContext& context,
                   bool depth,
                   bool stencil,
                   bool bottomLeftOrigin,
                   Type type = Type::CLIENT);
  ~CRenderBufferFBO() override;

  // Implementation of IRenderBuffer via CBaseRenderBuffer
  bool Allocate(AVPixelFormat format, unsigned int width, unsigned int height) override;
  size_t GetFrameSize() const override { return 0; }
  uint8_t* GetMemory() override { return nullptr; }
  bool UploadTexture() override { return true; }
  uintptr_t GetCurrentFramebuffer() override;

  GLuint TextureID() const { return m_resources->texture; }

  std::unique_lock<std::mutex> Lock() const { return std::unique_lock(m_resources->mutex); }
  void WaitForCapture();
  void FinishRender();
  bool SetReady();
  void PrepareForCapture();

  //! Client attachments can exceed frame size; captures are allocated at frame size.
  unsigned int TextureWidth() const { return m_textureWidth; }
  unsigned int TextureHeight() const { return m_textureHeight; }

  //! Captures are normalized to top-left origin; only client buffers can be bottom-left.
  bool BottomLeftOrigin() const { return m_bottomLeftOrigin; }
  bool IsCapture() const { return m_type == Type::CAPTURE; }

private:
  friend class CRenderBufferPoolFBO;

  struct Resources
  {
    // The pool deletes GL objects in their owning context, even when a renderer
    // still holds the CPU buffer. The lock excludes drawing during teardown.
    void Destroy();
    void Abandon();
    std::mutex mutex;
    GLuint framebuffer{0};
    GLuint texture{0};
    GLuint depthStencil{0};
    GLsync ready{nullptr};
    GLsync rendered{nullptr};
    bool retired{false};
  };

  // Construction parameters
  const bool m_depth;
  const bool m_stencil;
  const bool m_bottomLeftOrigin;
  const Type m_type;

  std::shared_ptr<Resources> m_resources = std::make_shared<Resources>();
  unsigned int m_textureWidth{0};
  unsigned int m_textureHeight{0};
};
} // namespace RETRO
} // namespace KODI
