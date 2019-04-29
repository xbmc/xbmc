/*
 *  Copyright (C) 2005-2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "GL.h"

#include <stdio.h>
#include <vector>
#include <string>

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
  virtual bool Compile(const std::string& extraBegin = "",
                       const std::string& extraEnd = "") = 0;
  virtual void Free() = 0;
  virtual GLuint Handle() = 0;

  bool LoadSource(const std::string& file)
  {
    char buffer[16384];

    kodi::vfs::CFile source;
    if (!source.OpenFile(file))
    {
      kodi::Log(ADDON_LOG_ERROR, "CShader::%s: Failed to open file '%s'", __FUNCTION__, file.c_str());
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

  bool Compile(const std::string& extraBegin = "",
               const std::string& extraEnd = "") override
  {
    GLint params[4];

    Free();

    m_vertexShader = glCreateShader(GL_VERTEX_SHADER);

    GLsizei count = 0;
    const char *sources[3];
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

  bool Compile(const std::string& extraBegin = "",
               const std::string& extraEnd = "") override
  {
    GLint params[4];

    Free();

    m_pixelShader = glCreateShader(GL_FRAGMENT_SHADER);

    GLsizei count = 0;
    const char *sources[3];
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

//========================================================================
/// CShaderProgram
class ATTRIBUTE_HIDDEN CShaderProgram
{
public:
  CShaderProgram() = default;
  CShaderProgram(const std::string &vert, const std::string &frag)
  {
    LoadShaderFiles(vert, frag);
  }

  virtual ~CShaderProgram()
  {
    ShaderFree();
  }

  bool LoadShaderFiles(const std::string &vert, const std::string &frag)
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
      fprintf(stderr, "CShaderProgram::%s: %s\n", __FUNCTION__, log);
      ShaderFree();
      return false;
    }

    m_validated = false;
    m_ok = true;
    OnCompiledAndLinked();
    return true;
  }

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

  void DisableShader()
  {
    if (ShaderOK())
    {
      glUseProgram(0);
      OnDisabled();
    }
  }

  ATTRIBUTE_FORCEINLINE bool ShaderOK() const { return m_ok; }
  ATTRIBUTE_FORCEINLINE CVertexShader& VertexShader() { return m_pVP; }
  ATTRIBUTE_FORCEINLINE CPixelShader& PixelShader() { return m_pFP; }
  ATTRIBUTE_FORCEINLINE GLuint ProgramHandle() { return m_shaderProgram; }

  virtual void OnCompiledAndLinked() {};
  virtual bool OnEnabled() { return false; };
  virtual void OnDisabled() {};

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
