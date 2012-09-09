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

#include "guilib/GUIWindow.h"
#include "utils/Stopwatch.h"

class CGUIWindowWeather : public CGUIWindow
{
public:
  CGUIWindowWeather(void);
  virtual ~CGUIWindowWeather(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual void FrameMove();

protected:
  virtual void OnInitWindow();

  void UpdateButtons();
  void UpdateLocations();
  void SetProperties();
  void ClearProperties();
  void SetLocation(int loc);

  unsigned int m_maxLocation;
};
