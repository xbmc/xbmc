/*!
\file GUIButtonControl.h
\brief 
*/

#ifndef GUILIB_GUIBUTTONCONTROL_H
#define GUILIB_GUIBUTTONCONTROL_H

#pragma once

#include "guiImage.h"
#include "GUILabelControl.h"  // for CInfoPortion

/*!
 \ingroup controls
 \brief 
 */
class CGUIButtonControl : public CGUIControl
{
public:
  CGUIButtonControl(DWORD dwParentID, DWORD dwControlId,
                    float posX, float posY, float width, float height,
                    const CImage& textureFocus, const CImage& textureNoFocus,
                    const CLabelInfo &label);

  virtual ~CGUIButtonControl(void);

  virtual void Render();
  virtual bool OnAction(const CAction &action) ;
  virtual bool OnMouseClick(DWORD dwButton, const CPoint &point);
  virtual bool OnMessage(CGUIMessage& message);
  virtual void PreAllocResources();
  virtual void AllocResources();
  virtual void FreeResources();
  virtual void DynamicResourceAlloc(bool bOnOff);
  virtual void SetPosition(float posX, float posY);
  virtual void SetColorDiffuse(D3DCOLOR color);
  void SetLabel(const string & aLabel);
  void SetLabel2(const string & aLabel2);
  void SetClickActions(const vector<CStdString>& clickActions) { m_clickActions = clickActions; };
  const vector<CStdString> &GetClickActions() const { return m_clickActions; };
  void SetFocusActions(const vector<CStdString>& focusActions) { m_focusActions = focusActions; };
  const CLabelInfo& GetLabelInfo() const { return m_label; };
  virtual const string& GetLabel() const { return m_strLabel; };
  virtual const string& GetLabel2() const { return m_strLabel2; };
  void SetTabButton(bool bIsTabButton = TRUE) { m_bTabButton = bIsTabButton; };
  void SetSelected(bool bSelected);
  void Flicker(bool bFlicker = TRUE);
  virtual void Update();
  virtual CStdString GetDescription() const;
  void SetAlpha(unsigned char alpha);

  void PythonSetLabel(const CStdString &strFont, const string &strText, DWORD dwTextColor, DWORD dwShadowColor, DWORD dwFocusedColor);
  void PythonSetDisabledColor(DWORD dwDisabledColor);

  void RAMSetTextColor(DWORD dwTextColor);
  void SettingsCategorySetTextAlign(DWORD dwAlign);

  virtual void OnClick();
  bool HasClickActions() { return m_clickActions.size() > 0; };

protected:
  void OnFocus();

  CGUIImage m_imgFocus;
  CGUIImage m_imgNoFocus;
  DWORD m_dwFocusCounter;
  DWORD m_dwFlickerCounter;
  DWORD m_dwFrameCounter;
  unsigned char m_alpha;

  string m_strLabel;
  string m_strLabel2;
  vector<CInfoPortion>  m_multiInfo;
  CLabelInfo m_label;
  CGUITextLayout m_textLayout;
  CGUITextLayout m_textLayout2;

  vector<CStdString> m_clickActions;
  vector<CStdString> m_focusActions;
  bool m_bTabButton;

  bool m_bSelected;
};
#endif
