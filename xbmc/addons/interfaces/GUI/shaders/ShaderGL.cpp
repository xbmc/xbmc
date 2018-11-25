/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ShaderGL.h"
#include "addons/kodi-addon-dev-kit/include/kodi/gui/shaders/ShaderGL.h"

#include "ServiceBroker.h"
#include "addons/binary-addons/AddonDll.h"
#if defined(HAS_GL) || HAS_GLES >= 2
#include "guilib/Shader.h"
#endif
#if defined(HAS_GL)
#include "rendering/gl/RenderSystemGL.h"
#elif HAS_GLES >= 2
#include "rendering/gles/RenderSystemGLES.h"
#elif defined(TARGET_WINDOWS)
#include "rendering/dx/RenderSystemDX.h"
#endif
#include "utils/log.h"

extern "C"
{
namespace ADDON
{

void Interface_GUIShaderGL::Init(AddonGlobalInterface* addonInterface)
{
  addonInterface->toKodi->kodi_gui->gl_shader = static_cast<AddonToKodiFuncTable_kodi_gui_gl_shader*>(malloc(sizeof(AddonToKodiFuncTable_kodi_gui_gl_shader)));

  addonInterface->toKodi->kodi_gui->gl_shader->present_enable = present_enable;
  addonInterface->toKodi->kodi_gui->gl_shader->present_disable = present_disable;
  addonInterface->toKodi->kodi_gui->gl_shader->present_get_pos = present_get_pos;
  addonInterface->toKodi->kodi_gui->gl_shader->present_get_col = present_get_col;
  addonInterface->toKodi->kodi_gui->gl_shader->present_get_coord0 = present_get_coord0;
  addonInterface->toKodi->kodi_gui->gl_shader->present_get_coord1 = present_get_coord1;
  addonInterface->toKodi->kodi_gui->gl_shader->present_get_uni_col = present_get_uni_col;
  addonInterface->toKodi->kodi_gui->gl_shader->present_get_model = present_get_model;

  addonInterface->toKodi->kodi_gui->gl_shader->shader_create  = shader_create;
  addonInterface->toKodi->kodi_gui->gl_shader->shader_destroy = shader_destroy;
  addonInterface->toKodi->kodi_gui->gl_shader->shader_compile = shader_compile;
  addonInterface->toKodi->kodi_gui->gl_shader->shader_free = shader_free;
  addonInterface->toKodi->kodi_gui->gl_shader->shader_handle = shader_handle;
  addonInterface->toKodi->kodi_gui->gl_shader->shader_set_source = shader_set_source;
  addonInterface->toKodi->kodi_gui->gl_shader->shader_load_source = shader_load_source;
  addonInterface->toKodi->kodi_gui->gl_shader->shader_append_source = shader_append_source;
  addonInterface->toKodi->kodi_gui->gl_shader->shader_insert_source = shader_insert_source;
  addonInterface->toKodi->kodi_gui->gl_shader->shader_ok = shader_ok;

  addonInterface->toKodi->kodi_gui->gl_shader->shader_program_create = shader_program_create;
  addonInterface->toKodi->kodi_gui->gl_shader->shader_program_destroy = shader_program_destroy;
  addonInterface->toKodi->kodi_gui->gl_shader->shader_program_enable = shader_program_enable;
  addonInterface->toKodi->kodi_gui->gl_shader->shader_program_disable = shader_program_disable;
  addonInterface->toKodi->kodi_gui->gl_shader->shader_program_ok = shader_program_ok;
  addonInterface->toKodi->kodi_gui->gl_shader->shader_program_vertex_shader = shader_program_vertex_shader;
  addonInterface->toKodi->kodi_gui->gl_shader->shader_program_pixel_shader = shader_program_pixel_shader;
  addonInterface->toKodi->kodi_gui->gl_shader->shader_program_compile_and_link = shader_program_compile_and_link;
  addonInterface->toKodi->kodi_gui->gl_shader->shader_program_program_handle = shader_program_program_handle;
}

void Interface_GUIShaderGL::DeInit(AddonGlobalInterface* addonInterface)
{
  free(addonInterface->toKodi->kodi_gui->gl_shader);
}

//------------------------------------------------------------------------------

void Interface_GUIShaderGL::present_enable(void* kodiBase, int method)
{
#if defined(HAS_GL) || HAS_GLES >= 2
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIShaderGL::{} - invalid kodi base data", __FUNCTION__);
    return;
  }

  ESHADERMETHOD usedMethod;
  switch (method)
  {
    case kodi::gui::gl::SHADER_DEFAULT:
      usedMethod = SM_DEFAULT;
      break;
    case kodi::gui::gl::SHADER_TEXTURE:
      usedMethod = SM_TEXTURE;
      break;
#if defined(HAS_GL)
    case kodi::gui::gl::SHADER_TEXTURE_LIM:
      usedMethod = SM_TEXTURE_LIM;
      break;
#endif
    case kodi::gui::gl::SHADER_MULTI:
      usedMethod = SM_MULTI;
      break;
    case kodi::gui::gl::SHADER_FONTS:
      usedMethod = SM_FONTS;
      break;
    case kodi::gui::gl::SHADER_TEXTURE_NOBLEND:
      usedMethod = SM_TEXTURE_NOBLEND;
      break;
    case kodi::gui::gl::SHADER_MULTI_BLENDCOLOR:
      usedMethod = SM_MULTI_BLENDCOLOR;
      break;
#if HAS_GLES >= 2
    case kodi::gui::gl::SHADER_TEXTURE_RGBA:
      usedMethod = SM_TEXTURE_RGBA;
      break;
    case kodi::gui::gl::SHADER_TEXTURE_RGBA_OES:
      usedMethod = SM_TEXTURE_RGBA_OES;
      break;
    case kodi::gui::gl::SHADER_TEXTURE_RGBA_BLENDCOLOR:
      usedMethod = SM_TEXTURE_RGBA_BLENDCOLOR;
      break;
    case kodi::gui::gl::SHADER_TEXTURE_RGBA_BOB:
      usedMethod = SM_TEXTURE_RGBA_BOB;
      break;
    case kodi::gui::gl::SHADER_TEXTURE_RGBA_BOB_OES:
      usedMethod = SM_TEXTURE_RGBA_BOB_OES;
      break;
#endif
    default:
#if defined(HAS_GL)
    CLog::Log(LOGERROR, "Interface_GUIShaderGL::{} - invalid GL shader method '{}' from addon '{}'", __FUNCTION__, method, addon->ID());
#elif HAS_GLES >= 2
    CLog::Log(LOGERROR, "Interface_GUIShaderGL::{} - invalid GLES shader method '{}' from addon '{}'", __FUNCTION__, method, addon->ID());
#endif
  }

#if defined(HAS_GL)
  dynamic_cast<CRenderSystemGL*>(CServiceBroker::GetRenderSystem())->EnableShader(usedMethod);
#elif HAS_GLES >= 2
  dynamic_cast<CRenderSystemGLES*>(CServiceBroker::GetRenderSystem())->EnableGUIShader(usedMethod);
#endif

#endif
}

void Interface_GUIShaderGL::present_disable(void* kodiBase)
{
#if defined(HAS_GL)
  dynamic_cast<CRenderSystemGL*>(CServiceBroker::GetRenderSystem())->DisableShader();
#elif HAS_GLES >= 2
  dynamic_cast<CRenderSystemGLES*>(CServiceBroker::GetRenderSystem())->DisableGUIShader();
#endif
}

int Interface_GUIShaderGL::present_get_pos(void* kodiBase)
{
#if defined(HAS_GL)
  return dynamic_cast<CRenderSystemGL*>(CServiceBroker::GetRenderSystem())->ShaderGetPos();
#elif HAS_GLES >= 2
  return dynamic_cast<CRenderSystemGLES*>(CServiceBroker::GetRenderSystem())->GUIShaderGetPos();
#else
  return -1;
#endif
}

int Interface_GUIShaderGL::present_get_col(void* kodiBase)
{
#if defined(HAS_GL)
  return dynamic_cast<CRenderSystemGL*>(CServiceBroker::GetRenderSystem())->ShaderGetCol();
#elif HAS_GLES >= 2
  return dynamic_cast<CRenderSystemGLES*>(CServiceBroker::GetRenderSystem())->GUIShaderGetCol();
#else
  return -1;
#endif
}

int Interface_GUIShaderGL::present_get_coord0(void* kodiBase)
{
#if defined(HAS_GL)
  return dynamic_cast<CRenderSystemGL*>(CServiceBroker::GetRenderSystem())->ShaderGetCoord0();
#elif HAS_GLES >= 2
  return dynamic_cast<CRenderSystemGLES*>(CServiceBroker::GetRenderSystem())->GUIShaderGetCoord0();
#else
  return -1;
#endif
}

int Interface_GUIShaderGL::present_get_coord1(void* kodiBase)
{
#if defined(HAS_GL)
  return dynamic_cast<CRenderSystemGL*>(CServiceBroker::GetRenderSystem())->ShaderGetCoord1();
#elif HAS_GLES >= 2
  return dynamic_cast<CRenderSystemGLES*>(CServiceBroker::GetRenderSystem())->GUIShaderGetCoord1();
#else
  return -1;
#endif
}

int Interface_GUIShaderGL::present_get_uni_col(void* kodiBase)
{
#if defined(HAS_GL)
  return dynamic_cast<CRenderSystemGL*>(CServiceBroker::GetRenderSystem())->ShaderGetUniCol();
#elif HAS_GLES >= 2
  return dynamic_cast<CRenderSystemGLES*>(CServiceBroker::GetRenderSystem())->GUIShaderGetUniCol();
#else
  return -1;
#endif
}

int Interface_GUIShaderGL::present_get_model(void* kodiBase)
{
#if defined(HAS_GL)
  return dynamic_cast<CRenderSystemGL*>(CServiceBroker::GetRenderSystem())->ShaderGetModel();
#elif HAS_GLES >= 2
  return dynamic_cast<CRenderSystemGLES*>(CServiceBroker::GetRenderSystem())->GUIShaderGetModel();
#else
  return -1;
#endif
}

//------------------------------------------------------------------------------

void* Interface_GUIShaderGL::shader_create(void* kodiBase, int type)
{
#if defined(HAS_GL) || HAS_GLES >= 2
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIShaderGL::{} - invalid kodi base data", __FUNCTION__);
    return nullptr;
  }

  // setup the progress dialog
  Shaders::CShader* shader = nullptr;

  switch(type)
  {
    case kodi::gui::gl::SHADER_VERTEX:
      shader = new Shaders::CGLSLVertexShader();
      break;
    case kodi::gui::gl::SHADER_PIXEL:
      shader = new Shaders::CGLSLPixelShader();
      break;
    default:
      CLog::Log(LOGERROR, "Interface_GUIShaderGL::{} - invalid shader type (%i) on addon '{}'", __FUNCTION__, type, addon->ID());
  }

  return shader;
#else
  return nullptr;
#endif
}

