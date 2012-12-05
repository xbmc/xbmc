/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "system.h"

#if defined(HAS_GL) || HAS_GLES == 2

#include "Shader.h"
#include "settings/Settings.h"
#include "filesystem/File.h"
#include "utils/log.h"
#include "utils/GLUtils.h"

#ifdef HAS_GLES
#define GLchar char
#endif

#define LOG_SIZE 1024

using namespace Shaders;
using namespace XFILE;
using namespace std;

//////////////////////////////////////////////////////////////////////
// CShader
//////////////////////////////////////////////////////////////////////
bool CShader::LoadSource(const string& filename, const string& prefix)
{
  if(filename.empty())
    return true;

  CFileStream file;

  if(!file.Open("special://xbmc/system/shaders/" + filename))
  {
    CLog::Log(LOGERROR, "CYUVShaderGLSL::CYUVShaderGLSL - failed to open file %s", filename.c_str());
    return false;
  }
  getline(file, m_source, '\0');
  m_source.insert(0, prefix);
  return true;
}

//////////////////////////////////////////////////////////////////////
// CGLSLVertexShader
//////////////////////////////////////////////////////////////////////

bool CGLSLVertexShader::Compile()
{
  GLint params[4];

  Free();

#ifdef HAS_GL
  if(!GLEW_VERSION_2_0)
  {
    CLog::Log(LOGERROR, "GL: GLSL vertex shaders not supported");
    return false;
  }
#endif

  m_vertexShader = glCreateShader(GL_VERTEX_SHADER);
  const char *ptr = m_source.c_str();
  glShaderSource(m_vertexShader, 1, &ptr, 0);
  glCompileShader(m_vertexShader);
  glGetShaderiv(m_vertexShader, GL_COMPILE_STATUS, params);
  VerifyGLState();
  if (params[0]!=GL_TRUE)
  {
    GLchar log[LOG_SIZE];
    CLog::Log(LOGERROR, "GL: Error compiling vertex shader");
    glGetShaderInfoLog(m_vertexShader, LOG_SIZE, NULL, log);
    CLog::Log(LOGERROR, "%s", log);
    m_lastLog = log;
    m_compiled = false;
  }
  else
  {
    GLchar log[LOG_SIZE];
    CLog::Log(LOGDEBUG, "GL: Vertex Shader compilation log:");
    glGetShaderInfoLog(m_vertexShader, LOG_SIZE, NULL, log);
    CLog::Log(LOGDEBUG, "%s", log);
    m_lastLog = log;
    m_compiled = true;
  }
  return m_compiled;
}

void CGLSLVertexShader::Free()
{
#ifdef HAS_GL
  if(!GLEW_VERSION_2_0)
    return;
#endif

  if (m_vertexShader)
    glDeleteShader(m_vertexShader);
  m_vertexShader = 0;
}

#ifndef HAS_GLES

//////////////////////////////////////////////////////////////////////
// CARBVertexShader
//////////////////////////////////////////////////////////////////////
bool CARBVertexShader::Compile()
{
  GLint err = 0;

  Free();

  // Pixel shaders are not mandatory.
  if (m_source.length()==0)
  {
    CLog::Log(LOGNOTICE, "GL: No vertex shader, fixed pipeline in use");
    return true;
  }

  glEnable(GL_VERTEX_PROGRAM_ARB);
  glGenProgramsARB(1, &m_vertexShader);
  glBindProgramARB(GL_VERTEX_PROGRAM_ARB, m_vertexShader);

  glProgramStringARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
                     m_source.length(), m_source.c_str());

  glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &err);
  if (err>0)
  {
    CLog::Log(LOGERROR, "GL: Error compiling ARB vertex shader");
    m_compiled = false;
  }
  else
  {
    m_compiled = true;
  }
  glDisable(GL_VERTEX_PROGRAM_ARB);
  return m_compiled;
}

void CARBVertexShader::Free()
{
  if (m_vertexShader)
    glDeleteProgramsARB(1, &m_vertexShader);
  m_vertexShader = 0;
}
#endif

