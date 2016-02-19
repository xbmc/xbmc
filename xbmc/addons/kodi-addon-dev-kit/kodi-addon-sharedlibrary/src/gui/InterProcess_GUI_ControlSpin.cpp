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

#include "InterProcess_GUI_ControlSpin.h"
#include "InterProcess.h"

extern "C"
{

void CKODIAddon_InterProcess_GUI_ControlSpin::Control_Spin_SetVisible(void* window, bool visible)
{
  g_interProcess.m_Callbacks->GUI.Control.Spin.SetVisible(g_interProcess.m_Handle->addonData, window, visible);
}

void CKODIAddon_InterProcess_GUI_ControlSpin::Control_Spin_SetEnabled(void* window, bool enabled)
{
  g_interProcess.m_Callbacks->GUI.Control.Spin.SetEnabled(g_interProcess.m_Handle->addonData, window, enabled);
}

void CKODIAddon_InterProcess_GUI_ControlSpin::Control_Spin_SetText(void* window, const std::string& label)
{
  g_interProcess.m_Callbacks->GUI.Control.Spin.SetText(g_interProcess.m_Handle->addonData, window, label.c_str());
}

void CKODIAddon_InterProcess_GUI_ControlSpin::Control_Spin_Reset(void* window)
{
  g_interProcess.m_Callbacks->GUI.Control.Spin.Reset(g_interProcess.m_Handle->addonData, window);
}

void CKODIAddon_InterProcess_GUI_ControlSpin::Control_Spin_SetType(void* window, int type)
{
  g_interProcess.m_Callbacks->GUI.Control.Spin.SetType(g_interProcess.m_Handle->addonData, window, (int)type);
}

void CKODIAddon_InterProcess_GUI_ControlSpin::Control_Spin_AddStringLabel(void* window, const std::string& label, const std::string& value)
{
  g_interProcess.m_Callbacks->GUI.Control.Spin.AddStringLabel(g_interProcess.m_Handle->addonData, window, label.c_str(), value.c_str());
}

void CKODIAddon_InterProcess_GUI_ControlSpin::Control_Spin_AddIntLabel(void* window, const std::string& label, int value)
{
  g_interProcess.m_Callbacks->GUI.Control.Spin.AddIntLabel(g_interProcess.m_Handle->addonData, window, label.c_str(), value);
}

void CKODIAddon_InterProcess_GUI_ControlSpin::Control_Spin_SetStringValue(void* window, const std::string& value)
{
  g_interProcess.m_Callbacks->GUI.Control.Spin.SetStringValue(g_interProcess.m_Handle->addonData, window, value.c_str());
}

std::string CKODIAddon_InterProcess_GUI_ControlSpin::Control_Spin_GetStringValue(void* window) const
{
  std::string text;
  text.resize(1024);
  unsigned int size = (unsigned int)text.capacity();
  g_interProcess.m_Callbacks->GUI.Control.Spin.GetStringValue(g_interProcess.m_Handle->addonData, window, text[0], size);
  text.resize(size);
  text.shrink_to_fit();
  return text;
}

void CKODIAddon_InterProcess_GUI_ControlSpin::Control_Spin_SetIntRange(void* window, int start, int end)
{
  g_interProcess.m_Callbacks->GUI.Control.Spin.SetIntRange(g_interProcess.m_Handle->addonData, window, start, end);
}

void CKODIAddon_InterProcess_GUI_ControlSpin::Control_Spin_SetIntValue(void* window, int value)
{
  g_interProcess.m_Callbacks->GUI.Control.Spin.SetIntValue(g_interProcess.m_Handle->addonData, window, value);
}

int CKODIAddon_InterProcess_GUI_ControlSpin::Control_Spin_GetIntValue(void* window) const
{
  return g_interProcess.m_Callbacks->GUI.Control.Spin.GetIntValue(g_interProcess.m_Handle->addonData, window);
}

void CKODIAddon_InterProcess_GUI_ControlSpin::Control_Spin_SetFloatRange(void* window, float start, float end)
{
  g_interProcess.m_Callbacks->GUI.Control.Spin.SetFloatRange(g_interProcess.m_Handle->addonData, window,start ,end);
}

void CKODIAddon_InterProcess_GUI_ControlSpin::Control_Spin_SetFloatValue(void* window, float value)
{
  g_interProcess.m_Callbacks->GUI.Control.Spin.SetFloatValue(g_interProcess.m_Handle->addonData, window, value);
}

float CKODIAddon_InterProcess_GUI_ControlSpin::Control_Spin_GetFloatValue(void* window) const
{
  return g_interProcess.m_Callbacks->GUI.Control.Spin.GetFloatValue(g_interProcess.m_Handle->addonData, window);
}

void CKODIAddon_InterProcess_GUI_ControlSpin::Control_Spin_SetFloatInterval(void* window, float interval)
{
  g_interProcess.m_Callbacks->GUI.Control.Spin.SetFloatInterval(g_interProcess.m_Handle->addonData, window, interval);
}


}; /* extern "C" */
