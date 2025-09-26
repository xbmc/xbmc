/*
 *  Copyright (C) 2017-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ShaderPreset.h"

#include "IShader.h"
#include "IShaderTexture.h"
#include "ServiceBroker.h"
#include "ShaderPresetFactory.h"
#include "cores/RetroPlayer/rendering/RenderContext.h"
#include "games/GameServices.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include <regex>

using namespace KODI::SHADER;

CShaderPreset::CShaderPreset(RETRO::CRenderContext& context,
                             unsigned videoWidth,
                             unsigned videoHeight)
  : m_context(context),
    m_videoSize(videoWidth, videoHeight)
{
}

CShaderPreset::~CShaderPreset()
{
  DisposeShaders();
}

bool CShaderPreset::ReadPresetFile(const std::string& presetPath)
{
  return CServiceBroker::GetGameServices().VideoShaders().LoadPreset(presetPath, *this);
}

bool CShaderPreset::RenderUpdate(const RETRO::ViewportCoordinates& dest,
                                 const float2 fullDestSize,
                                 IShaderTexture& source,
                                 IShaderTexture& target)
{
  // Save the viewport
  CRect viewPort;
  m_context.GetViewPort(viewPort);

  // Handle resizing of the viewport (window)
  UpdateViewPort(viewPort, fullDestSize);

  // Update shaders/shader textures if required
  if (!Update())
    return false;

  PrepareParameters(dest, source);

  const auto numPasses = static_cast<unsigned int>(m_pShaders.size());

  // Apply all passes except the last one (which needs to be applied to the backbuffer)
  IShaderTexture* sourceTexture = &source;
  for (unsigned shaderIdx = 0; shaderIdx + 1 < numPasses; ++shaderIdx)
  {
    IShader& shader = *m_pShaders[shaderIdx];
    IShaderTexture& texture = *m_pShaderTextures[shaderIdx];
    RenderShader(shader, *sourceTexture, texture);
    sourceTexture = &texture;
  }

  // Restore our viewport
  m_context.SetViewPort(viewPort);
  m_context.SetScissors(viewPort);

  // Apply the last pass and write to target (backbuffer) instead of the last texture
  IShader* lastShader = m_pShaders.back().get();
  lastShader->Render(*sourceTexture, target);

  m_frameCount += static_cast<float>(m_speed);
  return true;
}

void CShaderPreset::SetVideoSize(unsigned int videoWidth, unsigned int videoHeight)
{
  if (videoWidth != static_cast<unsigned int>(m_videoSize.x) ||
      videoHeight != static_cast<unsigned int>(m_videoSize.y))
  {
    m_videoSize = {videoWidth, videoHeight};
    m_bPresetNeedsUpdate = true;
  }
}

bool CShaderPreset::SetShaderPreset(const std::string& shaderPresetPath)
{
  auto updateFailed = [this](const std::string& msg)
  {
    m_failedPaths.insert(m_presetPath);
    CLog::Log(LOGWARNING, "CShaderPreset::SetShaderPreset: {}", msg);
    DisposeShaders();
    return false;
  };

  m_presetPath = shaderPresetPath;
  m_bPresetNeedsUpdate = true;

  if (m_presetPath.empty())
    // No preset should load, just return false, we shouldn't add "" to the failed paths
    return false;

  if (!ReadPresetFile(m_presetPath))
  {
    CLog::Log(LOGERROR, "CShaderPreset::SetShaderPreset: Couldn't read shader files for {}",
              m_presetPath);
    return false;
  }

  if (!HasPathFailed(m_presetPath))
  {
    if (!CreateShaders())
      return updateFailed("Failed to initialize shaders");

    if (!CreateLayouts())
      return updateFailed("Failed to create layouts");

    if (!CreateBuffers())
      return updateFailed("Failed to initialize buffers");

    if (!CreateSamplers())
      return updateFailed("Failed to create samplers");
  }

  if (m_pShaders.empty())
    return false;

  return true;
}

const std::string& CShaderPreset::GetShaderPreset() const
{
  return m_presetPath;
}

bool CShaderPreset::Update()
{
  auto updateFailed = [this](const std::string& msg)
  {
    m_failedPaths.insert(m_presetPath);
    CLog::Log(LOGWARNING, "CShaderPreset::Update: {}", msg);
    DisposeShaderTextures();
    return false;
  };

  if (m_bPresetNeedsUpdate && !HasPathFailed(m_presetPath))
  {
    DisposeShaderTextures();

    if (!CreateShaderTextures())
      return updateFailed("A shader texture failed to init");
  }

  // Each pass except the last one must have its own texture and the opposite is also true
  if (m_pShaders.size() != m_pShaderTextures.size() + 1)
    return updateFailed("A shader or texture failed to init");

  m_bPresetNeedsUpdate = false;
  return true;
}

void CShaderPreset::UpdateViewPort(CRect viewPort, const float2 fullDestSize)
{
  const float2 currentViewPortSize = {viewPort.Width(), viewPort.Height()};
  if (currentViewPortSize != m_outputSize || fullDestSize != m_fullDestSize)
  {
    m_outputSize = currentViewPortSize;
    m_fullDestSize = fullDestSize;
    m_bPresetNeedsUpdate = true;
  }
}

void CShaderPreset::UpdateMVPs()
{
  for (std::unique_ptr<IShader>& videoShader : m_pShaders)
    videoShader->UpdateMVP();
}

void CShaderPreset::PrepareParameters(const RETRO::ViewportCoordinates& dest,
                                      IShaderTexture& source)
{
  const auto numPasses = static_cast<unsigned int>(m_pShaders.size());

  // Prepare parameters for all shader passes
  for (unsigned int shaderIdx = 0; shaderIdx < numPasses; ++shaderIdx)
  {
    std::unique_ptr<IShader>& videoShader = m_pShaders[shaderIdx];
    videoShader->PrepareParameters(dest, m_fullDestSize, source, m_pShaderTextures, m_pShaders,
                                   static_cast<uint64_t>(m_frameCount));
  }
}

void CShaderPreset::CalculateScaledSize(const KODI::SHADER::ShaderPass& pass,
                                        const float2& prevSize,
                                        float2& scaledSize)
{
  switch (pass.fbo.scaleX.scaleType)
  {
    case ScaleType::ABSOLUTE_SCALE:
      scaledSize.x = static_cast<float>(pass.fbo.scaleX.abs);
      break;
    case ScaleType::VIEWPORT:
      scaledSize.x = pass.fbo.scaleX.scale != 0.0f ? pass.fbo.scaleX.scale * m_fullDestSize.x
                                                   : m_fullDestSize.x;
      break;
    case ScaleType::INPUT:
    default:
      scaledSize.x =
          pass.fbo.scaleX.scale != 0.0f ? pass.fbo.scaleX.scale * prevSize.x : prevSize.x;
      break;
  }
  switch (pass.fbo.scaleY.scaleType)
  {
    case ScaleType::ABSOLUTE_SCALE:
      scaledSize.y = static_cast<float>(pass.fbo.scaleY.abs);
      break;
    case ScaleType::VIEWPORT:
      scaledSize.y = pass.fbo.scaleY.scale != 0.0f ? pass.fbo.scaleY.scale * m_fullDestSize.y
                                                   : m_fullDestSize.y;
      break;
    case ScaleType::INPUT:
    default:
      scaledSize.y =
          pass.fbo.scaleY.scale != 0.0f ? pass.fbo.scaleY.scale * prevSize.y : prevSize.y;
      break;
  }
}

void CShaderPreset::DisposeShaders()
{
  m_pShaders.clear();
  m_passes.clear();
}

void CShaderPreset::DisposeShaderTextures()
{
  m_pShaderTextures.clear();
  m_bPresetNeedsUpdate = true;
}

bool CShaderPreset::HasPathFailed(const std::string& path) const
{
  return m_failedPaths.contains(path);
}

ShaderParameterMap CShaderPreset::GetShaderParameters(
    const std::vector<ShaderParameter>& parameters, const std::string& sourceStr) const
{
  static const std::regex pragmaParamRegex("#pragma parameter ([a-zA-Z_][a-zA-Z0-9_]*)");
  std::smatch matches;

  std::vector<std::string> validParams;
  auto searchStart(sourceStr.cbegin());
  while (regex_search(searchStart, sourceStr.cend(), matches, pragmaParamRegex))
  {
    validParams.push_back(matches[1].str());
    searchStart += matches.position() + matches.length();
  }

  ShaderParameterMap matchParams;

  // For each param found in the source code
  for (const std::string& match : validParams)
  {
    // For each param found in the preset file
    for (const ShaderParameter& parameter : parameters)
    {
      // Check if they match
      if (match == parameter.strId)
      {
        // The add-on has already handled parsing and overwriting default
        // parameter values from the preset file. The final value we
        // should use is in the 'current' field.
        matchParams[match] = parameter.current;
        break;
      }
    }
  }
  return matchParams;
}
