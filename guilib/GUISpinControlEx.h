/*!
\file GUISpinControlEx.h
\brief 
*/

#ifndef GUILIB_SPINCONTROLEX_H
#define GUILIB_SPINCONTROLEX_H

#pragma once
#include "GUISpinControl.h"
#include "GUIButtonControl.h"

/*!
 \ingroup controls
 \brief 
 */
class CGUISpinControlEx : public CGUISpinControl
{
public:
  CGUISpinControlEx(DWORD dwParentID, DWORD dwControlId, int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight, DWORD dwSpinWidth, DWORD dwSpinHeight, DWORD dwSpinColor, const CStdString &strFocus, const CStdString &strNoFocus, const CStdString& strUp, const CStdString& strDown, const CStdString& strUpFocus, const CStdString& strDownFocus, const CLabelInfo& labelInfo, int iType);
  virtual ~CGUISpinControlEx(void);
  virtual void Render();
  virtual void SetPosition(int iPosX, int iPosY);
  virtual DWORD GetWidth() const { return m_buttonControl.GetWidth();};
  virtual void SetWidth(int iWidth);
  virtual DWORD GetHeight() const { return m_buttonControl.GetHeight();};
  virtual void SetHeight(int iHeight);
  virtual void PreAllocResources();
  virtual void AllocResources();
  virtual void FreeResources();
  virtual void DynamicResourceAlloc(bool bOnOff);
  const CStdString& GetTextureFocusName() const { return m_buttonControl.GetTextureFocusName(); };
  const CStdString& GetTextureNoFocusName() const { return m_buttonControl.GetTextureNoFocusName(); };
  const wstring GetLabel() const { return m_buttonControl.GetLabel(); };
  const CStdString GetCurrentLabel() const;
  void SetText(const CStdString &aLabel) {m_buttonControl.SetText(aLabel);};
  void SetText(const wstring & aLabel) {m_buttonControl.SetText(aLabel);};
  virtual void SetVisible(bool bVisible);
  virtual void SetColourDiffuse(D3DCOLOR color);
  const CLabelInfo& GetButtonLabelInfo() { return m_buttonControl.GetLabelInfo(); };
  virtual void SetEnabled(bool bEnable);
  virtual int GetXPosition() const { return m_buttonControl.GetXPosition();};
  virtual int GetYPosition() const { return m_buttonControl.GetYPosition();};
  virtual CStdString GetDescription() const;

  void SettingsCategorySetSpinTextColor(D3DCOLOR color);
protected:
  CGUIButtonControl m_buttonControl;
};
#endif
