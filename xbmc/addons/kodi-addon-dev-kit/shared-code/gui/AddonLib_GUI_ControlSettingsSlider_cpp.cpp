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
#include "kodi/api2/gui/ControlSettingsSlider.hpp"
#include "kodi/api2/gui/Window.hpp"

namespace V2
{
namespace KodiAPI
{

namespace GUI
{

  CControlSettingsSlider::CControlSettingsSlider(CWindow* window, int controlId)
   : m_Window(window),
     m_ControlId(controlId)
  {
    m_ControlHandle = g_interProcess.Window_GetControl_SettingsSlider(m_Window->m_WindowHandle, controlId);
    if (!m_ControlHandle)
      fprintf(stderr, "libKODI_guilib-ERROR: CControlSettingsSlider can't create control class from Kodi !!!\n");
  }

  CControlSettingsSlider::~CControlSettingsSlider()
  {
  }

  void CControlSettingsSlider::SetVisible(bool visible)
  {
    g_interProcess.Control_SettingsSlider_SetVisible(m_ControlHandle, visible);
  }

  void CControlSettingsSlider::SetEnabled(bool enabled)
  {
    g_interProcess.Control_SettingsSlider_SetEnabled(m_ControlHandle, enabled);
  }

  void CControlSettingsSlider::SetText(const std::string& label)
  {
    g_interProcess.Control_SettingsSlider_SetText(m_ControlHandle, label);
  }

  void CControlSettingsSlider::Reset()
  {
    g_interProcess.Control_SettingsSlider_Reset(m_ControlHandle);
  }

  void CControlSettingsSlider::SetIntRange(int start, int end)
  {
    g_interProcess.Control_SettingsSlider_SetIntRange(m_ControlHandle, start, end);
  }

  void CControlSettingsSlider::SetIntValue(int value)
  {
    g_interProcess.Control_SettingsSlider_SetIntValue(m_ControlHandle, value);
  }

  int CControlSettingsSlider::GetIntValue() const
  {
    return g_interProcess.Control_SettingsSlider_GetIntValue(m_ControlHandle);
  }

  void CControlSettingsSlider::SetIntInterval(int interval)
  {
    g_interProcess.Control_SettingsSlider_SetIntInterval(m_ControlHandle, interval);
  }

  void CControlSettingsSlider::SetPercentage(float percent)
  {
    g_interProcess.Control_SettingsSlider_SetPercentage(m_ControlHandle, percent);
  }

  float CControlSettingsSlider::GetPercentage() const
  {
    return g_interProcess.Control_SettingsSlider_GetPercentage(m_ControlHandle);
  }

  void CControlSettingsSlider::SetFloatRange(float start, float end)
  {
    g_interProcess.Control_SettingsSlider_SetFloatRange(m_ControlHandle, start, end);
  }

  void CControlSettingsSlider::SetFloatValue(float value)
  {
    g_interProcess.Control_SettingsSlider_SetFloatValue(m_ControlHandle, value);
  }

  float CControlSettingsSlider::GetFloatValue() const
  {
    return g_interProcess.Control_SettingsSlider_GetFloatValue(m_ControlHandle);
  }

  void CControlSettingsSlider::SetFloatInterval(float interval)
  {
    g_interProcess.Control_SettingsSlider_SetFloatInterval(m_ControlHandle, interval);
  }

}; /* namespace GUI */

}; /* namespace KodiAPI */
}; /* namespace V2 */
