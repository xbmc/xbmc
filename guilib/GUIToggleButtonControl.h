#ifndef GUILIB_GUITOGGLEBUTTONCONTROL_H
#define GUILIB_GUITOGGLEBUTTONCONTROL_H

#pragma once
#include "gui3d.h"
#include "guicontrol.h"
#include "guimessage.h"
#include "guifont.h"
#include "guiimage.h"
#include "stdstring.h"
using namespace std;

class CGUIToggleButtonControl : public CGUIControl
{
public:
  CGUIToggleButtonControl(DWORD dwParentID, DWORD dwControlId, DWORD dwPosX, DWORD dwPosY, DWORD dwWidth, DWORD dwHeight, const CStdString& strTextureFocus,const CStdString& strTextureNoFocus, const CStdString& strAltTextureFocus,const CStdString& strAltTextureNoFocus);
  virtual ~CGUIToggleButtonControl(void);
  
  virtual void Render();
  virtual void OnKey(const CKey& key) ;
  virtual bool OnMessage(CGUIMessage& message);
  virtual void AllocResources();
  virtual void FreeResources();
  virtual void SetPosition(DWORD dwPosX, DWORD dwPosY);
  virtual void SetAlpha(DWORD dwAlpha);
  virtual void SetColourDiffuse(D3DCOLOR colour);
  virtual void SetDisabledColor(D3DCOLOR color);
	void        SetLabel(const CStdString& strFontName,const wstring& strLabel,D3DCOLOR dwColor);
	void        SetLabel(const CStdString& strFontName,const CStdString& strLabel,D3DCOLOR dwColor);
  void        SetHyperLink(long dwWindowID);

protected:
  virtual void       Update() ;
  CGUIImage               m_imgAltFocus;
  CGUIImage               m_imgAltNoFocus;  
  CGUIImage               m_imgFocus;
  CGUIImage               m_imgNoFocus;  
  DWORD                   m_dwFrameCounter;
	wstring									m_strLabel;
	CGUIFont*								m_pFont;
	D3DCOLOR								m_dwTextColor;
  D3DCOLOR                m_dwDisabledColor;
  long                    m_lHyperLinkWindowID;
};
#endif
