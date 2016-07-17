#pragma once

/*
 *      Copyright (C) 2015 Team XBMC
 *      http://xbmc.org
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

#include <memory>

#include "guilib/GUIWindow.h"

class CGUITextLayout;
class CGUIImage;

class CGUIWindowSplash : public CGUIWindow
{
public:
  CGUIWindowSplash(void);
  virtual ~CGUIWindowSplash(void);
  virtual bool OnAction(const CAction &action) { return false; };
  virtual void Render();
protected:
  virtual void OnInitWindow();
private:
  std::unique_ptr<CGUIImage> m_image;
};
