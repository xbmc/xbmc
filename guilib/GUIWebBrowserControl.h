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

class CGUIWebBrowserControl : public CGUIControl
{
public:
  CGUIWebBrowserControl(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height);
  virtual ~CGUIWebBrowserControl(void);
  virtual CGUIWebBrowserControl *Clone() const { return new CGUIWebBrowserControl(*this); };

  virtual void Render();
  virtual void AllocResources();
  virtual void FreeResources();
  virtual EVENT_RESULT OnMouseEvent(const CPoint &point, const CMouseEvent &event);
  void Back();
  void Forward();
private:
  unsigned int m_texture;
  CGUIEmbeddedBrowserWindowObserver *m_observer;
};

#endif
