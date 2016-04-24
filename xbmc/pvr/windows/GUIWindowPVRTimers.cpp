/*
 *      Copyright (C) 2012-2013 Team XBMC
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

#include "GUIWindowPVRTimers.h"
#include "utils/URIUtils.h"
#include "pvr/timers/PVRTimers.h"
#include "FileItem.h"

using namespace PVR;

CGUIWindowPVRTimers::CGUIWindowPVRTimers(bool bRadio) :
  CGUIWindowPVRTimersBase(bRadio, bRadio ? WINDOW_RADIO_TIMERS : WINDOW_TV_TIMERS, "MyPVRTimers.xml")
{
}

std::string CGUIWindowPVRTimers::GetDirectoryPath(void)
{
  const std::string basePath(CPVRTimersPath(m_bRadio, false).GetPath());
  return URIUtils::PathHasParent(m_vecItems->GetPath(), basePath) ? m_vecItems->GetPath() : basePath;
}
