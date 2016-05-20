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
#include KITINCLUDE(ADDON_API_LEVEL, gui/ControlEdit.hpp)
#include KITINCLUDE(ADDON_API_LEVEL, gui/Window.hpp)

API_NAMESPACE

namespace KodiAPI
{
namespace GUI
{

  CControlEdit::CControlEdit(CWindow* window, int controlId)
   : m_Window(window),
     m_ControlId(controlId)
  {
    m_ControlHandle = g_interProcess.m_Callbacks->GUI.Window.GetControl_Edit(g_interProcess.m_Handle, m_Window->m_WindowHandle, controlId);
    if (!m_ControlHandle)
      fprintf(stderr, "libKODI_guilib-ERROR: CControlEdit can't create control class from Kodi !!!\n");
  }

  CControlEdit::~CControlEdit()
  {
  }

  void CControlEdit::SetVisible(bool visible)
  {
    g_interProcess.m_Callbacks->GUI.Control.Edit.SetVisible(g_interProcess.m_Handle, m_ControlHandle, visible);
  }

  void CControlEdit::SetEnabled(bool enabled)
  {
    g_interProcess.m_Callbacks->GUI.Control.Edit.SetEnabled(g_interProcess.m_Handle, m_ControlHandle, enabled);
  }

  void CControlEdit::SetLabel(const std::string& label)
  {
    g_interProcess.m_Callbacks->GUI.Control.Edit.SetLabel(g_interProcess.m_Handle, m_ControlHandle, label.c_str());
  }

  std::string CControlEdit::GetLabel() const
  {
    std::string text;
    text.resize(1024);
    unsigned int size = (unsigned int)text.capacity();
    g_interProcess.m_Callbacks->GUI.Control.Edit.GetLabel(g_interProcess.m_Handle, m_ControlHandle, text[0], size);
    text.resize(size);
    text.shrink_to_fit();
    return text;
  }

  void CControlEdit::SetText(const std::string& text)
  {
    g_interProcess.m_Callbacks->GUI.Control.Edit.SetText(g_interProcess.m_Handle, m_ControlHandle, text.c_str());
  }

  std::string CControlEdit::GetText() const
  {
    std::string text;
    text.resize(1024);
    unsigned int size = (unsigned int)text.capacity();
    g_interProcess.m_Callbacks->GUI.Control.Edit.GetText(g_interProcess.m_Handle, m_ControlHandle, text[0], size);
    text.resize(size);
    text.shrink_to_fit();
    return text;
  }

  void CControlEdit::SetCursorPosition(unsigned int iPosition)
  {
    g_interProcess.m_Callbacks->GUI.Control.Edit.SetCursorPosition(g_interProcess.m_Handle, m_ControlHandle, iPosition);
  }

  unsigned int CControlEdit::GetCursorPosition()
  {
    return g_interProcess.m_Callbacks->GUI.Control.Edit.GetCursorPosition(g_interProcess.m_Handle, m_ControlHandle);
  }

  void CControlEdit::SetInputType(AddonGUIInputType type, const std::string& heading)
  {
    g_interProcess.m_Callbacks->GUI.Control.Edit.SetInputType(g_interProcess.m_Handle, m_ControlHandle, type, heading.c_str());
  }

} /* namespace GUI */
} /* namespace KodiAPI */

END_NAMESPACE()
