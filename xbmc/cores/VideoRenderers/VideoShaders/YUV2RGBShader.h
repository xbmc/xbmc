#ifndef __YUV2RGB_SHADERS_H__
#define __YUV2RGB_SHADERS_H__

#ifdef HAS_SDL_OPENGL
#pragma warning( push )
#pragma warning( disable : 4250 )

#include "../../../../guilib/Shader.h"

namespace Shaders {

  class BaseYUV2RGBShader
    : virtual public CShaderProgram
  {
  public:
    virtual ~BaseYUV2RGBShader()      {};
    virtual void SetField(int field)  {};
    virtual void SetWidth(int width)  {};
    virtual void SetHeight(int width) {};
  };


  class BaseYUV2RGBGLSLShader 
    : public BaseYUV2RGBShader
    , public CGLSLShaderProgram
  {
  public:
    BaseYUV2RGBGLSLShader(bool rect, unsigned flags);
   ~BaseYUV2RGBGLSLShader() {}
    virtual void SetField(int field) { m_field  = field; }
    virtual void SetWidth(int w)     { m_width  = w; }
    virtual void SetHeight(int h)    { m_height = h; }
    string       BuildYUVMatrix();
    
  protected:
    unsigned m_flags;
    int   m_width;
    int   m_height;
    int   m_field;

    // shader attribute handles
    GLint m_hYTex;
    GLint m_hUTex;
    GLint m_hVTex;
    GLint m_hStepX;
    GLint m_hStepY;
    GLint m_hField;
  };

  class BaseYUV2RGBARBShader 
    : public BaseYUV2RGBShader
    , public CARBShaderProgram
  {
  public:
    BaseYUV2RGBARBShader(unsigned flags);
   ~BaseYUV2RGBARBShader() {}
    virtual void SetField(int field) { m_field  = field; }
    virtual void SetWidth(int w)     { m_width  = w; }
    virtual void SetHeight(int h)    { m_height = h; }
    string       BuildYUVMatrix();

  protected:
    unsigned m_flags;
    int   m_width;
    int   m_height;
    int   m_field;

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
    YUV2RGBProgressiveShaderARB(bool rect=false, unsigned flags=0);
    void OnCompiledAndLinked();
    bool OnEnabled();
  };

  class YUV2RGBProgressiveShader : public BaseYUV2RGBGLSLShader
  {
  public:
    YUV2RGBProgressiveShader(bool rect=false, unsigned flags=0);
    void OnCompiledAndLinked();
    bool OnEnabled();
  };

  class YUV2RGBBobShader : public BaseYUV2RGBGLSLShader
  {
  public:
    YUV2RGBBobShader(bool rect=false, unsigned flags=0);
    void OnCompiledAndLinked();
    bool OnEnabled();
  };

} // end namespace

#pragma warning( pop )
#endif

#endif //__YUV2RGB_SHADERS_H__
