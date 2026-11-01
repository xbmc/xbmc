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
#include "rendering/gles/GainMapShaderGLES.h"
#include "rendering/gles/RenderSystemGLES.h"
#include "utils/GLUtils.h"
#include "utils/log.h"
#include "windowing/WinSystem.h"

#include <algorithm>

namespace
{
// log2 of how much brighter than SDR white a typical HDR TV goes (1000 nits over 203 nits). The
// display's real peak isn't known, so assume one that can show most phone pictures' gain maps.
constexpr float DISPLAY_HEADROOM = 2.3f;
} // namespace

std::unique_ptr<CSlideShowPic> CSlideShowPic::CreateSlideShowPicture()
{
  return std::make_unique<CSlideShowPicGLES>();
}

CSlideShowPicGLES::CSlideShowPicGLES() = default;

CSlideShowPicGLES::~CSlideShowPicGLES() = default;

void CSlideShowPicGLES::Free()
{
  m_posVBO.Destroy();
  m_texVBO.Destroy();
  m_IBO.Destroy();
  m_gainMapShader.reset();
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

bool CSlideShowPicGLES::RenderHdr(float* x,
                                  float* y,
                                  CTexture* pTexture,
                                  CTexture* gainMap,
                                  const KODI::PICTURE::GainMapParams& params,
                                  KODI::UTILS::COLOR::Color color)
{
  if (m_gainMapShaderFailed)
    return false;
  if (!pTexture)
    return true;

  if (!m_gainMapShader)
  {
    std::string defines;
    if (CServiceBroker::GetWinSystem()->UseLimitedColor())
      defines += "#define KODI_LIMITED_RANGE 1\n";
    m_gainMapShader = std::make_unique<CGainMapShaderGLES>(defines);
    if (!m_gainMapShader->CompileAndLink())
    {
      CLog::Log(LOGERROR, "CSlideShowPicGLES: failed to compile the gain map shader");
      m_gainMapShader.reset();
      m_gainMapShaderFailed = true;
      return false;
    }
  }

  pTexture->LoadToGPU();
  pTexture->BindToUnit(0);

  const float u2 = static_cast<float>(pTexture->GetWidth()) / pTexture->GetTextureWidth();
  const float v2 = static_cast<float>(pTexture->GetHeight()) / pTexture->GetTextureHeight();

  float weight = 0.0f;
  if (gainMap)
  {
    gainMap->LoadToGPU();
    gainMap->BindToUnit(1);
    m_gainMapShader->SetGainMapScale(
        static_cast<float>(gainMap->GetWidth()) / gainMap->GetTextureWidth() / u2,
        static_cast<float>(gainMap->GetHeight()) / gainMap->GetTextureHeight() / v2);
    weight = std::clamp((DISPLAY_HEADROOM - params.capacityMin) /
                            (params.capacityMax - params.capacityMin),
                        0.0f, 1.0f);
  }
  else
  {
    pTexture->BindToUnit(1);
  }
  m_gainMapShader->SetGainMap(params, weight);
  m_gainMapShader->SetAlpha(
      KODI::UTILS::GL::GetChannelFromARGB(KODI::UTILS::GL::ColorChannel::A, color) / 255.0f);

  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_BLEND);
  m_gainMapShader->Enable();

  GLfloat ver[4][3];
  GLfloat tex[4][2];
  GLubyte idx[4] = {0, 1, 3, 2}; //determines order of triangle strip

  for (int i = 0; i < 4; i++)
  {
    ver[i][0] = x[i];
    ver[i][1] = y[i];
    ver[i][2] = 0.0f;
  }
  tex[0][0] = tex[3][0] = 0.0f;
  tex[0][1] = tex[1][1] = 0.0f;
  tex[1][0] = tex[2][0] = u2;
  tex[2][1] = tex[3][1] = v2;

  const GLint posLoc = m_gainMapShader->GetPosLoc();
  const GLint texLoc = m_gainMapShader->GetTexLoc();

  m_posVBO.SetData(ver, GL_STREAM_DRAW);
  glVertexAttribPointer(posLoc, 3, GL_FLOAT, 0, 0, 0);
  glEnableVertexAttribArray(posLoc);

  m_texVBO.SetData(tex, GL_STREAM_DRAW);
  glVertexAttribPointer(texLoc, 2, GL_FLOAT, 0, 0, 0);
  glEnableVertexAttribArray(texLoc);

  m_IBO.SetDataOnce(idx);
  glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_BYTE, 0);

  glDisableVertexAttribArray(posLoc);
  glDisableVertexAttribArray(texLoc);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

  m_gainMapShader->Disable();
  glActiveTexture(GL_TEXTURE0);
  return true;
}