//////////////////////////////////////////////////////////////////////
// CGLSLPixelShader
//////////////////////////////////////////////////////////////////////
bool CGLSLPixelShader::Compile()
{
#ifdef HAS_GL
  if(!GLEW_VERSION_2_0)
  {
    CLog::Log(LOGERROR, "GL: GLSL pixel shaders not supported");
    return false;
  }
#endif

  GLint params[4];

  Free();

  // Pixel shaders are not mandatory.
  if (m_source.length()==0)
  {
    CLog::Log(LOGNOTICE, "GL: No pixel shader, fixed pipeline in use");
    return true;
  }

  m_pixelShader = glCreateShader(GL_FRAGMENT_SHADER);
  const char *ptr = m_source.c_str();
  glShaderSource(m_pixelShader, 1, &ptr, 0);
  glCompileShader(m_pixelShader);
  glGetShaderiv(m_pixelShader, GL_COMPILE_STATUS, params);
  if (params[0]!=GL_TRUE)
  {
    GLchar log[LOG_SIZE];
    CLog::Log(LOGERROR, "GL: Error compiling pixel shader");
    glGetShaderInfoLog(m_pixelShader, LOG_SIZE, NULL, log);
    CLog::Log(LOGERROR, "%s", log);
    m_lastLog = log;
    m_compiled = false;
  }
  else
  {
    GLchar log[LOG_SIZE];
    CLog::Log(LOGDEBUG, "GL: Pixel Shader compilation log:");
    glGetShaderInfoLog(m_pixelShader, LOG_SIZE, NULL, log);
    CLog::Log(LOGDEBUG, "%s", log);
    m_lastLog = log;
    m_compiled = true;
  }
  return m_compiled;
}

void CGLSLPixelShader::Free()
{
#ifdef HAS_GL
  if(!GLEW_VERSION_2_0)
    return;
#endif
  if (m_pixelShader)
    glDeleteShader(m_pixelShader);
  m_pixelShader = 0;
}

#ifndef HAS_GLES

//////////////////////////////////////////////////////////////////////
// CARBPixelShader
//////////////////////////////////////////////////////////////////////
bool CARBPixelShader::Compile()
{
  GLint err = 0;

  Free();

  // Pixel shaders are not mandatory.
  if (m_source.length()==0)
  {
    CLog::Log(LOGNOTICE, "GL: No pixel shader, fixed pipeline in use");
    return true;
  }

  glEnable(GL_FRAGMENT_PROGRAM_ARB);
  glGenProgramsARB(1, &m_pixelShader);
  glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, m_pixelShader);

  glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
                     m_source.length(), m_source.c_str());

  glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &err);
  if (err>0)
  {
    const char* errStr = (const char*)glGetString(GL_PROGRAM_ERROR_STRING_ARB);
    if (!errStr)
      errStr = "NULL";
    CLog::Log(LOGERROR, "GL: Error compiling ARB pixel shader, GL_PROGRAM_ERROR_STRING_ARB = %s", errStr);
    m_compiled = false;
  }
  else
  {
    m_compiled = true;
  }
  glDisable(GL_FRAGMENT_PROGRAM_ARB);
  return m_compiled;
}

void CARBPixelShader::Free()
{
  if (m_pixelShader)
    glDeleteProgramsARB(1, &m_pixelShader);
  m_pixelShader = 0;
}

#endif

//////////////////////////////////////////////////////////////////////
// CGLSLShaderProgram
//////////////////////////////////////////////////////////////////////
void CGLSLShaderProgram::Free()
{
#ifdef HAS_GL
  if(!GLEW_VERSION_2_0)
    return;
#endif
  m_pVP->Free();
  VerifyGLState();
  m_pFP->Free();
  VerifyGLState();
  if (m_shaderProgram)
  {
    glDeleteProgram(m_shaderProgram);
  }
  m_shaderProgram = 0;
  m_ok = false;
  m_lastProgram = 0;
}

