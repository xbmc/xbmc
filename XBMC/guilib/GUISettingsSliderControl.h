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
  CGUISettingsSliderControl(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, float sliderWidth, float sliderHeight, const CImage &textureFocus, const CImage &textureNoFocus, const CImage& backGroundTexture, const CImage& nibTexture, const CImage& nibTextureFocus, const CLabelInfo &labelInfo, int iType);
  virtual ~CGUISettingsSliderControl(void);
  virtual void Render();
  virtual bool OnAction(const CAction &action);
  virtual void PreAllocResources();
  virtual void AllocResources();
  virtual void FreeResources();
  virtual void DynamicResourceAlloc(bool bOnOff);
  virtual void SetPosition(float posX, float posY);
  virtual float GetWidth() const { return m_buttonControl.GetWidth();};
  virtual void SetWidth(float width);
  virtual float GetHeight() const { return m_buttonControl.GetHeight();};
  virtual void SetHeight(float height);
  virtual void SetEnabled(bool bEnable);
  virtual void SetColorDiffuse(D3DCOLOR color);

  void SetText(const string &label) {m_buttonControl.SetLabel(label);};
  virtual float GetXPosition() const { return m_buttonControl.GetXPosition();};
  virtual float GetYPosition() const { return m_buttonControl.GetYPosition();};
  virtual CStdString GetDescription() const;
  virtual bool HitTest(const CPoint &point) const { return m_buttonControl.HitTest(point); };
protected:
  virtual void Update() ;
  CGUIButtonControl m_buttonControl;
  CGUITextLayout m_textLayout;
};
#endif
