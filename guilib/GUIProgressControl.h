#ifndef GUILIB_GUIPROGRESSCONTROL_H
#define GUILIB_GUIPROGRESSCONTROL_H

#pragma once
#include "gui3d.h"
#include "guicontrol.h"
#include "guimessage.h"
#include "guiImage.h"
#include "stdstring.h"
using namespace std;

class CGUIProgressControl :
  public CGUIControl
{
public:
  CGUIProgressControl(DWORD dwParentID, DWORD dwControlId, DWORD dwPosX, DWORD dwPosY, DWORD dwWidth, DWORD dwHeight, CStdString& strBackGroundTexture,CStdString& strForGroundTexture);
  virtual ~CGUIProgressControl(void);
  virtual void Render();
  virtual bool CanFocus() const;
  virtual bool OnMessage(CGUIMessage& message);
	void				 SetPercentage(int iPercent);
	int 				 GetPercentage() const;
protected:
	CGUIImage				m_guiBackground;
	CGUIImage				m_guiForeground;
	int							m_iPercent;
};
#endif