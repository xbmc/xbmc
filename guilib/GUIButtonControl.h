/*!
	\file GUIButtonControl.h
	\brief 
	*/

#ifndef GUILIB_GUIBUTTONCONTROL_H
#define GUILIB_GUIBUTTONCONTROL_H

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
class CGUIButtonControl : public CGUIControl
{
public:
  CGUIButtonControl(DWORD dwParentID, DWORD dwControlId,
	  int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight,
	  const CStdString& strTextureFocus,const CStdString& strTextureNoFocus,
	  DWORD dwTextXOffset, DWORD dwTextYOffset, DWORD dwAlign=XBFONT_LEFT);

  virtual ~CGUIButtonControl(void);
  
  virtual void Render();
  virtual void OnAction(const CAction &action) ;
  virtual void OnMouseClick(DWORD dwButton);
  virtual bool OnMessage(CGUIMessage& message);
	virtual void PreAllocResources();
  virtual void AllocResources();
  virtual void FreeResources();
  virtual void SetPosition(int iPosX, int iPosY);
  virtual void SetAlpha(DWORD dwAlpha);
  virtual void SetColourDiffuse(D3DCOLOR colour);
  virtual void SetDisabledColor(D3DCOLOR color);
	void        SetLabel(const CStdString& strFontName,const wstring& strLabel,D3DCOLOR dwColor);
	void        SetLabel(const CStdString& strFontName,const CStdString& strLabel,D3DCOLOR dwColor);
	void				SetText(const CStdString &aLabel);
	void				SetText(const wstring & aLabel);
  void        SetHyperLink(long dwWindowID);
	void				SetExecuteAction(const CStdString& strExecuteAction);
	const	CStdString& GetTexutureFocusName() const { return m_imgFocus.GetFileName(); };
	const	CStdString& GetTexutureNoFocusName() const { return m_imgNoFocus.GetFileName(); };
	DWORD	GetTextOffsetX() const { return m_dwTextOffsetX;};
	DWORD	GetTextOffsetY() const { return m_dwTextOffsetY;};
	void							SetTextColor(D3DCOLOR dwTextColor) { m_dwTextColor=dwTextColor;};
	DWORD							GetTextColor() const { return m_dwTextColor;};
	DWORD							GetTextAlign() const { return m_dwTextAlignment;};
	DWORD							GetDisabledColor() const { return m_dwDisabledColor;};
	const char *			GetFontName() const { return m_pFont ? m_pFont->GetFontName().c_str() : ""; };
	const wstring			GetLabel() const { return m_strLabel; };
	DWORD							GetHyperLink() const { return m_lHyperLinkWindowID;};
	const CStdString& GetExecuteAction() const { return m_strExecuteAction; };

protected:
  virtual void				Update() ;
  CGUIImage						m_imgFocus;
  CGUIImage						m_imgNoFocus;  
  DWORD								m_dwFrameCounter;
  DWORD								m_dwTextOffsetX;
  DWORD								m_dwTextOffsetY;
	DWORD								m_dwTextAlignment;
	wstring							m_strLabel;
	CGUIFont*						m_pFont;
	D3DCOLOR						m_dwTextColor;
  D3DCOLOR						m_dwDisabledColor;
  long								m_lHyperLinkWindowID;
	CStdString					m_strExecuteAction;
};
#endif
