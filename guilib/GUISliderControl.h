/*!
\file GUISliderControl.h
\brief 
*/

#ifndef GUILIB_GUIsliderCONTROL_H
#define GUILIB_GUIsliderCONTROL_H

#pragma once

#include "guiImage.h"

#define SPIN_CONTROL_TYPE_INT    1
#define SPIN_CONTROL_TYPE_FLOAT  2
#define SPIN_CONTROL_TYPE_TEXT   3

/*!
 \ingroup controls
 \brief 
 */
class CGUISliderControl :
      public CGUIControl
{
public:
  CGUISliderControl(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, const CImage& backGroundTexture, const CImage& mibTexture, const CImage& nibTextureFocus, int iType);
  virtual ~CGUISliderControl(void);
  virtual void Render();
  virtual bool OnAction(const CAction &action);
  virtual void PreAllocResources();
  virtual void AllocResources();
  virtual void FreeResources();
  virtual void DynamicResourceAlloc(bool bOnOff);
  virtual void SetRange(int iStart, int iEnd);
  virtual void SetFloatRange(float fStart, float fEnd);
  virtual bool OnMessage(CGUIMessage& message);
  void SetInfo(int iInfo);
  void SetPercentage(int iPercent);
  int GetPercentage() const;
  void SetIntValue(int iValue);
  int GetIntValue() const;
  void SetFloatValue(float fValue);
  float GetFloatValue() const;
  void SetFloatInterval(float fInterval);
  void SetType(int iType) { m_iType = iType; };
  void SetControlOffsetX(float controlOffsetX) { m_controlOffsetX = controlOffsetX;};
  void SetControlOffsetY(float controlOffsetY) { m_controlOffsetY = controlOffsetY;};
  virtual bool HitTest(const CPoint &point) const;
  virtual bool OnMouseClick(DWORD dwButton, const CPoint &point);
  virtual bool OnMouseDrag(const CPoint &offset, const CPoint &point);
  virtual bool OnMouseWheel(char wheel, const CPoint &point);
  virtual CStdString GetDescription() const;
  void SetFormatString(const char *format) { if (format) m_formatString = format; };
  virtual void SetColorDiffuse(const CGUIInfoColor &color);
protected:
  virtual void Update() ;
  virtual void Move(int iNumSteps);
  virtual void SetFromPosition(const CPoint &point);

  CGUIImage m_guiBackground;
  CGUIImage m_guiMid;
  CGUIImage m_guiMidFocus;
  int m_iPercent;
  int m_iType;
  int m_iStart;
  int m_iEnd;
  float m_fStart;
  float m_fEnd;
  int m_iValue;
  float m_fValue;
  float m_fInterval;
  float m_controlOffsetX;
  float m_controlOffsetY;
  int m_iInfoCode;
  bool m_renderText;
  CStdString m_formatString;
};
#endif
