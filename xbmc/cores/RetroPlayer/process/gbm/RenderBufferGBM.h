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

#include "cores/RetroPlayer/buffers/BaseRenderBuffer.h"

#include "system_gl.h"

#include <memory>

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
                     int fourcc,
                     int bpp);
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
    const int m_bpp;

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
