/*
 *  Copyright (C) 2005-2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "GL.h"

#ifdef __cplusplus

#include <stdio.h>
#include <string>
#include <vector>

#include <kodi/AddonBase.h>
#include <kodi/Filesystem.h>

#define LOG_SIZE 1024
#define GLchar char

namespace kodi
{
namespace gui
{
namespace gl
{

//========================================================================
/// CShader - base class
class ATTRIBUTE_HIDDEN CShader
{
public:
  CShader() = default;
  virtual ~CShader() = default;
  virtual bool Compile(const std::string& extraBegin = "", const std::string& extraEnd = "") = 0;
  virtual void Free() = 0;
  virtual GLuint Handle() = 0;

  bool LoadSource(const std::string& file)
  {
    char buffer[16384];

    kodi::vfs::CFile source;
    if (!source.OpenFile(file))
    {
      kodi::Log(ADDON_LOG_ERROR, "CShader::%s: Failed to open file '%s'", __FUNCTION__,
                file.c_str());
      return false;
    }
    size_t len = source.Read(buffer, sizeof(buffer));
    m_source.assign(buffer);
    m_source[len] = 0;
    source.Close();
    return true;
  }

  bool OK() const { return m_compiled; }

protected:
  std::string m_source;
  std::string m_lastLog;
  bool m_compiled = false;
};
//------------------------------------------------------------------------

//========================================================================
/// CVertexShader
class ATTRIBUTE_HIDDEN CVertexShader : public CShader
{
public:
  CVertexShader() = default;
  ~CVertexShader() override { Free(); }

  void Free() override
  {
    if (m_vertexShader)
      glDeleteShader(m_vertexShader);
    m_vertexShader = 0;
  }

  bool Compile(const std::string& extraBegin = "", const std::string& extraEnd = "") override
  {
    GLint params[4];

    Free();

    m_vertexShader = glCreateShader(GL_VERTEX_SHADER);

    GLsizei count = 0;
    const char* sources[3];
    if (!extraBegin.empty())
      sources[count++] = extraBegin.c_str();
    if (!m_source.empty())
      sources[count++] = m_source.c_str();
    if (!extraEnd.empty())
      sources[count++] = extraEnd.c_str();

    glShaderSource(m_vertexShader, count, sources, nullptr);
    glCompileShader(m_vertexShader);
    glGetShaderiv(m_vertexShader, GL_COMPILE_STATUS, params);
    if (params[0] != GL_TRUE)
    {
      GLchar log[LOG_SIZE];
      glGetShaderInfoLog(m_vertexShader, LOG_SIZE, nullptr, log);
      kodi::Log(ADDON_LOG_ERROR, "CVertexShader::%s: %s", __FUNCTION__, log);
      fprintf(stderr, "CVertexShader::%s: %s\n", __FUNCTION__, log);
      m_lastLog = log;
      m_compiled = false;
    }
    else
    {
      GLchar log[LOG_SIZE];
      glGetShaderInfoLog(m_vertexShader, LOG_SIZE, nullptr, log);
      m_lastLog = log;
      m_compiled = true;
    }
    return m_compiled;
  }

  GLuint Handle() override { return m_vertexShader; }

protected:
  GLuint m_vertexShader = 0;
};
//------------------------------------------------------------------------

//========================================================================
/// CPixelShader
class ATTRIBUTE_HIDDEN CPixelShader : public CShader
{
public:
  CPixelShader() = default;
  ~CPixelShader() { Free(); }
  void Free() override
  {
    if (m_pixelShader)
      glDeleteShader(m_pixelShader);
    m_pixelShader = 0;
  }

  bool Compile(const std::string& extraBegin = "", const std::string& extraEnd = "") override
  {
    GLint params[4];

    Free();

    m_pixelShader = glCreateShader(GL_FRAGMENT_SHADER);

    GLsizei count = 0;
    const char* sources[3];
    if (!extraBegin.empty())
      sources[count++] = extraBegin.c_str();
    if (!m_source.empty())
      sources[count++] = m_source.c_str();
    if (!extraEnd.empty())
      sources[count++] = extraEnd.c_str();

    glShaderSource(m_pixelShader, count, sources, 0);
    glCompileShader(m_pixelShader);
    glGetShaderiv(m_pixelShader, GL_COMPILE_STATUS, params);
    if (params[0] != GL_TRUE)
    {
      GLchar log[LOG_SIZE];
      glGetShaderInfoLog(m_pixelShader, LOG_SIZE, nullptr, log);
      kodi::Log(ADDON_LOG_ERROR, "CPixelShader::%s: %s", __FUNCTION__, log);
      fprintf(stderr, "CPixelShader::%s: %s\n", __FUNCTION__, log);
      m_lastLog = log;
      m_compiled = false;
    }
    else
    {
      GLchar log[LOG_SIZE];
      glGetShaderInfoLog(m_pixelShader, LOG_SIZE, nullptr, log);
      m_lastLog = log;
      m_compiled = true;
    }
    return m_compiled;
  }

  GLuint Handle() override { return m_pixelShader; }

protected:
  GLuint m_pixelShader = 0;
};
//------------------------------------------------------------------------

//============================================================================
/// @defgroup cpp_kodi_gui_helpers_gl_CShaderProgram GL Shader Program
/// @ingroup cpp_kodi_gui_helpers_gl
/// @brief @cpp_class{ kodi::gui::gl::CShaderProgram }
/// **Class to manage an OpenGL shader program**\n
/// With this class the used GL shader code can be defined on the GPU and
/// its variables can be managed between CPU and GPU.
///
/// It has the header @ref Shader.h "#include <kodi/gui/gl/Shader.h>"
/// be included to enjoy it.
///
/// ----------------------------------------------------------------------------
///
/// <b>Example:</b>
///
/// ~~~~~~~~~~~~~{.cpp}
///
/// #include <kodi/gui/gl/Shader.h>
/// ...
///
/// class ATTRIBUTE_HIDDEN CExample
///   : ...,
///     public kodi::gui::gl::CShaderProgram
/// {
/// public:
///   CExample() = default;
///
///   bool Start();
///   void Render();
///
///   // override functions for kodi::gui::gl::CShaderProgram
///   void OnCompiledAndLinked() override;
///   bool OnEnabled() override;
///
/// private:
///   ...
///   GLint m_aPosition = -1;
///   GLint m_aColor = -1;
/// };
///
/// bool CExample::Start()
/// {
///   // Define shaders and load
///   std::string fraqShader = kodi::GetAddonPath("resources/shaders/" GL_TYPE_STRING "/glsl.frag");
///   std::string vertShader = kodi::GetAddonPath("resources/shaders/" GL_TYPE_STRING "/glsl.vert");
///   if (!LoadShaderFiles(vertShader, fraqShader) || !CompileAndLink())
///     return false;
///
///   ...
///   return true;
/// }
///
/// ...
///
/// void CExample::Render()
/// {
///   ...
///
///   EnableShader();
///   ...
///   DO WORK
///   ...
///   DisableShader();
/// }
///
/// void CExample::OnCompiledAndLinked()
/// {
///   ...
///   DO YOUR WORK HERE FOR WHAT IS ONE TIME REQUIRED DURING COMPILE OF SHADER, E.G.:
///
///   m_aPosition = glGetAttribLocation(ProgramHandle(), "a_position");
///   m_aColor = glGetAttribLocation(ProgramHandle(), "a_color");
/// }
///
/// bool OnEnabled() override
/// {
///   ...
///   DO YOUR WORK HERE FOR WHAT REQUIRED DURING ENABLE OF SHADER
///   ...
///   return true;
/// }
///
/// ADDONCREATOR(CExample);
/// ~~~~~~~~~~~~~
///
class ATTRIBUTE_HIDDEN CShaderProgram
{
public:
  //==========================================================================
  /// @ingroup cpp_kodi_gui_helpers_gl_CShaderProgram
  /// @brief Construct a new shader.
  ///
  /// Load must be done later with @ref LoadShaderFiles.
  ///
  CShaderProgram() = default;
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @ingroup cpp_kodi_gui_helpers_gl_CShaderProgram
  /// @brief Construct a new shader and load defined shader files.
  ///
  /// @param[in] vert Path to used GL vertext shader
  /// @param[in] frag Path to used GL fragment shader
  ///
  CShaderProgram(const std::string& vert, const std::string& frag) { LoadShaderFiles(vert, frag); }
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @ingroup cpp_kodi_gui_helpers_gl_CShaderProgram
  /// @brief Destructor.
  ///
  virtual ~CShaderProgram() { ShaderFree(); }
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @ingroup cpp_kodi_gui_helpers_gl_CShaderProgram
  /// @brief To load manually the needed shader files.
  ///
  /// @param[in] vert Path to used GL vertext shader
  /// @param[in] frag Path to used GL fragment shader
  ///
  ///
  /// @note The use of the files is optional, but it must either be passed over
  /// here or via @ref CompileAndLink, or both of the source code.
  ///
  bool LoadShaderFiles(const std::string& vert, const std::string& frag)
  {
    if (!kodi::vfs::FileExists(vert) || !m_pVP.LoadSource(vert))
    {
      kodi::Log(ADDON_LOG_ERROR, "%s: Failed to load '%s'", __func__, vert.c_str());
      return false;
    }

    if (!kodi::vfs::FileExists(frag) || !m_pFP.LoadSource(frag))
    {
      kodi::Log(ADDON_LOG_ERROR, "%s: Failed to load '%s'", __func__, frag.c_str());
      return false;
    }

    return true;
  }
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @ingroup cpp_kodi_gui_helpers_gl_CShaderProgram
  /// @brief To compile and link the shader to the GL interface.
  ///
  /// Optionally, additional source code can be transferred here, or it can be
  /// used independently without any files
  ///
  /// @param[in] vertexExtraBegin   [opt] To additionally add vextex source
  ///                               code to the beginning of the loaded file
  ///                               source code
  /// @param[in] vertexExtraEnd     [opt] To additionally add vextex source
  ///                               code to the end of the loaded file
  ///                               source code
  /// @param[in] fragmentExtraBegin [opt] To additionally add fragment source
  ///                               code to the beginning of the loaded file
  ///                               source code
  /// @param[in] fragmentExtraEnd   [opt] To additionally add fragment source
  ///                               code to the end of the loaded file
  ///                               source code
  /// @return                       true if compile was successed
  ///
  ///
  /// @note In the case of a compile error, it will be written once into the Kodi
  /// log and in addition to the console output to quickly detect the errors when
  /// writing the damage.
  ///
  ///
  bool CompileAndLink(const std::string& vertexExtraBegin = "",
                      const std::string& vertexExtraEnd = "",
                      const std::string& fragmentExtraBegin = "",
                      const std::string& fragmentExtraEnd = "")
  {
    GLint params[4];

    // free resources
    ShaderFree();
    m_ok = false;

    // compiled vertex shader
    if (!m_pVP.Compile(vertexExtraBegin, vertexExtraEnd))
    {
      kodi::Log(ADDON_LOG_ERROR, "GL: Error compiling vertex shader");
      return false;
    }

    // compile pixel shader
    if (!m_pFP.Compile(fragmentExtraBegin, fragmentExtraEnd))
    {
      m_pVP.Free();
      kodi::Log(ADDON_LOG_ERROR, "GL: Error compiling fragment shader");
      return false;
    }

    // create program object
    m_shaderProgram = glCreateProgram();
    if (!m_shaderProgram)
    {
      kodi::Log(ADDON_LOG_ERROR, "CShaderProgram::%s: Failed to create GL program", __FUNCTION__);
      ShaderFree();
      return false;
    }

    // attach the vertex shader
    glAttachShader(m_shaderProgram, m_pVP.Handle());
    glAttachShader(m_shaderProgram, m_pFP.Handle());

    // link the program
    glLinkProgram(m_shaderProgram);
    glGetProgramiv(m_shaderProgram, GL_LINK_STATUS, params);
    if (params[0] != GL_TRUE)
    {
      GLchar log[LOG_SIZE];
      glGetProgramInfoLog(m_shaderProgram, LOG_SIZE, nullptr, log);
      kodi::Log(ADDON_LOG_ERROR, "CShaderProgram::%s: %s", __FUNCTION__, log);
      fprintf(stderr, "CShaderProgram::%s: %s@n", __FUNCTION__, log);
      ShaderFree();
      return false;
    }

    m_validated = false;
    m_ok = true;
    OnCompiledAndLinked();
    return true;
  }
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @ingroup cpp_kodi_gui_helpers_gl_CShaderProgram
  /// @brief To activate the shader and use it on the GPU.
  ///
  /// @return true if enable was successfull done
  ///
  ///
  /// @note During this call, the @ref OnEnabled stored in the child is also
  /// called
  ///
  bool EnableShader()
  {
    if (ShaderOK())
    {
      glUseProgram(m_shaderProgram);
      if (OnEnabled())
      {
        if (!m_validated)
        {
          // validate the program
          GLint params[4];
          glValidateProgram(m_shaderProgram);
          glGetProgramiv(m_shaderProgram, GL_VALIDATE_STATUS, params);
          if (params[0] != GL_TRUE)
          {
            GLchar log[LOG_SIZE];
            glGetProgramInfoLog(m_shaderProgram, LOG_SIZE, nullptr, log);
            kodi::Log(ADDON_LOG_ERROR, "CShaderProgram::%s: %s", __FUNCTION__, log);
            fprintf(stderr, "CShaderProgram::%s: %s\n", __FUNCTION__, log);
          }
          m_validated = true;
        }
        return true;
      }
      else
      {
        glUseProgram(0);
        return false;
      }
      return true;
    }
    return false;
  }
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @ingroup cpp_kodi_gui_helpers_gl_CShaderProgram
  /// @brief To deactivate the shader use on the GPU.
  ///
  void DisableShader()
  {
    if (ShaderOK())
    {
      glUseProgram(0);
      OnDisabled();
    }
  }
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @ingroup cpp_kodi_gui_helpers_gl_CShaderProgram
  /// @brief Used to check if shader has been loaded before.
  ///
  /// @return true if enable was successfull done
  ///
  /// @note The CompileAndLink call sets these values
  ///
  ATTRIBUTE_FORCEINLINE bool ShaderOK() const { return m_ok; }
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @ingroup cpp_kodi_gui_helpers_gl_CShaderProgram
  /// @brief To get the vertex shader class used by Kodi at the addon.
  ///
  /// @return pointer to vertex shader class
  ///
  ATTRIBUTE_FORCEINLINE CVertexShader& VertexShader() { return m_pVP; }
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @ingroup cpp_kodi_gui_helpers_gl_CShaderProgram
  /// @brief To get the fragment shader class used by Kodi at the addon.
  ///
  /// @return pointer to fragment shader class
  ///
  ATTRIBUTE_FORCEINLINE CPixelShader& PixelShader() { return m_pFP; }
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @ingroup cpp_kodi_gui_helpers_gl_CShaderProgram
  /// @brief Used to get the definition created in the OpenGL itself.
  ///
  /// @return GLuint of GL shader program handler
  ///
  ATTRIBUTE_FORCEINLINE GLuint ProgramHandle() { return m_shaderProgram; }
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @defgroup cpp_kodi_gui_helpers_gl_CShaderProgram_child Child Functions
  /// @ingroup cpp_kodi_gui_helpers_gl_CShaderProgram
  /// @brief @cpp_class{ kodi::gui::gl::CShaderProgram child functions }
  ///
  /// Functions that are added by parent in the child
  /// @{
  //==========================================================================
  ///
  /// @ingroup cpp_kodi_gui_helpers_gl_CShaderProgram_child
  /// @brief Mandatory child function to set the necessary CPU to GPU data
  ///
  virtual void OnCompiledAndLinked(){};
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @ingroup cpp_kodi_gui_helpers_gl_CShaderProgram_child
  /// @brief Optional function to exchange data between CPU and GPU while
  /// activating the shader
  ///
  /// @return true if enable was successfull done
  ///
  virtual bool OnEnabled() { return true; };
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @ingroup cpp_kodi_gui_helpers_gl_CShaderProgram_child
  /// @brief Optional child function that may have to be performed when
  /// switching off the shader
  virtual void OnDisabled(){};
  //--------------------------------------------------------------------------
  /// @}

private:
  void ShaderFree()
  {
    if (m_shaderProgram)
      glDeleteProgram(m_shaderProgram);
    m_shaderProgram = 0;
    m_ok = false;
  }

  CVertexShader m_pVP;
  CPixelShader m_pFP;
  GLuint m_shaderProgram = 0;
  bool m_ok = false;
  bool m_validated = false;
};
//------------------------------------------------------------------------

} /* namespace gl */
} /* namespace gui */
} /* namespace kodi */

#endif /* __cplusplus */
