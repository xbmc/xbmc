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
  enum INPUT_TYPE {
                    INPUT_TYPE_TEXT = 0,
                    INPUT_TYPE_NUMBER,
                    INPUT_TYPE_SECONDS,
                    INPUT_TYPE_DATE,
                    INPUT_TYPE_IPADDRESS,
                    INPUT_TYPE_PASSWORD,
                    INPUT_TYPE_SEARCH,
                    INPUT_TYPE_FILTER
                  };

  CGUIEditControl(DWORD dwParentID, DWORD dwControlId, float posX, float posY,
                  float width, float height, const CTextureInfo &textureFocus, const CTextureInfo &textureNoFocus,
                  const CLabelInfo& labelInfo, const std::string &text);
  CGUIEditControl(const CGUIButtonControl &button);
  virtual ~CGUIEditControl(void);
  virtual CGUIEditControl *Clone() const { return new CGUIEditControl(*this); };

  virtual bool OnMessage(CGUIMessage &message);
  virtual bool OnAction(const CAction &action);
  virtual void OnClick();

  virtual void SetLabel(const std::string &text);
  virtual void SetLabel2(const std::string &text);

  virtual CStdString GetLabel2() const;

  void SetInputType(INPUT_TYPE type, int heading);
protected:
  virtual void RenderText();
  CStdStringW GetDisplayedText() const;
  void RecalcLabelPosition();
  void ValidateCursor();
  void OnTextChanged();

  CStdStringW m_text2;
  CStdString  m_text;
  float m_textOffset;
  float m_textWidth;

  static const int spaceWidth = 5;

  unsigned int m_cursorPos;
  unsigned int m_cursorBlink;

  int m_inputHeading;
  INPUT_TYPE m_inputType;
};
#endif
