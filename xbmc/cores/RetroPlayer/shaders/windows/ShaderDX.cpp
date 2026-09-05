/*
 *  Copyright (C) 2017-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ShaderDX.h"

#include "ShaderTextureDX.h"
#include "ShaderTextureDXRef.h"
#include "ShaderTypesDX.h"
#include "ShaderUtilsDX.h"
#include "application/Application.h"
#include "cores/RetroPlayer/rendering/RenderContext.h"
#include "cores/RetroPlayer/shaders/IShaderLut.h"
#include "cores/RetroPlayer/shaders/ShaderUtils.h"
#include "guilib/TextureDX.h"
#include "rendering/dx/RenderSystemDX.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

using namespace KODI::SHADER;

CShaderDX::CShaderDX() = default;

CShaderDX::~CShaderDX()
{
  if (m_pInputBuffer != nullptr)
    m_pInputBuffer->Release();
}

bool CShaderDX::Create(unsigned int passIdx,
                       std::string passAlias,
                       std::string shaderPath,
                       std::string shaderSource,
                       ShaderParameterMap shaderParameters,
                       std::vector<std::shared_ptr<IShaderLut>> luts,
                       unsigned int frameCountMod)
{
  if (shaderPath.empty())
  {
    CLog::Log(LOGERROR, "CShaderDX::Create: Can't load empty shader path");
    return false;
  }

  m_passIdx = passIdx;
  m_passAlias = std::move(passAlias);
  m_shaderPath = std::move(shaderPath);
  m_shaderSource = std::move(shaderSource);
  m_shaderParameters = std::move(shaderParameters);
  m_luts = std::move(luts);
  m_frameCountMod = frameCountMod;
  //m_pSampler = reinterpret_cast<ID3D11SamplerState*>(sampler);

  DefinesMap defines;

  defines["HLSL_4"] = ""; // Using Shader Model 4
  defines["HLSL_FX"] = ""; // And the FX11 framework

  // We implement runtime shader parameters ("#pragma parameter")
  // @note Runtime shader parameters allow convenient experimentation with real-time
  //       feedback, as well as override-ability by presets, but sometimes they are
  //       much slower because they prevent static evaluation of a lot of math.
  //       Disabling them drastically speeds up shaders that use them heavily.
  defines["PARAMETER_UNIFORM"] = "";

  m_effect.AddIncludePath(URIUtils::GetBasePath(m_shaderPath));

  if (!m_effect.Create(m_shaderSource, &defines))
  {
    CLog::Log(LOGERROR, "CShaderDX::Create: Failed to load video shader: {}", m_shaderPath);
    return false;
  }

  if (!m_effect.SetTechnique("TEQ"))
  {
    CLog::Log(LOGERROR,
              "CShaderDX::Create: Shader has no valid FX11 technique: pass={}, alias={}, "
              "shader={}, technique=TEQ",
              m_passIdx, m_passAlias, m_shaderPath);
    return false;
  }

  return true;
}

void CShaderDX::Render(IShaderTexture& sourceTexture, IShaderTexture& targetTexture)
{
  auto& sourceDX = static_cast<CShaderTextureDX&>(sourceTexture);
  auto& targetDX = static_cast<CShaderTextureDX&>(targetTexture);

  // Get source texture object
  const CD3DTexture& sourceD3DTexture = sourceDX.GetTexture();

  // Get target texture object
  CD3DTexture& targetD3DTexture = targetDX.GetTexture();

  SetShaderParameters(sourceD3DTexture);
  Execute({&targetD3DTexture}, 4);
}

void CShaderDX::SetSizes(const float2& nextSize,
                         const float2& prevSize,
                         const float2& prevTextureSize)
{
  m_outputSize = nextSize;

  if (prevSize.x > 0 && prevSize.y > 0)
    m_inputSize = prevSize;

  if (prevTextureSize.x > 0 && prevTextureSize.y > 0)
    m_inputTextureSize = prevTextureSize;
}

void CShaderDX::PrepareParameters(
    IShaderTexture& sourceTexture,
    const std::vector<std::unique_ptr<IShaderTexture>>& pShaderTextures,
    const std::vector<std::unique_ptr<IShader>>& pShaders,
    uint64_t frameCount)
{
  // Set destination rectangle size
  m_destSize = m_outputSize;

  CUSTOMVERTEX* v;
  LockVertexBuffer(reinterpret_cast<void**>(&v));

  // top left
  v[0].x = -m_outputSize.x / 2;
  v[0].y = -m_outputSize.y / 2;
  // top right
  v[1].x = m_outputSize.x / 2;
  v[1].y = -m_outputSize.y / 2;
  // bottom right
  v[2].x = m_outputSize.x / 2;
  v[2].y = m_outputSize.y / 2;
  // bottom left
  v[3].x = -m_outputSize.x / 2;
  v[3].y = m_outputSize.y / 2;

  // top left
  v[0].z = 0;
  v[0].tu = 0;
  v[0].tv = 0;
  v[0].tu2 = 0.0f;
  v[0].tv2 = 0.0f;
  // top right
  v[1].z = 0;
  v[1].tu = 1;
  v[1].tv = 0;
  v[1].tu2 = 1.0f;
  v[1].tv2 = 0.0f;
  // bottom right
  v[2].z = 0;
  v[2].tu = 1;
  v[2].tv = 1;
  v[2].tu2 = 1.0f;
  v[2].tv2 = 1.0f;
  // bottom left
  v[3].z = 0;
  v[3].tu = 0;
  v[3].tv = 1;
  v[3].tu2 = 0.0f;
  v[3].tv2 = 1.0f;

  UnlockVertexBuffer();
  UpdateInputBuffer(frameCount);
}

void CShaderDX::UpdateMVP()
{
  const float xScale = 1.0f / m_outputSize.x * 2.0f;
  const float yScale = -1.0f / m_outputSize.y * 2.0f;

  // Update projection matrix
  m_MVP = DirectX::XMFLOAT4X4(xScale, 0, 0, 0, 0, yScale, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
}

bool CShaderDX::CreateVertexBuffer(unsigned int vertCount, unsigned int vertSize)
{
  return CRPWinShader::CreateVertexBuffer(vertCount, vertSize);
}

bool CShaderDX::CreateInputLayout(D3D11_INPUT_ELEMENT_DESC* layout, unsigned int numElements)
{
  return CRPWinShader::CreateInputLayout(layout, numElements, "TEQ");
}

bool CShaderDX::CreateInputBuffer()
{
  ID3D11Device1* pDevice = DX::DeviceResources::Get()->GetD3DDevice();
  cbInput inputInitData = GetInputData();
  UINT inputBufSize = static_cast<UINT>((sizeof(cbInput) + 15) & ~15);
  CD3D11_BUFFER_DESC cbInputDesc(inputBufSize, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC,
                                 D3D11_CPU_ACCESS_WRITE);
  D3D11_SUBRESOURCE_DATA initInputSubresource = {&inputInitData, 0, 0};

  if (FAILED(pDevice->CreateBuffer(&cbInputDesc, &initInputSubresource, &m_pInputBuffer)))
  {
    CLog::Log(LOGERROR, "CShaderDX::CreateInputBuffer: Failed to create constant buffer for video "
                        "shader input data");
    return false;
  }

  return true;
}

void CShaderDX::UpdateInputBuffer(uint64_t frameCount)
{
  ID3D11DeviceContext1* pContext = DX::DeviceResources::Get()->GetD3DContext();
  cbInput input = GetInputData(frameCount);
  cbInput* pData;
  void** ppData = reinterpret_cast<void**>(&pData);
  D3D11_MAPPED_SUBRESOURCE resource;

  if (SUCCEEDED(pContext->Map(m_pInputBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource)))
  {
    *ppData = resource.pData;
    memcpy(*ppData, &input, sizeof(cbInput));
    pContext->Unmap(m_pInputBuffer, 0);
  }
}

CShaderDX::cbInput CShaderDX::GetInputData(uint64_t frameCount) const
{
  if (m_frameCountMod != 0)
    frameCount %= m_frameCountMod;

  cbInput input = {
      {CShaderUtilsDX::ToDXVector(m_inputSize)}, // video_size
      {CShaderUtilsDX::ToDXVector(m_inputTextureSize)}, // texture_size
      {CShaderUtilsDX::ToDXVector(m_destSize)}, // output_size
      // Current frame count that can be modulo'ed
      static_cast<float>(frameCount), // frame_count
      // Time always flows forward
      1.0f // frame_direction
  };
  return input;
}

void CShaderDX::SetShaderParameters(const CD3DTexture& sourceTexture)
{
  m_effect.SetTechnique("TEQ");
  m_effect.SetResources("decal", {const_cast<CD3DTexture&>(sourceTexture).GetAddressOfSRV()}, 1);
  m_effect.SetMatrix("modelViewProj", reinterpret_cast<const float*>(&m_MVP));
  //! @todo(optimization) Add frame_count to separate cbuffer
  m_effect.SetConstantBuffer("input", m_pInputBuffer);

  for (const auto& [paramName, paramValue] : m_shaderParameters)
    m_effect.SetFloatArray(paramName.c_str(), &paramValue, 1);

  for (const std::shared_ptr<IShaderLut>& lut : m_luts)
  {
    auto* texture = dynamic_cast<CDXTexture*>(lut->GetTexture());
    if (texture != nullptr)
      m_effect.SetTexture(lut->GetID().c_str(), texture->GetShaderResource());
  }
}
