#ifndef COMBO_RENDERER
#define COMBO_RENDERER

#include "XBoxRenderer.h"

//const DWORD FVF_YV12VERTEX = D3DFVF_XYZRHW|D3DFVF_TEX3;

class CComboRenderer : public CXBoxRenderer
{
public:
  CComboRenderer(LPDIRECT3DDEVICE8 pDevice);
  //~CComboRenderer();

  virtual void Update(bool bPauseDrawing);
  virtual void CheckScreenSaver();
  virtual void SetupScreenshot();

  // Functions called from mplayer
  // virtual void     WaitForFlip();
  virtual unsigned int Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, float fps);
  virtual unsigned int PreInit();
  virtual void UnInit();

protected:
  virtual void Render();
  virtual void ManageDisplay();
  virtual void ManageTextures();
  bool CreateYUY2Texture();
  void DeleteYUY2Texture();
  void ClearYUY2Texture();
  void YV12toYUY2();
  LONG YUV2RGB(BYTE y, BYTE u, BYTE v);

  DWORD m_hPixelShader;

  // RGB/YUY2 texture target(s)
  LPDIRECT3DTEXTURE8 m_RGBTexture;
  D3DTexture m_YUY2Texture;

  static const DWORD FVF_YUYVVERTEX = D3DFVF_XYZRHW | D3DFVF_TEX4;
  static const DWORD FVF_RGBVERTEX = D3DFVF_XYZRHW | D3DFVF_TEX1;

  int m_iScreenWidth;
  int m_iScreenHeight;
  // screensaver stuff
  bool m_bHasDimView;
};

#endif
