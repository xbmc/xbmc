/*!
\file GUISliderControl.h
\brief 
*/

#ifndef GUILIB_GUISettingsSliderCONTROL_H
#define GUILIB_GUISettingsSliderCONTROL_H

#pragma once

#include "GUISliderControl.h"
#include "GUIButtonControl.h"

#define SPIN_CONTROL_TYPE_INT    1
#define SPIN_CONTROL_TYPE_FLOAT  2
#define SPIN_CONTROL_TYPE_TEXT   3

/*!
 \ingroup controls
 \brief 
 */
class CGUISettingsSliderControl :
      public CGUISliderControl
{
public:
  CGUISettingsSliderControl(DWORD dwParentID, DWORD dwControlId, int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight, DWORD dwSliderWidth, DWORD dwSliderHeight, const CStdString &strFocus, const CStdString &strNoFocus, const CStdString& strBackGroundTexture, const CStdString& strMidTexture, const CStdString& strMidTextureFocus, int iTextXOffset, int iType);
  virtual ~CGUISettingsSliderControl(void);
  virtual void Render();
  // virtual bool CanFocus() const;
  virtual bool OnAction(const CAction &action);
  virtual void PreAllocResources();
  virtual void AllocResources();
  virtual void FreeResources();
  virtual void DynamicResourceAlloc(bool bOnOff);
  virtual void SetPosition(int iPosX, int iPosY);
  virtual DWORD GetWidth() const { return m_buttonControl.GetWidth();};
  virtual void SetWidth(int iWidth);
  virtual DWORD GetHeight() const { return m_buttonControl.GetHeight();};
  virtual void SetHeight(int iHeight);
  virtual void SetEnabled(bool bEnable);
  virtual void SetDisabledColor(D3DCOLOR color) { m_buttonControl.SetDisabledColor(color); };
  DWORD GetSliderWidth() const { return m_dwWidth; };
  DWORD GetSliderHeight() const { return m_dwHeight; };
  DWORD GetDisabledColor() const { return m_buttonControl.GetDisabledColor(); };
  DWORD GetTextColor() const { return m_buttonControl.GetTextColor(); };
  const char *GetFontName() const { return m_buttonControl.GetFontName(); };
  DWORD GetTextOffsetX() const { return m_buttonControl.GetTextOffsetX();};
  DWORD GetTextOffsetY() const { return m_buttonControl.GetTextOffsetY();};
  const CStdString& GetTextureFocusName() const { return m_buttonControl.GetTextureFocusName(); };
  const CStdString& GetTextureNoFocusName() const { return m_buttonControl.GetTextureNoFocusName(); };
  const wstring GetLabel() const { return m_buttonControl.GetLabel(); };
  void SetLabel(const CStdString &fontName, const wstring &label, D3DCOLOR color) {m_buttonControl.SetLabel(fontName, label, color);};
  void SetLabel(const CStdString &label) {m_buttonControl.SetText(label);};
  virtual int GetXPosition() const { return m_buttonControl.GetXPosition();};
  virtual int GetYPosition() const { return m_buttonControl.GetYPosition();};
  virtual CStdString GetDescription() const;

protected:
  virtual void Update() ;
  CGUIButtonControl m_buttonControl;
};
#endif
