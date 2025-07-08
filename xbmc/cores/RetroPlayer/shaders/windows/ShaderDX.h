/*
 *  Copyright (C) 2017-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "ShaderTextureDX.h"
#include "ShaderTypesDX.h"
#include "cores/RetroPlayer/shaders/IShader.h"
#include "cores/RetroPlayer/shaders/windows/RPWinOutputShader.h"
#include "guilib/D3DResource.h"

#include <stdint.h>

#include <DirectXMath.h>

namespace KODI::SHADER
{
class IShaderLut;
class IShaderSampler;

class CShaderDX : protected CRPWinShader, public IShader
{
public:
  CShaderDX();
  ~CShaderDX() override;

  // Implementation of IShader
  bool Create(std::string shaderSource,
              std::string shaderPath,
              ShaderParameterMap shaderParameters,
              std::vector<std::shared_ptr<IShaderLut>> luts,
              unsigned int passIdx,
              unsigned int frameCountMod = 0) override;
  void Render(IShaderTexture& source, IShaderTexture& target) override;
  void SetSizes(const float2& prevSize,
                const float2& prevTextureSize,
                const float2& nextSize) override;
  void PrepareParameters(const RETRO::ViewportCoordinates& dest,
                         const float2 fullDestSize,
                         IShaderTexture& sourceTexture,
                         const std::vector<std::unique_ptr<IShaderTexture>>& pShaderTextures,
                         const std::vector<std::unique_ptr<IShader>>& pShaders,
                         uint64_t frameCount) override;
  void UpdateMVP() override;

  /*!
   * \brief Construct the vertex buffer that will be used to render the shader
   *
   * \param vertCount Number of vertices to construct. Commonly 4, for rectangular screens
   * \param vertSize Size of each vertex's data in bytes
   *
   * \return False if creating the vertex buffer failed, true otherwise
   */
  bool CreateVertexBuffer(unsigned int vertCount, unsigned int vertSize);

  /*!
   * \brief Creates the data layout of the input-assembler stage
   *
   * \param layout Description of the inputs to the vertex shader
   * \param numElements Number of inputs to the vertex shader
   *
   * \return False if creating the input layout failed, true otherwise.
   */
  bool CreateInputLayout(D3D11_INPUT_ELEMENT_DESC* layout, unsigned int numElements);

  /*!
   * \brief Creates the buffer that will be used to send "input" (as per the
   *        spec) data to the shader
   *
   * \return False if creating the input buffer failed, true otherwise
   */
  bool CreateInputBuffer();

private:
  struct cbInput
  {
    DirectX::XMFLOAT2 video_size;
    DirectX::XMFLOAT2 texture_size;
    DirectX::XMFLOAT2 output_size;
    float frame_count;
    float frame_direction;
  };

  void UpdateInputBuffer(uint64_t frameCount);
  cbInput GetInputData(uint64_t frameCount = 0) const;
  void SetShaderParameters(const CD3DTexture& sourceTexture);

  // Currently loaded shader's source code
  std::string m_shaderSource;

  // Currently loaded shader's relative path
  std::string m_shaderPath;

  // Struct with all parameters pertaining to the shader
  ShaderParameterMap m_shaderParameters;

  // Look-up textures pertaining to the shader
  std::vector<std::shared_ptr<IShaderLut>> m_luts; //! @todo Back to DX maybe

  // Resolution of the input of the shader
  float2 m_inputSize;

  // Resolution of the texture that holds the input
  float2 m_inputTextureSize;

  // Resolution of the output viewport of the shader
  float2 m_outputSize;

  // Resolution of the destination rectangle of the shader
  float2 m_destSize;

  // Projection matrix
  DirectX::XMFLOAT4X4 m_MVP{};

  // Index of the video shader pass
  unsigned int m_passIdx{0};

  // Value to modulo (%) frame count with
  // Unused if 0
  unsigned int m_frameCountMod{0};

  // Holds the data bound to the input cbuffer (cbInput here)
  ID3D11Buffer* m_pInputBuffer{nullptr};

  // Sampler state
  //ID3D11SamplerState* m_pSampler{nullptr};
};

} // namespace KODI::SHADER
