/*
 *  Copyright (C) 2017-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ShaderPreset.h"

#include "addons/addoninfo/AddonInfo.h"
#include "addons/addoninfo/AddonType.h"
#include "addons/binary-addons/BinaryAddonBase.h"
#include "cores/RetroPlayer/shaders/IShaderPreset.h"
#include "filesystem/SpecialProtocol.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

using namespace KODI;
using namespace ADDON;

// --- CShaderPreset -----------------------------------------------------------

CShaderPreset::CShaderPreset(preset_file file, AddonInstance_ShaderPreset& instanceStruct)
  : m_file(file), m_struct(instanceStruct)
{
}

CShaderPreset::~CShaderPreset()
{
  m_struct.toAddon->preset_file_free(&m_struct, m_file);
}

bool CShaderPreset::ReadShaderPreset(video_shader& shader)
{
  return m_struct.toAddon->video_shader_read(&m_struct, m_file, &shader);
}

void CShaderPreset::WriteShaderPreset(const video_shader& shader)
{
  return m_struct.toAddon->video_shader_write(&m_struct, m_file, &shader);
}

/*
void CShaderPreset::ResolveRelative(video_shader& shader, const std::string& ref_path)
{
  return m_struct.toAddon->video_shader_resolve_relative(&m_struct, &shader, ref_path.c_str());
}

bool CShaderPreset::ResolveCurrentParameters(video_shader &shader)
{
  return m_struct.toAddon->video_shader_resolve_current_parameters(&m_struct, m_file, &shader);
}
*/

bool CShaderPreset::ResolveParameters(video_shader& shader)
{
  return m_struct.toAddon->video_shader_resolve_parameters(&m_struct, m_file, &shader);
}

void CShaderPreset::FreeShaderPreset(video_shader& shader)
{
  m_struct.toAddon->video_shader_free(&m_struct, &shader);
}

// --- CShaderPresetAddon ------------------------------------------------------

CShaderPresetAddon::CShaderPresetAddon(const AddonInfoPtr& addonInfo)
  : IAddonInstanceHandler(ADDON_INSTANCE_SHADERPRESET, addonInfo)
{
  // Create "C" interface structures, used to prevent API problems on update
  m_ifc.shaderpreset = new AddonInstance_ShaderPreset;
  m_ifc.shaderpreset->props = new AddonProps_ShaderPreset();
  m_ifc.shaderpreset->toAddon = new KodiToAddonFuncTable_ShaderPreset();
  m_ifc.shaderpreset->toKodi = new AddonToKodiFuncTable_ShaderPreset();

  ResetProperties();

  // Initialize properties
  m_strUserPath = CSpecialProtocol::TranslatePath(Profile());
  m_strClientPath = CSpecialProtocol::TranslatePath(Path());
  m_extensions = StringUtils::Split(
      addonInfo->Type(ADDON::AddonType::SHADERDLL)->GetValue("@extensions").asString(), "|");
}

CShaderPresetAddon::~CShaderPresetAddon(void)
{
  DestroyAddon();
}

bool CShaderPresetAddon::CreateAddon(void)
{
  std::unique_lock<CSharedSection> lock(m_dllSection);

  // Reset all properties to defaults
  ResetProperties();

  // Initialise the add-on
  CLog::Log(LOGDEBUG, "{} - creating ShaderPreset add-on instance '{}'", __FUNCTION__,
            Name().c_str());

  if (CreateInstance() != ADDON_STATUS_OK)
    return false;

  return true;
}

void CShaderPresetAddon::DestroyAddon()
{
  std::unique_lock<CSharedSection> lock(m_dllSection);

  DestroyInstance();
}

void CShaderPresetAddon::ResetProperties(void)
{
  m_ifc.shaderpreset->props->user_path = m_strUserPath.c_str();
  m_ifc.shaderpreset->props->addon_path = m_strClientPath.c_str();

  m_ifc.shaderpreset->toKodi->kodiInstance = this;

  memset(m_ifc.shaderpreset->toAddon, 0, sizeof(KodiToAddonFuncTable_ShaderPreset));
}

bool CShaderPresetAddon::LoadPreset(const std::string& presetPath,
                                    SHADER::IShaderPreset& shaderPreset)
{
  bool bSuccess = false;

  std::string translatedPath = CSpecialProtocol::TranslatePath(presetPath);

  preset_file file =
      m_ifc.shaderpreset->toAddon->preset_file_new(m_ifc.shaderpreset, translatedPath.c_str());

  if (file != nullptr)
  {
    std::unique_ptr<CShaderPreset> shaderPresetAddon(new CShaderPreset(file, *m_ifc.shaderpreset));

    video_shader videoShader = {};
    if (shaderPresetAddon->ReadShaderPreset(videoShader))
    {
      if (shaderPresetAddon->ResolveParameters(videoShader))
      {
        TranslateShaderPreset(videoShader, shaderPreset);
        bSuccess = true;
      }
      shaderPresetAddon->FreeShaderPreset(videoShader);
    }
  }

  return bSuccess;
}

