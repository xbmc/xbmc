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
  CGUIProgressControl(DWORD dwParentID, DWORD dwControlId, int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight, CStdString& strBackGroundTexture, CStdString& strLeftTexture, CStdString& strMidTexture, CStdString& strRightTexture, CStdString& strOverlayTexture);
  virtual ~CGUIProgressControl(void);
  virtual void Render();
  virtual bool CanFocus() const;
  virtual void PreAllocResources();
  virtual void AllocResources();
  virtual void FreeResources();
  virtual void DynamicResourceAlloc(bool bOnOff);
  virtual bool OnMessage(CGUIMessage& message);
  virtual void SetPosition(int iPosX, int iPosY);
  void SetPercentage(float fPercent);
  float GetPercentage() const;
  const CStdString& GetBackGroundTextureName() const { return m_guiBackground.GetFileName();};
  const CStdString& GetBackTextureLeftName() const { return m_guiLeft.GetFileName();};
  const CStdString& GetBackTextureRightName() const { return m_guiRight.GetFileName();};
  const CStdString& GetBackTextureMidName() const { return m_guiMid.GetFileName();};
  const CStdString& GetBackTextureOverlayName() const { return m_guiOverlay.GetFileName();};
protected:
  CGUIImage m_guiBackground;
  CGUIImage m_guiLeft;
  CGUIImage m_guiRight;
  CGUIImage m_guiMid;
  CGUIImage m_guiOverlay;
  float m_fPercent;
};
#endif