void Interface_GUIShaderGL::shader_destroy(void* kodiBase, void* handle)
{
#if defined(HAS_GL) || HAS_GLES >= 2
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIShaderGL::{} - invalid kodi base data", __FUNCTION__);
    return;
  }

  if (!handle)
  {
    CLog::Log(LOGERROR, "Interface_GUIShaderGL::{} - invalid handler data (handle='%p') on addon '{}'", __FUNCTION__, handle, addon->ID());
    return;
  }

  delete static_cast<Shaders::CShader*>(handle);
#endif
}

bool Interface_GUIShaderGL::shader_compile(void* kodiBase, void* handle)
{
#if defined(HAS_GL) || HAS_GLES >= 2
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIShaderGL::{} - invalid kodi base data", __FUNCTION__);
    return false;
  }

  if (!handle)
  {
    CLog::Log(LOGERROR, "Interface_GUIShaderGL::{} - invalid handler data (handle='%p') on addon '{}'", __FUNCTION__, handle, addon->ID());
    return false;
  }

  return static_cast<Shaders::CShader*>(handle)->Compile();
#else
  return false;
#endif
}

void Interface_GUIShaderGL::shader_free(void* kodiBase, void* handle)
{
#if defined(HAS_GL) || HAS_GLES >= 2
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIShaderGL::{} - invalid kodi base data", __FUNCTION__);
    return;
  }

  if (!handle)
  {
    CLog::Log(LOGERROR, "Interface_GUIShaderGL::{} - invalid handler data (handle='%p') on addon '{}'", __FUNCTION__, handle, addon->ID());
    return;
  }

  static_cast<Shaders::CShader*>(handle)->Free();
