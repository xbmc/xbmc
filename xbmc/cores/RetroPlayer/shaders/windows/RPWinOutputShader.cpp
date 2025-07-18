/*
 *  Copyright (C) 2017-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RPWinOutputShader.h"

#include "ShaderTypesDX.h"
#include "filesystem/File.h"
#include "rendering/dx/DeviceResources.h"
#include "utils/log.h"

using namespace KODI::SHADER;

bool CRPWinShader::CreateVertexBuffer(unsigned int count, unsigned int size)
{
  if (!m_vb.Create(D3D11_BIND_VERTEX_BUFFER, count, size, DXGI_FORMAT_UNKNOWN, D3D11_USAGE_DYNAMIC))
    return false;

  uint16_t id[4] = {3, 0, 2, 1};
  if (!m_ib.Create(D3D11_BIND_INDEX_BUFFER, ARRAYSIZE(id), sizeof(uint16_t), DXGI_FORMAT_R16_UINT,
                   D3D11_USAGE_IMMUTABLE, id))
    return false;

  m_vbsize = count * size;
  m_vertsize = size;

  return true;
}

bool CRPWinShader::CreateInputLayout(D3D11_INPUT_ELEMENT_DESC* layout, unsigned numElements)
{
  D3DX11_PASS_DESC desc = {};
  if (FAILED(m_effect.Get()->GetTechniqueByIndex(0)->GetPassByIndex(0)->GetDesc(&desc)))
  {
    CLog::LogF(LOGERROR, "Failed to get description");
    return false;
  }

  Microsoft::WRL::ComPtr<ID3D11Device> pDevice = DX::DeviceResources::Get()->GetD3DDevice();
  return SUCCEEDED(pDevice->CreateInputLayout(layout, numElements, desc.pIAInputSignature,
                                              desc.IAInputSignatureSize, &m_inputLayout));
}

bool CRPWinShader::LockVertexBuffer(void** data)
{
  if (!m_vb.Map(data))
  {
    CLog::LogF(LOGERROR, "Failed to lock vertex buffer");
    return false;
  }

  return true;
}

bool CRPWinShader::UnlockVertexBuffer()
{
  if (!m_vb.Unmap())
  {
    CLog::LogF(LOGERROR, "Failed to unlock vertex buffer");
    return false;
  }

  return true;
}

bool CRPWinShader::LoadEffect(const std::string& filename, DefinesMap* defines)
{
  CLog::LogF(LOGDEBUG, "Loading shader: {}", filename);

  XFILE::CFileStream file;
  if (!file.Open(filename))
  {
    CLog::LogF(LOGERROR, "Failed to open file: {}", filename);
    return false;
  }

  std::string pStrEffect;
  getline(file, pStrEffect, '\0');

  if (!m_effect.Create(pStrEffect, defines))
  {
    CLog::LogF(LOGERROR, "{} failed", pStrEffect);
    return false;
  }

  return true;
}

bool CRPWinShader::Execute(const std::vector<CD3DTexture*>& targets, unsigned int vertexIndexStep)
{
  ID3D11DeviceContext* pContext = DX::DeviceResources::Get()->GetD3DContext();
  Microsoft::WRL::ComPtr<ID3D11RenderTargetView> oldRT;

  // The render target will be overridden: save the caller's original RT
  if (!targets.empty())
    pContext->OMGetRenderTargets(1, &oldRT, nullptr);

  unsigned int cPasses;
  if (!m_effect.Begin(&cPasses, 0))
  {
    CLog::LogF(LOGERROR, "Failed to begin D3D effect");
    return false;
  }

  ID3D11Buffer* vertexBuffer = m_vb.Get();
  ID3D11Buffer* indexBuffer = m_ib.Get();
  unsigned int stride = m_vb.GetStride();
  unsigned int offset = 0;
  pContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
  pContext->IASetIndexBuffer(indexBuffer, m_ib.GetFormat(), 0);
  pContext->IASetInputLayout(m_inputLayout.Get());
  pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

  for (unsigned int iPass = 0; iPass < cPasses; ++iPass)
  {
    SetTarget(targets.size() > iPass ? targets.at(iPass) : nullptr);
    SetStepParams(iPass);

    if (!m_effect.BeginPass(iPass))
    {
      CLog::LogF(LOGERROR, "Failed to begin D3D effect pass");
      break;
    }

    pContext->DrawIndexed(4, 0, iPass * vertexIndexStep);

    if (!m_effect.EndPass())
      CLog::LogF(LOGERROR, "Failed to end D3D effect pass");

    CD3DHelper::PSClearShaderResources(pContext);
  }
  if (!m_effect.End())
    CLog::LogF(LOGERROR, "Failed to end D3D effect");

  if (oldRT)
    pContext->OMSetRenderTargets(1, oldRT.GetAddressOf(), nullptr);

  return true;
}

void CRPWinShader::SetTarget(CD3DTexture* target)
{
  m_target = target;
  if (m_target)
  {
    DX::DeviceResources::Get()->GetD3DContext()->OMSetRenderTargets(1, target->GetAddressOfRTV(),
                                                                    nullptr);
  }
}

bool CRPWinOutputShader::Create(RETRO::SCALINGMETHOD scalingMethod)
{
  CreateVertexBuffer(4, sizeof(CUSTOMVERTEX));

  DefinesMap defines;
  switch (scalingMethod)
  {
    case RETRO::SCALINGMETHOD::NEAREST:
      defines["SAMP_NEAREST"] = "";
      break;
    case RETRO::SCALINGMETHOD::LINEAR:
    default:
      break;
  }

  const std::string effectPath("special://xbmc/system/shaders/rp_output_d3d.fx");

  if (!LoadEffect(effectPath, &defines))
  {
    CLog::LogF(LOGERROR, "Failed to load shader: {}", effectPath);
    return false;
  }

  // Create input layout
  D3D11_INPUT_ELEMENT_DESC layout[] = {
      {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
      {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
  };
  return CreateInputLayout(layout, ARRAYSIZE(layout));
}

void CRPWinOutputShader::Render(CD3DTexture& sourceTexture,
                                CRect sourceRect,
                                const KODI::RETRO::ViewportCoordinates& points,
                                CRect& viewPort,
                                CD3DTexture& target,
                                unsigned int range /* = 0 */)
{
  PrepareParameters(sourceTexture.GetWidth(), sourceTexture.GetHeight(), sourceRect, points);
  SetShaderParameters(sourceTexture, range, viewPort);
  Execute({&target}, 4);
}

