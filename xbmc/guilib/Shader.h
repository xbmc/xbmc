/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>
#include <vector>

#include "system_gl.h"

namespace Shaders {

  //////////////////////////////////////////////////////////////////////
  // CShader - base class
  //////////////////////////////////////////////////////////////////////
  class CShader
  {
  public:
    CShader() = default;
    virtual ~CShader() = default;
    virtual bool Compile() = 0;
    virtual void Free() = 0;
    virtual GLuint Handle() = 0;
    virtual void SetSource(const std::string& src) { m_source = src; }
    virtual bool LoadSource(const std::string& filename, const std::string& prefix = "");
    virtual bool AppendSource(const std::string& filename);
    virtual bool InsertSource(const std::string& filename, const std::string& loc);
    bool OK() const { return m_compiled; }

    std::string GetName() const { return m_filenames; }
    std::string GetSourceWithLineNumbers() const;

  protected:
    std::string m_source;
    std::string m_lastLog;
    std::vector<std::string> m_attr;
    bool m_compiled = false;

  private:
    std::string m_filenames;
  };


  //////////////////////////////////////////////////////////////////////
  // CVertexShader - vertex shader class
  //////////////////////////////////////////////////////////////////////
  class CVertexShader : public CShader
  {
  public:
    CVertexShader() = default;
    ~CVertexShader() override { Free(); }
    void Free() override {}
    GLuint Handle() override { return m_vertexShader; }

  protected:
    GLuint m_vertexShader = 0;
  };

  class CGLSLVertexShader : public CVertexShader
  {
  public:
    void Free() override;
    bool Compile() override;
  };


  //////////////////////////////////////////////////////////////////////
  // CPixelShader - abstract pixel shader class
  //////////////////////////////////////////////////////////////////////
  class CPixelShader : public CShader
  {
  public:
    CPixelShader() = default;
    ~CPixelShader() override { Free(); }
    void Free() override {}
    GLuint Handle() override { return m_pixelShader; }

  protected:
    GLuint m_pixelShader = 0;
  };

  class CGLSLPixelShader : public CPixelShader
  {
  public:
    void Free() override;
    bool Compile() override;
  };

  //////////////////////////////////////////////////////////////////////
  // CShaderProgram - the complete shader consisting of both the vertex
  //                  and pixel programs. (abstract)
  //////////////////////////////////////////////////////////////////////
  class CShaderProgram
  {
  public:
    CShaderProgram() = default;

    virtual ~CShaderProgram()
      {
        delete m_pFP;
        delete m_pVP;
      }

    // enable the shader
    virtual bool Enable() = 0;

    // disable the shader
    virtual void Disable() = 0;

    // returns true if shader is compiled and linked
    bool OK() const { return m_ok; }

    // return the vertex shader object
    CVertexShader* VertexShader() { return m_pVP; }

    // return the pixel shader object
    CPixelShader* PixelShader() { return m_pFP; }

    // compile and link the shaders
    virtual bool CompileAndLink() = 0;

    // override to perform custom tasks on successful compilation
    // and linkage. E.g. obtaining handles to shader attributes.
    virtual void OnCompiledAndLinked() {}

    // override to perform custom tasks before shader is enabled
    // and after it is disabled. Return false in OnDisabled() to
    // disable shader.
    // E.g. setting attributes, disabling texture unites, etc
    virtual bool OnEnabled() { return true; }
    virtual void OnDisabled() { }

    virtual GLuint ProgramHandle() { return m_shaderProgram; }

  protected:
    CVertexShader* m_pVP = nullptr;
    CPixelShader*  m_pFP = nullptr;
    GLuint m_shaderProgram = 0;
    bool m_ok = false;
  };


  class CGLSLShaderProgram : virtual public CShaderProgram
  {
  public:
    CGLSLShaderProgram();
    CGLSLShaderProgram(const std::string& vert
                       , const std::string& frag);
    ~CGLSLShaderProgram() override;

    // enable the shader
    bool Enable() override;

    // disable the shader
    void Disable() override;

    // compile and link the shaders
    bool CompileAndLink() override;

  protected:
    void Free();

    GLint m_lastProgram;
    bool m_validated = false;
  };


} // close namespace

