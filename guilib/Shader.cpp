/*
* XBoxMediaCenter
* Shader Classes
* Copyright (c) 2007 d4rk
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "stdafx.h"
#include <GL/glew.h>
#include "include.h"
#include "../xbmc/Settings.h"
#include "Shader.h"

#ifdef HAS_SDL_OPENGL

#define LOG_SIZE 1024

using namespace Shaders;
using namespace std;

//////////////////////////////////////////////////////////////////////
// CShader
//////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////
// CGLSLVertexShader
//////////////////////////////////////////////////////////////////////

bool CGLSLVertexShader::Compile()
{
  GLint params[4];

  Free();

  /*
     Workaround for locale bug in nVidia's shader compiler.
     Save the current locale, set to a neutral locale while compiling and switch back afterwards.
  */
  char * currentLocale = setlocale(LC_NUMERIC, NULL);
  setlocale(LC_NUMERIC, "C");

  m_vertexShader = glCreateShader(GL_VERTEX_SHADER);
  const char *ptr = m_source.c_str();
  glShaderSource(m_vertexShader, 1, &ptr, 0);
  glCompileShader(m_vertexShader);
  glGetShaderiv(m_vertexShader, GL_COMPILE_STATUS, params);
  VerifyGLState();
  if (params[0]!=GL_TRUE)
  {
    GLchar log[LOG_SIZE];
    CLog::Log(LOGERROR, "GL: Error compiling shader");
    glGetShaderInfoLog(m_vertexShader, LOG_SIZE, NULL, log);
    CLog::Log(LOGERROR, (const char*)log);
    m_lastLog = log;
    m_compiled = false;
  }
  else
  {
    GLchar log[LOG_SIZE];
    CLog::Log(LOGDEBUG, "GL: Shader compilation log:");
    glGetShaderInfoLog(m_vertexShader, LOG_SIZE, NULL, log);
    CLog::Log(LOGDEBUG, (const char*)log);
    m_lastLog = log;
    m_compiled = true;
  }
  setlocale(LC_NUMERIC, currentLocale);
  return m_compiled;
}

void CGLSLVertexShader::Free()
{
  if (m_vertexShader)
    glDeleteShader(m_vertexShader);
  m_vertexShader = 0;
}


//////////////////////////////////////////////////////////////////////
// CARBVertexShader
//////////////////////////////////////////////////////////////////////
bool CARBVertexShader::Compile()
{
  int err = 0;

  Free();

  // Pixel shaders are not mandatory.
  if (m_source.length()==0)
  {
    CLog::Log(LOGNOTICE, "GL: No vertex shader, fixed pipeline in use");
    return true;
  }

  // Workaround for locale bug in nVidia's shader compiler.
  // Save the current locale, set to a neutral while compiling and switch back afterwards.

  char * currentLocale = setlocale(LC_NUMERIC, NULL);
  setlocale(LC_NUMERIC, "C");

  glEnable(GL_FRAGMENT_PROGRAM_ARB);
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
  setlocale(LC_NUMERIC, currentLocale);
  return m_compiled;
}

void CARBVertexShader::Free()
{
  if (m_vertexShader)
    glDeleteProgramsARB(1, &m_vertexShader);
  m_vertexShader = 0;
}


//////////////////////////////////////////////////////////////////////
// CGLSLPixelShader
//////////////////////////////////////////////////////////////////////
bool CGLSLPixelShader::Compile()
{
  GLint params[4];

  Free();

  // Pixel shaders are not mandatory.
  if (m_source.length()==0)
  {
    CLog::Log(LOGNOTICE, "GL: No pixel shader, fixed pipeline in use");
    return true;
  }

  /*
     Workaround for locale bug in nVidia's shader compiler.
     Save the current locale, set to a neutral while compiling and switch back afterwards.
  */
  char * currentLocale = setlocale(LC_NUMERIC, NULL);
  setlocale(LC_NUMERIC, "C");

  m_pixelShader = glCreateShader(GL_FRAGMENT_SHADER);
  const char *ptr = m_source.c_str();
  glShaderSource(m_pixelShader, 1, &ptr, 0);
  glCompileShader(m_pixelShader);
  glGetShaderiv(m_pixelShader, GL_COMPILE_STATUS, params);
  if (params[0]!=GL_TRUE)
  {
    GLchar log[LOG_SIZE];
    CLog::Log(LOGERROR, "GL: Error compiling shader");
    glGetShaderInfoLog(m_pixelShader, LOG_SIZE, NULL, log);
    CLog::Log(LOGERROR, (const char*)log);
    m_lastLog = log;
    m_compiled = false;
  }
  else
  {
    GLchar log[LOG_SIZE];
    CLog::Log(LOGDEBUG, "GL: Shader compilation log:");
    glGetShaderInfoLog(m_pixelShader, LOG_SIZE, NULL, log);
    CLog::Log(LOGDEBUG, (const char*)log);
    m_lastLog = log;
    m_compiled = true;
  }
  setlocale(LC_NUMERIC, currentLocale);
  return m_compiled;
}

