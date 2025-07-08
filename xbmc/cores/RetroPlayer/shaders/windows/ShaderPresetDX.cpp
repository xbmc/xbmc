/*
 *  Copyright (C) 2017-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ShaderPresetDX.h"

#include "cores/RetroPlayer/rendering/RenderContext.h"
#include "cores/RetroPlayer/shaders/windows/ShaderDX.h"
#include "cores/RetroPlayer/shaders/windows/ShaderLutDX.h"
#include "cores/RetroPlayer/shaders/windows/ShaderTextureDX.h"
#include "cores/RetroPlayer/shaders/windows/ShaderTypesDX.h"
#include "rendering/dx/RenderSystemDX.h"
#include "utils/log.h"

#include <regex>

using namespace KODI::SHADER;

CShaderPresetDX::CShaderPresetDX(RETRO::CRenderContext& context,
                                 unsigned videoWidth,
                                 unsigned videoHeight)
  : CShaderPreset(context, videoWidth, videoHeight)
{
}

bool CShaderPresetDX::CreateShaders()
{
  const auto numPasses = static_cast<unsigned int>(m_passes.size());

  //! @todo Is this pass specific?
  for (unsigned int shaderIdx = 0; shaderIdx < numPasses; ++shaderIdx)
  {
    std::vector<std::shared_ptr<IShaderLut>> passLUTsDX;

    const ShaderPass& pass = m_passes[shaderIdx];
    const auto numPassLuts = static_cast<unsigned int>(pass.luts.size());

    for (unsigned int i = 0; i < numPassLuts; ++i)
    {
      const ShaderLut& lutStruct = pass.luts[i];

      auto passLut = std::make_shared<CShaderLutDX>(lutStruct.strId, lutStruct.path);
      if (passLut->Create(lutStruct))
        passLUTsDX.emplace_back(std::move(passLut));
    }

    // Create the shader
    auto videoShader = std::make_unique<CShaderDX>();

    const std::string& shaderSource = pass.vertexSource; // Also contains fragment source
    const std::string& shaderPath = pass.sourcePath;

    // Get only the parameters belonging to this specific shader
    ShaderParameterMap passParameters = GetShaderParameters(pass.parameters, pass.vertexSource);

    if (!videoShader->Create(shaderSource, shaderPath, std::move(passParameters),
                             std::move(passLUTsDX), shaderIdx, pass.frameCountMod))
    {
      CLog::Log(LOGERROR, "CShaderPresetDX::CreateShaders: Couldn't create a video shader");
      return false;
    }
    m_pShaders.push_back(std::move(videoShader));

    /*
    IShaderSampler* passSampler = reinterpret_cast<IShaderSampler*>(
        pass.filter == FILTER_TYPE_LINEAR
            ? m_pSampLinear
            : m_pSampNearest); //! @todo Wrap in CShaderSamplerDX instead of reinterpret_cast

    //! @todo Set passSampler to m_pSampler in the videoShader
    videoShader->SetSampler(passSampler);
    */
  }

  return true;
}

