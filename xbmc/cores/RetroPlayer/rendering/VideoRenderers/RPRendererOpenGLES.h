/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "RPBaseRenderer.h"
#include "cores/RetroPlayer/process/RPProcessInfo.h"

#include <map>
#include <memory>
#include <stdint.h>

#include "system_gl.h"

namespace KODI
{
namespace SHADER
{
class CShaderTextureGLESRef;
} // namespace SHADER

namespace RETRO
{
class CRenderBufferOpenGLES;

class CRendererFactoryOpenGLES : public IRendererFactory
{
public:
  ~CRendererFactoryOpenGLES() override = default;

  // implementation of IRendererFactory
  std::string RenderSystemName() const override;
  CRPBaseRenderer* CreateRenderer(const CRenderSettings& settings,
                                  CRenderContext& context,
                                  std::shared_ptr<IRenderBufferPool> bufferPool) override;
  RenderBufferPoolVector CreateBufferPools(CRenderContext& context) override;
};

class CRPRendererOpenGLES : public CRPBaseRenderer
{
public:
  CRPRendererOpenGLES(const CRenderSettings& renderSettings,
                      CRenderContext& context,
                      std::shared_ptr<IRenderBufferPool> bufferPool);
  ~CRPRendererOpenGLES() override;

  // implementation of CRPBaseRenderer
  bool Supports(RENDERFEATURE feature) const override;
  SCALINGMETHOD GetDefaultScalingMethod() const override { return SCALINGMETHOD::NEAREST; }

  static bool SupportsScalingMethod(SCALINGMETHOD method);

protected:
  struct PackedVertex
  {
    float x, y, z;
    float u1, v1;
  };

  struct Svertex
  {
    float x;
    float y;
    float z;
  };

  struct RenderBufferTextures
  {
    std::shared_ptr<SHADER::CShaderTextureGLESRef> source;
    std::shared_ptr<SHADER::CShaderTextureGLESRef> target;
  };

  // implementation of CRPBaseRenderer
  void RenderInternal(bool clear, uint8_t alpha) override;
  void FlushInternal() override;

  /*!
   * \brief Set the entire backbuffer to black
   */
  void ClearBackBuffer();

  /*!
   * \brief Draw black bars around the video quad
   *
   * This is more efficient than glClear() since it only sets pixels to
   * black that aren't going to be overwritten by the game.
   */
  void DrawBlackBars();

  virtual void Render(uint8_t alpha);

  std::map<CRenderBufferOpenGLES*, std::unique_ptr<RenderBufferTextures>> m_RBTexturesMap;

  GLuint m_mainIndexVBO;
  GLuint m_mainVertexVBO;

  GLuint m_blackbarsVertexVBO;

  GLenum m_textureTarget = GL_TEXTURE_2D;
  float m_clearColor = 0.0f;
};
} // namespace RETRO
} // namespace KODI
