/*!
\file guiImage.h
\brief 
*/

#ifndef GUILIB_GUIIMAGECONTROL_H
#define GUILIB_GUIIMAGECONTROL_H

#pragma once

#include "GUIControl.h"

struct FRECT
{
  float left;
  float top;
  float right;
  float bottom;
};

// image alignment for <aspect>keep</aspect>, <aspect>scale</aspect> or <aspect>center</aspect>
#define ASPECT_ALIGN_CENTER  0
#define ASPECT_ALIGN_LEFT    1
#define ASPECT_ALIGN_RIGHT   2
#define ASPECT_ALIGNY_CENTER 0
#define ASPECT_ALIGNY_TOP    4
#define ASPECT_ALIGNY_BOTTOM 8
#define ASPECT_ALIGN_MASK    3
#define ASPECT_ALIGNY_MASK  ~3

class CImage
{
public:
  CImage(const CStdString &fileName)
  {
    file = fileName;
    memset(&border, 0, sizeof(FRECT));
    flipX = flipY = false;
  };

  CImage()
  {
    memset(&border, 0, sizeof(FRECT));
    flipX = flipY = false;
  };

  void operator=(const CImage &left)
  {
    file = left.file;
    memcpy(&border, &left.border, sizeof(FRECT));
    flipX = left.flipX;
    flipY = left.flipY;
    diffuse = left.diffuse;
  };
  CStdString file;
  FRECT      border;  // scaled  - unneeded if we get rid of scale on load
  bool       flipX;   // flip horizontally
  bool       flipY;   // flip vertically
  CStdString diffuse; // diffuse overlay texture (unimplemented)
};

/*!
 \ingroup controls
 \brief 
 */

class CGUIImage : public CGUIControl
{
public:
  enum GUIIMAGE_ASPECT_RATIO { ASPECT_RATIO_STRETCH = 0, ASPECT_RATIO_SCALE, ASPECT_RATIO_KEEP, ASPECT_RATIO_CENTER };

  CGUIImage(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, const CImage& texture, DWORD dwColorKey = 0);
  CGUIImage(const CGUIImage &left);
  virtual ~CGUIImage(void);

  virtual void Render();
  virtual bool OnAction(const CAction &action) ;
  virtual bool OnMessage(CGUIMessage& message);
  virtual void PreAllocResources();
  virtual void AllocResources();
  virtual void FreeResources();
  virtual void DynamicResourceAlloc(bool bOnOff);
  virtual bool IsDynamicallyAllocated() { return m_bDynamicResourceAlloc; };
  virtual bool CanFocus() const;
  virtual bool IsAllocated() const;

  void PythonSetColorKey(DWORD dwColorKey);
  void SetFileName(const CStdString& strFileName);
  void SetAspectRatio(GUIIMAGE_ASPECT_RATIO ratio, DWORD align = ASPECT_ALIGN_CENTER | ASPECT_ALIGNY_CENTER);
  void SetAlpha(unsigned char alpha);
  void SetAlpha(unsigned char a0, unsigned char a1, unsigned char a2, unsigned char a3);
  void SetInfo(int info) { m_Info = info; };

  const CStdString& GetFileName() const { return m_strFileName;};
  int GetTextureWidth() const;
  int GetTextureHeight() const;

  void CalculateSize();
#ifdef _DEBUG
  virtual void DumpTextureUse();
#endif
protected:
  void FreeTextures();
  void Process();
#ifndef _LINUX  
  static const DWORD FVF_VERTEX = D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1;
  static const DWORD FVF_VERTEX2 = D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX2;
#endif
  void Render(float left, float top, float bottom, float right, float u1, float v1, float u2, float v2);

  DWORD m_dwColorKey;
  unsigned char m_alpha[4];
  CStdString m_strFileName;
  int m_iTextureWidth;
  int m_iTextureHeight;
  int m_iImageWidth;
  int m_iImageHeight;
  int m_iCurrentLoop;
  int m_iCurrentImage;
  DWORD m_dwFrameCounter;
  GUIIMAGE_ASPECT_RATIO m_aspectRatio;
  DWORD                 m_aspectAlign;
#ifndef HAS_SDL
  vector <LPDIRECT3DTEXTURE8> m_vecTextures;
  LPDIRECT3DTEXTURE8 m_diffuseTexture;
  LPDIRECT3DPALETTE8 m_diffusePalette;
  LPDIRECT3DPALETTE8 m_pPalette;
#else
  vector <SDL_Surface*> m_vecTextures;
  vector <SDL_Surface*> m_vecZoomedTextures;
  SDL_Surface* m_diffuseTexture;
  SDL_Palette* m_diffusePalette;
  SDL_Palette* m_pPalette;  
#endif  
  float m_diffuseScaleU, m_diffuseScaleV;  
  bool m_bWasVisible;
  bool m_bDynamicResourceAlloc;

  // for when we are changing textures
  bool m_texturesAllocated;

  //vertex values
  float m_fX;
  float m_fY;
  float m_fU;
  float m_fV;
  float m_fNW;
  float m_fNH;
  bool m_linearTexture; // true if it's a linear 32bit texture

  // conditional info
  int m_Info;

  // border
  CImage m_image;
};
#endif
