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

class CGUIEditControl : public CGUILabelControl
{
public:
  CGUIEditControl(DWORD dwParentID, DWORD dwControlId, float posX, float posY,
                  float width, float height, const CLabelInfo& labelInfo, const std::string& strLabel);

  virtual ~CGUIEditControl(void);

  virtual bool OnAction(const CAction &action);
  virtual void Render();

  virtual bool CanFocus() const { return true; }; // TODO:EDIT only needed because we're based on labelcontrol

protected:
  void RecalcLabelPosition();
  void ValidateCursor(int maxLength);

  float m_originalPosX;
  float m_originalWidth;
};
#endif
