/*
 *  Copyright (C) 2017-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/RetroPlayer/shaders/IShaderPreset.h"
#include "cores/RetroPlayer/shaders/ShaderTypes.h"
#include "utils/Geometry.h"

#include <memory>
#include <set>
#include <string>
#include <vector>

namespace KODI
{
namespace RETRO
{
class CRenderContext;
}

namespace SHADER
{
class IShader;
class IShaderTexture;

class CShaderPreset : public IShaderPreset
{
public:
  // Instance of CShaderPreset
  explicit CShaderPreset(RETRO::CRenderContext& context,
                         unsigned videoWidth = 0,
                         unsigned videoHeight = 0);
  ~CShaderPreset() override;

  // Implementation of IShaderPreset
  bool ReadPresetFile(const std::string& presetPath) override;
  bool RenderUpdate(const RETRO::ViewportCoordinates& dest,
                    const float2 fullDestSize,
                    IShaderTexture& source,
                    IShaderTexture& target) override;
  void SetSpeed(double speed) override { m_speed = speed; }
  void SetVideoSize(unsigned int videoWidth, unsigned int videoHeight) override;
  bool SetShaderPreset(const std::string& shaderPresetPath) override;
  const std::string& GetShaderPreset() const override;
  std::vector<ShaderPass>& GetPasses() override { return m_passes; }

protected:
  // Shader interface
  virtual bool CreateShaders() = 0;
  virtual bool CreateLayouts() = 0;
  virtual bool CreateBuffers() = 0;
  virtual bool CreateShaderTextures() = 0;
  virtual bool CreateSamplers() = 0;
  virtual void RenderShader(IShader& shader, IShaderTexture& source, IShaderTexture& target) = 0;

  // Helper functions
  bool Update();
  void UpdateViewPort(CRect viewPort, const float2 fullDestSize);
  void UpdateMVPs();
  void PrepareParameters(const RETRO::ViewportCoordinates& dest, IShaderTexture& source);
  void CalculateScaledSize(const KODI::SHADER::ShaderPass& pass,
                           const float2& prevSize,
                           float2& scaledSize);
  void DisposeShaders();
  void DisposeShaderTextures();
  bool HasPathFailed(const std::string& path) const;
  ShaderParameterMap GetShaderParameters(const std::vector<ShaderParameter>& parameters,
                                         const std::string& sourceStr) const;

  // Construction parameters
  RETRO::CRenderContext& m_context;

  // Relative path of the currently loaded shader preset
  // If empty, it means that a preset is not currently loaded
  std::string m_presetPath;

  // Set of paths of presets that are known to not load correctly
  // Should not contain "" (empty path) because this signifies that a preset is not loaded
  std::set<std::string, std::less<>> m_failedPaths;

  // All video shader passes of the currently loaded preset
  std::vector<ShaderPass> m_passes;

  // Video shaders for the shader passes
  std::vector<std::unique_ptr<IShader>> m_pShaders;

  // Intermediate textures used for pixel shader passes
  std::vector<std::unique_ptr<IShaderTexture>> m_pShaderTextures;

  // Was the shader preset changed during the last frame?
  bool m_bPresetNeedsUpdate = true;

  // Resolution of the output viewport
  float2 m_outputSize;

  // Resolution of the destination rectangle for the fullscreen game window
  float2 m_fullDestSize;

  // Size of the actual source video data (ie. 160x144 for the Game Boy)
  float2 m_videoSize;

  // Number of frames that have passed
  float m_frameCount{0.0f};

  // Playback speed
  double m_speed{1.0};
};

} // namespace SHADER
} // namespace KODI
