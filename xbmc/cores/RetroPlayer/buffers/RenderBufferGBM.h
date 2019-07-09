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
class CGBMBufferObject;

namespace KODI
{
namespace RETRO
{
  class CRenderContext;

  class CRenderBufferGBM : public CBaseRenderBuffer
  {
  public:
    CRenderBufferGBM(CRenderContext &context,
                     int fourcc);
    ~CRenderBufferGBM() override;

    // implementation of IRenderBuffer via CRenderBufferSysMem
    bool Allocate(AVPixelFormat format, unsigned int width, unsigned int height) override;
    size_t GetFrameSize() const override;
    uint8_t *GetMemory() override;
    void ReleaseMemory() override;

    // implementation of IRenderBuffer
    bool UploadTexture() override;

    GLuint TextureID() const { return m_textureId; }

  protected:
    // Construction parameters
    CRenderContext &m_context;
    const int m_fourcc = 0;

    const GLenum m_textureTarget = GL_TEXTURE_EXTERNAL_OES;
    GLuint m_textureId = 0;

  private:
    void CreateTexture();
    void DeleteTexture();

    std::unique_ptr<CEGLImage> m_egl;
    std::unique_ptr<CGBMBufferObject> m_bo;
  };
}
}
