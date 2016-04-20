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
#include KITINCLUDE(ADDON_API_LEVEL, gui/ControlSlider.hpp)
#include KITINCLUDE(ADDON_API_LEVEL, gui/Window.hpp)

API_NAMESPACE

namespace KodiAPI
{
namespace GUI
{

  CControlSlider::CControlSlider(CWindow* window, int controlId)
   : m_Window(window),
     m_ControlId(controlId)
  {
    m_ControlHandle = g_interProcess.m_Callbacks->GUI.Window.GetControl_Slider(g_interProcess.m_Handle, m_Window->m_WindowHandle, controlId);
    if (!m_ControlHandle)
      fprintf(stderr, "libKODI_guilib-ERROR: CControlSlider can't create control class from Kodi !!!\n");
  }

  CControlSlider::~CControlSlider()
  {
  }

  void CControlSlider::SetVisible(bool yesNo)
  {
    g_interProcess.m_Callbacks->GUI.Control.Slider.SetVisible(g_interProcess.m_Handle, m_ControlHandle, yesNo);
  }

  void CControlSlider::SetEnabled(bool yesNo)
  {
    g_interProcess.m_Callbacks->GUI.Control.Slider.SetEnabled(g_interProcess.m_Handle, m_ControlHandle, yesNo);
  }

  std::string CControlSlider::GetDescription() const
  {
    std::string text;
    text.resize(1024);
    unsigned int size = (unsigned int)text.capacity();
    g_interProcess.m_Callbacks->GUI.Control.Slider.GetDescription(g_interProcess.m_Handle, m_ControlHandle, text[0], size);
    text.resize(size);
    text.shrink_to_fit();
    return text;
  }

  void CControlSlider::SetIntRange(int start, int end)
  {
    g_interProcess.m_Callbacks->GUI.Control.Slider.SetIntRange(g_interProcess.m_Handle, m_ControlHandle, start, end);
  }

  void CControlSlider::SetIntValue(int value)
  {
    g_interProcess.m_Callbacks->GUI.Control.Slider.SetIntValue(g_interProcess.m_Handle, m_ControlHandle, value);
  }

  int CControlSlider::GetIntValue() const
  {
    return g_interProcess.m_Callbacks->GUI.Control.Slider.GetIntValue(g_interProcess.m_Handle, m_ControlHandle);
  }

  void CControlSlider::SetIntInterval(int interval)
  {
    g_interProcess.m_Callbacks->GUI.Control.Slider.SetIntInterval(g_interProcess.m_Handle, m_ControlHandle, interval);
  }

  void CControlSlider::SetPercentage(float percent)
  {
    g_interProcess.m_Callbacks->GUI.Control.Slider.SetPercentage(g_interProcess.m_Handle, m_ControlHandle, percent);
  }

  float CControlSlider::GetPercentage() const
  {
    return g_interProcess.m_Callbacks->GUI.Control.Slider.GetPercentage(g_interProcess.m_Handle, m_ControlHandle);
  }

  void CControlSlider::SetFloatRange(float start, float end)
  {
    g_interProcess.m_Callbacks->GUI.Control.Slider.SetFloatRange(g_interProcess.m_Handle, m_ControlHandle, start, end);
  }

  void CControlSlider::SetFloatValue(float value)
  {
    g_interProcess.m_Callbacks->GUI.Control.Slider.SetFloatValue(g_interProcess.m_Handle, m_ControlHandle, value);
  }
  
  float CControlSlider::GetFloatValue() const
  {
    return g_interProcess.m_Callbacks->GUI.Control.Slider.GetFloatValue(g_interProcess.m_Handle, m_ControlHandle);
  }
  
  void CControlSlider::SetFloatInterval(float interval)
  {
    g_interProcess.m_Callbacks->GUI.Control.Slider.SetFloatInterval(g_interProcess.m_Handle, m_ControlHandle, interval);
  }

} /* namespace GUI */
} /* namespace KodiAPI */

END_NAMESPACE()
