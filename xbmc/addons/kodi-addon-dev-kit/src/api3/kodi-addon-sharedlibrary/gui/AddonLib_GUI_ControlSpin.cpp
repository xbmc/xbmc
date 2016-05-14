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
#include KITINCLUDE(ADDON_API_LEVEL, gui/ControlSpin.hpp)
#include KITINCLUDE(ADDON_API_LEVEL, gui/Window.hpp)

API_NAMESPACE

namespace KodiAPI
{
namespace GUI
{

  CControlSpin::CControlSpin(CWindow* window, int controlId)
   : m_Window(window),
     m_ControlId(controlId)
  {
    m_ControlHandle = g_interProcess.m_Callbacks->GUI.Window.GetControl_Spin(g_interProcess.m_Handle, m_Window->m_WindowHandle, controlId);
    if (!m_ControlHandle)
      fprintf(stderr, "libKODI_guilib-ERROR: CControlSpin can't create control class from Kodi !!!\n");
  }

  CControlSpin::~CControlSpin()
  {
  }

  void CControlSpin::SetVisible(bool visible)
  {
    g_interProcess.m_Callbacks->GUI.Control.Spin.SetVisible(g_interProcess.m_Handle, m_ControlHandle, visible);
  }

  void CControlSpin::SetEnabled(bool enabled)
  {
    g_interProcess.m_Callbacks->GUI.Control.Spin.SetEnabled(g_interProcess.m_Handle, m_ControlHandle, enabled);
  }

  void CControlSpin::SetText(const std::string& label)
  {
    g_interProcess.m_Callbacks->GUI.Control.Spin.SetText(g_interProcess.m_Handle, m_ControlHandle, label.c_str());
  }

  void CControlSpin::Reset()
  {
    g_interProcess.m_Callbacks->GUI.Control.Spin.Reset(g_interProcess.m_Handle, m_ControlHandle);
  }

  void CControlSpin::SetType(AddonGUISpinControlType type)
  {
    g_interProcess.m_Callbacks->GUI.Control.Spin.SetType(g_interProcess.m_Handle, m_ControlHandle, (int)type);
  }

  void CControlSpin::AddLabel(const std::string& label, const std::string& value)
  {
    g_interProcess.m_Callbacks->GUI.Control.Spin.AddStringLabel(g_interProcess.m_Handle, m_ControlHandle, label.c_str(), value.c_str());
  }

  void CControlSpin::AddLabel(const std::string& label, int value)
  {
    g_interProcess.m_Callbacks->GUI.Control.Spin.AddIntLabel(g_interProcess.m_Handle, m_ControlHandle, label.c_str(), value);
  }

  void CControlSpin::SetStringValue(const std::string& value)
  {
    g_interProcess.m_Callbacks->GUI.Control.Spin.SetStringValue(g_interProcess.m_Handle, m_ControlHandle, value.c_str());
  }

  std::string CControlSpin::GetStringValue() const
  {
    std::string text;
    text.resize(1024);
    unsigned int size = (unsigned int)text.capacity();
    g_interProcess.m_Callbacks->GUI.Control.Spin.GetStringValue(g_interProcess.m_Handle, m_ControlHandle, text[0], size);
    text.resize(size);
    text.shrink_to_fit();
    return text;
  }

  void CControlSpin::SetIntRange(int start, int end)
  {
    g_interProcess.m_Callbacks->GUI.Control.Spin.SetIntRange(g_interProcess.m_Handle, m_ControlHandle, start, end);
  }

  void CControlSpin::SetIntValue(int value)
  {
    g_interProcess.m_Callbacks->GUI.Control.Spin.SetIntValue(g_interProcess.m_Handle, m_ControlHandle, value);
  }

  int CControlSpin::GetIntValue() const
  {
    return g_interProcess.m_Callbacks->GUI.Control.Spin.GetIntValue(g_interProcess.m_Handle, m_ControlHandle);
  }
  
  void CControlSpin::SetFloatRange(float start, float end)
  {
    g_interProcess.m_Callbacks->GUI.Control.Spin.SetFloatRange(g_interProcess.m_Handle, m_ControlHandle, start, end);
  }
  
  void CControlSpin::SetFloatValue(float value)
  {
    g_interProcess.m_Callbacks->GUI.Control.Spin.SetFloatValue(g_interProcess.m_Handle, m_ControlHandle, value);
  }
  
  float CControlSpin::GetFloatValue() const
  {
    return g_interProcess.m_Callbacks->GUI.Control.Spin.GetFloatValue(g_interProcess.m_Handle, m_ControlHandle);
  }
  
  void CControlSpin::SetFloatInterval(float interval)
  {
    g_interProcess.m_Callbacks->GUI.Control.Spin.SetFloatInterval(g_interProcess.m_Handle, m_ControlHandle, interval);
  }

} /* namespace GUI */
} /* namespace KodiAPI */

END_NAMESPACE()