//! @todo Instead of copying every parameter to every pass and resolving them
//! later in GetShaderParameters, we should resolve which param goes to which
//! shader in the add-on
void CShaderPresetAddon::TranslateShaderPreset(const video_shader& shader,
                                               SHADER::IShaderPreset& shaderPreset)
{
  if (shader.passes != nullptr)
  {
    for (unsigned int passIdx = 0; passIdx < shader.pass_count; ++passIdx)
    {
      SHADER::ShaderPass shaderPass;
      TranslateShaderPass(shader.passes[passIdx], shaderPass);

      if (shader.luts != nullptr)
      {
        for (unsigned int lutIdx = 0; lutIdx < shader.lut_count; ++lutIdx)
        {
          SHADER::ShaderLut shaderLut;
          TranslateShaderLut(shader.luts[lutIdx], shaderLut);
          shaderPass.luts.emplace_back(std::move(shaderLut));
        }
      }

      if (shader.parameters != nullptr)
      {
        for (unsigned int parIdx = 0; parIdx < shader.parameter_count; ++parIdx)
        {
          SHADER::ShaderParameter shaderParam;
          TranslateShaderParameter(shader.parameters[parIdx], shaderParam);
          shaderPass.parameters.emplace_back(std::move(shaderParam));
        }
      }

      shaderPreset.GetPasses().emplace_back(std::move(shaderPass));
    }
  }
}

void CShaderPresetAddon::TranslateShaderPass(const video_shader_pass& pass,
                                             SHADER::ShaderPass& shaderPass)
{
  shaderPass.sourcePath = pass.source_path ? pass.source_path : "";
  shaderPass.vertexSource = pass.vertex_source ? pass.vertex_source : "";
  shaderPass.fragmentSource = pass.fragment_source ? pass.fragment_source : "";
  shaderPass.filterType = TranslateFilterType(pass.filter);
  shaderPass.wrapType = TranslateWrapType(pass.wrap);
  shaderPass.frameCountMod = pass.frame_count_mod;

  const auto& fbo = pass.fbo;
  auto& shaderFbo = shaderPass.fbo;

  shaderFbo.scaleX.scaleType = TranslateScaleType(fbo.scale_x.type);
  switch (fbo.scale_x.type)
  {
    case SHADER_SCALE_TYPE_ABSOLUTE:
      shaderFbo.scaleX.abs = fbo.scale_x.abs;
      break;
    default:
      shaderFbo.scaleX.scale = fbo.scale_x.scale;
      break;
  }
  shaderFbo.scaleY.scaleType = TranslateScaleType(fbo.scale_y.type);
  switch (fbo.scale_y.type)
  {
    case SHADER_SCALE_TYPE_ABSOLUTE:
      shaderFbo.scaleY.abs = fbo.scale_y.abs;
      break;
    default:
      shaderFbo.scaleY.scale = fbo.scale_y.scale;
      break;
  }

  shaderFbo.floatFramebuffer = fbo.fp_fbo;
  shaderFbo.sRgbFramebuffer = fbo.srgb_fbo;

  shaderPass.mipmap = pass.mipmap;
}

void CShaderPresetAddon::TranslateShaderLut(const video_shader_lut& lut,
                                            SHADER::ShaderLut& shaderLut)
{
  shaderLut.strId = lut.id ? lut.id : "";
  shaderLut.path = lut.path ? lut.path : "";
  shaderLut.filterType = TranslateFilterType(lut.filter);
  shaderLut.wrapType = TranslateWrapType(lut.wrap);
  shaderLut.mipmap = lut.mipmap;
}

void CShaderPresetAddon::TranslateShaderParameter(const video_shader_parameter& param,
                                                  SHADER::ShaderParameter& shaderParam)
{
  shaderParam.strId = param.id ? param.id : "";
  shaderParam.description = param.desc ? param.desc : "";
  shaderParam.current = param.current;
  shaderParam.minimum = param.minimum;
  shaderParam.initial = param.initial;
  shaderParam.maximum = param.maximum;
  shaderParam.step = param.step;
}

SHADER::FilterType CShaderPresetAddon::TranslateFilterType(SHADER_FILTER_TYPE type)
{
  switch (type)
  {
    case SHADER_FILTER_TYPE_LINEAR:
      return SHADER::FilterType::LINEAR;
    case SHADER_FILTER_TYPE_NEAREST:
      return SHADER::FilterType::NEAREST;
    default:
      break;
  }

  return SHADER::FilterType::NONE;
}

SHADER::WrapType CShaderPresetAddon::TranslateWrapType(SHADER_WRAP_TYPE type)
{
  switch (type)
  {
    case SHADER_WRAP_TYPE_BORDER:
      return SHADER::WrapType::BORDER;
    case SHADER_WRAP_TYPE_EDGE:
      return SHADER::WrapType::EDGE;
    case SHADER_WRAP_TYPE_REPEAT:
      return SHADER::WrapType::REPEAT;
    case SHADER_WRAP_TYPE_MIRRORED_REPEAT:
      return SHADER::WrapType::MIRRORED_REPEAT;
    default:
      break;
  }

  return SHADER::WrapType::BORDER;
}

SHADER::ScaleType CShaderPresetAddon::TranslateScaleType(SHADER_SCALE_TYPE type)
{
  switch (type)
  {
    case SHADER_SCALE_TYPE_INPUT:
      return SHADER::ScaleType::INPUT;
    case SHADER_SCALE_TYPE_ABSOLUTE:
      return SHADER::ScaleType::ABSOLUTE_SCALE;
    case SHADER_SCALE_TYPE_VIEWPORT:
      return SHADER::ScaleType::VIEWPORT;
    default:
      break;
  }

  return SHADER::ScaleType::INPUT;
}
