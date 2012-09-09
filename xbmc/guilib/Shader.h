#ifndef __SHADER_H__
#define __SHADER_H__

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

#include "system.h" // for HAS_GL/HAS_GLES

#include <vector>
#include <string>

#if defined(HAS_GL) || defined(HAS_GLES)
#include "system_gl.h"

namespace Shaders {

  using namespace std;

  //////////////////////////////////////////////////////////////////////
  // CShader - base class
  //////////////////////////////////////////////////////////////////////
  class CShader
  {
  public:
    CShader() { m_compiled = false; }
    virtual ~CShader() {}
    virtual bool Compile() = 0;
    virtual void Free() = 0;
    virtual GLuint Handle() = 0;
    virtual void SetSource(const string& src) { m_source = src; }
    virtual bool LoadSource(const string& filename, const string& prefix = "");
    bool OK() { return m_compiled; }

  protected:
    string m_source;
    string m_lastLog;
    vector<string> m_attr;
    bool m_compiled;

  };


  //////////////////////////////////////////////////////////////////////
  // CVertexShader - vertex shader class
  //////////////////////////////////////////////////////////////////////
  class CVertexShader : public CShader
  {
  public:
    CVertexShader() { m_vertexShader = 0; }
    virtual ~CVertexShader() { Free(); }
    virtual void Free() {}
    virtual GLuint Handle() { return m_vertexShader; }

  protected:
    GLuint m_vertexShader;
  };

  class CGLSLVertexShader : public CVertexShader
  {
  public:
    virtual void Free();
    virtual bool Compile();
  };

#ifndef HAS_GLES
  class CARBVertexShader : public CVertexShader
  {
  public:
    virtual void Free();
    virtual bool Compile();
  };
#endif


  //////////////////////////////////////////////////////////////////////
  // CPixelShader - abstract pixel shader class
  //////////////////////////////////////////////////////////////////////
  class CPixelShader : public CShader
  {
  public:
    CPixelShader() { m_pixelShader = 0; }
    virtual ~CPixelShader() { Free(); }
    virtual void Free() {}
    virtual GLuint Handle() { return m_pixelShader; }

  protected:
    GLuint m_pixelShader;
  };


  class CGLSLPixelShader : public CPixelShader
  {
  public:
    virtual void Free();
    virtual bool Compile();
  };

#ifndef HAS_GLES
  class CARBPixelShader : public CPixelShader
  {
  public:
    virtual void Free();
    virtual bool Compile();
  };
#endif

  //////////////////////////////////////////////////////////////////////
  // CShaderProgram - the complete shader consisting of both the vertex
  //                  and pixel programs. (abstract)
  //////////////////////////////////////////////////////////////////////
  class CShaderProgram
  {
  public:
    CShaderProgram()
      {
        m_ok = false;
        m_shaderProgram = 0;
        m_pFP = NULL;
        m_pVP = NULL;
      }

    virtual ~CShaderProgram()
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
    CVertexShader* VertexShader() { return m_pVP; }

    // return the pixel shader object
    CPixelShader* PixelShader() { return m_pFP; }

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
    CVertexShader* m_pVP;
    CPixelShader*  m_pFP;
    GLuint         m_shaderProgram;
    bool           m_ok;
  };


  class CGLSLShaderProgram
    : virtual public CShaderProgram
  {
  public:
    CGLSLShaderProgram()
      {
        m_pFP = new CGLSLPixelShader();
        m_pVP = new CGLSLVertexShader();
      }
    CGLSLShaderProgram(const std::string& vert
                     , const std::string& frag)
      {
        m_pFP = new CGLSLPixelShader();
        m_pFP->LoadSource(frag);
        m_pVP = new CGLSLVertexShader();
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


#ifndef HAS_GLES
  class CARBShaderProgram
    : virtual public CShaderProgram
  {
  public:
    CARBShaderProgram()
      {
        m_pFP = new CARBPixelShader();
        m_pVP = new CARBVertexShader();
      }
    CARBShaderProgram(const std::string& vert
                    , const std::string& frag)
      {
        m_pFP = new CARBPixelShader();
        m_pFP->LoadSource(vert);
        m_pVP = new CARBVertexShader();
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

  };
#endif

} // close namespace

#endif

#endif //__SHADER_H__
