#ifndef __YUV2RGB_SHADERS_H__
#define __YUV2RGB_SHADERS_H__

#ifdef HAS_SDL_OPENGL

#include "../../../../guilib/Shader.h"

namespace Shaders {

  class BaseYUV2RGBGLSLShader : public CGLSLShaderProgram
  {
  public:
    BaseYUV2RGBGLSLShader();
    virtual void SetYTexture(GLint ytex) { m_yTexUnit = ytex; }
    virtual void SetUTexture(GLint utex) { m_uTexUnit = utex; }
    virtual void SetVTexture(GLint vtex) { m_vTexUnit = vtex; }
    virtual void SetField(int field) { field    = field; }
    virtual void SetWidth(int w)     { m_width  = w; }
    virtual void SetHeight(int h)    { m_height = h; }

  protected:
    int   m_width;
    int   m_height;
    float m_stepX;
    float m_stepY;
    int   m_field;
    GLint m_yTexUnit;
    GLint m_uTexUnit;
    GLint m_vTexUnit;

    // shader attribute handles
    GLint m_hYTex;
    GLint m_hUTex;
    GLint m_hVTex;
    GLint m_hStepX;
    GLint m_hStepY;
    GLint m_hField;
  };

  class BaseYUV2RGBARBShader : public CARBShaderProgram
  {
  public:
    BaseYUV2RGBARBShader();
    virtual void SetYTexture(GLint ytex) { m_yTexUnit = ytex; }
    virtual void SetUTexture(GLint utex) { m_uTexUnit = utex; }
    virtual void SetVTexture(GLint vtex) { m_vTexUnit = vtex; }
    virtual void SetField(int field) { field    = field; }
    virtual void SetWidth(int w)     { m_width  = w; }
    virtual void SetHeight(int h)    { m_height = h; }

  protected:
    int   m_width;
    int   m_height;
    float m_stepX;
    float m_stepY;
    int   m_field;
    GLint m_yTexUnit;
    GLint m_uTexUnit;
    GLint m_vTexUnit;

    // shader attribute handles
    GLint m_hYTex;
    GLint m_hUTex;
    GLint m_hVTex;
    GLint m_hStepX;
    GLint m_hStepY;
    GLint m_hField;
  };

  class YUV2RGBProgressiveShaderARB : public BaseYUV2RGBARBShader
  {
  public:
    YUV2RGBProgressiveShaderARB(bool rect=false);
    void OnCompiledAndLinked();
    bool OnEnabled();
  };

  class YUV2RGBProgressiveShader : public BaseYUV2RGBGLSLShader
  {
  public:
    YUV2RGBProgressiveShader(bool rect=false);
    void OnCompiledAndLinked();
    bool OnEnabled();
  };

  class YUV2RGBBobShader : public BaseYUV2RGBGLSLShader
  {
  public:
    YUV2RGBBobShader(bool rect=false);
    void OnCompiledAndLinked();
    bool OnEnabled();
  };

} // end namespace

#endif

#endif //__YUV2RGB_SHADERS_H__
