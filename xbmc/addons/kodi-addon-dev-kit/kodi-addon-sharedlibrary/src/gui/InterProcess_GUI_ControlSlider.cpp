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

#include "InterProcess_GUI_ControlSlider.h"
#include "InterProcess.h"

extern "C"
{

void CKODIAddon_InterProcess_GUI_ControlSlider::Control_Slider_SetVisible(void* window, bool visible)
{
  g_interProcess.m_Callbacks->GUI.Control.Slider.SetVisible(g_interProcess.m_Handle->addonData, window, visible);
}

void CKODIAddon_InterProcess_GUI_ControlSlider::Control_Slider_SetEnabled(void* window, bool enabled)
{
  g_interProcess.m_Callbacks->GUI.Control.Slider.SetEnabled(g_interProcess.m_Handle->addonData, window, enabled);
}

std::string CKODIAddon_InterProcess_GUI_ControlSlider::Control_Slider_GetDescription(void* window) const
{
  std::string text;
  text.resize(1024);
  unsigned int size = (unsigned int)text.capacity();
  g_interProcess.m_Callbacks->GUI.Control.Slider.GetDescription(g_interProcess.m_Handle->addonData, window, text[0], size);
  text.resize(size);
  text.shrink_to_fit();
  return text;
}

void CKODIAddon_InterProcess_GUI_ControlSlider::Control_Slider_SetIntRange(void* window, int start, int end)
{
  g_interProcess.m_Callbacks->GUI.Control.Slider.SetIntRange(g_interProcess.m_Handle->addonData, window, start, end);
}

void CKODIAddon_InterProcess_GUI_ControlSlider::Control_Slider_SetIntValue(void* window, int value)
{
  g_interProcess.m_Callbacks->GUI.Control.Slider.SetIntValue(g_interProcess.m_Handle->addonData, window, value);
}

int CKODIAddon_InterProcess_GUI_ControlSlider::Control_Slider_GetIntValue(void* window) const
{
  return g_interProcess.m_Callbacks->GUI.Control.Slider.GetIntValue(g_interProcess.m_Handle->addonData, window);
}

void CKODIAddon_InterProcess_GUI_ControlSlider::Control_Slider_SetIntInterval(void* window, int interval)
{
  g_interProcess.m_Callbacks->GUI.Control.Slider.SetIntInterval(g_interProcess.m_Handle->addonData, window, interval);
}

void CKODIAddon_InterProcess_GUI_ControlSlider::Control_Slider_SetPercentage(void* window, float percent)
{
  g_interProcess.m_Callbacks->GUI.Control.Slider.SetPercentage(g_interProcess.m_Handle->addonData, window, percent);
}

float CKODIAddon_InterProcess_GUI_ControlSlider::Control_Slider_GetPercentage(void* window) const
{
  return g_interProcess.m_Callbacks->GUI.Control.Slider.GetPercentage(g_interProcess.m_Handle->addonData, window);
}

void CKODIAddon_InterProcess_GUI_ControlSlider::Control_Slider_SetFloatRange(void* window, float start, float end)
{
  g_interProcess.m_Callbacks->GUI.Control.Slider.SetFloatRange(g_interProcess.m_Handle->addonData, window, start, end);
}

void CKODIAddon_InterProcess_GUI_ControlSlider::Control_Slider_SetFloatValue(void* window, float value)
{
  g_interProcess.m_Callbacks->GUI.Control.Slider.SetFloatValue(g_interProcess.m_Handle->addonData, window, value);
}

float CKODIAddon_InterProcess_GUI_ControlSlider::Control_Slider_GetFloatValue(void* window) const
{
  return g_interProcess.m_Callbacks->GUI.Control.Slider.GetFloatValue(g_interProcess.m_Handle->addonData, window);
}

void CKODIAddon_InterProcess_GUI_ControlSlider::Control_Slider_SetFloatInterval(void* window, float interval)
{
  g_interProcess.m_Callbacks->GUI.Control.Slider.SetFloatInterval(g_interProcess.m_Handle->addonData, window, interval);
}

}; /* extern "C" */
