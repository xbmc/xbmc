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

#include "GUIWindow.h"
#ifdef HAS_SCREENSAVER
#include "screensavers/ScreenSaver.h"
#endif

#include "utils/CriticalSection.h"

#define SCREENSAVER_FADE   1
#define SCREENSAVER_BLACK  2
#define SCREENSAVER_XBS    3

class CGUIWindowScreensaver : public CGUIWindow
{
public:
  CGUIWindowScreensaver(void);
  virtual ~CGUIWindowScreensaver(void);

  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);
  virtual bool OnMouse(const CPoint &point);
  virtual void Render();

private:
#ifdef HAS_SCREENSAVER
  CScreenSaver* m_pScreenSaver;
#endif
  bool m_bInitialized;
  CCriticalSection m_critSection;
};
