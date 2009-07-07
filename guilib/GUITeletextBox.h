/*!
\file GUITeletextBox.h
\brief
*/

#ifndef GUILIB_GUICHARBOX_H
#define GUILIB_GUICHARBOX_H

#pragma once

/*
 *      Copyright (C) 2005-2009 Team XBMC
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
#include "GUITextLayout.h"
#include "utils/TeletextRender.h"

/*!
 \ingroup controls
 \brief
 */

class TiXmlNode;

class CGUITeletextBox : public CGUIControl, public CGUITextLayout
{
public:
  struct CharEntry {
    DWORD chr;
    DWORD fgColor;
    DWORD bgColor; 
    enumCharsets font;
  };
  CGUITeletextBox(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height,
              const CLabelInfo &labelInfo);
  CGUITeletextBox(const CGUITeletextBox &from);
  virtual ~CGUITeletextBox(void);
  virtual CGUITeletextBox *Clone() const { return new CGUITeletextBox(*this); };

  virtual void DoRender(DWORD currentTime);
  virtual void Render();
  virtual bool OnMessage(CGUIMessage& message);

  void SetPageControl(DWORD pageControl);

  virtual bool CanFocus() const;
  void SetInfo(const CGUIInfoLabel &info);
  CStdString GetLabel(int info) const;

  void SetCharacter(cTeletextChar c, int x, int y);

protected:
  virtual void UpdateColors();
  void UpdatePageControl();
  DWORD GetColorRGB(enumTeletextColor ttc);

  unsigned int m_itemsPerPage;
  float m_itemHeight;
  DWORD m_renderTime;
  DWORD m_lastRenderTime;

  CLabelInfo m_label;

  DWORD m_pageControl;

  CGUIInfoLabel m_info;

private:
  CharEntry *m_charBuffer;
};
#endif
