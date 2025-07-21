/*
 *  Copyright (C) 2017-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IShaderPreset.h"
#include "addons/Addon.h"

#include <map>
#include <string>

namespace ADDON
{
class CAddonMgr;
class CBinaryAddonManager;
class CShaderPresetAddon;
} // namespace ADDON

namespace KODI::SHADER
{
class IShaderPresetLoader;

class CShaderPresetFactory
{
public:
  /*!
   * \brief Create the factory and register all shader preset add-ons
   */
  explicit CShaderPresetFactory(ADDON::CAddonMgr& addons);
  ~CShaderPresetFactory();

  /*!
   * \brief Register an object that can load shader presets
   *
   * \param loader The instance of a preset loader
   * \param extension The extension for which the loader can load presets
   */
  void RegisterLoader(IShaderPresetLoader* loader, const std::string& extension);

  /*!
   * \brief Unregister the shader preset loader
   *
   * \param load The loader that was passed to RegisterLoader()
   */
  void UnregisterLoader(const IShaderPresetLoader* loader);

  /*!
   * \brief Check if any shader preset add-ons have been loaded
   *
   * This includes add-ons in a failed state.
   *
   * \return True if any shader preset add-ons are present, false otherwise
   */
  bool HasAddons() const;

  /*!
   * \brief Load a preset from the given path
   *
   * \param presetPath The path to the shader preset
   * \param[out] shaderPreset The loaded shader preset
   *
   * \return True if the preset was loaded, false otherwise
   */
  bool LoadPreset(const std::string& presetPath, IShaderPreset& shaderPreset);

  /*!
   * \brief Check if a registered loader can load a given preset
   *
   * \param presetPath The path to the shader preset
   *
   * \return True if a loader can load the preset, false otherwise
   */
  bool CanLoadPreset(const std::string& presetPath) const;

private:
  void UpdateAddons();

  // Construction parameters
  ADDON::CAddonMgr& m_addons;

  std::map<std::string, IShaderPresetLoader*, std::less<>> m_loaders;
  std::map<std::string, std::unique_ptr<ADDON::CShaderPresetAddon>, std::less<>> m_shaderAddons;
  std::map<std::string, std::unique_ptr<ADDON::CShaderPresetAddon>, std::less<>> m_failedAddons;
};
} // namespace KODI::SHADER
