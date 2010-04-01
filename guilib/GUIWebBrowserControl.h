/*!
\file GUIWebBrowserControl.h
\brief
*/

#ifndef GUILIB_GUIWEBBROWSERCONTROL_H
#define GUILIB_GUIWEBBROWSERCONTROL_H

#pragma once

/*
 *      Copyright (C) 2010 Team XBMC
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

#include "GUIControl.h"
#include "GUIEmbeddedBrowserWindowObserver.h"

// #include "berkelium/Berkelium.hpp"
// #include "berkelium/Window.hpp"
// #include "berkelium/WindowDelegate.hpp"

// class GlDelegate;

class CGUIWebBrowserControl : public CGUIControl
{
public:
  CGUIWebBrowserControl(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height);
  virtual ~CGUIWebBrowserControl(void);
  virtual CGUIWebBrowserControl *Clone() const { return new CGUIWebBrowserControl(*this); };

  virtual void Render();
  //virtual bool OnMessage(CGUIMessage& message);
  //virtual bool OnAction(const CAction &action);
  virtual void AllocResources();
  virtual void FreeResources();
  virtual EVENT_RESULT OnMouseEvent(const CPoint &point, const CMouseEvent &event);

  void Back();
  void Forward();

#if 0
  bool m_bKeepSession;
  bool m_isZoomed;

protected:
  bool OnMouseOver(const CPoint &point);
  bool OnMouseWheel(char wheel, const CPoint &point);
  bool OnMouseClick(DWORD dwButton, const CPoint &point);

  int m_realPosX, m_realPosY, m_realHeight, m_realWidth;
  bool m_bDirty;    /* to prevent two operations in the same loop which cause bad things during scrolling */
#endif

private:
  unsigned int m_texture;
  CGUIEmbeddedBrowserWindowObserver *m_observer;
//   Berkelium::Window *m_window;
//   GlDelegate *m_delegate;
};

#endif
