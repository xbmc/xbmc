/*!
\file GUIItem.h
\brief 
*/

#ifndef GUILIB_GUIItem_H
#define GUILIB_GUIItem_H

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

class CGUIItem
{
public:
  class RenderContext
  {
  public:
    RenderContext()
    {
      m_positionX = m_positionY = 0;
      m_bFocused = false;
    };
    virtual ~RenderContext(){};

    float m_positionX;
    float m_positionY;
    bool m_bFocused;
  };

  CGUIItem(CStdString& aItemName);
  virtual ~CGUIItem(void);
  virtual void AllocResources() {}
  virtual void FreeResources() {}
  virtual void OnPaint(CGUIItem::RenderContext* pContext) = 0;
  virtual void GetDisplayText(CStdString& aString)
  {
    aString = m_strName;
  };

  CStdString GetName();
  void SetCookie(DWORD aCookie);
  DWORD GetCookie();

protected:
  DWORD m_dwCookie;
  CStdString m_strName;
};
#endif
