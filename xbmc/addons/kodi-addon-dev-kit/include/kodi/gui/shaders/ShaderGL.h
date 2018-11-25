/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "../definitions.h"
#include "../../AddonBase.h"

#ifdef HAS_GL
  // always define GL_GLEXT_PROTOTYPES before include gl headers
  #if !defined(GL_GLEXT_PROTOTYPES)
    #define GL_GLEXT_PROTOTYPES
  #endif
  #if defined(__APPLE__)
    #include <OpenGL/gl3.h>
    #include <OpenGL/glu.h>
    #include <OpenGL/gl3ext.h>
  #else
    #include <GL/gl.h>
    #include <GL/glu.h>
    #include <GL/glext.h>
  #endif
#elif HAS_GLES >= 2
  #if defined(__APPLE__)
    #include <OpenGLES/ES2/gl.h>
    #include <OpenGLES/ES2/glext.h>
  #else
    #include <GLES2/gl2.h>
    #include <GLES2/gl2ext.h>
  #endif
  #if HAS_GLES == 3
    #include <GLES3/gl3.h>
  #endif
#endif

namespace kodi
{
namespace gui
{
namespace gl
{

  enum SHADERMETHODS
  {
    SHADER_DEFAULT = 0,
    SHADER_TEXTURE,
#if HAS_GL
    SHADER_TEXTURE_LIM,
#endif
    SHADER_MULTI,
    SHADER_FONTS,
    SHADER_TEXTURE_NOBLEND,
    SHADER_MULTI_BLENDCOLOR,
#if HAS_GLES >= 2
    SHADER_TEXTURE_RGBA,
    SHADER_TEXTURE_RGBA_OES,
    SHADER_TEXTURE_RGBA_BLENDCOLOR,
    SHADER_TEXTURE_RGBA_BOB,
    SHADER_TEXTURE_RGBA_BOB_OES,
#endif
  };

  class ATTRIBUTE_HIDDEN CPresentShader
  {
    public:
      inline static void EnableShader(SHADERMETHODS method)
      {
        using namespace ::kodi::addon;
        CAddonBase::m_interface->toKodi->kodi_gui->gl_shader->present_enable(CAddonBase::m_interface->toKodi->kodiBase, method);
      }

      inline static void DisableShader()
      {
        using namespace ::kodi::addon;
        CAddonBase::m_interface->toKodi->kodi_gui->gl_shader->present_disable(CAddonBase::m_interface->toKodi->kodiBase);
      }

      inline static int ShaderGetPos()
      {
        using namespace ::kodi::addon;
        return CAddonBase::m_interface->toKodi->kodi_gui->gl_shader->present_get_pos(CAddonBase::m_interface->toKodi->kodiBase);
      }

      inline static int ShaderGetCol()
      {
        using namespace ::kodi::addon;
        return CAddonBase::m_interface->toKodi->kodi_gui->gl_shader->present_get_col(CAddonBase::m_interface->toKodi->kodiBase);
      }

      inline static int ShaderGetCoord0()
      {
        using namespace ::kodi::addon;
        return CAddonBase::m_interface->toKodi->kodi_gui->gl_shader->present_get_coord0(CAddonBase::m_interface->toKodi->kodiBase);
      }

      inline static int ShaderGetCoord1()
      {
        using namespace ::kodi::addon;
        return CAddonBase::m_interface->toKodi->kodi_gui->gl_shader->present_get_coord1(CAddonBase::m_interface->toKodi->kodiBase);
      }

      inline static int ShaderGetUniCol()
      {
        using namespace ::kodi::addon;
        return CAddonBase::m_interface->toKodi->kodi_gui->gl_shader->present_get_uni_col(CAddonBase::m_interface->toKodi->kodiBase);
      }

      inline static int ShaderGetModel()
      {
        using namespace ::kodi::addon;
        return CAddonBase::m_interface->toKodi->kodi_gui->gl_shader->present_get_model(CAddonBase::m_interface->toKodi->kodiBase);
      }
  };

  typedef enum GLShaderType
  {
    SHADER_VERTEX = 0,
    SHADER_PIXEL = 1,
  } GLShaderType;

  class CShaderProgram;

  class ATTRIBUTE_HIDDEN CShader
  {
  public:
    explicit CShader(GLShaderType type)
      : m_prevent_delete(false),
        m_interface(::kodi::addon::CAddonBase::m_interface->toKodi)
    {
      m_shaderHandle = m_interface->kodi_gui->gl_shader->shader_create(m_interface->kodiBase, type);
      if (!m_shaderHandle)
        kodi::Log(ADDON_LOG_FATAL, "kodi::gui::CShader can't create shader class from Kodi !!!");
    }

    virtual ~CShader()
    {
      if (m_shaderHandle && !m_prevent_delete)
        m_interface->kodi_gui->gl_shader->shader_destroy(m_interface->kodiBase, m_shaderHandle);
    }

    bool Compile()
    {
      return m_interface->kodi_gui->gl_shader->shader_compile(m_interface->kodiBase, m_shaderHandle);
    }

    void Free()
    {
      m_interface->kodi_gui->gl_shader->shader_free(m_interface->kodiBase, m_shaderHandle);
    }

    unsigned int Handle()
    {
      return m_interface->kodi_gui->gl_shader->shader_handle(m_interface->kodiBase, m_shaderHandle);
    }

    void SetSource(const std::string& src)
    {
      m_interface->kodi_gui->gl_shader->shader_set_source(m_interface->kodiBase, m_shaderHandle, src.c_str());
    }

