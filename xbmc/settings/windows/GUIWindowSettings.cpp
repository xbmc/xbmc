/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#ifndef WINDOWS_SYSTEM_H_INCLUDED
#define WINDOWS_SYSTEM_H_INCLUDED
#include "system.h"
#endif

#ifndef WINDOWS_GUIWINDOWSETTINGS_H_INCLUDED
#define WINDOWS_GUIWINDOWSETTINGS_H_INCLUDED
#include "GUIWindowSettings.h"
#endif

#ifndef WINDOWS_GUILIB_WINDOWIDS_H_INCLUDED
#define WINDOWS_GUILIB_WINDOWIDS_H_INCLUDED
#include "guilib/WindowIDs.h"
#endif


CGUIWindowSettings::CGUIWindowSettings(void)
    : CGUIWindow(WINDOW_SETTINGS_MENU, "Settings.xml")
{
  m_loadType = KEEP_IN_MEMORY;
}

CGUIWindowSettings::~CGUIWindowSettings(void)
{
}
