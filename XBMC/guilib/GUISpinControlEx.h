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
  CGUISpinControlEx(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, float spinWidth, float spinHeight, const CLabelInfo& spinInfo, const CImage &textureFocus, const CImage &textureNoFocus, const CImage& textureUp, const CImage& textureDown, const CImage& textureUpFocus, const CImage& textureDownFocus, const CLabelInfo& labelInfo, int iType);
  virtual ~CGUISpinControlEx(void);
  virtual void Render();
  virtual void SetPosition(float posX, float posY);
  virtual float GetWidth() const { return m_buttonControl.GetWidth();};
  virtual void SetWidth(float width);
  virtual float GetHeight() const { return m_buttonControl.GetHeight();};
  virtual void SetHeight(float height);
  virtual void PreAllocResources();
  virtual void AllocResources();
  virtual void FreeResources();
  virtual void DynamicResourceAlloc(bool bOnOff);
  const CStdString GetCurrentLabel() const;
  void SetText(const std::string & aLabel) {m_buttonControl.SetLabel(aLabel);};
  virtual void SetVisible(bool bVisible);
  virtual void SetColorDiffuse(D3DCOLOR color);
  const CLabelInfo& GetButtonLabelInfo() { return m_buttonControl.GetLabelInfo(); };
  virtual void SetEnabled(bool bEnable);
  virtual float GetXPosition() const { return m_buttonControl.GetXPosition();};
  virtual float GetYPosition() const { return m_buttonControl.GetYPosition();};
  virtual CStdString GetDescription() const;
  virtual bool HitTest(const CPoint &point) const { return m_buttonControl.HitTest(point); };
  void SetSpinPosition(float spinPosX);

  void SettingsCategorySetSpinTextColor(D3DCOLOR color);
protected:
  CGUIButtonControl m_buttonControl;
  float m_spinPosX;
};
#endif
