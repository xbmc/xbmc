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

#include "InterProcess.h"
#include KITINCLUDE(ADDON_API_LEVEL, gui/ControlSettingsSlider.hpp)
#include KITINCLUDE(ADDON_API_LEVEL, gui/Window.hpp)

API_NAMESPACE

namespace KodiAPI
{
namespace GUI
{

  CControlSettingsSlider::CControlSettingsSlider(CWindow* window, int controlId)
   : m_Window(window),
     m_ControlId(controlId)
  {
    m_ControlHandle = g_interProcess.m_Callbacks->GUI.Window.GetControl_SettingsSlider(g_interProcess.m_Handle, m_Window->m_WindowHandle, controlId);
    if (!m_ControlHandle)
      fprintf(stderr, "libKODI_guilib-ERROR: CControlSettingsSlider can't create control class from Kodi !!!\n");
  }

  CControlSettingsSlider::~CControlSettingsSlider()
  {
  }

  void CControlSettingsSlider::SetVisible(bool visible)
  {
    g_interProcess.m_Callbacks->GUI.Control.SettingsSlider.SetVisible(g_interProcess.m_Handle, m_ControlHandle, visible);
  }

  void CControlSettingsSlider::SetEnabled(bool enabled)
  {
    g_interProcess.m_Callbacks->GUI.Control.SettingsSlider.SetEnabled(g_interProcess.m_Handle, m_ControlHandle, enabled);
  }

  void CControlSettingsSlider::SetText(const std::string& label)
  {
    g_interProcess.m_Callbacks->GUI.Control.SettingsSlider.SetText(g_interProcess.m_Handle, m_ControlHandle, label.c_str());
  }

  void CControlSettingsSlider::Reset()
  {
    g_interProcess.m_Callbacks->GUI.Control.SettingsSlider.Reset(g_interProcess.m_Handle, m_ControlHandle);
  }

  void CControlSettingsSlider::SetIntRange(int start, int end)
  {
    g_interProcess.m_Callbacks->GUI.Control.SettingsSlider.SetIntRange(g_interProcess.m_Handle, m_ControlHandle, start, end);
  }

  void CControlSettingsSlider::SetIntValue(int value)
  {
    g_interProcess.m_Callbacks->GUI.Control.SettingsSlider.SetIntValue(g_interProcess.m_Handle, m_ControlHandle, value);
  }

  int CControlSettingsSlider::GetIntValue() const
  {
    return g_interProcess.m_Callbacks->GUI.Control.SettingsSlider.GetIntValue(g_interProcess.m_Handle, m_ControlHandle);
  }

  void CControlSettingsSlider::SetIntInterval(int interval)
  {
    g_interProcess.m_Callbacks->GUI.Control.SettingsSlider.SetIntInterval(g_interProcess.m_Handle, m_ControlHandle, interval);
  }

  void CControlSettingsSlider::SetPercentage(float percent)
  {
    g_interProcess.m_Callbacks->GUI.Control.SettingsSlider.SetPercentage(g_interProcess.m_Handle, m_ControlHandle, percent);
  }

  float CControlSettingsSlider::GetPercentage() const
  {
    return g_interProcess.m_Callbacks->GUI.Control.SettingsSlider.GetPercentage(g_interProcess.m_Handle, m_ControlHandle);
  }

  void CControlSettingsSlider::SetFloatRange(float start, float end)
  {
    g_interProcess.m_Callbacks->GUI.Control.SettingsSlider.SetFloatRange(g_interProcess.m_Handle, m_ControlHandle, start, end);
  }

  void CControlSettingsSlider::SetFloatValue(float value)
  {
    g_interProcess.m_Callbacks->GUI.Control.SettingsSlider.SetFloatValue(g_interProcess.m_Handle, m_ControlHandle, value);
  }

  float CControlSettingsSlider::GetFloatValue() const
  {
    return g_interProcess.m_Callbacks->GUI.Control.SettingsSlider.GetFloatValue(g_interProcess.m_Handle, m_ControlHandle);
  }

  void CControlSettingsSlider::SetFloatInterval(float interval)
  {
    g_interProcess.m_Callbacks->GUI.Control.SettingsSlider.SetFloatInterval(g_interProcess.m_Handle, m_ControlHandle, interval);
  }

} /* namespace GUI */
} /* namespace KodiAPI */

END_NAMESPACE()
