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

#include <string>
#include <vector>

namespace KODI::SHADER
{
class IShaderTexture;

class IShaderPreset
{
public:
  virtual ~IShaderPreset() = default;

  //! @todo Implement once and for all
  /*!
   * \brief Reads/parses a shader preset file and loads its state to the object
   *
   * The state is implementation specific.
   *
   * \param presetPath Full path of the preset file
   *
   * \return Returns true on successful parsing, false on failed
   */
  virtual bool ReadPresetFile(const std::string& presetPath) = 0;

  /*!
   * \brief Updates state if needed and renders the preset to the target texture
   *
   * \param dest Coordinates of the 4 corners of the destination rectangle
   * \param fullDestSize Destination rectangle size for the fullscreen game window
   * \param source The source of the video frame, in its original resolution (unscaled)
   * \param target The target texture that the final result will be rendered to
   *
   * \return Returns false if updating or rendering failed, true if both succeeded
   */
  virtual bool RenderUpdate(const RETRO::ViewportCoordinates& dest,
                            const float2 fullDestSize,
                            IShaderTexture& source,
                            IShaderTexture& target) = 0;

  /*!
   * \brief Informs about the speed of playback
   *
   * \param speed Commonly 1.0 for normal playback, and 0.0 if paused
   */
  virtual void SetSpeed(double speed) = 0;

  /*!
   * \brief Size of the input/source frame in pixels
   *
   * \param videoWidth Height of the source frame in pixels
   * \param videoHeight Height of the source frame in pixels
   */
  virtual void SetVideoSize(unsigned int videoWidth, unsigned int videoHeight) = 0;

  /*!
   * \brief Set the preset to be rendered on the next frame
   *
   * \param shaderPresetPath Full path to the preset file to be loaded
   *
   * \return Returns false if loading the preset failed, true otherwise
   */
  virtual bool SetShaderPreset(const std::string& shaderPresetPath) = 0;

  /*!
   * \brief Gets the full path to the shader preset
   *
   * \return The full path to the currently loaded preset file
   */
  virtual const std::string& GetShaderPreset() const = 0;

  /*!
   * \brief Gets the passes of the loaded preset
   *
   * \return All the video shader passes of the currently loaded preset
   */
  virtual std::vector<ShaderPass>& GetPasses() = 0;
};
} // namespace KODI::SHADER
