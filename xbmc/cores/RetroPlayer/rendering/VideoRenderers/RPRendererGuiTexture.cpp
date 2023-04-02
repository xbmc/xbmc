/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RPRendererGuiTexture.h"

#include "cores/RetroPlayer/buffers/video/RenderBufferGuiTexture.h"
#include "cores/RetroPlayer/rendering/RenderContext.h"
#include "cores/RetroPlayer/rendering/RenderVideoSettings.h"

#if defined(HAS_DX)
#include "guilib/GUIShaderDX.h"
#include "guilib/TextureDX.h"

#include <DirectXMath.h>
using namespace DirectX;
#endif

#if defined(HAS_GL) || defined(HAS_GLES)
#include "utils/GLUtils.h"

#include "system_gl.h"
#endif

#include <cstddef>

using namespace KODI;
using namespace RETRO;

// --- CRendererFactoryGuiTexture ----------------------------------------------

std::string CRendererFactoryGuiTexture::RenderSystemName() const
{
  return "GUITexture";
}

CRPBaseRenderer* CRendererFactoryGuiTexture::CreateRenderer(
    const CRenderSettings& settings,
    CRenderContext& context,
    std::shared_ptr<IRenderBufferPool> bufferPool)
{
  return new CRPRendererGuiTexture(settings, context, std::move(bufferPool));
}

RenderBufferPoolVector CRendererFactoryGuiTexture::CreateBufferPools(CRenderContext& context)
{
  return
  {
#if !defined(HAS_DX)
    std::make_shared<CRenderBufferPoolGuiTexture>(SCALINGMETHOD::NEAREST),
#endif
        std::make_shared<CRenderBufferPoolGuiTexture>(SCALINGMETHOD::LINEAR),
  };
}

// --- CRenderBufferPoolGuiTexture -----------------------------------------------

CRenderBufferPoolGuiTexture::CRenderBufferPoolGuiTexture(SCALINGMETHOD scalingMethod)
  : m_scalingMethod(scalingMethod)
{
}

bool CRenderBufferPoolGuiTexture::IsCompatible(const CRenderVideoSettings& renderSettings) const
{
  return renderSettings.GetScalingMethod() == m_scalingMethod;
}

IRenderBuffer* CRenderBufferPoolGuiTexture::CreateRenderBuffer(void* header /* = nullptr */)
{
  return new CRenderBufferGuiTexture(m_scalingMethod);
}

// --- CRPRendererGuiTexture -----------------------------------------------------

CRPRendererGuiTexture::CRPRendererGuiTexture(const CRenderSettings& renderSettings,
                                             CRenderContext& context,
                                             std::shared_ptr<IRenderBufferPool> bufferPool)
  : CRPBaseRenderer(renderSettings, context, std::move(bufferPool))
{
}

bool CRPRendererGuiTexture::Supports(RENDERFEATURE feature) const
{
  return feature == RENDERFEATURE::STRETCH || feature == RENDERFEATURE::ZOOM ||
         feature == RENDERFEATURE::PIXEL_RATIO || feature == RENDERFEATURE::ROTATION;
}

