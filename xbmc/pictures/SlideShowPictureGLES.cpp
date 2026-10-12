/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SlideShowPictureGLES.h"

#include "ServiceBroker.h"
#include "guilib/Texture.h"
#include "rendering/gles/RenderSystemGLES.h"
#include "utils/GLUtils.h"
#include "windowing/WinSystem.h"

std::unique_ptr<CSlideShowPic> CSlideShowPic::CreateSlideShowPicture()
{
  return std::make_unique<CSlideShowPicGLES>();
}

void CSlideShowPicGLES::Render(float* x,
                               float* y,
                               CTexture* pTexture,
                               KODI::UTILS::COLOR::Color color)
{
  CRenderSystemGLES* renderSystem =
      dynamic_cast<CRenderSystemGLES*>(CServiceBroker::GetRenderSystem());
  if (pTexture)
  {
    pTexture->LoadToGPU();
    pTexture->BindToUnit(0);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND); // Turn Blending On

    renderSystem->EnableGUIShader(ShaderMethodGLES::SM_TEXTURE);
  }
  else
  {
    renderSystem->EnableGUIShader(ShaderMethodGLES::SM_DEFAULT);
  }

  float u1 = 0, u2 = 1, v1 = 0, v2 = 1;
  if (pTexture)
  {
    u2 = (float)pTexture->GetWidth() / pTexture->GetTextureWidth();
    v2 = (float)pTexture->GetHeight() / pTexture->GetTextureHeight();
  }

  GLubyte col[4];

  GLint uniColLoc = renderSystem->GUIShaderGetUniCol();
  GLint depthLoc = renderSystem->GUIShaderGetDepth();

  // Setup Colour values
  col[0] = KODI::UTILS::GL::GetChannelFromARGB(KODI::UTILS::GL::ColorChannel::R, color);
  col[1] = KODI::UTILS::GL::GetChannelFromARGB(KODI::UTILS::GL::ColorChannel::G, color);
  col[2] = KODI::UTILS::GL::GetChannelFromARGB(KODI::UTILS::GL::ColorChannel::B, color);
  col[3] = KODI::UTILS::GL::GetChannelFromARGB(KODI::UTILS::GL::ColorChannel::A, color);

  if (CServiceBroker::GetWinSystem()->UseLimitedColor())
  {
    col[0] = (235 - 16) * col[0] / 255 + 16;
    col[1] = (235 - 16) * col[1] / 255 + 16;
    col[2] = (235 - 16) * col[2] / 255 + 16;
  }

  glUniform4f(uniColLoc, (col[0] / 255.0f), (col[1] / 255.0f), (col[2] / 255.0f),
              (col[3] / 255.0f));
  glUniform1f(depthLoc, -1.0f);

  // The corners are always a transformed rectangle, so three of them define it.
  const CRect texCoords(u1, v1, u2, v2);
  renderSystem->DrawGUIQuad({x[0], y[0]}, {x[1], y[1]}, {x[3], y[3]}, &texCoords);

  renderSystem->DisableGUIShader();
}
