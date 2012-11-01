#ifndef __VIDEOFILTER_SHADER_H__
#define __VIDEOFILTER_SHADER_H__

/*
 *      Copyright (C) 2007-2012 Team XBMC
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
#include "system_gl.h"


#include "guilib/Shader.h"
#include "settings/VideoSettings.h"

namespace Shaders {

  class BaseVideoFilterShader : public CGLSLShaderProgram
  {
  public:
    BaseVideoFilterShader();
    void Free() { CGLSLShaderProgram::Free(); }
    virtual void  SetSourceTexture(GLint ytex) { m_sourceTexUnit = ytex; }
    virtual void  SetWidth(int w)     { m_width  = w; m_stepX = w>0?1.0f/w:0; }
    virtual void  SetHeight(int h)    { m_height = h; m_stepY = h>0?1.0f/h:0; }
    virtual void  SetNonLinStretch(float stretch) { m_stretch = stretch; }
    virtual bool  GetTextureFilter(GLint& filter) { return false; }

  protected:
    int   m_width;
    int   m_height;
    float m_stepX;
    float m_stepY;
    float m_stretch;
    GLint m_sourceTexUnit;

    // shader attribute handles
    GLint m_hSourceTex;
    GLint m_hStepXY;
    GLint m_hStretch;
  };

  class ConvolutionFilterShader : public BaseVideoFilterShader
  {
  public:
    ConvolutionFilterShader(ESCALINGMETHOD method, bool stretch);
    void OnCompiledAndLinked();
    bool OnEnabled();
    void Free();

    virtual bool GetTextureFilter(GLint& filter) { filter = GL_NEAREST; return true; }

  protected:
    // kernel textures
    GLuint m_kernelTex1;

    // shader handles to kernel textures
    GLint m_hKernTex;

    ESCALINGMETHOD m_method;
    bool           m_floattex; //if float textures are supported
    GLint          m_internalformat;
  };

  class StretchFilterShader : public BaseVideoFilterShader
  {
    public:
      StretchFilterShader();
      void  OnCompiledAndLinked();
      bool  OnEnabled();
  };

} // end namespace

#endif

#endif //__VIDEOFILTER_SHADER_H__