void CRPRendererGuiTexture::RenderInternal(bool clear, uint8_t alpha)
{
  CRenderBufferGuiTexture* renderBuffer = static_cast<CRenderBufferGuiTexture*>(m_renderBuffer);

  CRect rect = m_sourceRect;

  rect.x1 /= renderBuffer->GetWidth();
  rect.x2 /= renderBuffer->GetWidth();
  rect.y1 /= renderBuffer->GetHeight();
  rect.y2 /= renderBuffer->GetHeight();

  float u1 = rect.x1;
  float u2 = rect.x2;
  float v1 = rect.y1;
  float v2 = rect.y2;

  const uint32_t color = (alpha << 24) | 0xFFFFFF;

#if defined(HAS_DX)

  Vertex vertex[5];
  for (int i = 0; i < 4; i++)
  {
    vertex[i].pos = XMFLOAT3(m_rotatedDestCoords[i].x, m_rotatedDestCoords[i].y, 0);
    CD3DHelper::XMStoreColor(&vertex[i].color, color);
    vertex[i].texCoord = XMFLOAT2(0.0f, 0.0f);
    vertex[i].texCoord2 = XMFLOAT2(0.0f, 0.0f);
  }

  (void)u1;
  (void)v1;
  vertex[1].texCoord.x = vertex[2].texCoord.x = u2;
  vertex[2].texCoord.y = vertex[3].texCoord.y = v2;

  vertex[4] = vertex[0]; // Not used when renderBuffer != nullptr

  CGUIShaderDX* pGUIShader = m_context.GetGUIShader();
  if (pGUIShader != nullptr)
  {
    pGUIShader->Begin(SHADER_METHOD_RENDER_TEXTURE_BLEND);

    // Set state to render the image
    auto dxTexture = static_cast<CDXTexture*>(renderBuffer->GetTexture());
    ID3D11ShaderResourceView* shaderRes = dxTexture->GetShaderResource();
    pGUIShader->SetShaderViews(1, &shaderRes);
    pGUIShader->DrawQuad(vertex[0], vertex[1], vertex[2], vertex[3]);
  }

#elif defined(HAS_GL)

  renderBuffer->BindToUnit(0);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_BLEND);

  m_context.EnableGUIShader(GL_SHADER_METHOD::TEXTURE);

  GLubyte colour[4];
  GLubyte idx[4] = {0, 1, 3, 2}; // Determines order of the vertices
  GLuint vertexVBO;
  GLuint indexVBO;

  struct PackedVertex
  {
    float x, y, z;
    float u1, v1;
  } vertex[4];

  // Setup vertex position values
  vertex[0].x = m_rotatedDestCoords[0].x;
  vertex[0].y = m_rotatedDestCoords[0].y;
  vertex[0].z = 0;
  vertex[0].u1 = u1;
  vertex[0].v1 = v1;

  vertex[1].x = m_rotatedDestCoords[1].x;
  vertex[1].y = m_rotatedDestCoords[1].y;
  vertex[1].z = 0;
  vertex[1].u1 = u2;
  vertex[1].v1 = v1;

  vertex[2].x = m_rotatedDestCoords[2].x;
  vertex[2].y = m_rotatedDestCoords[2].y;
  vertex[2].z = 0;
  vertex[2].u1 = u2;
  vertex[2].v1 = v2;

  vertex[3].x = m_rotatedDestCoords[3].x;
  vertex[3].y = m_rotatedDestCoords[3].y;
  vertex[3].z = 0;
  vertex[3].u1 = u1;
  vertex[3].v1 = v2;

  GLint posLoc = m_context.GUIShaderGetPos();
  GLint tex0Loc = m_context.GUIShaderGetCoord0();
  GLint uniColLoc = m_context.GUIShaderGetUniCol();
  GLint depthLoc = m_context.GUIShaderGetDepth();

  glGenBuffers(1, &vertexVBO);
  glBindBuffer(GL_ARRAY_BUFFER, vertexVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(PackedVertex) * 4, &vertex[0], GL_STATIC_DRAW);

  glVertexAttribPointer(posLoc, 3, GL_FLOAT, 0, sizeof(PackedVertex),
                        reinterpret_cast<const GLvoid*>(offsetof(PackedVertex, x)));
  glVertexAttribPointer(tex0Loc, 2, GL_FLOAT, 0, sizeof(PackedVertex),
                        reinterpret_cast<const GLvoid*>(offsetof(PackedVertex, u1)));

  glEnableVertexAttribArray(posLoc);
  glEnableVertexAttribArray(tex0Loc);

  // Setup Colour values
  colour[0] = UTILS::GL::GetChannelFromARGB(UTILS::GL::ColorChannel::R, color);
  colour[1] = UTILS::GL::GetChannelFromARGB(UTILS::GL::ColorChannel::G, color);
  colour[2] = UTILS::GL::GetChannelFromARGB(UTILS::GL::ColorChannel::B, color);
  colour[3] = UTILS::GL::GetChannelFromARGB(UTILS::GL::ColorChannel::A, color);

  if (m_context.UseLimitedColor())
  {
    colour[0] = (235 - 16) * colour[0] / 255 + 16;
    colour[1] = (235 - 16) * colour[1] / 255 + 16;
    colour[2] = (235 - 16) * colour[2] / 255 + 16;
  }

  glUniform4f(uniColLoc, (colour[0] / 255.0f), (colour[1] / 255.0f), (colour[2] / 255.0f),
              (colour[3] / 255.0f));
  glUniform1f(depthLoc, -1.0f);

  glGenBuffers(1, &indexVBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexVBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLubyte) * 4, idx, GL_STATIC_DRAW);

  glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_BYTE, 0);

  glDisableVertexAttribArray(posLoc);
  glDisableVertexAttribArray(tex0Loc);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glDeleteBuffers(1, &vertexVBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  glDeleteBuffers(1, &indexVBO);

  m_context.DisableGUIShader();

#elif defined(HAS_GLES)

  renderBuffer->BindToUnit(0);

  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_BLEND); // Turn blending On

  m_context.EnableGUIShader(GL_SHADER_METHOD::TEXTURE);

  GLubyte col[4];
  GLfloat ver[4][3];
  GLfloat tex[4][2];
  GLubyte idx[4] = {0, 1, 3, 2}; // Determines order of triangle strip

  GLint posLoc = m_context.GUIShaderGetPos();
  GLint tex0Loc = m_context.GUIShaderGetCoord0();
  GLint uniColLoc = m_context.GUIShaderGetUniCol();
  GLint depthLoc = m_context.GUIShaderGetDepth();

  glVertexAttribPointer(posLoc, 3, GL_FLOAT, 0, 0, ver);
  glVertexAttribPointer(tex0Loc, 2, GL_FLOAT, 0, 0, tex);

  glEnableVertexAttribArray(posLoc);
  glEnableVertexAttribArray(tex0Loc);

  // Setup color values
  col[0] = UTILS::GL::GetChannelFromARGB(UTILS::GL::ColorChannel::R, color);
  col[1] = UTILS::GL::GetChannelFromARGB(UTILS::GL::ColorChannel::G, color);
  col[2] = UTILS::GL::GetChannelFromARGB(UTILS::GL::ColorChannel::B, color);
  col[3] = UTILS::GL::GetChannelFromARGB(UTILS::GL::ColorChannel::A, color);

  for (unsigned int i = 0; i < 4; i++)
  {
    // Setup vertex position values
    ver[i][0] = m_rotatedDestCoords[i].x;
    ver[i][1] = m_rotatedDestCoords[i].y;
    ver[i][2] = 0.0f;
  }

  // Setup texture coordinates
  tex[0][0] = tex[3][0] = u1;
  tex[0][1] = tex[1][1] = v1;
  tex[1][0] = tex[2][0] = u2;
  tex[2][1] = tex[3][1] = v2;

  glUniform4f(uniColLoc, (col[0] / 255.0f), (col[1] / 255.0f), (col[2] / 255.0f),
              (col[3] / 255.0f));
  glUniform1f(depthLoc, -1.0f);
  glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_BYTE, idx);

  glDisableVertexAttribArray(posLoc);
  glDisableVertexAttribArray(tex0Loc);

  m_context.DisableGUIShader();

#endif
}
