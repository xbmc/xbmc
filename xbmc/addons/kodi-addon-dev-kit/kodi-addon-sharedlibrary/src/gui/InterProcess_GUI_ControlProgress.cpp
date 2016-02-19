/*
 *      Copyright (C) 2016 Team KODI
 *      http://kodi.tv
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
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "InterProcess_GUI_ControlProgress.h"
#include "InterProcess.h"

extern "C"
{

void CKODIAddon_InterProcess_GUI_ControlProgress::Control_Progress_SetVisible(void* window, bool visible)
{
  g_interProcess.m_Callbacks->GUI.Control.Progress.SetVisible(g_interProcess.m_Handle->addonData, window, visible);
}

void CKODIAddon_InterProcess_GUI_ControlProgress::Control_Progress_SetPercentage(void* window, float percent)
{
  g_interProcess.m_Callbacks->GUI.Control.Progress.SetPercentage(g_interProcess.m_Handle->addonData, window, percent);
}

float CKODIAddon_InterProcess_GUI_ControlProgress::Control_Progress_GetPercentage(void* window) const
{
  return g_interProcess.m_Callbacks->GUI.Control.Progress.GetPercentage(g_interProcess.m_Handle->addonData, window);
}

}; /* extern "C" */

