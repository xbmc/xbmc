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
#include KITINCLUDE(ADDON_API_LEVEL, gui/ControlTextBox.hpp)
#include KITINCLUDE(ADDON_API_LEVEL, gui/Window.hpp)

API_NAMESPACE

namespace KodiAPI
{
namespace GUI
{

  CControlTextBox::CControlTextBox(CWindow* window, int controlId)
   : m_Window(window),
     m_ControlId(controlId)
  {
    m_ControlHandle = g_interProcess.m_Callbacks->GUI.Window.GetControl_TextBox(g_interProcess.m_Handle, m_Window->m_WindowHandle, controlId);
    if (!m_ControlHandle)
      fprintf(stderr, "libKODI_guilib-ERROR: CControlTextBox can't create control class from Kodi !!!\n");
  }

  CControlTextBox::~CControlTextBox()
  {
  }

  void CControlTextBox::SetVisible(bool visible)
  {
    g_interProcess.m_Callbacks->GUI.Control.TextBox.SetVisible(g_interProcess.m_Handle, m_ControlHandle,  visible);
  }

  void CControlTextBox::Reset()
  {
    g_interProcess.m_Callbacks->GUI.Control.TextBox.Reset(m_ControlHandle, m_ControlHandle);
  }

  void CControlTextBox::SetText(const std::string& text)
  {
    g_interProcess.m_Callbacks->GUI.Control.TextBox.SetText(g_interProcess.m_Handle, m_ControlHandle,  text.c_str());
  }

  std::string CControlTextBox::GetText() const
  {
    std::string text;
    text.resize(16384);
    unsigned int size = (unsigned int)text.capacity();
    g_interProcess.m_Callbacks->GUI.Control.TextBox.GetText(g_interProcess.m_Handle, m_ControlHandle, text[0], size);
    text.resize(size);
    text.shrink_to_fit();
    return text;
  }

  void CControlTextBox::Scroll(unsigned int position)
  {
    g_interProcess.m_Callbacks->GUI.Control.TextBox.Scroll(g_interProcess.m_Handle, m_ControlHandle,  position);
  }

  void CControlTextBox::SetAutoScrolling(int delay, int time, int repeat)
  {
    g_interProcess.m_Callbacks->GUI.Control.TextBox.SetAutoScrolling(g_interProcess.m_Handle, m_ControlHandle,  delay, time, repeat);
  }

} /* namespace GUI */
} /* namespace KodiAPI */

END_NAMESPACE()