#endif
}

unsigned int Interface_GUIShaderGL::shader_handle(void* kodiBase, void* handle)
{
#if defined(HAS_GL) || HAS_GLES >= 2
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIShaderGL::{} - invalid kodi base data", __FUNCTION__);
    return 0;
  }

  if (!handle)
  {
    CLog::Log(LOGERROR, "Interface_GUIShaderGL::{} - invalid handler data (handle='%p') on addon '{}'", __FUNCTION__, handle, addon->ID());
    return 0;
  }

  return static_cast<Shaders::CShader*>(handle)->Handle();
#else
  return 0;
#endif
}

void Interface_GUIShaderGL::shader_set_source(void* kodiBase, void* handle, const char* src)
{
#if defined(HAS_GL) || HAS_GLES >= 2
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIShaderGL::{} - invalid kodi base data", __FUNCTION__);
    return;
  }

  if (!handle || !src)
  {
    CLog::Log(LOGERROR, "Interface_GUIShaderGL::{} - invalid handler data (handle='%p', src='%p') on addon '{}'", __FUNCTION__, handle, src, addon->ID());
    return;
  }

  static_cast<Shaders::CShader*>(handle)->SetSource(src);
#endif
}

bool Interface_GUIShaderGL::shader_load_source(void* kodiBase, void* handle, const char* filename, const char* prefix, const char* base_path)
{
#if defined(HAS_GL) || HAS_GLES >= 2
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIShaderGL::{} - invalid kodi base data", __FUNCTION__);
    return false;
  }

  if (!handle || !filename || !prefix || !base_path)
  {
    CLog::Log(LOGERROR, "Interface_GUIShaderGL::{} - invalid handler data (handle='%p', filename='%p', prefix='%p', base_path='%p') on addon '{}'",
                        __FUNCTION__, handle, filename, prefix, base_path, addon->ID());
    return false;
  }

  return static_cast<Shaders::CShader*>(handle)->LoadSource(filename, prefix, base_path);
