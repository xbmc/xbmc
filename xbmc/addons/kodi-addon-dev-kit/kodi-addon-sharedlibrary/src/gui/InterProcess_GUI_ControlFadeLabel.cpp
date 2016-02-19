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

#include "InterProcess_GUI_ControlFadeLabel.h"
#include "InterProcess.h"

extern "C"
{

void CKODIAddon_InterProcess_GUI_ControlFadeLabel::Control_FadeLabel_SetVisible(void* window, bool visible)
{
  g_interProcess.m_Callbacks->GUI.Control.FadeLabel.SetVisible(g_interProcess.m_Handle->addonData, window, visible);
}

void CKODIAddon_InterProcess_GUI_ControlFadeLabel::Control_FadeLabel_AddLabel(void* window, const std::string& text)
{
  g_interProcess.m_Callbacks->GUI.Control.FadeLabel.AddLabel(g_interProcess.m_Handle->addonData, window, text.c_str());
}

std::string CKODIAddon_InterProcess_GUI_ControlFadeLabel::Control_FadeLabel_GetLabel(void* window)
{
  std::string text;
  text.resize(1024);
  unsigned int size = (unsigned int)text.capacity();
  g_interProcess.m_Callbacks->GUI.Control.FadeLabel.GetLabel(g_interProcess.m_Handle->addonData, window, text[0], size);
  text.resize(size);
  text.shrink_to_fit();
  return text;
}

void CKODIAddon_InterProcess_GUI_ControlFadeLabel::Control_FadeLabel_SetScrolling(void* window, bool scroll)
{
  g_interProcess.m_Callbacks->GUI.Control.FadeLabel.SetScrolling(g_interProcess.m_Handle->addonData, window, scroll);
}

void CKODIAddon_InterProcess_GUI_ControlFadeLabel::Control_FadeLabel_Reset(void* window)
{
  g_interProcess.m_Callbacks->GUI.Control.FadeLabel.Reset(g_interProcess.m_Handle->addonData, window);
}


}; /* extern "C" */
