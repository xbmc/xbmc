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

#include "InterProcess_GUI_ControlSettingsSlider.h"
#include "InterProcess.h"

extern "C"
{

void CKODIAddon_InterProcess_GUI_ControlSettingsSlider::Control_SettingsSlider_SetVisible(void* window, bool visible)
{
  g_interProcess.m_Callbacks->GUI.Control.SettingsSlider.SetVisible(g_interProcess.m_Handle->addonData, window, visible);
}

void CKODIAddon_InterProcess_GUI_ControlSettingsSlider::Control_SettingsSlider_SetEnabled(void* window, bool enabled)
{
  g_interProcess.m_Callbacks->GUI.Control.SettingsSlider.SetEnabled(g_interProcess.m_Handle->addonData, window, enabled);
}

void CKODIAddon_InterProcess_GUI_ControlSettingsSlider::Control_SettingsSlider_SetText(void* window, const std::string& label)
{
  g_interProcess.m_Callbacks->GUI.Control.SettingsSlider.SetText(g_interProcess.m_Handle->addonData, window, label.c_str());
}

void CKODIAddon_InterProcess_GUI_ControlSettingsSlider::Control_SettingsSlider_Reset(void* window)
{
  g_interProcess.m_Callbacks->GUI.Control.SettingsSlider.Reset(g_interProcess.m_Handle->addonData, window);
}

void CKODIAddon_InterProcess_GUI_ControlSettingsSlider::Control_SettingsSlider_SetIntRange(void* window, int start, int end)
{
  g_interProcess.m_Callbacks->GUI.Control.SettingsSlider.SetIntRange(g_interProcess.m_Handle->addonData, window, start, end);
}

void CKODIAddon_InterProcess_GUI_ControlSettingsSlider::Control_SettingsSlider_SetIntValue(void* window, int value)
{
  g_interProcess.m_Callbacks->GUI.Control.SettingsSlider.SetIntValue(g_interProcess.m_Handle->addonData, window, value);
}

int CKODIAddon_InterProcess_GUI_ControlSettingsSlider::Control_SettingsSlider_GetIntValue(void* window) const
{
  return g_interProcess.m_Callbacks->GUI.Control.SettingsSlider.GetIntValue(g_interProcess.m_Handle->addonData, window);
}

void CKODIAddon_InterProcess_GUI_ControlSettingsSlider::Control_SettingsSlider_SetIntInterval(void* window, int interval)
{
  g_interProcess.m_Callbacks->GUI.Control.SettingsSlider.SetIntInterval(g_interProcess.m_Handle->addonData, window, interval);
}

void CKODIAddon_InterProcess_GUI_ControlSettingsSlider::Control_SettingsSlider_SetPercentage(void* window, float percent)
{
  g_interProcess.m_Callbacks->GUI.Control.SettingsSlider.SetPercentage(g_interProcess.m_Handle->addonData, window, percent);
}

float CKODIAddon_InterProcess_GUI_ControlSettingsSlider::Control_SettingsSlider_GetPercentage(void* window) const
{
  return g_interProcess.m_Callbacks->GUI.Control.SettingsSlider.GetPercentage(g_interProcess.m_Handle->addonData, window);
}

void CKODIAddon_InterProcess_GUI_ControlSettingsSlider::Control_SettingsSlider_SetFloatRange(void* window, float start, float end)
{
  g_interProcess.m_Callbacks->GUI.Control.SettingsSlider.SetFloatRange(g_interProcess.m_Handle->addonData, window, start, end);
}

void CKODIAddon_InterProcess_GUI_ControlSettingsSlider::Control_SettingsSlider_SetFloatValue(void* window, float value)
{
  g_interProcess.m_Callbacks->GUI.Control.SettingsSlider.SetFloatValue(g_interProcess.m_Handle->addonData, window, value);
}

float CKODIAddon_InterProcess_GUI_ControlSettingsSlider::Control_SettingsSlider_GetFloatValue(void* window) const
{
  return g_interProcess.m_Callbacks->GUI.Control.SettingsSlider.GetFloatValue(g_interProcess.m_Handle->addonData, window);
}

void CKODIAddon_InterProcess_GUI_ControlSettingsSlider::Control_SettingsSlider_SetFloatInterval(void* window, float interval)
{
  g_interProcess.m_Callbacks->GUI.Control.SettingsSlider.SetFloatInterval(g_interProcess.m_Handle->addonData, window, interval);
}

}; /* extern "C" */
