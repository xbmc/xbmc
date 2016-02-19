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

#include "InterProcess_GUI_ControlTextBox.h"
#include "InterProcess.h"

extern "C"
{

void CKODIAddon_InterProcess_GUI_ControlTextBox::Control_TextBox_SetVisible(void* window, bool visible)
{
  g_interProcess.m_Callbacks->GUI.Control.TextBox.SetVisible(g_interProcess.m_Handle->addonData, window, visible);
}

void CKODIAddon_InterProcess_GUI_ControlTextBox::Control_TextBox_Reset(void* window)
{
  g_interProcess.m_Callbacks->GUI.Control.TextBox.Reset(g_interProcess.m_Handle->addonData, window);
}

void CKODIAddon_InterProcess_GUI_ControlTextBox::Control_TextBox_SetText(void* window, const std::string& text)
{
  g_interProcess.m_Callbacks->GUI.Control.TextBox.SetText(g_interProcess.m_Handle->addonData, window, text.c_str());
}

std::string CKODIAddon_InterProcess_GUI_ControlTextBox::Control_TextBox_GetText(void* window) const
{
  std::string text;
  text.resize(16384);
  unsigned int size = (unsigned int)text.capacity();
  g_interProcess.m_Callbacks->GUI.Control.TextBox.GetText(g_interProcess.m_Handle->addonData, window, text[0], size);
  text.resize(size);
  text.shrink_to_fit();
  return text;
}

void CKODIAddon_InterProcess_GUI_ControlTextBox::Control_TextBox_Scroll(void* window, unsigned int position)
{
  g_interProcess.m_Callbacks->GUI.Control.TextBox.Scroll(g_interProcess.m_Handle->addonData, window, position);
}

void CKODIAddon_InterProcess_GUI_ControlTextBox::Control_TextBox_SetAutoScrolling(void* window, int delay, int time, int repeat)
{
  g_interProcess.m_Callbacks->GUI.Control.TextBox.SetAutoScrolling(g_interProcess.m_Handle->addonData, window, delay, time, repeat);
}


}; /* extern "C" */
