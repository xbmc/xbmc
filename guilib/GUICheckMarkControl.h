/*!
	\file GUICheckMarkControl.h
	\brief 
	*/

#ifndef CGUILIB_GUICHECKMARK_CONTROL_H
#define CGUILIB_GUICHECKMARK_CONTROL_H

#pragma once

#include "GUIImage.h"

/*!
	\ingroup controls
	\brief 
	*/
class CGUICheckMarkControl: public CGUIControl
{
public:
  CGUICheckMarkControl(DWORD dwParentID, DWORD dwControlId, int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight, const CStdString& strTextureCheckMark, const CStdString& strTextureCheckMarkNF,DWORD dwCheckWidth, DWORD dwCheckHeight,DWORD dwAlign=XBFONT_RIGHT);
  virtual ~CGUICheckMarkControl(void);
  virtual void Render();
  virtual void OnAction(const CAction &action) ;
  virtual bool OnMessage(CGUIMessage& message);
	virtual void PreAllocResources();
  virtual void AllocResources();
  virtual void FreeResources();
  virtual void SetDisabledColor(D3DCOLOR color);

	void SetLabel(const CStdString& strFontName,const wstring& strLabel,D3DCOLOR dwColor);
	void SetLabel(const CStdString& strFontName,const CStdString& strLabel,D3DCOLOR dwColor);
	DWORD							GetDisabledColor() const { return m_dwDisabledColor;};
	DWORD							GetTextColor() const { return m_dwTextColor;};
	DWORD							GetAlignment() const { return m_dwAlign;};
	const char*				GetFontName() const { return m_pFont ? m_pFont->GetFontName().c_str() : ""; };
	const wstring			GetLabel() const { return m_strLabel; };
	DWORD							GetCheckMarkWidth() const { return m_imgCheckMark.GetWidth(); };
	DWORD							GetCheckMarkHeight() const { return m_imgCheckMark.GetHeight(); };
	const CStdString& GetCheckMarkTextureName() const { return m_imgCheckMark.GetFileName(); };
  const CStdString& GetCheckMarkTextureNameNF() const { return m_imgCheckMarkNoFocus.GetFileName(); };
  void              SetSelected(bool bOnOff);
  bool              GetSelected() const;
  void              SetShadow(bool bOnOff);
  bool              GetShadow() const;
  void				OnMouseClick(DWORD dwButton);
protected:
  CGUIImage     m_imgCheckMark;
  CGUIImage     m_imgCheckMarkNoFocus;
  DWORD         m_dwTextColor	;
  CGUIFont*     m_pFont;
  wstring       m_strLabel;
  DWORD         m_dwDisabledColor;
	DWORD					m_dwAlign;  
  bool          m_bShadow;
};
#endif