#else
  return false;
#endif
}

bool Interface_GUIShaderGL::shader_append_source(void* kodiBase, void* handle, const char* filename, const char* base_path)
{
#if defined(HAS_GL) || HAS_GLES >= 2
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIShaderGL::{} - invalid kodi base data", __FUNCTION__);
    return false;
  }

  if (!handle || !filename || !base_path)
  {
    CLog::Log(LOGERROR, "Interface_GUIShaderGL::{} - invalid handler data (handle='%p', filename='%p', base_path='%p') on addon '{}'",
                        __FUNCTION__, handle, filename, base_path, addon->ID());
    return false;
  }

  return static_cast<Shaders::CShader*>(handle)->AppendSource(filename, base_path);
#else
  return false;
#endif
}

bool Interface_GUIShaderGL::shader_insert_source(void* kodiBase, void* handle, const char* filename, const char* loc, const char* base_path)
{
#if defined(HAS_GL) || HAS_GLES >= 2
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIShaderGL::{} - invalid kodi base data", __FUNCTION__);
    return false;
  }

  if (!handle || !filename || !loc || !base_path)
  {
    CLog::Log(LOGERROR, "Interface_GUIShaderGL::{} - invalid handler data (handle='%p', filename='%p', loc='%p', base_path='%p') on addon '{}'",
                        __FUNCTION__, handle, filename, loc, base_path, addon->ID());
    return false;
  }

  return static_cast<Shaders::CShader*>(handle)->InsertSource(filename, loc, base_path);
#else
  return false;
#endif
}

