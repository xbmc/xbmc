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
#include KITINCLUDE(ADDON_API_LEVEL, gui/ControlButton.hpp)
#include KITINCLUDE(ADDON_API_LEVEL, gui/Window.hpp)

API_NAMESPACE

namespace KodiAPI
{
namespace GUI
{

  CControlButton::CControlButton(CWindow* window, int controlId)
   : m_Window(window),
     m_ControlId(controlId)
  {
    m_ControlHandle = g_interProcess.m_Callbacks->GUI.Window.GetControl_Button(g_interProcess.m_Handle, m_Window->m_WindowHandle, controlId);
    if (!m_ControlHandle)
      fprintf(stderr, "libKODI_guilib-ERROR: CControlButton can't create control class from Kodi !!!\n");
  }

  CControlButton::~CControlButton()
  {
  }

  void CControlButton::SetVisible(bool visible)
  {
    g_interProcess.m_Callbacks->GUI.Control.Button.SetVisible(g_interProcess.m_Handle, m_ControlHandle, visible);
  }

  void CControlButton::SetEnabled(bool enabled)
  {
    g_interProcess.m_Callbacks->GUI.Control.Button.SetEnabled(g_interProcess.m_Handle, m_ControlHandle, enabled);
  }

  void CControlButton::SetLabel(const std::string& label)
  {
    g_interProcess.m_Callbacks->GUI.Control.Button.SetLabel(g_interProcess.m_Handle, m_ControlHandle, label.c_str());
  }

  std::string CControlButton::GetLabel() const
  {
    std::string text;
    text.resize(1024);
    unsigned int size = (unsigned int)text.capacity();
    g_interProcess.m_Callbacks->GUI.Control.Button.GetLabel(g_interProcess.m_Handle, m_ControlHandle, text[0], size);
    text.resize(size);
    text.shrink_to_fit();
    return text;
  }

  void CControlButton::SetLabel2(const std::string& label)
  {
    g_interProcess.m_Callbacks->GUI.Control.Button.SetLabel2(g_interProcess.m_Handle, m_ControlHandle, label.c_str());
  }

  std::string CControlButton::GetLabel2() const
  {
    std::string text;
    text.resize(1024);
    unsigned int size = (unsigned int)text.capacity();
    g_interProcess.m_Callbacks->GUI.Control.Button.GetLabel2(g_interProcess.m_Handle, m_ControlHandle, text[0], size);
    text.resize(size);
    text.shrink_to_fit();
    return text;
  }

} /* namespace GUI */
} /* namespace KodiAPI */

END_NAMESPACE()
