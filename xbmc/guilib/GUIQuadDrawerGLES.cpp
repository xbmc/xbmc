/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIQuadDrawerGLES.h"

#include "ServiceBroker.h"
#include "Texture.h"
#include "rendering/gles/RenderSystemGLES.h"
#include "utils/GLUtils.h"

void CGUIQuadDrawerGLES::DrawQuad(const CRect& rect,
                                  KODI::UTILS::COLOR::Color color,
                                  CTexture* texture,
                                  const CRect* texCoords,
                                  float depth,
                                  bool blending)
{
  CRenderSystemGLES* renderSystem =
      dynamic_cast<CRenderSystemGLES*>(CServiceBroker::GetRenderSystem());
  if (texture)
  {
    texture->LoadToGPU();
    texture->BindToUnit(0);
  }

  if (blending)
  {
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
  }
  else
  {
    glDisable(GL_BLEND);
  }

  VerifyGLState();

  GLubyte col[4];
  GLfloat ver[4][3];
  GLfloat tex[4][2];
  GLubyte idx[4] = {0, 1, 3, 2}; // Determines order of triangle strip

  if (texture)
    renderSystem->EnableGUIShader(ShaderMethodGLES::SM_TEXTURE);
  else
    renderSystem->EnableGUIShader(ShaderMethodGLES::SM_DEFAULT);

  GLint posLoc = renderSystem->GUIShaderGetPos();
  GLint tex0Loc = renderSystem->GUIShaderGetCoord0();
  GLint uniColLoc = renderSystem->GUIShaderGetUniCol();
  GLint depthLoc = renderSystem->GUIShaderGetDepth();

  // Setup Colors
  col[0] = KODI::UTILS::GL::GetChannelFromARGB(KODI::UTILS::GL::ColorChannel::R, color);
  col[1] = KODI::UTILS::GL::GetChannelFromARGB(KODI::UTILS::GL::ColorChannel::G, color);
  col[2] = KODI::UTILS::GL::GetChannelFromARGB(KODI::UTILS::GL::ColorChannel::B, color);
  col[3] = KODI::UTILS::GL::GetChannelFromARGB(KODI::UTILS::GL::ColorChannel::A, color);

  glUniform4f(uniColLoc, col[0] / 255.0f, col[1] / 255.0f, col[2] / 255.0f, col[3] / 255.0f);
  glUniform1f(depthLoc, depth);

  ver[0][0] = ver[3][0] = rect.x1;
  ver[0][1] = ver[1][1] = rect.y1;
  ver[1][0] = ver[2][0] = rect.x2;
  ver[2][1] = ver[3][1] = rect.y2;
  ver[0][2] = ver[1][2] = ver[2][2] = ver[3][2] = 0;

  if (texture)
  {
    // Setup texture coordinates
    CRect coords = texCoords ? *texCoords : CRect(0.0f, 0.0f, 1.0f, 1.0f);
    tex[0][0] = tex[3][0] = coords.x1;
    tex[0][1] = tex[1][1] = coords.y1;
    tex[1][0] = tex[2][0] = coords.x2;
    tex[2][1] = tex[3][1] = coords.y2;
  }

  m_posVBO.SetData(ver, GL_STREAM_DRAW);
  glVertexAttribPointer(posLoc, 3, GL_FLOAT, 0, 0, 0);
  glEnableVertexAttribArray(posLoc);
  if (texture)
  {
    m_texVBO.SetData(tex, GL_STREAM_DRAW);
    glVertexAttribPointer(tex0Loc, 2, GL_FLOAT, 0, 0, 0);
    glEnableVertexAttribArray(tex0Loc);
  }
  m_IBO.SetDataOnce(idx);

  glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_BYTE, 0);
  CRenderSystemBase::m_GUIElementCount++;

  glDisableVertexAttribArray(posLoc);
  if (texture)
    glDisableVertexAttribArray(tex0Loc);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

  renderSystem->DisableGUIShader();
}

void CGUIQuadDrawerGLES::Destroy()
{
  m_posVBO.Destroy();
  m_texVBO.Destroy();
  m_IBO.Destroy();
}
