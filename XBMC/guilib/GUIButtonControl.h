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
  CGUIButtonControl(DWORD dwParentID, DWORD dwControlId, DWORD dwPosX, DWORD dwPosY, DWORD dwWidth, DWORD dwHeight, const CStdString& strTextureFocus,const CStdString& strTextureNoFocus);
  virtual ~CGUIButtonControl(void);
  
  virtual void Render();
  virtual void OnAction(const CAction &action) ;
  virtual bool OnMessage(CGUIMessage& message);
	virtual void PreAllocResources();
  virtual void AllocResources();
  virtual void FreeResources();
  virtual void SetPosition(DWORD dwPosX, DWORD dwPosY);
  virtual void SetAlpha(DWORD dwAlpha);
  virtual void SetColourDiffuse(D3DCOLOR colour);
  virtual void SetDisabledColor(D3DCOLOR color);
	void        SetLabel(const CStdString& strFontName,const wstring& strLabel,D3DCOLOR dwColor);
	void        SetLabel(const CStdString& strFontName,const CStdString& strLabel,D3DCOLOR dwColor);
	void		SetText(CStdString aLabel);
  void        SetHyperLink(long dwWindowID);
	void				SetScriptAction(const CStdString& strScriptAction);
	const	CStdString& GetTexutureFocusName() const { return m_imgFocus.GetFileName(); };
	const	CStdString& GetTexutureNoFocusName() const { return m_imgNoFocus.GetFileName(); };
	DWORD							GetTextColor() const { return m_dwTextColor;};
	DWORD							GetDisabledColor() const { return m_dwDisabledColor;};
	const CStdString& GetFontName() const { return m_pFont->GetFontName(); };
	const wstring			GetLabel() const { return m_strLabel; };
	DWORD							GetHyperLink() const { return m_lHyperLinkWindowID;};
	const CStdString& GetScriptAction() const { return m_strScriptAction; };

protected:
  virtual void       Update() ;
  CGUIImage               m_imgFocus;
  CGUIImage               m_imgNoFocus;  
  DWORD                   m_dwFrameCounter;
	wstring									m_strLabel;
	CGUIFont*								m_pFont;
	D3DCOLOR								m_dwTextColor;
  D3DCOLOR                m_dwDisabledColor;
  long                    m_lHyperLinkWindowID;
	CStdString							m_strScriptAction;
};
#endif
