/*!
\file GUIProgressControl.h
\brief 
*/

#ifndef GUILIB_GUIPROGRESSCONTROL_H
#define GUILIB_GUIPROGRESSCONTROL_H

#pragma once

#include "GUIImage.h"

/*!
 \ingroup controls
 \brief 
 */
class CGUIProgressControl :
      public CGUIControl
{
public:
  CGUIProgressControl(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, const CImage& backGroundTexture, const CImage& leftTexture, const CImage& midTexture, const CImage& rightTexture, const CImage& overlayTexture);
  virtual ~CGUIProgressControl(void);
  virtual void Render();
  virtual bool CanFocus() const;
  virtual void PreAllocResources();
  virtual void AllocResources();
  virtual void FreeResources();
  virtual void DynamicResourceAlloc(bool bOnOff);
  virtual bool OnMessage(CGUIMessage& message);
  virtual void SetPosition(float posX, float posY);
  virtual void SetColorDiffuse(D3DCOLOR color);
  void SetPercentage(float fPercent);
  void SetInfo(int iInfo);
  int GetInfo() const {return m_iInfoCode;};

  float GetPercentage() const;
protected:
  CGUIImage m_guiBackground;
  CGUIImage m_guiLeft;
  CGUIImage m_guiRight;
  CGUIImage m_guiMid;
  CGUIImage m_guiOverlay;
  int m_iInfoCode;
  float m_fPercent;
};
#endif
