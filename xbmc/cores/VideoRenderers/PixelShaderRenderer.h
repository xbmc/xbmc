#ifndef PIXELSHADER_RENDERER
#define PIXELSHADER_RENDERER

#include "XBoxRenderer.h"

class CPixelShaderRenderer : public CXBoxRenderer
{
public:
  CPixelShaderRenderer(LPDIRECT3DDEVICE8 pDevice);
  //~CPixelShaderRenderer();

  // Functions called from mplayer
  // virtual void     WaitForFlip();
  virtual unsigned int Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, float fps);

protected:
  virtual void Render(DWORD flags);
};

#endif
