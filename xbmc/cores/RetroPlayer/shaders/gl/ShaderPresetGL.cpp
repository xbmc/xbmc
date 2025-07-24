/*
 *  Copyright (C) 2019-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ShaderPresetGL.h"

#include "ServiceBroker.h"
#include "ShaderGL.h"
#include "ShaderLutGL.h"
#include "ShaderUtilsGL.h"
#include "cores/RetroPlayer/rendering/RenderContext.h"
#include "rendering/gl/RenderSystemGL.h"
#include "utils/log.h"

#include <regex>

using namespace KODI::SHADER;

CShaderPresetGL::CShaderPresetGL(RETRO::CRenderContext& context,
                                 unsigned int videoWidth,
                                 unsigned int videoHeight)
  : CShaderPreset(context, videoWidth, videoHeight)
{
}

bool CShaderPresetGL::CreateShaders()
{
  const auto numPasses = static_cast<unsigned int>(m_passes.size());

  //! @todo Is this pass specific?
  std::vector<std::shared_ptr<IShaderLut>> passLUTsGL;
  for (unsigned int shaderIdx = 0; shaderIdx < numPasses; ++shaderIdx)
  {
    const ShaderPass& pass = m_passes[shaderIdx];
    const auto numPassLuts = static_cast<unsigned int>(pass.luts.size());

    for (unsigned int i = 0; i < numPassLuts; ++i)
    {
      const ShaderLut& lutStruct = pass.luts[i];

      auto passLut = std::make_shared<CShaderLutGL>(lutStruct.strId, lutStruct.path);
      if (passLut->Create(lutStruct))
        passLUTsGL.emplace_back(std::move(passLut));
    }

    // Create the shader
    auto videoShader = std::make_unique<CShaderGL>();

    const std::string& shaderSource = pass.vertexSource; // Also contains fragment source
    const std::string& shaderPath = pass.sourcePath;

    // Get only the parameters belonging to this specific shader
    ShaderParameterMap passParameters = GetShaderParameters(pass.parameters, pass.vertexSource);

    if (!videoShader->Create(shaderSource, shaderPath, passParameters, passLUTsGL, shaderIdx,
                             pass.frameCountMod))
    {
      CLog::Log(LOGERROR, "CShaderPresetGL::CreateShaders: Couldn't create a video shader");
      return false;
    }
    m_pShaders.push_back(std::move(videoShader));
  }

  return true;
}

bool CShaderPresetGL::CreateShaderTextures()
{
  m_pShaderTextures.clear();

  unsigned int major{0};
  unsigned int minor{0};
  CServiceBroker::GetRenderSystem()->GetRenderVersion(major, minor);

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
      GLint internalFormat;
      GLenum pixelFormat;
      if (pass.fbo.floatFramebuffer && major >= 3)
      {
        // Give priority to float framebuffer parameter (we can't use both float and sRGB)
        internalFormat = GL_RGBA32F;
        pixelFormat = GL_RGBA;
      }
      else
      {
        if (pass.fbo.sRgbFramebuffer && major >= 3)
        {
          internalFormat = GL_SRGB8_ALPHA8;
          pixelFormat = GL_RGBA;
        }
        else
        {
          internalFormat = GL_RGBA8;
          pixelFormat = GL_BGRA;
        }
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

      auto textureGL = std::make_shared<CGLTexture>(static_cast<unsigned int>(textureSize.x),
                                                    static_cast<unsigned int>(textureSize.y),
                                                    XB_FMT_A8R8G8B8); // Format is not used

      textureGL->CreateTextureObject();

      if (textureGL->GetTextureID() <= 0)
      {
        CLog::Log(
            LOGERROR,
            "CShaderPresetGL::CreateShaderTextures: Couldn't create texture for video shader: {}",
            pass.sourcePath);
        return false;
      }

      const ShaderPass& nextPass = m_passes[shaderIdx + 1];

      if (nextPass.mipmap)
        textureGL->SetMipmapping();

      textureGL->SetScalingMethod(nextPass.filterType == FilterType::LINEAR
                                      ? TEXTURE_SCALING::LINEAR
                                      : TEXTURE_SCALING::NEAREST);

      const GLint wrapType = CShaderUtilsGL::TranslateWrapType(nextPass.wrapType);
      const GLuint magFilterType =
          (nextPass.filterType == FilterType::LINEAR ? GL_LINEAR : GL_NEAREST);
      const GLuint minFilterType =
          (nextPass.mipmap ? (nextPass.filterType == FilterType::LINEAR ? GL_LINEAR_MIPMAP_LINEAR
                                                                        : GL_NEAREST_MIPMAP_NEAREST)
                           : magFilterType);

      glBindTexture(GL_TEXTURE_2D, textureGL->GetTextureID());
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilterType);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilterType);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapType);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapType);
      glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, textureSize.x, textureSize.y, 0, pixelFormat,
                   internalFormat == GL_RGBA32F ? GL_FLOAT : GL_UNSIGNED_BYTE, nullptr);

      GLfloat blackBorder[4] = {0.0f, 0.0f, 0.0f, 0.0f};
      glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, blackBorder);

      m_pShaderTextures.emplace_back(
          std::make_unique<CShaderTextureGL>(std::move(textureGL), pass.fbo.sRgbFramebuffer));
    }

    // Notify shader of its source and dest size
    m_pShaders[shaderIdx]->SetSizes(prevSize, prevTextureSize, scaledSize);

    prevSize = scaledSize;
    prevTextureSize = textureSize;
  }

  UpdateMVPs();
  return true;
}

void CShaderPresetGL::RenderShader(IShader& shader, IShaderTexture& source, IShaderTexture& target)
{
  if (static_cast<CShaderTextureGL&>(target).BindFBO())
  {
    const CRect newViewPort(0.f, 0.f, target.GetWidth(), target.GetHeight());
    glViewport((GLsizei)newViewPort.x1, (GLsizei)newViewPort.y1, (GLsizei)newViewPort.x2,
               (GLsizei)newViewPort.y2);
    glScissor((GLsizei)newViewPort.x1, (GLsizei)newViewPort.y1, (GLsizei)newViewPort.x2,
              (GLsizei)newViewPort.y2);
    shader.Render(source, target);
    static_cast<CShaderTextureGL&>(target).UnbindFBO();
  }
}
