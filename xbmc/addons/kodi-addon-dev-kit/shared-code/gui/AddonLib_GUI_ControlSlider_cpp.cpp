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
#include "kodi/api2/gui/ControlSlider.hpp"
#include "kodi/api2/gui/Window.hpp"

namespace V2
{
namespace KodiAPI
{

namespace GUI
{

  CControlSlider::CControlSlider(CWindow* window, int controlId)
   : m_Window(window),
     m_ControlId(controlId)
  {
    m_ControlHandle = g_interProcess.Window_GetControl_Slider(m_Window->m_WindowHandle, controlId);
    if (!m_ControlHandle)
      fprintf(stderr, "libKODI_guilib-ERROR: CControlSlider can't create control class from Kodi !!!\n");
  }

  CControlSlider::~CControlSlider()
  {
  }

  void CControlSlider::SetVisible(bool yesNo)
  {
    g_interProcess.Control_Slider_SetVisible(m_ControlHandle, yesNo);
  }

  void CControlSlider::SetEnabled(bool yesNo)
  {
    g_interProcess.Control_Slider_SetEnabled(m_ControlHandle, yesNo);
  }

  std::string CControlSlider::GetDescription() const
  {
    return g_interProcess.Control_Slider_GetDescription(m_ControlHandle);
  }

  void CControlSlider::SetIntRange(int start, int end)
  {
    g_interProcess.Control_Slider_SetIntRange(m_ControlHandle, start, end);
  }

  void CControlSlider::SetIntValue(int value)
  {
    g_interProcess.Control_Slider_SetIntValue(m_ControlHandle, value);
  }

  int CControlSlider::GetIntValue() const
  {
    return g_interProcess.Control_Slider_GetIntValue(m_ControlHandle);
  }

  void CControlSlider::SetIntInterval(int interval)
  {
    g_interProcess.Control_Slider_SetIntInterval(m_ControlHandle, interval);
  }

  void CControlSlider::SetPercentage(float percent)
  {
    g_interProcess.Control_Slider_SetPercentage(m_ControlHandle, percent);
  }

  float CControlSlider::GetPercentage() const
  {
    return g_interProcess.Control_Slider_GetPercentage(m_ControlHandle);
  }

  void CControlSlider::SetFloatRange(float start, float end)
  {
    g_interProcess.Control_Slider_SetFloatRange(m_ControlHandle, start, end);
  }

  void CControlSlider::SetFloatValue(float value)
  {
    g_interProcess.Control_Slider_SetFloatValue(m_ControlHandle, value);
  }
  
  float CControlSlider::GetFloatValue() const
  {
    return g_interProcess.Control_Slider_GetFloatValue(m_ControlHandle);
  }
  
  void CControlSlider::SetFloatInterval(float interval)
  {
    g_interProcess.Control_Slider_SetFloatInterval(m_ControlHandle, interval);
  }

}; /* namespace GUI */

}; /* namespace KodiAPI */
}; /* namespace V2 */