bool Interface_GUIShaderGL::shader_ok(void* kodiBase, void* handle)
{
#if defined(HAS_GL) || HAS_GLES >= 2
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIShaderGL::{} - invalid kodi base data", __FUNCTION__);
    return false;
  }

  if (!handle)
  {
    CLog::Log(LOGERROR, "Interface_GUIShaderGL::{} - invalid handler data (handle='%p') on addon '{}'", __FUNCTION__, handle, addon->ID());
    return false;
  }

  return static_cast<Shaders::CShader*>(handle)->OK();
#else
  return false;
#endif
}

//------------------------------------------------------------------------------

#if defined(HAS_GL) || HAS_GLES >= 2
class CAddonGLShaderProgram : public Shaders::CGLSLShaderProgram
{
public:
  CAddonGLShaderProgram(void* clienthandle,
                        void (*OnCompiledAndLinked)(void*),
                        bool (*OnEnabled)(void*),
                        void (*OnDisabled)(void*))
    : m_clientHandle(clienthandle),
      m_OnCompiledAndLinked(OnCompiledAndLinked),
      m_OnEnabled(OnEnabled),
      m_OnDisabled(OnDisabled)
  {
  }

  CAddonGLShaderProgram(void* clienthandle,
                        void (*OnCompiledAndLinked)(void*),
                        bool (*OnEnabled)(void*),
                        void (*OnDisabled)(void*),
                        const std::string& vert, const std::string& frag,
                        const char* base_path)
    : Shaders::CGLSLShaderProgram(vert, frag, base_path),
      m_clientHandle(clienthandle),
      m_OnCompiledAndLinked(OnCompiledAndLinked),
      m_OnEnabled(OnEnabled),
      m_OnDisabled(OnDisabled)
  {
  }

  void OnCompiledAndLinked() override { m_OnCompiledAndLinked(m_clientHandle); }
  bool OnEnabled() override { return m_OnEnabled(m_clientHandle); }
  void OnDisabled() override { m_OnDisabled(m_clientHandle); }
  void Free() { Shaders::CGLSLShaderProgram::Free(); }

private:
  void* m_clientHandle;
  void (*m_OnCompiledAndLinked)(void*);
  bool (*m_OnEnabled)(void*);
  void (*m_OnDisabled)(void*);
};
#endif

void* Interface_GUIShaderGL::shader_program_create(void* kodiBase, void* clienthandle,
                                                 void (*OnCompiledAndLinked)(void*),
                                                 bool (*OnEnabled)(void*),
                                                 void (*OnDisabled)(void*),
                                                 const char* vert, const char* frag,
                                                 const char* base_path)
{
#if defined(HAS_GL) || HAS_GLES >= 2
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIShaderGL::{} - invalid kodi base data", __FUNCTION__);
    return nullptr;
  }

  if (vert && frag)
    return new CAddonGLShaderProgram(clienthandle, OnCompiledAndLinked, OnEnabled, OnDisabled, vert, frag, base_path);
  else
    return new CAddonGLShaderProgram(clienthandle, OnCompiledAndLinked, OnEnabled, OnDisabled);
#else
  return nullptr;
#endif
}

void Interface_GUIShaderGL::shader_program_destroy(void* kodiBase, void* handle)
{
#if defined(HAS_GL) || HAS_GLES >= 2
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIShaderGL::{} - invalid kodi base data", __FUNCTION__);
    return;
  }

  if (!handle)
  {
    CLog::Log(LOGERROR, "Interface_GUIShaderGL::{} - invalid handler data (handle='%p') on addon '{}'", __FUNCTION__, handle, addon->ID());
    return;
  }

  delete static_cast<CAddonGLShaderProgram*>(handle);
#endif
}

bool Interface_GUIShaderGL::shader_program_enable(void* kodiBase, void* handle)
{
#if defined(HAS_GL) || HAS_GLES >= 2
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIShaderGL::{} - invalid kodi base data", __FUNCTION__);
    return false;
  }

  if (!handle)
  {
    CLog::Log(LOGERROR, "Interface_GUIShaderGL::{} - invalid handler data (handle='%p') on addon '{}'", __FUNCTION__, handle, addon->ID());
    return false;
  }

  return static_cast<CAddonGLShaderProgram*>(handle)->Enable();
#else
  return false;
#endif
}

