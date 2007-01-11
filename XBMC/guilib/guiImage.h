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

class CImage
{
public:
  CImage(const CStdString &fileName)
  {
    file = fileName;
    memset(&border, 0, sizeof(FRECT));
  };

  CImage()
  {
    memset(&border, 0, sizeof(FRECT));
  };

  void operator=(const CImage &left)
  {
    file = left.file;
    memcpy(&border, &left.border, sizeof(FRECT));
  };
  CStdString file;
  FRECT      border;  // scaled  - unneeded if we get rid of scale on load
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
  void SetAspectRatio(GUIIMAGE_ASPECT_RATIO ratio);
  void SetAlpha(const CColorDiffuse &alpha);
  void SetInfo(int info) { m_Info = info; };

  void GetBottomRight(float &x, float &y) const;
  const CStdString& GetFileName() const { return m_strFileName;};
  float GetRenderWidth() const;
  float GetRenderHeight() const;
  int GetTextureWidth() const;
  int GetTextureHeight() const;

  void CalculateSize();
protected:
  void FreeTextures();
  void Process();
  static const DWORD FVF_VERTEX = D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1;
  void Render(float left, float top, float bottom, float right, float u1, float v1, float u2, float v2);

  DWORD m_dwColorKey;
  CColorDiffuse m_alpha;
  CStdString m_strFileName;
  int m_iTextureWidth;
  int m_iTextureHeight;
  int m_iImageWidth;
  int m_iImageHeight;
  int m_iCurrentLoop;
  int m_iCurrentImage;
  DWORD m_dwFrameCounter;
  GUIIMAGE_ASPECT_RATIO m_aspectRatio;
  vector <LPDIRECT3DTEXTURE8> m_vecTextures;
  LPDIRECT3DPALETTE8 m_pPalette;
  float m_renderWidth;
  float m_renderHeight;
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
