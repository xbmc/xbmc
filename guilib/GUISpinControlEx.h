/*!
	\file GUISpinControlEx.h
	\brief 
	*/

#ifndef GUILIB_SPINCONTROLEX_H
#define GUILIB_SPINCONTROLEX_H

#pragma once
#include "GUISpinControl.h"
#include "GUIButtonControl.h"

using namespace std;

/*!
	\ingroup controls
	\brief 
	*/
class CGUISpinControlEx :  public CGUISpinControl
{
public:  
  CGUISpinControlEx(DWORD dwParentID, DWORD dwControlId, int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight, DWORD dwSpinWidth, DWORD dwSpinHeight, const CStdString &strFocus, const CStdString &strNoFocus, const CStdString& strUp, const CStdString& strDown, const CStdString& strUpFocus, const CStdString& strDownFocus, const CStdString& strFont, DWORD dwTextColor,int iTextXOffset, int iTextYOffset, DWORD dwAlign, int iType);
  virtual ~CGUISpinControlEx(void);
  virtual void			Render();
  virtual void			SetPosition(int iPosX, int iPosY);
	virtual DWORD			GetWidth() const { return m_buttonControl.GetWidth();};
	virtual void			SetWidth(int iWidth) { m_buttonControl.SetWidth(iWidth); };
	virtual DWORD			GetHeight() const { return m_buttonControl.GetHeight();};
	virtual void			SetHeight(int iWidth) { m_buttonControl.SetHeight(iWidth); };
	virtual void			PreAllocResources();
  virtual void			AllocResources();
  virtual void			FreeResources();
	DWORD							GetAlignment() const { return m_buttonControl.GetTextAlign() & 0x00000004;};
	DWORD							GetTextOffsetX() const { return m_buttonControl.GetTextOffsetX();};
	DWORD							GetTextOffsetY() const { return m_buttonControl.GetTextOffsetY();};
	const	CStdString& GetTextureFocusName() const { return m_buttonControl.GetTexutureFocusName(); };
	const	CStdString& GetTextureNoFocusName() const { return m_buttonControl.GetTexutureNoFocusName(); };
	const wstring			GetLabel() const { return m_buttonControl.GetLabel(); };
	const CStdString	GetCurrentLabel() const;
	void							SetLabel(const CStdString &aLabel) {m_buttonControl.SetText(aLabel);};
	void							SetLabel(const wstring & aLabel) {m_buttonControl.SetText(aLabel);};
	virtual void			SetVisible(bool bVisible);
  virtual void			SetColourDiffuse(D3DCOLOR color);
	D3DCOLOR					GetButtonTextColor() { return m_buttonControl.GetTextColor(); };
	void							SetSpinTextColor(D3DCOLOR color);
  virtual void			SetDisabledColor(D3DCOLOR color);
	virtual void			SetEnabled(bool bEnable);
	virtual int				GetXPosition() const { return m_buttonControl.GetXPosition();};
	virtual int				GetYPosition() const { return m_buttonControl.GetYPosition();};

protected:
	CGUIButtonControl m_buttonControl;
};
#endif
