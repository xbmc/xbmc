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
    virtual void SetSourceTexture(GLint ytex) { m_sourceTexUnit = ytex; }
    virtual void SetWidth(int w)     { m_width  = w; m_stepX = w>0?1.0f/w:0; }
    virtual void SetHeight(int h)    { m_height = h; m_stepY = h>0?1.0f/h:0; }

  protected:
    int   m_width;
    int   m_height;
    float m_stepX;
    float m_stepY;
    GLint m_sourceTexUnit;

    // shader attribute handles
    GLint m_hSourceTex;
    GLint m_hStepX;
    GLint m_hStepY;
  };


  class BicubicFilterShader : public BaseVideoFilterShader
  {
  public:
    BicubicFilterShader(float B=-1.0f, float C=-1.0f);
    void OnCompiledAndLinked();
    bool OnEnabled();
    void Free();

  protected:
    float MitchellNetravali(float x, float B, float C);
    bool CreateKernels(int size, float B, float C);

    // kernel textures
    GLuint m_kernelTex1;

    // shader handles to kernel textures
    GLint m_hKernTex;

    // cubic interpolations parameters
    float m_B;
    float m_C;
  };

  class ConvolutionFilterShader : public BaseVideoFilterShader
  {
  public:
    ConvolutionFilterShader(ESCALINGMETHOD method);
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
  };

} // end namespace

#endif

#endif //__VIDEOFILTER_SHADER_H__
