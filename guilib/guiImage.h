/*!
	\file guiImage.h
	\brief 
	*/

#ifndef GUILIB_GUIIMAGECONTROL_H
#define GUILIB_GUIIMAGECONTROL_H

#pragma once
#include "gui3d.h"
#include "guicontrol.h"
#include "guimessage.h"
#include "stdstring.h"
#include <vector>
using namespace std;

/*!
	\ingroup controls
	\brief 
	*/
class CGUIImage : public CGUIControl
{
public:
  CGUIImage(DWORD dwParentID, DWORD dwControlId, DWORD dwPosX, DWORD dwPosY, DWORD dwWidth, DWORD dwHeight, const CStdString& strTexture,DWORD dwColorKey=0);
  virtual ~CGUIImage(void);
  
  virtual void Render();
  virtual void Render(DWORD dwPosX, DWORD dwPosY, DWORD dwWidth, DWORD dwHeight);
  virtual void OnAction(const CAction &action) ;
  virtual bool OnMessage(CGUIMessage& message);
  virtual void AllocResources();
  virtual void FreeResources();
  virtual bool CanFocus() const;
  void         Select(int iBitmap);
  void         SetItems(int iItems);
	void				 SetTextureWidth(int iWidth);
	void				 SetTextureHeight(int iHeight);
	int					 GetTextureWidth() const;
	int					 GetTextureHeight() const;

	const CStdString& GetFileName() const {return m_strFileName;};
	DWORD						  GetColorKey() const {return m_dwColorKey;};
  void              SetKeepAspectRatio(bool bOnOff);
  bool              GetKeepAspectRatio() const;
  int               GetRenderWidth() const;
  int               GetRenderHeight() const;
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
	int m_iImageWidth;
	int m_iImageHeight;
  int                     m_iBitmap;
  DWORD                   m_dwItems;
  int                     m_iCurrentLoop;
	int										  m_iCurrentImage;
	DWORD										m_dwFrameCounter;
  bool                    m_bKeepAspectRatio;
  vector <LPDIRECT3DTEXTURE8> m_vecTextures;
  int                     m_iRenderWidth;
  int                     m_iRenderHeight;
};
#endif