bool CShaderPresetDX::CreateLayouts()
{
  for (std::unique_ptr<IShader>& videoShader : m_pShaders)
  {
    auto* videoShaderDX = static_cast<CShaderDX*>(videoShader.get());
    videoShaderDX->CreateVertexBuffer(4, sizeof(CUSTOMVERTEX));

    // Create input layout
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        {"SV_POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0}};

    if (!videoShaderDX->CreateInputLayout(layout, ARRAYSIZE(layout)))
    {
      CLog::Log(
          LOGERROR,
          "CShaderPresetDX::CreateLayouts: Failed to create input layout for Input Assembler");
      return false;
    }
  }

  return true;
}

bool CShaderPresetDX::CreateBuffers()
{
  for (std::unique_ptr<IShader>& videoShader : m_pShaders)
  {
    auto* videoShaderDX = static_cast<CShaderDX*>(videoShader.get());
    videoShaderDX->CreateInputBuffer();
  }

  return true;
}

bool CShaderPresetDX::CreateShaderTextures()
{
  m_pShaderTextures.clear();

  float2 prevSize = m_videoSize;
  float2 prevTextureSize = m_videoSize;

  const auto numPasses = static_cast<unsigned int>(m_passes.size());

  for (unsigned int shaderIdx = 0; shaderIdx < numPasses; ++shaderIdx)
  {
    const auto& pass = m_passes[shaderIdx];

    // Resolve final texture resolution, taking scale type and scale multiplier into account
    float2 scaledSize;
    float2 textureSize;
    CalculateScaledSize(pass, prevSize, scaledSize);

    if (shaderIdx + 1 == numPasses)
    {
      // We're supposed to output at full (viewport) resolution
      scaledSize.x = m_outputSize.x;
      scaledSize.y = m_outputSize.y;
    }
    else
    {
      // Determine the framebuffer data format
      DXGI_FORMAT textureFormat;
      if (pass.fbo.floatFramebuffer)
      {
        // Give priority to float framebuffer parameter (we can't use both float and sRGB)
        textureFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
      }
      else
      {
        if (pass.fbo.sRgbFramebuffer)
          textureFormat = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
        else
          textureFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
      }

      //! @todo Enable usage of optimal texture sizes once multi-pass preset
      // geometry and LUT rendering are fixed.
      //
      // Current issues:
      //   - Enabling optimal texture sizes breaks geometry for many multi-pass
      //     presets
      //   - LUTs render incorrectly due to missing per-pass and per-LUT
      //     TexCoord attributes.
      //
      // Planned solution:
      //   - Implement additional TexCoord attributes for each pass and LUT,
      //     setting coordinates to `xamt` and `yamt` instead of 1
      //
      // Reference implementation in RetroArch:
      //   https://github.com/libretro/RetroArch/blob/09a59edd6b415b7bd124b03bda68ccc4d60b0ea8/gfx/drivers/gl2.c#L3018
      //
      textureSize = scaledSize; // CShaderUtils::GetOptimalTextureSize(scaledSize)

      auto textureDX = std::make_shared<CD3DTexture>();

      if (!textureDX->Create(static_cast<UINT>(textureSize.x), static_cast<UINT>(textureSize.y), 1,
                             D3D11_USAGE_DEFAULT, textureFormat, nullptr, 0))
      {
        CLog::Log(
            LOGERROR,
            "CShaderPresetDX::CreateShaderTextures: Couldn't create texture for video shader: {}",
            pass.sourcePath);
        return false;
      }

      m_pShaderTextures.emplace_back(std::make_unique<CShaderTextureDX>(std::move(textureDX)));
    }

    // Notify shader of its source and dest size
    m_pShaders[shaderIdx]->SetSizes(prevSize, prevTextureSize, scaledSize);

    prevSize = scaledSize;
    prevTextureSize = textureSize;
  }

  UpdateMVPs();
  return true;
}

bool CShaderPresetDX::CreateSamplers()
{
  // Describe the Sampler States
  // As specified in the common-shaders spec
  D3D11_SAMPLER_DESC sampDesc;
  ZeroMemory(&sampDesc, sizeof(D3D11_SAMPLER_DESC));
  sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
  sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
  sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
  sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
  sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
  sampDesc.MinLOD = 0;
  sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
  FLOAT blackBorder[4] = {1, 0, 0, 1}; //! @todo Turn this back to black
  memcpy(sampDesc.BorderColor, &blackBorder, 4 * sizeof(FLOAT));

  ID3D11Device1* pDevice = DX::DeviceResources::Get()->GetD3DDevice();

  if (FAILED(pDevice->CreateSamplerState(&sampDesc, &m_pSampNearest)))
    return false;

  D3D11_SAMPLER_DESC sampDescLinear = sampDesc;
  sampDescLinear.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
  if (FAILED(pDevice->CreateSamplerState(&sampDescLinear, &m_pSampLinear)))
    return false;

  return true;
}

void CShaderPresetDX::RenderShader(IShader& shader, IShaderTexture& source, IShaderTexture& target)
{
  const CRect newViewPort(0.f, 0.f, target.GetWidth(), target.GetHeight());
  m_context.SetViewPort(newViewPort);
  m_context.SetScissors(newViewPort);

  shader.Render(source, target);
}
