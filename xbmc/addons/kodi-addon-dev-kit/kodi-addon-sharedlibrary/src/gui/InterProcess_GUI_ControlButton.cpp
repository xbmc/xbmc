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

#include "InterProcess_GUI_ControlButton.h"
#include "InterProcess.h"

extern "C"
{

void CKODIAddon_InterProcess_GUI_ControlButton::Control_Button_SetVisible(void* control, bool visible)
{
  g_interProcess.m_Callbacks->GUI.Control.Button.SetVisible(g_interProcess.m_Handle->addonData, control, visible);
}

void CKODIAddon_InterProcess_GUI_ControlButton::Control_Button_SetEnabled(void* control, bool enabled)
{
  g_interProcess.m_Callbacks->GUI.Control.Button.SetEnabled(g_interProcess.m_Handle->addonData, control, enabled);
}

void CKODIAddon_InterProcess_GUI_ControlButton::Control_Button_SetLabel(void* control, const std::string& label)
{
  g_interProcess.m_Callbacks->GUI.Control.Button.SetLabel(g_interProcess.m_Handle->addonData, control, label.c_str());
}

std::string CKODIAddon_InterProcess_GUI_ControlButton::Control_Button_GetLabel(void* control) const
{
  std::string text;
  text.resize(1024);
  unsigned int size = (unsigned int)text.capacity();
  g_interProcess.m_Callbacks->GUI.Control.Button.GetLabel(g_interProcess.m_Handle->addonData, control, text[0], size);
  text.resize(size);
  text.shrink_to_fit();
  return text;
}

void CKODIAddon_InterProcess_GUI_ControlButton::Control_Button_SetLabel2(void* control, const std::string& label)
{
  g_interProcess.m_Callbacks->GUI.Control.Button.SetLabel2(g_interProcess.m_Handle->addonData, control, label.c_str());
}

std::string CKODIAddon_InterProcess_GUI_ControlButton::Control_Button_GetLabel2(void* control) const
{
  std::string text;
  text.resize(1024);
  unsigned int size = (unsigned int)text.capacity();
  g_interProcess.m_Callbacks->GUI.Control.Button.GetLabel2(g_interProcess.m_Handle->addonData, control, text[0], size);
  text.resize(size);
  text.shrink_to_fit();
  return text;
}

}; /* extern "C" */
