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
  virtual bool Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, float fps, unsigned flags);
  virtual unsigned int PreInit();
  virtual void UnInit();

protected:
  virtual void Render(DWORD flags);
  virtual void ManageTextures();
  bool Create444PTexture();
  void Delete444PTexture();
  void Clear444PTexture();
  bool CreateLookupTextures(const YUVCOEF &coef, const YUVRANGE &range);
  void DeleteLookupTextures();

  void InterleaveYUVto444P(
      YUVPLANES          pSources,
      LPDIRECT3DSURFACE8 pTarget,
      RECT &source,       RECT &target,
      unsigned cshift_x,  unsigned cshift_y,
      float    offset_x,  float    offset_y,
      float    coffset_x, float    coffset_y);

  // YUV interleaved texture
  LPDIRECT3DTEXTURE8 m_444PTexture[MAX_FIELDS];

  // textures for YUV->RGB lookup
  LPDIRECT3DTEXTURE8 m_UVLookup;
  LPDIRECT3DTEXTURE8 m_UVErrorLookup;
  YUVRANGE m_yuvrange_last;
  YUVCOEF  m_yuvcoef_last;

  // Pixel shaders
  DWORD m_hInterleavingShader;
  DWORD m_hYUVtoRGBLookup;


  // Vertex types
  static const DWORD FVF_YUVRGBVERTEX = D3DFVF_XYZRHW | D3DFVF_TEX4;
  static const DWORD FVF_RGBVERTEX = D3DFVF_XYZRHW | D3DFVF_TEX1;
};

#endif
