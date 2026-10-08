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

  if (texture)
    renderSystem->EnableGUIShader(ShaderMethodGLES::SM_TEXTURE);
  else
    renderSystem->EnableGUIShader(ShaderMethodGLES::SM_DEFAULT);

  GLint uniColLoc = renderSystem->GUIShaderGetUniCol();
  GLint depthLoc = renderSystem->GUIShaderGetDepth();

  // Setup Colors
  col[0] = KODI::UTILS::GL::GetChannelFromARGB(KODI::UTILS::GL::ColorChannel::R, color);
  col[1] = KODI::UTILS::GL::GetChannelFromARGB(KODI::UTILS::GL::ColorChannel::G, color);
  col[2] = KODI::UTILS::GL::GetChannelFromARGB(KODI::UTILS::GL::ColorChannel::B, color);
  col[3] = KODI::UTILS::GL::GetChannelFromARGB(KODI::UTILS::GL::ColorChannel::A, color);

  glUniform4f(uniColLoc, col[0] / 255.0f, col[1] / 255.0f, col[2] / 255.0f, col[3] / 255.0f);
  glUniform1f(depthLoc, depth);

  if (texture)
  {
    const CRect coords = texCoords ? *texCoords : CRect(0.0f, 0.0f, 1.0f, 1.0f);
    renderSystem->DrawGUIQuad(rect, &coords);
  }
  else
  {
    renderSystem->DrawGUIQuad(rect);
  }
  CRenderSystemBase::m_GUIElementCount++;

  renderSystem->DisableGUIShader();
}
