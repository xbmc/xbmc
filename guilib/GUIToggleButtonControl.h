/*!
	\file GUIToggleButtonControl.h
	\brief 
	*/

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

/*!
	\ingroup controls
	\brief 
	*/
class CGUIToggleButtonControl : public CGUIControl
{
public:
  CGUIToggleButtonControl(DWORD dwParentID, DWORD dwControlId, int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight, const CStdString& strTextureFocus,const CStdString& strTextureNoFocus, const CStdString& strAltTextureFocus,const CStdString& strAltTextureNoFocus);
  virtual ~CGUIToggleButtonControl(void);
  
  virtual void Render();
  virtual void OnAction(const CAction &action) ;
  virtual bool OnMessage(CGUIMessage& message);
	virtual void PreAllocResources();
  virtual void AllocResources();
  virtual void FreeResources();
  virtual void SetPosition(int iPosX, int iPosY);
  virtual void SetAlpha(DWORD dwAlpha);
  virtual void SetColourDiffuse(D3DCOLOR colour);
  virtual void SetDisabledColor(D3DCOLOR color);
  	DWORD	GetTextOffsetX() const { return m_lTextOffsetX;};
	DWORD	GetTextOffsetY() const { return m_lTextOffsetY;};
  	void	SetTextOffsetX(long lTextOffsetX) { m_lTextOffsetX=lTextOffsetX;};
	void	SetTextOffsetY(long lTextOffsetY) { m_lTextOffsetY=lTextOffsetY;};
	void        SetLabel(const CStdString& strFontName,const wstring& strLabel,D3DCOLOR dwColor);
	void        SetLabel(const CStdString& strFontName,const CStdString& strLabel,D3DCOLOR dwColor);
  void        SetHyperLink(long dwWindowID);
	const	CStdString& GetTexutureAltFocusName() const { return m_imgAltFocus.GetFileName(); };
	const	CStdString& GetTexutureAltNoFocusName() const { return m_imgAltNoFocus.GetFileName(); };
	const	CStdString& GetTexutureFocusName() const { return m_imgFocus.GetFileName(); };
	const	CStdString& GetTexutureNoFocusName() const { return m_imgNoFocus.GetFileName(); };
	DWORD							GetTextColor() const { return m_dwTextColor;};
	DWORD							GetDisabledColor() const { return m_dwDisabledColor;};
	const char*				GetFontName() const { return m_pFont ? m_pFont->GetFontName().c_str() : ""; };
	const wstring			GetLabel() const { return m_strLabel; };
	DWORD							GetHyperLink() const { return m_lHyperLinkWindowID;};
	virtual void	OnMouseClick(DWORD dwButton);

protected:
  virtual void       Update() ;
  CGUIImage               m_imgAltFocus;
  CGUIImage               m_imgAltNoFocus;  
  CGUIImage               m_imgFocus;
  CGUIImage               m_imgNoFocus;  
  DWORD                   m_dwFrameCounter;
	wstring									m_strLabel;
	CGUIFont*								m_pFont;
  long		m_lTextOffsetX;
  long		m_lTextOffsetY;
	D3DCOLOR								m_dwTextColor;
  D3DCOLOR                m_dwDisabledColor;
  long                    m_lHyperLinkWindowID;
};
#endif
