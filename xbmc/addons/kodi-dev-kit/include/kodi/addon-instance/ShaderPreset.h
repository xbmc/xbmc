/*
 *  Copyright (C) 2017-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "../AddonBase.h"
#include "../c-api/addon-instance/shaderpreset.h"

#include <stdint.h>

#ifdef __cplusplus

namespace kodi
{
namespace addon
{

//==============================================================================
/// @addtogroup cpp_kodi_addon_shader_preset
/// @brief @cpp_class{ kodi::addon::CInstanceShaderPreset }
/// **Shader preset add-on instance**\n
/// This class provides the basic shader preset processing system for use as
/// an add-on in Kodi.
///
/// This class is created at addon by Kodi.
///
class ATTR_DLL_LOCAL CInstanceShaderPreset : public IAddonInstance
{
public:
  //============================================================================
  /// @defgroup cpp_kodi_addon_game_Base 1. Basic functions
  /// @ingroup cpp_kodi_addon_game
  /// @brief **Functions to manage the addon and get basic information about it**
  ///
  ///@{

  //============================================================================
  /// @brief Shader preset class constructor
  ///
  /// Used by an add-on that only supports Shader Presets, and only in one
  /// instance.
  ///
  /// This class is created in the addon by Kodi.
  ///
  ///
  /// --------------------------------------------------------------------------
  ///
  ///
  /// **Here's an example of how to use this class:**
  /// ~~~~~~~~~~~~~{.cpp}
  /// #include <kodi/addon-instance/ShaderPreset.h>
  /// ...
  ///
  /// class ATTR_DLL_LOCAL CShaderPresetExample
  ///   : public kodi::addon::CAddonBase,
  ///     public kodi::addon::CInstanceShaderPreset
  /// {
  /// public:
  ///   CShaderPresetExample()
  ///   {
  ///   }
  ///
  ///   virtual ~CShaderPresetExample();
  ///   {
  ///   }
  ///
  ///   ...
  /// };
  ///
  /// ADDONCREATOR(CShaderPresetExample)
  /// ~~~~~~~~~~~~~
  ///
  CInstanceShaderPreset()
    : IAddonInstance(IInstanceInfo(CPrivateBase::m_interface->firstKodiInstance))
  {
    if (CPrivateBase::m_interface->globalSingleInstance != nullptr)
      throw std::logic_error("kodi::addon::CInstanceShaderPreset: Creation of "
                             "more than one in single instance is not allowed!");

    SetAddonStruct(CPrivateBase::m_interface->firstKodiInstance);
    CPrivateBase::m_interface->globalSingleInstance = this;
  }

  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Destructor
  ///
  ~CInstanceShaderPreset() override = default;
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Used to get the full path to the add-on's user profile
  ///
  /// @return Path to the user profile
  ///
  /// @remarks Only called from the add-on
  ///
  std::string UserPath() const
  {
    if (m_instanceData->props->user_path != nullptr)
      return m_instanceData->props->user_path;
    return "";
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Used to get the full path where the add-on is installed
  ///
  /// @return The add-on installation path
  ///
  /// @remarks Only called from the add-on itself
  ///
  std::string AddonPath() const
  {
    if (m_instanceData->props->addon_path != nullptr)
      return m_instanceData->props->addon_path;
    return "";
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief **Loads a preset file**
  ///
  /// @param path The path to the preset file
  ///
  /// @return The preset file, or NULL if file doesn't exist
  ///
  virtual preset_file PresetFileNew(const char* path) { return nullptr; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief **Free a preset file**
  ///
  virtual void PresetFileFree(preset_file file) {}
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Loads preset file and all associated state (passes, textures,
  /// imports, etc)
  ///
  /// @param file Preset file to read from
  /// @param shader Shader passes handle
  ///
  /// @return True if successful, otherwise false
  ///
  virtual bool ShaderPresetRead(preset_file file, video_shader& shader) { return false; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Save preset and all associated state (passes, textures, imports,
  /// etc) to disk
  ///
  /// @param file Preset file to read from
  /// @param shader Shader passes handle
  ///
  /// @return True if successful, otherwise false
  ///
  virtual bool ShaderPresetWrite(preset_file file, const video_shader& shader) { return false; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Resolve all shader parameters belonging to the shader preset
  ///
  /// @param file Preset file to read from
  /// @param shader Shader passes handle
  ///
  /// @return True if successful, otherwise false
  ///
  virtual bool ShaderPresetResolveParameters(preset_file file, video_shader& shader)
  {
    return false;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Free all state related to shader preset
  ///
  /// @param The shader object to free
  ///
  virtual void ShaderPresetFree(video_shader& shader) {}
  //----------------------------------------------------------------------------

  ///@}

private:
  void SetAddonStruct(KODI_ADDON_INSTANCE_STRUCT* instance)
  {
    instance->hdl = this;

    instance->shaderpreset->toAddon->PresetFileNew = ADDON_preset_file_new;
    instance->shaderpreset->toAddon->PresetFileFree = ADDON_preset_file_free;

    instance->shaderpreset->toAddon->VideoShaderRead = ADDON_video_shader_read_file;
    instance->shaderpreset->toAddon->VideoShaderWrite = ADDON_video_shader_write_file;
    instance->shaderpreset->toAddon->VideoShaderResolveParameters =
        ADDON_video_shader_resolve_parameters;
    instance->shaderpreset->toAddon->VideoShaderFree = ADDON_video_shader_free;

    m_instanceData = instance->shaderpreset;
    m_instanceData->toAddon->addonInstance = this;
  }

  // --- Shader preset operations ----------------------------------------------

  inline static preset_file ADDON_preset_file_new(const AddonInstance_ShaderPreset* addonInstance,
                                                  const char* path)
  {
    return static_cast<CInstanceShaderPreset*>(
               static_cast<CInstanceShaderPreset*>(addonInstance->toAddon->addonInstance))
        ->PresetFileNew(path);
  }

  inline static void ADDON_preset_file_free(const AddonInstance_ShaderPreset* addonInstance,
                                            preset_file file)
  {
    return static_cast<CInstanceShaderPreset*>(addonInstance->toAddon->addonInstance)
        ->PresetFileFree(file);
  }

  inline static bool ADDON_video_shader_read_file(const AddonInstance_ShaderPreset* addonInstance,
                                                  preset_file file,
                                                  video_shader* shader)
  {
    if (shader != nullptr)
      return static_cast<CInstanceShaderPreset*>(addonInstance->toAddon->addonInstance)
          ->ShaderPresetRead(file, *shader);

    return false;
  }

  inline static bool ADDON_video_shader_write_file(const AddonInstance_ShaderPreset* addonInstance,
                                                   preset_file file,
                                                   const video_shader* shader)
  {
    if (shader != nullptr)
      return static_cast<CInstanceShaderPreset*>(addonInstance->toAddon->addonInstance)
          ->ShaderPresetWrite(file, *shader);

    return false;
  }

  inline static bool ADDON_video_shader_resolve_parameters(
      const AddonInstance_ShaderPreset* addonInstance, preset_file file, video_shader* shader)
  {
    if (shader != nullptr)
      return static_cast<CInstanceShaderPreset*>(addonInstance->toAddon->addonInstance)
          ->ShaderPresetResolveParameters(file, *shader);

    return false;
  }

  inline static void ADDON_video_shader_free(const AddonInstance_ShaderPreset* addonInstance,
                                             video_shader* shader)
  {
    if (shader != nullptr)
      static_cast<CInstanceShaderPreset*>(addonInstance->toAddon->addonInstance)
          ->ShaderPresetFree(*shader);
  }

  AddonInstance_ShaderPreset* m_instanceData;
};

} /* namespace addon */
} /* namespace kodi */

#endif /* __cplusplus */
