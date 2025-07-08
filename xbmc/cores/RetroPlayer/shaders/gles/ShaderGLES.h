/*
 *  Copyright (C) 2019-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "ShaderTextureGLES.h"
#include "cores/RetroPlayer/shaders/IShader.h"
#include "guilib/TextureGLES.h"
#include "rendering/gl/GLShader.h"

#include <array>
#include <stdint.h>

namespace KODI::SHADER
{
class CShaderGLES : public IShader
{
public:
  CShaderGLES();
  ~CShaderGLES() override;

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

private:
  struct UniformInputs
  {
    float2 video_size;
    float2 texture_size;
    float2 output_size;
    GLint frame_count;
    GLfloat frame_direction;
  };

  struct UniformFrameInputs
  {
    float2 input_size;
    float2 texture_size;
    GLuint texture;
  };

  void UpdateUniformInputs(IShaderTexture& sourceTexture,
                           const std::vector<std::unique_ptr<IShaderTexture>>& pShaderTextures,
                           const std::vector<std::unique_ptr<IShader>>& pShaders,
                           uint64_t frameCount);
  UniformInputs GetInputData(uint64_t frameCount = 0);
  UniformFrameInputs GetFrameInputData(GLuint texture);
  UniformFrameInputs GetFrameUniformInputs() const { return m_uniformFrameInputs; }
  void GetUniformLocs();
  void SetShaderParameters(CGLESTexture& sourceTexture);

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
  std::array<std::array<GLfloat, 4>, 4> m_MVP;

  // Index of the video shader pass
  unsigned int m_passIdx{0};

  // Value to modulo (%) frame count with
  // Unused if 0
  unsigned int m_frameCountMod{0};

  GLuint m_shaderProgram{0};
  std::array<GLubyte, 4> m_indices;
  std::array<std::array<float, 3>, 4> m_VertexCoords;
  std::array<std::array<float, 3>, 4> m_colors;
  std::array<std::array<float, 2>, 4> m_TexCoords;

  UniformInputs m_uniformInputs;
  UniformFrameInputs m_uniformFrameInputs;
  std::vector<UniformFrameInputs> m_passesUniformFrameInputs;

  GLint m_FrameDirectionLoc{-1};
  GLint m_FrameCountLoc{-1};
  GLint m_OutputSizeLoc{-1};
  GLint m_TextureSizeLoc{-1};
  GLint m_InputSizeLoc{-1};
  GLint m_MVPMatrixLoc{-1};

  std::array<GLuint, 3> m_shaderVertexVBO{GL_NONE};
  GLuint m_shaderIndexVBO{GL_NONE};
};
} // namespace KODI::SHADER
