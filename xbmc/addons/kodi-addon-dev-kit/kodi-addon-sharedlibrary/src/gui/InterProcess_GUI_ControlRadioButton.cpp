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

#include "InterProcess_GUI_ControlRadioButton.h"
#include "InterProcess.h"

extern "C"
{

void CKODIAddon_InterProcess_GUI_ControlRadioButton::Control_RadioButton_SetVisible(void* window, bool visible)
{
  g_interProcess.m_Callbacks->GUI.Control.RadioButton.SetVisible(g_interProcess.m_Handle->addonData, window, visible);
}

void CKODIAddon_InterProcess_GUI_ControlRadioButton::Control_RadioButton_SetEnabled(void* window, bool enabled)
{
  g_interProcess.m_Callbacks->GUI.Control.RadioButton.SetEnabled(g_interProcess.m_Handle->addonData, window, enabled);
}

void CKODIAddon_InterProcess_GUI_ControlRadioButton::Control_RadioButton_SetLabel(void* window, const std::string& label)
{
  g_interProcess.m_Callbacks->GUI.Control.RadioButton.SetLabel(g_interProcess.m_Handle->addonData, window, label.c_str());
}

std::string CKODIAddon_InterProcess_GUI_ControlRadioButton::Control_RadioButton_GetLabel(void* window) const
{
  std::string text;
  text.resize(1024);
  unsigned int size = (unsigned int)text.capacity();
  g_interProcess.m_Callbacks->GUI.Control.RadioButton.GetLabel(g_interProcess.m_Handle->addonData, window, text[0], size);
  text.resize(size);
  text.shrink_to_fit();
  return text;
}

void CKODIAddon_InterProcess_GUI_ControlRadioButton::Control_RadioButton_SetSelected(void* window, bool yesNo)
{
  g_interProcess.m_Callbacks->GUI.Control.RadioButton.SetSelected(g_interProcess.m_Handle->addonData, window, yesNo);
}

bool CKODIAddon_InterProcess_GUI_ControlRadioButton::Control_RadioButton_IsSelected(void* window) const
{
  return g_interProcess.m_Callbacks->GUI.Control.RadioButton.IsSelected(g_interProcess.m_Handle->addonData, window);
}



}; /* extern "C" */
