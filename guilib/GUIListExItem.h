/*!
\file GUIListExItem.h
\brief 
*/

#ifndef GUILIB_GUIListExItem_H
#define GUILIB_GUIListExItem_H

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
#include "GUIItem.h"

/*!
 \ingroup controls
 \brief 
 */
class CGUIListExItem : public CGUIItem
{
public:
class RenderContext : public CGUIItem::RenderContext
  {
  public:
    RenderContext()
    {
      m_pButton = NULL;
      m_bActive = FALSE;
    };
    virtual ~RenderContext(){};

    CGUIButtonControl* m_pButton;
    CLabelInfo m_label;
    bool m_bActive;
  };

  CGUIListExItem(CStdString& aItemName);
  virtual ~CGUIListExItem(void);
  virtual void AllocResources();
  virtual void FreeResources();
  virtual void OnPaint(CGUIItem::RenderContext* pContext);
  void SetIcon(CGUIImage* pImage);
  void SetIcon(float width, float height, const CStdString& aTexture);
  DWORD GetFramesFocused() { return m_dwFocusedDuration; };

protected:
  CGUIImage* m_pIcon;
  DWORD m_dwFocusedDuration;
  DWORD m_dwFrameCounter;
};
#endif
