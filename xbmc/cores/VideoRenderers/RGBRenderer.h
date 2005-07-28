#ifndef RGB_RENDERER
#define RGB_RENDERER

#include "XBoxRenderer.h"

class CRGBRenderer : public CXBoxRenderer
{
public:
  CRGBRenderer(LPDIRECT3DDEVICE8 pDevice);
  //~CRGBRenderer();

  // Functions called from mplayer
  // virtual void     WaitForFlip();
  virtual unsigned int Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, float fps);
  virtual void FlipPage();
  virtual unsigned int PreInit();
  virtual void UnInit();

protected:
  virtual void Render();
  virtual void ManageTextures();
  bool CreateYUVTexture();
  void DeleteYUVTexture();
  void ClearYUVTexture();
  bool CreateLookupTextures();
  void DeleteLookupTextures();

  // YUV interleaved texture
  LPDIRECT3DTEXTURE8 m_YUVTexture;

  // textures for YUV->RGB lookup
  LPDIRECT3DTEXTURE8 m_UVLookup;
  LPDIRECT3DTEXTURE8 m_UVErrorLookup;

  // Pixel shaders
  DWORD m_hInterleavingShader;
  DWORD m_hYUVtoRGBLookup;

  // Texture for Field deinterlacing
  unsigned int m_YUVFieldPitch;
  D3DTexture m_YUVFieldTexture;
  // Vertex types
  static const DWORD FVF_YUVRGBVERTEX = D3DFVF_XYZRHW | D3DFVF_TEX4;
  static const DWORD FVF_RGBVERTEX = D3DFVF_XYZRHW | D3DFVF_TEX1;
};

#endif
