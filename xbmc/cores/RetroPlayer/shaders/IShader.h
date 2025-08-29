/*
 *  Copyright (C) 2017-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "ShaderTypes.h"
#include "cores/RetroPlayer/RetroPlayerTypes.h"

#include <map>
#include <stdint.h>
#include <string>
#include <vector>

namespace KODI::SHADER
{
class IShaderLut;
class IShaderTexture;

class IShader
{
public:
  virtual ~IShader() = default;

  /*!
   * \brief Construct the video shader instance
   *
   * \param shaderSource Source code of the shader (both vertex and pixel/fragment)
   * \param shaderPath Full path to the shader file
   * \param shaderParameters Struct with all parameters pertaining to the shader
   * \param luts Look-up textures pertaining to the shader
   * \param passIdx Index of the video shader pass
   * \param frameCountMod Modulo applied to the frame count before sending it to the shader
   *
   * \return Returns false if creating the shader failed, true otherwise
   */
  virtual bool Create(std::string shaderSource,
                      std::string shaderPath,
                      ShaderParameterMap shaderParameters,
                      std::vector<std::shared_ptr<IShaderLut>> luts,
                      unsigned int passIdx,
                      unsigned int frameCountMod = 0) = 0;

  /*!
   * \brief Renders the video shader to the target texture
   *
   * \param source Source texture to pass to the shader as input
   * \param target Target texture to render the shader to
   */
  virtual void Render(IShaderTexture& source, IShaderTexture& target) = 0;

  /*!
   * \brief Sets the input and output sizes in pixels
   *
   * \param prevSize Input image size of the shader in pixels
   * \param prevTextureSize Power-of-two input texture size in pixels
   * \param nextSize Output image size of the shader in pixels
   */
  virtual void SetSizes(const float2& prevSize,
                        const float2& prevTextureSize,
                        const float2& nextSize) = 0;

  /*!
   * \brief Called before rendering
   *
   * Updates any internal state needed to ensure that correct data is passed to
   * the shader when rendering.
   *
   * \param dest Coordinates of the 4 corners of the destination rectangle
   * \param fullDestSize Destination rectangle size for the fullscreen game window
   * \param sourceTexture Source texture of the first shader pass
   * \param pShaderTextures Intermediate textures used for all shader passes
   * \param pShaders All shader passes
   * \param frameCount Number of frames that have passed
   */
  virtual void PrepareParameters(
      const RETRO::ViewportCoordinates& dest,
      const float2 fullDestSize,
      IShaderTexture& sourceTexture,
      const std::vector<std::unique_ptr<IShaderTexture>>& pShaderTextures,
      const std::vector<std::unique_ptr<IShader>>& pShaders,
      uint64_t frameCount) = 0;

  /*!
   * \brief Updates the model view projection matrix
   *
   * Should usually only be called when the viewport/window size changes.
   */
  virtual void UpdateMVP() = 0;
};
} // namespace KODI::SHADER
