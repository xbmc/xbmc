/*
 *  Copyright (C) 2017-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
#pragma once

#include "addons/binary-addons/AddonInstanceHandler.h"
#include "addons/kodi-dev-kit/include/kodi/addon-instance/ShaderPreset.h"
#include "cores/RetroPlayer/shaders/IShaderPresetLoader.h"
#include "cores/RetroPlayer/shaders/ShaderTypes.h"
#include "threads/SharedSection.h"

#include <string>
#include <vector>

namespace ADDON
{
class CAddonInfo;
using AddonInfoPtr = std::shared_ptr<CAddonInfo>;

class CShaderPreset
{
public:
  CShaderPreset(preset_file file, AddonInstance_ShaderPreset& instanceStruct);
  ~CShaderPreset();

  /*!
   * \brief Loads preset file and all associated state (passes, textures,
   * imports, etc)
   *
   * \param shader Shader passes handle
   *
   * \return True if successful, otherwise false
   */
  bool ReadShaderPreset(video_shader& shader);

  /*!
   * \brief Save preset and all associated state (passes, textures, imports,
   * etc) to disk
   *
   * \param shader Shader passes handle
   *
   * \return True if successful, otherwise false
   */
  bool WriteShaderPreset(const video_shader& shader);

  /*!
   * \brief Resolve all shader parameters belonging to the shader preset
   *
   * \param shader Shader passes handle
   *
   * \return True if successful, otherwise false
   */
  bool ResolveParameters(video_shader& shader);

  /*!
   * \brief Frees a preset file and all associated resources
   *
   * \param shader Shader passes handle
   */
  void FreeShaderPreset(video_shader& shader) const;

private:
  preset_file m_file;
  AddonInstance_ShaderPreset& m_struct;
};

/*!
 * \brief Wrapper class that wraps the shader presets add-on
 */
class CShaderPresetAddon : public IAddonInstanceHandler, public KODI::SHADER::IShaderPresetLoader
{
public:
  explicit CShaderPresetAddon(const AddonInfoPtr& addonInfo);
  ~CShaderPresetAddon() override;

  /*!
   * \brief Initialise the instance of this add-on
   */
  bool CreateAddon();

  /*!
   * \brief Deinitialize the instance of this add-on
   */
  void DestroyAddon();

  /*!
   * \brief Get the shader preset extensions supported by this add-on
   */
  const std::vector<std::string>& GetExtensions() const { return m_extensions; }

  // Implementation of IShaderPresetLoader
  bool LoadPreset(const std::string& presetPath,
                  KODI::SHADER::IShaderPreset& shaderPreset) override;

private:
  /*!
   * \brief Reset all class members to their defaults. Called by the constructors
   */
  void ResetProperties(void);

  static void TranslateShaderPreset(const video_shader& shader,
                                    KODI::SHADER::IShaderPreset& shaderPreset);
  static void TranslateShaderPass(const video_shader_pass& pass,
                                  KODI::SHADER::ShaderPass& shaderPass);
  static void TranslateShaderLut(const video_shader_lut& lut, KODI::SHADER::ShaderLut& shaderLut);
  static void TranslateShaderParameter(const video_shader_parameter& param,
                                       KODI::SHADER::ShaderParameter& shaderParam);
  static KODI::SHADER::FilterType TranslateFilterType(SHADER_FILTER_TYPE type);
  static KODI::SHADER::WrapType TranslateWrapType(SHADER_WRAP_TYPE type);
  static KODI::SHADER::ScaleType TranslateScaleType(SHADER_SCALE_TYPE type);

  // Cache for const char* members in AddonProps_ShaderPreset
  std::string m_strUserPath; /*!< \brief Translated path to the user profile */
  std::string m_strClientPath; /*!< \brief Translated path to this add-on */

  std::vector<std::string> m_extensions;

  CSharedSection m_dllSection;
};
} // namespace ADDON
