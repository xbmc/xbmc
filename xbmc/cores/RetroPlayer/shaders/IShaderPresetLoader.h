/*
 *  Copyright (C) 2017-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

namespace KODI::SHADER
{
class IShaderPreset;

/*!
 * \brief API for a class that can load shader presets
 */
class IShaderPresetLoader
{
public:
  virtual ~IShaderPresetLoader() = default;

  /*!
   * \brief Load a shader preset from the given path
   *
   * \param presetPath The path to the shader preset
   * \param shaderPreset The loaded shader preset, or untouched if false is returned
   *
   * \return True if the preset was loaded, false otherwise
   */
  virtual bool LoadPreset(const std::string& presetPath, IShaderPreset& shaderPreset) = 0;
};
} // namespace KODI::SHADER
