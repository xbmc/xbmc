/*!
	\file GUIVideoControl.h
	\brief 
	*/

#ifndef GUILIB_GUIVIDEOCONTROL_H
#define GUILIB_GUIVIDEOCONTROL_H

#pragma once
#include "gui3d.h"
#include "guicontrol.h"
#include "guimessage.h"
#include "guifont.h"
#include "stdstring.h"
using namespace std;

/*!
	\ingroup controls
	\brief 
	*/
class CGUIVideoControl :
  public CGUIControl
{
public:
  CGUIVideoControl(DWORD dwParentID, DWORD dwControlId, int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight);
  virtual ~CGUIVideoControl(void);
  virtual void Render();
  virtual void OnMouseClick(DWORD dwButton);
};
#endif
