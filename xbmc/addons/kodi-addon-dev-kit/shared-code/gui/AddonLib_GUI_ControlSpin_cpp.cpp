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
#include "kodi/api2/gui/ControlSpin.hpp"
#include "kodi/api2/gui/Window.hpp"

namespace V2
{
namespace KodiAPI
{

namespace GUI
{

  CControlSpin::CControlSpin(CWindow* window, int controlId)
   : m_Window(window),
     m_ControlId(controlId)
  {
    m_ControlHandle = g_interProcess.Window_GetControl_Spin(m_Window->m_WindowHandle, controlId);
    if (!m_ControlHandle)
      fprintf(stderr, "libKODI_guilib-ERROR: CControlSpin can't create control class from Kodi !!!\n");
  }

  CControlSpin::~CControlSpin()
  {
  }

  void CControlSpin::SetVisible(bool visible)
  {
    g_interProcess.Control_Spin_SetVisible(m_ControlHandle, visible);
  }

  void CControlSpin::SetEnabled(bool enabled)
  {
    g_interProcess.Control_Spin_SetEnabled(m_ControlHandle, enabled);
  }

  void CControlSpin::SetText(const std::string& label)
  {
    g_interProcess.Control_Spin_SetText(m_ControlHandle, label);
  }

  void CControlSpin::Reset()
  {
    g_interProcess.Control_Spin_Reset(m_ControlHandle);
  }

  void CControlSpin::SetType(AddonGUISpinControlType type)
  {
    g_interProcess.Control_Spin_SetType(m_ControlHandle, (int)type);
  }

  void CControlSpin::AddLabel(const std::string& label, const std::string& value)
  {
    g_interProcess.Control_Spin_AddStringLabel(m_ControlHandle, label, value);
  }

  void CControlSpin::AddLabel(const std::string& label, int value)
  {
    g_interProcess.Control_Spin_AddIntLabel(m_ControlHandle, label, value);
  }

  void CControlSpin::SetStringValue(const std::string& value)
  {
    g_interProcess.Control_Spin_SetStringValue(m_ControlHandle, value);
  }

  std::string CControlSpin::GetStringValue() const
  {
    return g_interProcess.Control_Spin_GetStringValue(m_ControlHandle);
  }

  void CControlSpin::SetIntRange(int start, int end)
  {
    g_interProcess.Control_Spin_SetIntRange(m_ControlHandle, start, end);
  }

  void CControlSpin::SetIntValue(int value)
  {
    g_interProcess.Control_Spin_SetIntValue(m_ControlHandle, value);
  }

  int CControlSpin::GetIntValue() const
  {
    return g_interProcess.Control_Spin_GetIntValue(m_ControlHandle);
  }
  
  void CControlSpin::SetFloatRange(float start, float end)
  {
    g_interProcess.Control_Spin_SetFloatRange(m_ControlHandle, start, end);
  }
  
  void CControlSpin::SetFloatValue(float value)
  {
    g_interProcess.Control_Spin_SetFloatValue(m_ControlHandle, value);
  }
  
  float CControlSpin::GetFloatValue() const
  {
    return g_interProcess.Control_Spin_GetFloatValue(m_ControlHandle);
  }
  
  void CControlSpin::SetFloatInterval(float interval)
  {
    g_interProcess.Control_Spin_SetFloatInterval(m_ControlHandle, interval);
  }

}; /* namespace GUI */

}; /* namespace KodiAPI */
}; /* namespace V2 */
