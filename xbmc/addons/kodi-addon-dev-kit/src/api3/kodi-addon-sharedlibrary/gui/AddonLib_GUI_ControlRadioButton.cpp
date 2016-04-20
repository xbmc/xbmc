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
#include KITINCLUDE(ADDON_API_LEVEL, gui/ControlRadioButton.hpp)
#include KITINCLUDE(ADDON_API_LEVEL, gui/Window.hpp)

API_NAMESPACE

namespace KodiAPI
{
namespace GUI
{

  CControlRadioButton::CControlRadioButton(CWindow* window, int controlId)
   : m_Window(window),
     m_ControlId(controlId)
  {
    m_ControlHandle = g_interProcess.m_Callbacks->GUI.Window.GetControl_RadioButton(g_interProcess.m_Handle, m_Window->m_WindowHandle, controlId);
    if (!m_ControlHandle)
      fprintf(stderr, "libKODI_guilib-ERROR: CControlRadioButton can't create control class from Kodi !!!\n");
  }

  CControlRadioButton::~CControlRadioButton()
  {
  }

  void CControlRadioButton::SetVisible(bool visible)
  {
    g_interProcess.m_Callbacks->GUI.Control.RadioButton.SetVisible(g_interProcess.m_Handle, m_ControlHandle, visible);
  }

  void CControlRadioButton::SetEnabled(bool enabled)
  {
    g_interProcess.m_Callbacks->GUI.Control.RadioButton.SetEnabled(g_interProcess.m_Handle, m_ControlHandle, enabled);
  }

  void CControlRadioButton::SetLabel(const std::string& label)
  {
    g_interProcess.m_Callbacks->GUI.Control.RadioButton.SetLabel(g_interProcess.m_Handle, m_ControlHandle, label.c_str());
  }

  std::string CControlRadioButton::GetLabel() const
  {
    std::string text;
    text.resize(1024);
    unsigned int size = (unsigned int)text.capacity();
    g_interProcess.m_Callbacks->GUI.Control.RadioButton.GetLabel(g_interProcess.m_Handle, m_ControlHandle, text[0], size);
    text.resize(size);
    text.shrink_to_fit();
    return text;
  }

  void CControlRadioButton::SetSelected(bool yesNo)
  {
    g_interProcess.m_Callbacks->GUI.Control.RadioButton.SetSelected(g_interProcess.m_Handle, m_ControlHandle, yesNo);
  }

  bool CControlRadioButton::IsSelected() const
  {
    return g_interProcess.m_Callbacks->GUI.Control.RadioButton.IsSelected(g_interProcess.m_Handle, m_ControlHandle);
  }

} /* namespace GUI */
} /* namespace KodiAPI */

END_NAMESPACE()
