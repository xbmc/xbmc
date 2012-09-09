#pragma once
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

#if defined(__APPLE__)                                                                                                                                                                                           
#include <OpenGLES/ES2/gl.h>                                                                                                                                                                                     
#include <OpenGLES/ES2/glext.h>                                                                                                                                                                                  
#else                                                                                                                                                                                                            
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#endif//__APPLE__

#include <vector>
#include <string>

//////////////////////////////////////////////////////////////////////
// CVisShader - base class
//////////////////////////////////////////////////////////////////////
class CVisShader
{
public:
  CVisShader() { m_compiled = false; }
  virtual ~CVisShader() {}
  virtual bool Compile() = 0;
  virtual void Free() = 0;
  virtual GLuint Handle() = 0;
  virtual void SetSource(const char *src) { m_source = src; }
  virtual bool LoadSource(const char *buffer);
  bool OK() { return m_compiled; }

protected:
  std::string m_source;
  std::string m_lastLog;
  std::vector<std::string> m_attr;
  bool m_compiled;

};


//////////////////////////////////////////////////////////////////////
// CVisVertexShader - vertex shader class
//////////////////////////////////////////////////////////////////////
class CVisVertexShader : public CVisShader
{
public:
  CVisVertexShader() { m_vertexShader = 0; }
  virtual ~CVisVertexShader() { Free(); }
  virtual void Free() {}
  virtual GLuint Handle() { return m_vertexShader; }

protected:
  GLuint m_vertexShader;
};

class CVisGLSLVertexShader : public CVisVertexShader
{
public:
  virtual void Free();
  virtual bool Compile();
};

//////////////////////////////////////////////////////////////////////
// CVisPixelShader - abstract pixel shader class
//////////////////////////////////////////////////////////////////////
class CVisPixelShader : public CVisShader
{
public:
  CVisPixelShader() { m_pixelShader = 0; }
  virtual ~CVisPixelShader() { Free(); }
  virtual void Free() {}
  virtual GLuint Handle() { return m_pixelShader; }

protected:
  GLuint m_pixelShader;
};


class CVisGLSLPixelShader : public CVisPixelShader
{
public:
  virtual void Free();
  virtual bool Compile();
};

//////////////////////////////////////////////////////////////////////
// CShaderProgram - the complete shader consisting of both the vertex
//                  and pixel programs. (abstract)
//////////////////////////////////////////////////////////////////////
class CVisShaderProgram
{
public:
  CVisShaderProgram()
    {
      m_ok = false;
      m_shaderProgram = 0;
      m_pFP = NULL;
      m_pVP = NULL;
    }

  virtual ~CVisShaderProgram()
    {
      Free();
      delete m_pFP;
      delete m_pVP;
    }

  // enable the shader
  virtual bool Enable() = 0;

  // disable the shader
  virtual void Disable() = 0;

  // returns true if shader is compiled and linked
  bool OK() { return m_ok; }

  // free resources
  virtual void Free() {}

  // return the vertex shader object
  CVisVertexShader* VertexShader() { return m_pVP; }

  // return the pixel shader object
  CVisPixelShader* PixelShader() { return m_pFP; }

  // compile and link the shaders
  virtual bool CompileAndLink() = 0;

  // override to to perform custom tasks on successfull compilation
  // and linkage. E.g. obtaining handles to shader attributes.
  virtual void OnCompiledAndLinked() {}

  // override to to perform custom tasks before shader is enabled
  // and after it is disabled. Return false in OnDisabled() to
  // disable shader.
  // E.g. setting attributes, disabling texture unites, etc
  virtual bool OnEnabled() { return true; }
  virtual void OnDisabled() { }

  virtual GLuint ProgramHandle() { return m_shaderProgram; }

protected:
  CVisVertexShader* m_pVP;
  CVisPixelShader*  m_pFP;
  GLuint         m_shaderProgram;
  bool           m_ok;
};


class CVisGLSLShaderProgram
  : virtual public CVisShaderProgram
{
public:
  CVisGLSLShaderProgram()
    {
      m_pFP = new CVisGLSLPixelShader();
      m_pVP = new CVisGLSLVertexShader();
    }
  CVisGLSLShaderProgram(const char *vert, const char *frag)
    {
      m_pFP = new CVisGLSLPixelShader();
      m_pFP->LoadSource(frag);
      m_pVP = new CVisGLSLVertexShader();
      m_pVP->LoadSource(vert);
    }

  // enable the shader
  virtual bool Enable();

  // disable the shader
  virtual void Disable();

  // free resources
  virtual void Free();

  // compile and link the shaders
  virtual bool CompileAndLink();

protected:
  GLint         m_lastProgram;
  bool          m_validated;
};
