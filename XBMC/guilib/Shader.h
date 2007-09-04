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
    virtual void Free();
    virtual GLuint Handle() { return m_vertexShader; }
    bool Compile();

  protected:
    GLuint m_vertexShader;
  };


  //////////////////////////////////////////////////////////////////////
  // CPixelShader - pixel shader class
  //////////////////////////////////////////////////////////////////////
  class CPixelShader : public CShader
  {
  public:
    CPixelShader() { m_pixelShader = 0; }
    virtual ~CPixelShader() { Free(); }
    virtual void Free();
    virtual GLuint Handle() { return m_pixelShader; }
    bool Compile();

  protected:
    GLuint m_pixelShader;
  };


  //////////////////////////////////////////////////////////////////////
  // CShaderProgram - the complete shader consisting of both the vertex
  //                  and pixel programs.
  //////////////////////////////////////////////////////////////////////
  class CShaderProgram
  {
  public:
    CShaderProgram() { m_ok = false; m_shaderProgram = 0; m_lastProgram = 0; }
    virtual ~CShaderProgram() { Free(); }

    // enable the shader
    virtual bool Enable();

    // disable the shader
    virtual void Disable();

    // returns true if shader is compiled and linked
    bool OK() { return m_ok; }

    // free resources
    virtual void Free();

    // return the vertex shader object
    CVertexShader& VertexShader() { return m_VP; }

    // return the pixel shader object
    CPixelShader& PixelShader() { return m_FP; }

    // compile and link the shaders
    virtual bool CompileAndLink();

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
    void SetVertexShaderSource(const char* src) { m_VP.SetSource(src); }
    void SetVertexShaderSource(string& src) { m_VP.SetSource(src); }

    // sets the pixel shader's source (in GLSL)
    void SetPixelShaderSource(const char* src) { m_FP.SetSource(src); }
    void SetPixelShaderSource(string& src) { m_FP.SetSource(src); }

    GLuint ProgramHandle() { return m_shaderProgram; }
  
  protected:
    CVertexShader m_VP;
    CPixelShader  m_FP;
    GLuint        m_shaderProgram;
    GLint         m_lastProgram;
    bool          m_ok;
  };

} // close namespace

#endif

#endif //__SHADER_H__
