/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/RetroPlayer/buffers/BaseRenderBuffer.h"

#include <memory>

#include "system_gl.h"

class CEGLImage;
class IBufferObject;

namespace KODI
{
namespace RETRO
{
/**
 * @brief Common DMA-buffer allocation, CPU access and EGL import behavior.
 *
 * API-specific subclasses provide the upload fallback used for backends that
 * currently require the coherency workaround.
 *
 */
class CRenderBufferDMA : public CBaseRenderBuffer
{
public:
  explicit CRenderBufferDMA(int fourcc);
  ~CRenderBufferDMA() override;

  // Implementation of IRenderBuffer via CBaseRenderBuffer
  bool Allocate(AVPixelFormat format, unsigned int width, unsigned int height) override;
  size_t GetFrameSize() const override;
  uint8_t* GetMemory() override;
  void ReleaseMemory() override;
  bool UploadTexture() override;

  GLuint TextureID() const { return m_textureId; }

protected:
  virtual bool UploadFromMemory() = 0;
  virtual void ConfigureTexture() = 0;

  uint32_t GetStride() const;

  // Construction parameters
  const int m_fourcc = 0;

  const GLenum m_textureTarget = GL_TEXTURE_2D;
  GLuint m_textureId = 0;

private:
  void CreateTexture();
  void DeleteTexture();
  bool RequiresCoherencyWorkaround() const;

  std::unique_ptr<CEGLImage> m_egl;
  std::unique_ptr<IBufferObject> m_bo;
};
} // namespace RETRO
} // namespace KODI