void Interface_GUIShaderGL::shader_program_disable(void* kodiBase, void* handle)
{
#if defined(HAS_GL) || HAS_GLES >= 2
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIShaderGL::{} - invalid kodi base data", __FUNCTION__);
    return;
  }

  if (!handle)
  {
    CLog::Log(LOGERROR, "Interface_GUIShaderGL::{} - invalid handler data (handle='%p') on addon '{}'", __FUNCTION__, handle, addon->ID());
    return;
  }

  static_cast<CAddonGLShaderProgram*>(handle)->Disable();
#endif
}

bool Interface_GUIShaderGL::shader_program_ok(void* kodiBase, void* handle)
{
#if defined(HAS_GL) || HAS_GLES >= 2
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIShaderGL::{} - invalid kodi base data", __FUNCTION__);
    return false;
  }

  if (!handle)
  {
    CLog::Log(LOGERROR, "Interface_GUIShaderGL::{} - invalid handler data (handle='%p') on addon '{}'", __FUNCTION__, handle, addon->ID());
    return false;
  }

  return static_cast<CAddonGLShaderProgram*>(handle)->OK();
#else
  return false;
#endif
}

void* Interface_GUIShaderGL::shader_program_vertex_shader(void* kodiBase, void* handle)
{
#if defined(HAS_GL) || HAS_GLES >= 2
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIShaderGL::{} - invalid kodi base data", __FUNCTION__);
    return nullptr;
  }

  if (!handle)
  {
    CLog::Log(LOGERROR, "Interface_GUIShaderGL::{} - invalid handler data (handle='%p') on addon '{}'", __FUNCTION__, handle, addon->ID());
    return nullptr;
  }

  return static_cast<CAddonGLShaderProgram*>(handle)->VertexShader();
#else
  return nullptr;
#endif
}

void* Interface_GUIShaderGL::shader_program_pixel_shader(void* kodiBase, void* handle)
{
#if defined(HAS_GL) || HAS_GLES >= 2
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIShaderGL::{} - invalid kodi base data", __FUNCTION__);
    return nullptr;
  }

  if (!handle)
  {
    CLog::Log(LOGERROR, "Interface_GUIShaderGL::{} - invalid handler data (handle='%p') on addon '{}'", __FUNCTION__, handle, addon->ID());
    return nullptr;
  }

  return static_cast<CAddonGLShaderProgram*>(handle)->PixelShader();
#else
  return nullptr;
#endif
}

bool Interface_GUIShaderGL::shader_program_compile_and_link(void* kodiBase, void* handle)
{
#if defined(HAS_GL) || HAS_GLES >= 2
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIShaderGL::{} - invalid kodi base data", __FUNCTION__);
    return false;
  }

  if (!handle)
  {
    CLog::Log(LOGERROR, "Interface_GUIShaderGL::{} - invalid handler data (handle='%p') on addon '{}'", __FUNCTION__, handle, addon->ID());
    return false;
  }

  return static_cast<CAddonGLShaderProgram*>(handle)->CompileAndLink();
#else
  return false;
#endif
}

unsigned int Interface_GUIShaderGL::shader_program_program_handle(void* kodiBase, void* handle)
{
#if defined(HAS_GL) || HAS_GLES >= 2
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIShaderGL::{} - invalid kodi base data", __FUNCTION__);
    return 0;
  }

  if (!handle)
  {
    CLog::Log(LOGERROR, "Interface_GUIShaderGL::{} - invalid handler data (handle='%p') on addon '{}'", __FUNCTION__, handle, addon->ID());
    return 0;
  }

  return static_cast<CAddonGLShaderProgram*>(handle)->ProgramHandle();
#else
  return 0;
#endif
}

void shader_program_free(void* kodiBase, void* handle)
{
#if defined(HAS_GL) || HAS_GLES >= 2
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIShaderGL::{} - invalid kodi base data", __FUNCTION__);
    return;
  }

  if (!handle)
  {
    CLog::Log(LOGERROR, "Interface_GUIShaderGL::{} - invalid handler data (handle='%p') on addon '{}'", __FUNCTION__, handle, addon->ID());
    return;
  }

  static_cast<CAddonGLShaderProgram*>(handle)->Free();
#endif
}

} /* namespace ADDON */
} /* extern "C" */
