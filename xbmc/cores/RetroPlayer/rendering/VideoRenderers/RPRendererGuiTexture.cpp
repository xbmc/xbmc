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

#include "RPRendererGuiTexture.h"
#include "cores/RetroPlayer/process/RenderBufferGuiTexture.h"
#include "cores/RetroPlayer/rendering/RenderContext.h"
#include "cores/RetroPlayer/rendering/RenderVideoSettings.h"

#if defined(HAS_DX)
#include "guilib/GUIShaderDX.h"
#endif

using namespace KODI;
using namespace RETRO;

// --- CRendererFactoryGuiTexture ------------------------------------------------

CRPBaseRenderer *CRendererFactoryGuiTexture::CreateRenderer(const CRenderSettings &settings, CRenderContext &context, std::shared_ptr<IRenderBufferPool> bufferPool)
{
  return new CRPRendererGuiTexture(settings, context, std::move(bufferPool));
}

RenderBufferPoolVector CRendererFactoryGuiTexture::CreateBufferPools()
{
  return {
#if !defined(HAS_DX)
    std::make_shared<CRenderBufferPoolGuiTexture>(VS_SCALINGMETHOD_NEAREST),
#endif
    std::make_shared<CRenderBufferPoolGuiTexture>(VS_SCALINGMETHOD_LINEAR),
  };
}

// --- CRenderBufferPoolGuiTexture -----------------------------------------------

CRenderBufferPoolGuiTexture::CRenderBufferPoolGuiTexture(ESCALINGMETHOD scalingMethod) :
  m_scalingMethod(scalingMethod)
{
}

bool CRenderBufferPoolGuiTexture::IsCompatible(const CRenderVideoSettings &renderSettings) const
{
  if (renderSettings.GetScalingMethod() != m_scalingMethod)
    return false;

  return true;
}

IRenderBuffer *CRenderBufferPoolGuiTexture::CreateRenderBuffer(void *header /* = nullptr */)
{
  return new CRenderBufferGuiTexture(m_scalingMethod);
}

// --- CRPRendererGuiTexture -----------------------------------------------------

CRPRendererGuiTexture::CRPRendererGuiTexture(const CRenderSettings &renderSettings, CRenderContext &context, std::shared_ptr<IRenderBufferPool> bufferPool) :
  CRPBaseRenderer(renderSettings, context, std::move(bufferPool))
{
}

bool CRPRendererGuiTexture::Supports(ERENDERFEATURE feature) const
{
  if (feature == RENDERFEATURE_STRETCH         ||
      feature == RENDERFEATURE_ZOOM            ||
      feature == RENDERFEATURE_PIXEL_RATIO     ||
      feature == RENDERFEATURE_ROTATION)
  {
    return true;
  }

  return false;
}

void CRPRendererGuiTexture::RenderInternal(bool clear, uint8_t alpha)
{
  CRenderBufferGuiTexture *renderBuffer = static_cast<CRenderBufferGuiTexture*>(m_renderBuffer);

  CRect rect = m_sourceRect;

  rect.x1 /= m_sourceWidth;
  rect.x2 /= m_sourceWidth;
  rect.y1 /= m_sourceHeight;
  rect.y2 /= m_sourceHeight;

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

  CGUIShaderDX *pGUIShader = m_context.GetGUIShader();
  if (pGUIShader != nullptr)
  {
    pGUIShader->Begin(SHADER_METHOD_RENDER_TEXTURE_BLEND);

    // Set state to render the image
    CTexture *dxTexture = renderBuffer->GetTexture();
    ID3D11ShaderResourceView *shaderRes = dxTexture->GetShaderResource();
    pGUIShader->SetShaderViews(1, &shaderRes);
    pGUIShader->DrawQuad(vertex[0], vertex[1], vertex[2], vertex[3]);
  }

#elif defined(HAS_GL)

  // Removed OpenGL rendering due to presence of deprecated code
  (void)renderBuffer;
  (void)u1;
  (void)u2;
  (void)v1;
  (void)v2;
  (void)color;

#elif defined(HAS_GLES)

  renderBuffer->BindToUnit(0);

  glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_BLEND); // Turn blending On

  m_context.EnableGUIShader(SM_TEXTURE);

  GLubyte col[4];
  GLfloat ver[4][3];
  GLfloat tex[4][2];
  GLubyte idx[4] = {0, 1, 3, 2}; // Determines order of triangle strip

  GLint posLoc = m_context.GUIShaderGetPos();
  GLint tex0Loc = m_context.GUIShaderGetCoord0();
  GLint uniColLoc = m_context.GUIShaderGetUniCol();

  glVertexAttribPointer(posLoc, 3, GL_FLOAT, 0, 0, ver);
  glVertexAttribPointer(tex0Loc, 2, GL_FLOAT, 0, 0, tex);

  glEnableVertexAttribArray(posLoc);
  glEnableVertexAttribArray(tex0Loc);

  // Setup color values
  col[0] = static_cast<GLubyte>(GET_R(color));
  col[1] = static_cast<GLubyte>(GET_G(color));
  col[2] = static_cast<GLubyte>(GET_B(color));
  col[3] = static_cast<GLubyte>(GET_A(color));

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

  glUniform4f(uniColLoc,(col[0] / 255.0f), (col[1] / 255.0f), (col[2] / 255.0f), (col[3] / 255.0f));
  glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_BYTE, idx);

  glDisableVertexAttribArray(posLoc);
  glDisableVertexAttribArray(tex0Loc);

  m_context.DisableGUIShader();

#endif
}