void CGLSLPixelShader::Free()
{
  if (m_pixelShader)
    glDeleteShader(m_pixelShader);
  m_pixelShader = 0;
}

//////////////////////////////////////////////////////////////////////
// CARBPixelShader
//////////////////////////////////////////////////////////////////////
bool CARBPixelShader::Compile()
{
  int err = 0;

  Free();

  // Pixel shaders are not mandatory.
  if (m_source.length()==0)
  {
    CLog::Log(LOGNOTICE, "GL: No pixel shader, fixed pipeline in use");
    return true;
  }

  // Workaround for locale bug in nVidia's shader compiler.
  // Save the current locale, set to a neutral while compiling and switch back afterwards.

  char * currentLocale = setlocale(LC_NUMERIC, NULL);
  setlocale(LC_NUMERIC, "C");

  glEnable(GL_FRAGMENT_PROGRAM_ARB);
  glGenProgramsARB(1, &m_pixelShader);
  glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, m_pixelShader);

  glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
                     m_source.length(), m_source.c_str());

  glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &err);
  if (err>0)
  {
    CLog::Log(LOGERROR, "GL: Error compiling ARB pixel shader");
    m_compiled = false;
  }
  else
  {
    m_compiled = true;
  }
  glDisable(GL_FRAGMENT_PROGRAM_ARB);
  setlocale(LC_NUMERIC, currentLocale);
  return m_compiled;
}

void CARBPixelShader::Free()
{
  if (m_pixelShader)
    glDeleteProgramsARB(1, &m_pixelShader);
  m_pixelShader = 0;
}


//////////////////////////////////////////////////////////////////////
// CGLSLShaderProgram
//////////////////////////////////////////////////////////////////////
void CGLSLShaderProgram::Free()
{
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
  GLint params[4];

  // free resources
  Free();

  // compiled vertex shader
  if (!m_pVP->Compile())
  {
    CLog::Log(LOGERROR, "GL: Error compiling vertex shader");
    return false;
  }

  // compile pixel shader
  if (!m_pFP->Compile())
  {
    m_pVP->Free();
    CLog::Log(LOGERROR, "GL: Error compiling fragment shader");
    return false;
  }

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
    CLog::Log(LOGERROR, (const char*)log);
    goto error;
  }
  VerifyGLState();

  // validate the program
  glValidateProgram(m_shaderProgram);
  glGetProgramiv(m_shaderProgram, GL_VALIDATE_STATUS, params);
  if (params[0]!=GL_TRUE)
  {
    GLchar log[LOG_SIZE];
    CLog::Log(LOGERROR, "GL: Error validating shader");
    glGetProgramInfoLog(m_shaderProgram, LOG_SIZE, NULL, log);
    CLog::Log(LOGERROR, (const char*)log);
    goto error;
  }
  VerifyGLState();

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
  if (OK())
  {
    glGetIntegerv(GL_CURRENT_PROGRAM, &m_lastProgram);
    glUseProgram((GLuint)m_shaderProgram);
    if (OnEnabled())
    {
      VerifyGLState();
      return true;
    }
    else
    {
      glUseProgram(m_lastProgram);
      return false;
    }
    return true;
  }
  return false;
}

void CGLSLShaderProgram::Disable()
{
  if (OK())
  {
    glUseProgram((GLuint)m_lastProgram);
    OnDisabled();
  }
}

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
