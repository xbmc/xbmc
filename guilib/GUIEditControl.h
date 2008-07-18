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

#include "GUIButtonControl.h"

/*!
 \ingroup controls
 \brief 
 */

class CGUIEditControl : public CGUIButtonControl
{
public:
  CGUIEditControl(DWORD dwParentID, DWORD dwControlId, float posX, float posY,
                  float width, float height, const CImage &textureFocus, const CImage &textureNoFocus,
                  const CLabelInfo& labelInfo, const CStdString &text);

  virtual ~CGUIEditControl(void);

  virtual bool OnAction(const CAction &action);
  virtual void OnClick();

  void SetText(const CStdString &strLabel);
protected:
  virtual void RenderText();
  void RecalcLabelPosition();
  void ValidateCursor();

  CStdStringW m_text;
  float m_textOffset;
  float m_textWidth;

  unsigned int m_cursorPos;
  unsigned int m_cursorBlink;
};
#endif