bool CGLSLShaderProgram::CompileAndLink()
{
#ifdef HAS_GL
  // check that we support shaders
  if(!GLEW_VERSION_2_0)
  {
    CLog::Log(LOGERROR, "GL: GLSL shaders not supported");
    return false;
  }
#endif

  GLint params[4];

  // free resources
  Free();

  // compiled vertex shader
  if (!m_pVP->Compile())
  {
    CLog::Log(LOGERROR, "GL: Error compiling vertex shader");
    return false;
  }
  CLog::Log(LOGDEBUG, "GL: Vertex Shader compiled successfully");

  // compile pixel shader
  if (!m_pFP->Compile())
  {
    m_pVP->Free();
    CLog::Log(LOGERROR, "GL: Error compiling fragment shader");
    return false;
  }
  CLog::Log(LOGDEBUG, "GL: Fragment Shader compiled successfully");

  // create program object
  if (!(m_shaderProgram = glCreateProgram()))
  {
    CLog::Log(LOGERROR, "GL: Error creating shader program handle");
    goto error;
  }

  // attach the vertex shader
  glAttachShader(m_shaderProgram, m_pVP->Handle());
  VerifyGLState();

  // if we have a pixel shader, attach it. If not, fixed pipeline
  // will be used.
  if (m_pFP->Handle())
  {
    glAttachShader(m_shaderProgram, m_pFP->Handle());
    VerifyGLState();
  }

  // link the program
  glLinkProgram(m_shaderProgram);
  glGetProgramiv(m_shaderProgram, GL_LINK_STATUS, params);
  if (params[0]!=GL_TRUE)
  {
    GLchar log[LOG_SIZE];
    CLog::Log(LOGERROR, "GL: Error linking shader");
    glGetProgramInfoLog(m_shaderProgram, LOG_SIZE, NULL, log);
    CLog::Log(LOGERROR, "%s", log);
    goto error;
  }
  VerifyGLState();

  m_validated = false;
  m_ok = true;
  OnCompiledAndLinked();
  VerifyGLState();
  return true;

 error:
  m_ok = false;
  Free();
  return false;
}

bool CGLSLShaderProgram::Enable()
{
#ifdef HAS_GL
  if(!GLEW_VERSION_2_0)
    return false;
#endif

  if (OK())
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
        if (params[0]!=GL_TRUE)
        {
          GLchar log[LOG_SIZE];
          CLog::Log(LOGERROR, "GL: Error validating shader");
          glGetProgramInfoLog(m_shaderProgram, LOG_SIZE, NULL, log);
          CLog::Log(LOGERROR, "%s", log);
        }
        m_validated = true;
      }
      VerifyGLState();
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

void CGLSLShaderProgram::Disable()
{
#ifdef HAS_GL
  if(!GLEW_VERSION_2_0)
    return;
#endif

  if (OK())
  {
    glUseProgram(0);
    OnDisabled();
  }
}

#ifndef HAS_GLES

//////////////////////////////////////////////////////////////////////
// CARBShaderProgram
//////////////////////////////////////////////////////////////////////
void CARBShaderProgram::Free()
{
  m_pVP->Free();
  VerifyGLState();
  m_pFP->Free();
  VerifyGLState();
  m_ok = false;
}

bool CARBShaderProgram::CompileAndLink()
{
  // free resources
  Free();

  // compiled vertex shader
  if (!m_pVP->Compile())
  {
    CLog::Log(LOGERROR, "GL: Error compiling vertex shader");
    goto error;
  }

  // compile pixel shader
  if (!m_pFP->Compile())
  {
    m_pVP->Free();
    CLog::Log(LOGERROR, "GL: Error compiling fragment shader");
    goto error;
  }

  m_ok = true;
  OnCompiledAndLinked();
  VerifyGLState();
  return true;

 error:
  m_ok = false;
  Free();
  return false;
}

bool CARBShaderProgram::Enable()
{
  if (OK())
  {
    if (m_pFP->OK())
    {
      glEnable(GL_FRAGMENT_PROGRAM_ARB);
      glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, m_pFP->Handle());
    }
    if (m_pVP->OK())
    {
      glEnable(GL_VERTEX_PROGRAM_ARB);
      glBindProgramARB(GL_VERTEX_PROGRAM_ARB, m_pVP->Handle());
    }
    if (OnEnabled())
    {
      VerifyGLState();
      return true;
    }
    else
    {
      glDisable(GL_FRAGMENT_PROGRAM_ARB);
      glDisable(GL_VERTEX_PROGRAM_ARB);
      return false;
    }
  }
  return false;
}

void CARBShaderProgram::Disable()
{
  if (OK())
  {
    glDisable(GL_FRAGMENT_PROGRAM_ARB);
    glDisable(GL_VERTEX_PROGRAM_ARB);
    OnDisabled();
  }
}

#endif

#endif
