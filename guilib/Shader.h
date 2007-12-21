#ifndef __SHADER_H__
#define __SHADER_H__

#include "include.h"
#include <vector>
#include <string>
#ifdef HAS_SDL_OPENGL

namespace Shaders {

  using namespace std;

  //////////////////////////////////////////////////////////////////////
  // CShader - base class
  //////////////////////////////////////////////////////////////////////
  class CShader
  {
  public:
    CShader() { m_compiled = false; }
    virtual ~CShader() { }
    virtual bool Compile() = 0;
    virtual void Free() = 0;
    virtual GLuint Handle() = 0;
    virtual void SetSource(string& src) { m_source = src; }
    virtual void SetSource(const char* src) { m_source = src; }
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

  class CARBVertexShader : public CVertexShader
  {
  public:
    virtual void Free();
    virtual bool Compile();
  };


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

  class CARBPixelShader : public CPixelShader
  {
  public:
    virtual void Free();
    virtual bool Compile();
  };


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
        m_lastProgram = 0;
        m_pFP = NULL;
        m_pVP = NULL;
      }

    virtual ~CShaderProgram()
      {
        Free();
        if (m_pFP)
          delete m_pFP;
        if (m_pVP)
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

    // sets the vertex shader's source (in GLSL)
    virtual void SetVertexShaderSource(const char* src) { m_pVP->SetSource(src); }
    virtual void SetVertexShaderSource(string& src) { m_pVP->SetSource(src); }

    // sets the pixel shader's source (in GLSL)
    virtual void SetPixelShaderSource(const char* src) { m_pFP->SetSource(src); }
    virtual void SetPixelShaderSource(string& src) { m_pFP->SetSource(src); }

    virtual GLuint ProgramHandle() { return m_shaderProgram; }

  protected:
    CVertexShader* m_pVP;
    CPixelShader*  m_pFP;
    GLuint         m_shaderProgram;
    GLint          m_lastProgram;
    bool           m_ok;
  };


  class CGLSLShaderProgram : public CShaderProgram
  {
  public:
    CGLSLShaderProgram()
      {
        m_pFP = new CGLSLPixelShader();
        m_pVP = new CGLSLVertexShader();
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
  };


  class CARBShaderProgram : public CShaderProgram
  {
  public:
    CARBShaderProgram()
      {
        m_pFP = new CARBPixelShader();
        m_pVP = new CARBVertexShader();
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

} // close namespace

#endif

#endif //__SHADER_H__
