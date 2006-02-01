/*!
\file GUIButtonControl.h
\brief 
*/

#ifndef GUILIB_GUIBUTTONCONTROL_H
#define GUILIB_GUIBUTTONCONTROL_H

#pragma once

#include "GUIImage.h"

/*!
 \ingroup controls
 \brief 
 */
class CGUIButtonControl : public CGUIControl
{
public:
  CGUIButtonControl(DWORD dwParentID, DWORD dwControlId,
                    int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight,
                    const CStdString& strTextureFocus, const CStdString& strTextureNoFocus,
                    const CLabelInfo &label);

  virtual ~CGUIButtonControl(void);

  virtual void Render();
  virtual bool OnAction(const CAction &action) ;
  virtual bool OnMouseClick(DWORD dwButton);
  virtual bool OnMessage(CGUIMessage& message);
  virtual void PreAllocResources();
  virtual void AllocResources();
  virtual void FreeResources();
  virtual void DynamicResourceAlloc(bool bOnOff);
  virtual void SetPosition(int iPosX, int iPosY);
  virtual void SetAlpha(DWORD dwAlpha);
  virtual void SetColourDiffuse(D3DCOLOR colour);
  void SetText(const CStdString &aLabel);
  void SetText(const wstring & aLabel);
  void SetText2(const CStdString &aLabel2);
  void SetText2(const wstring & aLabel2);
  void SetHyperLink(long dwWindowID);
  void SetClickAction(const CStdString& clickAction) { m_clickAction = clickAction; };
  const CStdString &GetClickAction() const { return m_clickAction; };
  void SetFocusAction(const CStdString& focusAction) { m_focusAction = focusAction; };
  const CStdString &GetFocusAction() const { return m_focusAction; };
  const CStdString& GetTextureFocusName() const { return m_imgFocus.GetFileName(); };
  const CStdString& GetTextureNoFocusName() const { return m_imgNoFocus.GetFileName(); };
  const CLabelInfo& GetLabelInfo() const { return m_label; };
  const wstring GetLabel() const { return m_strLabel; };
  DWORD GetHyperLink() const { return m_lHyperLinkWindowID;};
  void SetTabButton(bool bIsTabButton = TRUE) { m_bTabButton = bIsTabButton; };
  void Flicker(bool bFlicker = TRUE);
  virtual void Update();
  virtual CStdString GetDescription() const;
  void PythonSetLabel(const CStdString &strFont, const wstring &strText, DWORD dwTextColor);
  void PythonSetDisabledColor(DWORD dwDisabledColor);

  void RAMSetTextColor(DWORD dwTextColor);
  void SettingsCategorySetTextAlign(DWORD dwAlign);
protected:
  void OnClick();
  void OnFocus();

  CGUIImage m_imgFocus;
  CGUIImage m_imgNoFocus;
  DWORD m_dwFocusCounter;
  DWORD m_dwFlickerCounter;
  DWORD m_dwFrameCounter;

  wstring m_strLabel;
  wstring m_strLabel2;
  CLabelInfo m_label;

  long m_lHyperLinkWindowID;
  CStdString m_clickAction;
  CStdString m_focusAction;
  bool m_bTabButton;
};
#endif
