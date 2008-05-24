/*!
\file GUIEditControl.h
\brief 
*/

#ifndef GUILIB_GUIEditControl_H
#define GUILIB_GUIEditControl_H

#pragma once

/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "GUILabelControl.h"

/*!
 \ingroup controls
 \brief 
 */

class IEditControlObserver
{
public:
  virtual void OnEditTextComplete(CStdString& strLineOfText) = 0;
  virtual ~IEditControlObserver() {}
};

class CGUIEditControl : public CGUILabelControl
{
public:
  CGUIEditControl(DWORD dwParentID, DWORD dwControlId, float posX, float posY,
                  float width, float height, const CLabelInfo& labelInfo, const std::string& strLabel);

  virtual ~CGUIEditControl(void);

  virtual void SetObserver(IEditControlObserver* aObserver);
  virtual void OnKeyPress(const CAction &action); // FIXME TESTME: NEW/CHANGED parameter and NOT tested CAN'T do it/DON'T know where (window 2700)/how exactly 
  virtual void Render();

protected:
  void RecalcLabelPosition();

protected:
  IEditControlObserver* m_pObserver;
  float m_originalPosX;
};
#endif
