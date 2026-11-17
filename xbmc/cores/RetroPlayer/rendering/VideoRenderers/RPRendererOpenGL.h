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
class CShaderTextureGL;
class CShaderTextureGLRef;
} // namespace SHADER

namespace RETRO
{
class CRenderBufferOpenGL;
class CRPHdrShader;

class CRendererFactoryOpenGL : public IRendererFactory
{
public:
  ~CRendererFactoryOpenGL() override = default;

  // Implementation of IRendererFactory
  std::string RenderSystemName() const override;
  CRPBaseRenderer* CreateRenderer(const CRenderSettings& settings,
                                  CRenderContext& context,
                                  std::shared_ptr<IRenderBufferPool> bufferPool) override;
  RenderBufferPoolVector CreateBufferPools(CRenderContext& context) override;
};

class CRPRendererOpenGL : public CRPBaseRenderer
{
public:
  CRPRendererOpenGL(const CRenderSettings& renderSettings,
                    CRenderContext& context,
                    std::shared_ptr<IRenderBufferPool> bufferPool);
  ~CRPRendererOpenGL() override;

  // Implementation of CRPBaseRenderer
  bool Supports(RENDERFEATURE feature) const override;
  SCALINGMETHOD GetDefaultScalingMethod() const override { return SCALINGMETHOD::NEAREST; }

  static bool SupportsScalingMethod(SCALINGMETHOD method);
  bool SupportsHdr() const override { return !m_hdrShaderFailed; }

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
    std::shared_ptr<SHADER::CShaderTextureGLRef> sourceTexture;
    std::shared_ptr<SHADER::CShaderTextureGL> targetTexture;
  };

  /*!
   * \brief Make the texture a shader preset's last pass renders into, deeper than 8 bits in HDR
   */
  std::shared_ptr<SHADER::CShaderTextureGL> CreateTargetTexture() const;

  /*!
   * \brief Draw the texture bound to unit 0 into the destination, for SDR or HDR
   *
   * \param alpha The frame's opacity
   * \param width, height Size of the texture, which the source rectangle is in
   */
  void DrawFrame(uint8_t alpha, float width, float height);

  // Implementation of CRPBaseRenderer
  void RenderInternal(uint8_t alpha) override;
  void FlushInternal() override;

  virtual void Render(uint8_t alpha);

  std::map<CRenderBufferOpenGL*, std::unique_ptr<RenderBufferTextures>> m_RBTexturesMap;
  bool m_lastTargetHdr{false};

  GLuint m_mainVAO;
  GLuint m_mainVertexVBO;
  GLuint m_mainIndexVBO;

  const GLenum m_textureTarget = GL_TEXTURE_2D;

private:
  bool LoadHdrShader();

  std::unique_ptr<CRPHdrShader> m_hdrShader;
  bool m_hdrShaderFailed{false};
  GLuint m_hdrVAO{0};
  GLuint m_hdrVBO{0};
};
} // namespace RETRO
} // namespace KODI