    void LoadSource(const std::string& filename, const std::string& prefix = "", const std::string& basePath = "")
    {
      m_interface->kodi_gui->gl_shader->shader_load_source(m_interface->kodiBase, m_shaderHandle, filename.c_str(), prefix.c_str(), basePath.c_str());
    }

    bool AppendSource(const std::string& filename, const std::string& basePath = "")
    {
      return m_interface->kodi_gui->gl_shader->shader_append_source(m_interface->kodiBase, m_shaderHandle, filename.c_str(), basePath.c_str());
    }

    bool InsertSource(const std::string& filename, const std::string& loc, const std::string& basePath = "")
    {
      return m_interface->kodi_gui->gl_shader->shader_insert_source(m_interface->kodiBase, m_shaderHandle, filename.c_str(), loc.c_str(), basePath.c_str());
    }

    bool OK()
    {
      return m_interface->kodi_gui->gl_shader->shader_ok(m_interface->kodiBase, m_shaderHandle);
    }

  private:
    friend class CShaderProgram;

    CShader(void* shader)
      : m_prevent_delete(true),
        m_shaderHandle(shader),
        m_interface(::kodi::addon::CAddonBase::m_interface->toKodi)
    {
    }

    bool m_prevent_delete;
    void* m_shaderHandle;
    AddonToKodiFuncTable_Addon* m_interface;
  };


  class ATTRIBUTE_HIDDEN CShaderProgram
  {
  public:
    CShaderProgram()
      : m_interface(::kodi::addon::CAddonBase::m_interface->toKodi)
    {
      m_shaderProgram = m_interface->kodi_gui->gl_shader->shader_program_create(m_interface->kodiBase, this,
                                                                             OnCompiledAndLinkedCB,
                                                                             OnEnabledCB,
                                                                             OnDisabledCB,
                                                                             nullptr, nullptr,
                                                                             nullptr);
      if (!m_shaderProgram)
        kodi::Log(ADDON_LOG_FATAL, "kodi::gui::CShaderProgram can't create shader class from Kodi !!!");
      else
      {
        m_pVP = new CShader(m_interface->kodi_gui->gl_shader->shader_program_vertex_shader(m_interface->kodiBase, m_shaderProgram));
        m_pFP = new CShader(m_interface->kodi_gui->gl_shader->shader_program_pixel_shader(m_interface->kodiBase, m_shaderProgram));
      }
    }

    CShaderProgram(const std::string& vert, const std::string& frag, const std::string& basePath = "")
      : m_interface(::kodi::addon::CAddonBase::m_interface->toKodi)
    {
      m_shaderProgram = m_interface->kodi_gui->gl_shader->shader_program_create(m_interface->kodiBase, this,
                                                                             OnCompiledAndLinkedCB,
                                                                             OnEnabledCB,
                                                                             OnDisabledCB,
                                                                             vert.c_str(), frag.c_str(),
                                                                             basePath.c_str());
      if (!m_shaderProgram)
        kodi::Log(ADDON_LOG_FATAL, "kodi::gui::CShaderProgram can't create shader class from Kodi !!!");
      else
      {
        m_pVP = new CShader(m_interface->kodi_gui->gl_shader->shader_program_vertex_shader(m_interface->kodiBase, m_shaderProgram));
        m_pFP = new CShader(m_interface->kodi_gui->gl_shader->shader_program_pixel_shader(m_interface->kodiBase, m_shaderProgram));
      }
    }

    virtual ~CShaderProgram()
    {
      if (m_shaderProgram)
        m_interface->kodi_gui->gl_shader->shader_program_destroy(m_interface->kodiBase, m_shaderProgram);

      delete m_pFP;
      delete m_pVP;
    }

    bool Enable()
    {
      return m_interface->kodi_gui->gl_shader->shader_program_enable(m_interface->kodiBase, m_shaderProgram);
    }

    void Disable()
    {
      m_interface->kodi_gui->gl_shader->shader_program_disable(m_interface->kodiBase, m_shaderProgram);
    }

    bool OK()
    {
      return m_interface->kodi_gui->gl_shader->shader_program_ok(m_interface->kodiBase, m_shaderProgram);
    }

    CShader* VertexShader()
    {
      return m_pVP;
    }

    CShader* PixelShader()
    {
      return m_pFP;
    }

    bool CompileAndLink()
    {
      return m_interface->kodi_gui->gl_shader->shader_program_compile_and_link(m_interface->kodiBase, m_shaderProgram);
    }

    unsigned int ProgramHandle()
    {
      return m_interface->kodi_gui->gl_shader->shader_program_program_handle(m_interface->kodiBase, m_shaderProgram);
    }

    virtual void OnCompiledAndLinked() { }

    virtual bool OnEnabled() { return true; }

    virtual void OnDisabled() { }

  protected:
    void Free()
    {
      m_interface->kodi_gui->gl_shader->shader_program_free(m_interface->kodiBase, m_shaderProgram);
    }

  private:
    /*
     * Defined callback functions from Kodi to add-on, for use in parent / child system
     * (is private)!
     */
    inline static void OnCompiledAndLinkedCB(void* cbhdl)
    {
      static_cast<CShaderProgram*>(cbhdl)->OnCompiledAndLinked();
    }

    inline static bool OnEnabledCB(void* cbhdl)
    {
      return static_cast<CShaderProgram*>(cbhdl)->OnEnabled();
    }

    inline static void OnDisabledCB(void* cbhdl)
    {
      static_cast<CShaderProgram*>(cbhdl)->OnDisabled();
    }

    CShader* m_pVP = nullptr;
    CShader* m_pFP = nullptr;

    void* m_shaderProgram;
    AddonToKodiFuncTable_Addon* m_interface;
  };


} /* namespace gl */
} /* namespace gui */
} /* namespace kodi */
