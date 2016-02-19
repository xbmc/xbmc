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

#include "InterProcess_GUI_ControlEdit.h"
#include "InterProcess.h"

extern "C"
{

void CKODIAddon_InterProcess_GUI_ControlEdit::Control_Edit_SetVisible(void* control, bool visible)
{
  g_interProcess.m_Callbacks->GUI.Control.Edit.SetVisible(g_interProcess.m_Handle->addonData, control, visible);
}

void CKODIAddon_InterProcess_GUI_ControlEdit::Control_Edit_SetEnabled(void* control, bool enabled)
{
  g_interProcess.m_Callbacks->GUI.Control.Edit.SetEnabled(g_interProcess.m_Handle->addonData, control, enabled);
}

void CKODIAddon_InterProcess_GUI_ControlEdit::Control_Edit_SetLabel(void* control, const std::string& label)
{
  g_interProcess.m_Callbacks->GUI.Control.Edit.SetLabel(g_interProcess.m_Handle->addonData, control, label.c_str());
}

std::string CKODIAddon_InterProcess_GUI_ControlEdit::Control_Edit_GetLabel(void* control) const
{
  std::string text;
  text.resize(1024);
  unsigned int size = (unsigned int)text.capacity();
  g_interProcess.m_Callbacks->GUI.Control.Edit.GetLabel(g_interProcess.m_Handle->addonData, control, text[0], size);
  text.resize(size);
  text.shrink_to_fit();
  return text;
}

void CKODIAddon_InterProcess_GUI_ControlEdit::Control_Edit_SetText(void* control, const std::string& label)
{
  g_interProcess.m_Callbacks->GUI.Control.Edit.SetText(g_interProcess.m_Handle->addonData, control, label.c_str());
}

std::string CKODIAddon_InterProcess_GUI_ControlEdit::Control_Edit_GetText(void* control) const
{
  std::string text;
  text.resize(1024);
  unsigned int size = (unsigned int)text.capacity();
  g_interProcess.m_Callbacks->GUI.Control.Edit.GetText(g_interProcess.m_Handle->addonData, control, text[0], size);
  text.resize(size);
  text.shrink_to_fit();
  return text;
}

void CKODIAddon_InterProcess_GUI_ControlEdit::Control_Edit_SetCursorPosition(void* control, unsigned int iPosition)
{
  g_interProcess.m_Callbacks->GUI.Control.Edit.SetCursorPosition(g_interProcess.m_Handle->addonData, control, iPosition);
}

unsigned int CKODIAddon_InterProcess_GUI_ControlEdit::Control_Edit_GetCursorPosition(void* control)
{
  return g_interProcess.m_Callbacks->GUI.Control.Edit.GetCursorPosition(g_interProcess.m_Handle->addonData, control);
}

void CKODIAddon_InterProcess_GUI_ControlEdit::Control_Edit_SetInputType(void* control, AddonGUIInputType type, const std::string& heading)
{
  g_interProcess.m_Callbacks->GUI.Control.Edit.SetInputType(g_interProcess.m_Handle->addonData, control, type, heading.c_str());
}

}; /* extern "C" */
