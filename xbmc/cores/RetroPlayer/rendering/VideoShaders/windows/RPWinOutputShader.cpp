/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
#include "RPWinOutputShader.h"

#include "utils/log.h"

using namespace KODI;
using namespace RETRO;

bool CRPWinOutputShader::Create(SCALINGMETHOD scalingMethod)
{
  CWinShader::CreateVertexBuffer(4, sizeof(CUSTOMVERTEX));

  DefinesMap defines;
  switch (scalingMethod)
  {
    case SCALINGMETHOD::NEAREST:
      defines["SAMP_NEAREST"] = "";
      break;
    case SCALINGMETHOD::LINEAR:
    default:
      break;
  }

  std::string effectPath("special://xbmc/system/shaders/rp_output_d3d.fx");

  if (!LoadEffect(effectPath, &defines))
  {
    CLog::LogF(LOGERROR, "Failed to load shader {}.", effectPath);
    return false;
  }

  // Create input layout
  D3D11_INPUT_ELEMENT_DESC layout[] = {
      {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
      {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
  };
  return CWinShader::CreateInputLayout(layout, ARRAYSIZE(layout));
}

void CRPWinOutputShader::Render(CD3DTexture& sourceTexture,
                                CRect sourceRect,
                                const CPoint points[4],
                                CRect& viewPort,
                                CD3DTexture* target,
                                unsigned range)
{
  PrepareParameters(sourceTexture.GetWidth(), sourceTexture.GetHeight(), sourceRect, points);
  SetShaderParameters(sourceTexture, range, viewPort);
  Execute({target}, 4);
}

void CRPWinOutputShader::PrepareParameters(unsigned sourceWidth,
                                           unsigned sourceHeight,
                                           CRect sourceRect,
                                           const CPoint points[4])
{
  bool changed = false;
  for (int i = 0; i < 4 && !changed; ++i)
    changed = points[i] != m_destPoints[i];

  if (m_sourceWidth != sourceWidth || m_sourceHeight != sourceHeight ||
      m_sourceRect != sourceRect || changed)
  {
    m_sourceWidth = sourceWidth;
    m_sourceHeight = sourceHeight;
    m_sourceRect = sourceRect;

    for (int i = 0; i < 4; ++i)
      m_destPoints[i] = points[i];

    CUSTOMVERTEX* v = nullptr;
    CWinShader::LockVertexBuffer(static_cast<void**>(static_cast<void*>(&v)));

    v[0].x = m_destPoints[0].x;
    v[0].y = m_destPoints[0].y;
    v[0].z = 0.0f;
    v[0].tu = m_sourceRect.x1 / m_sourceWidth;
    v[0].tv = m_sourceRect.y1 / m_sourceHeight;

    v[1].x = m_destPoints[1].x;
    v[1].y = m_destPoints[1].y;
    v[1].z = 0.0f;
    v[1].tu = m_sourceRect.x2 / m_sourceWidth;
    v[1].tv = m_sourceRect.y1 / m_sourceHeight;

    v[2].x = m_destPoints[2].x;
    v[2].y = m_destPoints[2].y;
    v[2].z = 0.0f;
    v[2].tu = m_sourceRect.x2 / m_sourceWidth;
    v[2].tv = m_sourceRect.y2 / m_sourceHeight;

    v[3].x = m_destPoints[3].x;
    v[3].y = m_destPoints[3].y;
    v[3].z = 0.0f;
    v[3].tu = m_sourceRect.x1 / m_sourceWidth;
    v[3].tv = m_sourceRect.y2 / m_sourceHeight;

    CWinShader::UnlockVertexBuffer();
  }
}

void CRPWinOutputShader::SetShaderParameters(CD3DTexture& sourceTexture,
                                             unsigned range,
                                             CRect& viewPort)
{
  m_effect.SetTechnique("OUTPUT_T");
  m_effect.SetResources("g_Texture", sourceTexture.GetAddressOfSRV(), 1);

  float viewPortArray[2] = {viewPort.Width(), viewPort.Height()};
  m_effect.SetFloatArray("g_viewPort", viewPortArray, 2);

  float params[3] = {static_cast<float>(range)};
  m_effect.SetFloatArray("m_params", params, 1);
}
