#ifndef GUILIB_GUIIMAGECONTROL_H
#define GUILIB_GUIIMAGECONTROL_H

#pragma once
#include "gui3d.h"
#include "guicontrol.h"
#include "guimessage.h"
#include "stdstring.h"
#include <vector>
using namespace std;

class CGUIImage : public CGUIControl
{
public:
  CGUIImage(DWORD dwParentID, DWORD dwControlId, DWORD dwPosX, DWORD dwPosY, DWORD dwWidth, DWORD dwHeight, const CStdString& strTexture,DWORD dwColorKey=0);
  virtual ~CGUIImage(void);
  
  virtual void Render();
  virtual void OnKey(const CKey& key) ;
  virtual bool OnMessage(CGUIMessage& message);
  virtual void AllocResources();
  virtual void FreeResources();
  virtual bool CanFocus() const;
  void         Select(int iBitmap);
  void         SetItems(int iItems);

protected:
  virtual void       Update();
	void							 Process();
  struct VERTEX 
	{ 
    D3DXVECTOR4 p;
		D3DCOLOR col; 
		FLOAT tu, tv; 
	};
  static const DWORD FVF_VERTEX = D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_TEX1;

  DWORD                   m_dwColorKey;
  LPDIRECT3DVERTEXBUFFER8 m_pVB;
  CStdString              m_strFileName;
  int                     m_iTextureWidth;
  int                     m_iTextureHeight;
  int                     m_iBitmap;
  DWORD                   m_dwItems;
	int										  m_iCurrentImage;
	DWORD										m_dwFrameCounter;
  vector <LPDIRECT3DTEXTURE8> m_vecTextures;
};
#endif