void CRPWinOutputShader::PrepareParameters(unsigned int sourceWidth,
                                           unsigned int sourceHeight,
                                           CRect sourceRect,
                                           const KODI::RETRO::ViewportCoordinates& points)
{
  bool changed = false;
  for (unsigned int i = 0; i < 4 && !changed; ++i)
    changed = points[i] != m_destPoints[i];

  if (m_sourceWidth != sourceWidth || m_sourceHeight != sourceHeight ||
      m_sourceRect != sourceRect || changed)
  {
    m_sourceWidth = sourceWidth;
    m_sourceHeight = sourceHeight;
    m_sourceRect = sourceRect;

    for (unsigned int i = 0; i < 4; ++i)
      m_destPoints[i] = points[i];

    CUSTOMVERTEX* v = nullptr;
    LockVertexBuffer(static_cast<void**>(static_cast<void*>(&v)));

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

    UnlockVertexBuffer();
  }
}

void CRPWinOutputShader::SetShaderParameters(CD3DTexture& sourceTexture,
                                             unsigned int range,
                                             CRect& viewPort)
{
  m_effect.SetTechnique("OUTPUT_T");
  m_effect.SetResources("g_Texture", sourceTexture.GetAddressOfSRV(), 1);

  const float viewPortArray[2] = {viewPort.Width(), viewPort.Height()};
  m_effect.SetFloatArray("g_viewPort", viewPortArray, 2);

  const float params[3] = {static_cast<float>(range)};
  m_effect.SetFloatArray("m_params", params, 1);
}
