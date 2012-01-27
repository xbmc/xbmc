/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#ifdef HAS_GLES

#include "Shader.h"
#include "utils/GLUtils.h"
#include <stdio.h>

#define LOG_SIZE 1024
#define GLchar char

using namespace VisShaders;
using namespace std;

//////////////////////////////////////////////////////////////////////
// CShader
//////////////////////////////////////////////////////////////////////
bool CShader::LoadSource(const string& buffer)
{
  fprintf(stderr, "GL: loadsource\n");

  m_source = buffer;
  return true;
}

//////////////////////////////////////////////////////////////////////
// CGLSLVertexShader
//////////////////////////////////////////////////////////////////////

bool CGLSLVertexShader::Compile()
{
  GLint params[4];

  Free();

  m_vertexShader = glCreateShader(GL_VERTEX_SHADER);
  const char *ptr = m_source.c_str();
  glShaderSource(m_vertexShader, 1, &ptr, 0);
  glCompileShader(m_vertexShader);
  glGetShaderiv(m_vertexShader, GL_COMPILE_STATUS, params);
  VerifyGLState();
  if (params[0]!=GL_TRUE)
  {
    GLchar log[LOG_SIZE];
    fprintf(stderr, "GL: Error compiling vertex shader ");
    glGetShaderInfoLog(m_vertexShader, LOG_SIZE, NULL, log);
    printf("%s\n", log);
    m_lastLog = log;
    m_compiled = false;
  }
  else
  {
    GLchar log[LOG_SIZE];
    fprintf(stderr, "GL: Vertex Shader compilation log: ");
    glGetShaderInfoLog(m_vertexShader, LOG_SIZE, NULL, log);
    printf("%s\n", log);
    m_lastLog = log;
    m_compiled = true;
  }
  return m_compiled;
}

void CGLSLVertexShader::Free()
{
  if (m_vertexShader)
    glDeleteShader(m_vertexShader);
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
    fprintf(stderr, "GL: No pixel shader, fixed pipeline in use\n");
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
    fprintf(stderr, "GL: Error compiling pixel shader ");
    glGetShaderInfoLog(m_pixelShader, LOG_SIZE, NULL, log);
    printf("%s \n", log);
    m_lastLog = log;
    m_compiled = false;
  }
  else
  {
    GLchar log[LOG_SIZE];
    fprintf(stderr, "GL: Pixel Shader compilation log: ");
    glGetShaderInfoLog(m_pixelShader, LOG_SIZE, NULL, log);
    fprintf(stderr, "%s \n", log);
    m_lastLog = log;
    m_compiled = true;
  }
  return m_compiled;
}

void CGLSLPixelShader::Free()
{
  if (m_pixelShader)
    glDeleteShader(m_pixelShader);
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
    fprintf(stderr, "GL: Error compiling vertex shader\n");
    return false;
  }
  fprintf(stderr, "GL: Vertex Shader compiled successfully\n");

  // compile pixel shader
  if (!m_pFP->Compile())
  {
    m_pVP->Free();
    fprintf(stderr, "GL: Error compiling fragment shader\n");
    return false;
  }
  fprintf(stderr, "GL: Fragment Shader compiled successfully\n");

  // create program object
  if (!(m_shaderProgram = glCreateProgram()))
  {
    fprintf(stderr, "GL: Error creating shader program handle\n");
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
    fprintf(stderr, "GL: Error linking shader ");
    glGetProgramInfoLog(m_shaderProgram, LOG_SIZE, NULL, log);
    fprintf(stderr, "%s\n", log);
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
          fprintf(stderr, "GL: Error validating shader ");
          glGetProgramInfoLog(m_shaderProgram, LOG_SIZE, NULL, log);
          fprintf(stderr, "%s \n", log);
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
  if (OK())
  {
    glUseProgram(0);
    OnDisabled();
  }
}

#endif
