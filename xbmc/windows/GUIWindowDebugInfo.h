#pragma once

/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "guilib/GUIDialog.h"
#ifdef _LINUX
#include "linux/LinuxResourceCounter.h"
#endif

class CGUITextLayout;

class CGUIWindowDebugInfo :
      public CGUIDialog
{
public:
  CGUIWindowDebugInfo();
  virtual ~CGUIWindowDebugInfo();
  virtual void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions);
  virtual void Render();
  virtual bool OnMessage(CGUIMessage &message);
protected:
  virtual void UpdateVisibility();
private:
  CGUITextLayout *m_layout;
#ifdef _LINUX
  CLinuxResourceCounter m_resourceCounter;
#endif
};
