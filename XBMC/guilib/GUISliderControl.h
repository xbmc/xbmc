#ifndef GUILIB_GUIsliderCONTROL_H
#define GUILIB_GUIsliderCONTROL_H

#pragma once
#include "gui3d.h"
#include "guicontrol.h"
#include "guimessage.h"
#include "guiImage.h"
#include "stdstring.h"
using namespace std;

class CGUISliderControl :
  public CGUIControl
{
public:
  CGUISliderControl(DWORD dwParentID, DWORD dwControlId, DWORD dwPosX, DWORD dwPosY, DWORD dwWidth, DWORD dwHeight, const CStdString& strBackGroundTexture,const CStdString& strMidTexture);
  virtual ~CGUISliderControl(void);
  virtual void Render();
  virtual bool CanFocus() const;  
	virtual void AllocResources();
  virtual void FreeResources();
  virtual bool OnMessage(CGUIMessage& message);
	void				 SetPercentage(int iPercent);
	int 				 GetPercentage() const;
	const CStdString& GetBackGroundTextureName() const { return m_guiBackground.GetFileName();};
	const CStdString& GetBackTextureMidName() const { return m_guiMid.GetFileName();};
protected:
  virtual void       Update() ;
	CGUIImage				m_guiBackground;
	CGUIImage				m_guiMid;
	int							m_iPercent;
};
#endif