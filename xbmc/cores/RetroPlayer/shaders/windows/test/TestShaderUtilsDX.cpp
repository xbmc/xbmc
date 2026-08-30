/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/RetroPlayer/shaders/windows/ShaderUtilsDX.h"

#include <cstring>
#include <string>

#include <d3d11.h>
#include <d3dx11effect.h>
#include <gtest/gtest.h>
#include <windows.h>
#include <wrl/client.h>

using namespace KODI::SHADER;

namespace
{
Microsoft::WRL::ComPtr<ID3D11Device> CreateWarpDevice()
{
  Microsoft::WRL::ComPtr<ID3D11Device> device;
  const D3D_FEATURE_LEVEL featureLevels[] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1,
                                             D3D_FEATURE_LEVEL_10_0};
  D3D_FEATURE_LEVEL featureLevel{};

  const HRESULT result = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, featureLevels,
                                           ARRAYSIZE(featureLevels), D3D11_SDK_VERSION,
                                           device.GetAddressOf(), &featureLevel, nullptr);
  EXPECT_TRUE(SUCCEEDED(result));
  return device;
}

Microsoft::WRL::ComPtr<ID3DX11Effect> CompileEffect(ID3D11Device* device, const char* source)
{
  Microsoft::WRL::ComPtr<ID3DX11Effect> effect;
  Microsoft::WRL::ComPtr<ID3DBlob> errors;
  const HRESULT result =
      D3DX11CompileEffectFromMemory(source, strlen(source), "TestShaderUtilsDX", nullptr, nullptr,
                                    0, 0, device, effect.GetAddressOf(), errors.GetAddressOf());

  std::string errorText;
  if (errors)
  {
    errorText.assign(static_cast<const char*>(errors->GetBufferPointer()), errors->GetBufferSize());
  }
  EXPECT_TRUE(SUCCEEDED(result)) << errorText;
  return effect;
}
} // namespace

TEST(TestShaderUtilsDX, RejectsEffectWithoutNamedTechnique)
{
  constexpr const char* source = R"(
float4 main_vertex(float4 position : POSITION) : SV_POSITION { return position; }
float4 main_fragment() : SV_Target { return float4(1.0, 1.0, 1.0, 1.0); }
)";

  const auto device = CreateWarpDevice();
  ASSERT_NE(nullptr, device);
  const auto effect = CompileEffect(device.Get(), source);
  ASSERT_NE(nullptr, effect);

  D3DX11_PASS_DESC passDesc{};
  EXPECT_FALSE(GetShaderPassDescription(effect.Get(), "TEQ", 0, passDesc));
}

TEST(TestShaderUtilsDX, ResolvesNamedTechniqueInsteadOfFirstTechnique)
{
  constexpr const char* source = R"(
struct ExpectedInput
{
  float3 position : SV_POSITION;
  float2 texCoord : TEXCOORD0;
  float2 texCoord2 : TEXCOORD1;
};

float4 other_vertex(float4 position : POSITION) : SV_POSITION { return position; }
float4 expected_vertex(ExpectedInput input) : SV_POSITION { return float4(input.position, 1.0); }
float4 main_fragment() : SV_Target { return float4(1.0, 1.0, 1.0, 1.0); }

technique11 OTHER
{
  pass P0
  {
    SetVertexShader(CompileShader(vs_4_0, other_vertex()));
    SetPixelShader(CompileShader(ps_4_0, main_fragment()));
  }
}

technique11 TEQ
{
  pass P0
  {
    SetVertexShader(CompileShader(vs_4_0, expected_vertex()));
    SetPixelShader(CompileShader(ps_4_0, main_fragment()));
  }
}
)";

  const auto device = CreateWarpDevice();
  ASSERT_NE(nullptr, device);
  const auto effect = CompileEffect(device.Get(), source);
  ASSERT_NE(nullptr, effect);

  D3DX11_PASS_DESC passDesc{};
  ASSERT_TRUE(GetShaderPassDescription(effect.Get(), "TEQ", 0, passDesc));

  const D3D11_INPUT_ELEMENT_DESC layout[] = {
      {"SV_POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
      {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
      {"TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0},
  };
  Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout;
  EXPECT_TRUE(SUCCEEDED(
      device->CreateInputLayout(layout, ARRAYSIZE(layout), passDesc.pIAInputSignature,
                                passDesc.IAInputSignatureSize, inputLayout.GetAddressOf())));
}
