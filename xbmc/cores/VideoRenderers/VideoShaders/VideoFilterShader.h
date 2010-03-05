#ifndef __VIDEOFILTER_SHADER_H__
#define __VIDEOFILTER_SHADER_H__

#ifdef HAS_GL

#include "../../../../guilib/Shader.h"
#include "../../../settings/VideoSettings.h"

using namespace Shaders;

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
    virtual GLint GetTextureFilter()  { return GL_NEAREST; }

  protected:
    int   m_width;
    int   m_height;
    float m_stepX;
    float m_stepY;
    float m_stretch;
    GLint m_sourceTexUnit;

    // shader attribute handles
    GLint m_hSourceTex;
    GLint m_hStepX;
    GLint m_hStepY;
    GLint m_hStretch;
  };

  class ConvolutionFilterShader : public BaseVideoFilterShader
  {
  public:
    ConvolutionFilterShader(ESCALINGMETHOD method, bool stretch);
    void OnCompiledAndLinked();
    bool OnEnabled();
    void Free();

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
      GLint GetTextureFilter() { return GL_LINEAR; }
  };

} // end namespace

#endif

#endif //__VIDEOFILTER_SHADER_H__
