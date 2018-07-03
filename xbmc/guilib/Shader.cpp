/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
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

#include "ServiceBroker.h"
#include "Shader.h"
#include "filesystem/File.h"
#include "utils/log.h"
#include "utils/GLUtils.h"
#include "rendering/RenderSystem.h"

#ifdef HAS_GLES
#define GLchar char
#endif

#define LOG_SIZE 1024

using namespace Shaders;
using namespace XFILE;

//////////////////////////////////////////////////////////////////////
// CShader
//////////////////////////////////////////////////////////////////////
bool CShader::LoadSource(const std::string& filename, const std::string& prefix)
{
  if(filename.empty())
    return true;

  CFileStream file;

  std::string path = "special://xbmc/system/shaders/";
  path += CServiceBroker::GetRenderSystem()->GetShaderPath(filename);
  path += filename;
  if(!file.Open(path))
  {
    CLog::Log(LOGERROR, "CYUVShaderGLSL::CYUVShaderGLSL - failed to open file %s", filename.c_str());
    return false;
  }
  getline(file, m_source, '\0');

  size_t pos = 0;
  size_t versionPos = m_source.find("#version");
  if (versionPos != std::string::npos)
  {
    versionPos = m_source.find("\n", versionPos);
    if (versionPos != std::string::npos)
      pos = versionPos + 1;
  }
  m_source.insert(pos, prefix);
  return true;
}

bool CShader::AppendSource(const std::string& filename)
{
  if(filename.empty())
    return true;

  CFileStream file;
  std::string temp;

  std::string path = "special://xbmc/system/shaders/";
  path += CServiceBroker::GetRenderSystem()->GetShaderPath(filename);
  path += filename;
  if(!file.Open(path))
  {
    CLog::Log(LOGERROR, "CShader::AppendSource - failed to open file %s", filename.c_str());
    return false;
  }
  getline(file, temp, '\0');
  m_source.append(temp);
  return true;
}

bool CShader::InsertSource(const std::string& filename, const std::string& loc)
{
  if(filename.empty())
    return true;

  CFileStream file;
  std::string temp;

  std::string path = "special://xbmc/system/shaders/";
  path += CServiceBroker::GetRenderSystem()->GetShaderPath(filename);
  path += filename;
  if(!file.Open(path))
  {
    CLog::Log(LOGERROR, "CShader::InsertSource - failed to open file %s", filename.c_str());
    return false;
  }
  getline(file, temp, '\0');

  size_t locPos = m_source.find(loc);
  if (locPos == std::string::npos)
  {
    CLog::Log(LOGERROR, "CShader::InsertSource - could not find location %s", loc.c_str());
    return false;
  }

  m_source.insert(locPos, temp);

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
  if (params[0] != GL_TRUE)
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
    GLsizei length;
    glGetShaderInfoLog(m_vertexShader, LOG_SIZE, &length, log);
    if (length > 0)
    {
      CLog::Log(LOGDEBUG, "GL: Vertex Shader compilation log:");
      CLog::Log(LOGDEBUG, "%s", log);
    }
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
  if (m_pixelShader)
    glDeleteShader(m_pixelShader);
  m_pixelShader = 0;
}

//////////////////////////////////////////////////////////////////////
// CGLSLShaderProgram
//////////////////////////////////////////////////////////////////////
CGLSLShaderProgram::CGLSLShaderProgram()
{
  m_pFP = new CGLSLPixelShader();
  m_pVP = new CGLSLVertexShader();
}

CGLSLShaderProgram::CGLSLShaderProgram(const std::string& vert,
                                       const std::string& frag)
{
  m_pFP = new CGLSLPixelShader();
  m_pFP->LoadSource(frag);
  m_pVP = new CGLSLVertexShader();
  m_pVP->LoadSource(vert);
}

CGLSLShaderProgram::~CGLSLShaderProgram()
{
  Free();
}

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
  if (OK())
  {
    glUseProgram(0);
    OnDisabled();
  }
}

