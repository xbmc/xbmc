#ifndef __YUV2RGB_SHADERS_H__
#define __YUV2RGB_SHADERS_H__

#ifdef HAS_SDL_OPENGL

#include "../../../../guilib/Shader.h"

namespace Shaders {

  class BaseYUV2RGBShader : public CShaderProgram
  {
  public:
    BaseYUV2RGBShader();
    virtual SetYTexture(GLint ytex) { m_ytex = ytex; }
    virtual SetUTexture(GLint utex) { m_utex = utex; }
    virtual SetVTexture(GLint vtex) { m_vtex = vtex; }
    virtual SetField(int field) {}
    virtual SetWidth(int width);
    virtual SetHeight(int width);

  protected:
    int   m_width;
    int   m_height;
    float m_stepX;
    float m_stepY;
    int   field;
    GLint m_ytex;
    GLint m_utex;
    GLint m_vtex;
  };

} // end namespace

#endif

#endif //__YUV2RGB_SHADERS_H__
