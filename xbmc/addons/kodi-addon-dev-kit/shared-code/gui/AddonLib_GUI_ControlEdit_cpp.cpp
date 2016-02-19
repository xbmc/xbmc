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
#include "kodi/api2/gui/ControlEdit.hpp"
#include "kodi/api2/gui/Window.hpp"

namespace V2
{
namespace KodiAPI
{

namespace GUI
{

  CControlEdit::CControlEdit(CWindow* window, int controlId)
   : m_Window(window),
     m_ControlId(controlId)
  {
    m_ControlHandle = g_interProcess.Window_GetControl_Edit(m_Window->m_WindowHandle, controlId);
    if (!m_ControlHandle)
      fprintf(stderr, "libKODI_guilib-ERROR: CControlEdit can't create control class from Kodi !!!\n");
  }

  CControlEdit::~CControlEdit()
  {
  }

  void CControlEdit::SetVisible(bool visible)
  {
    g_interProcess.Control_Edit_SetVisible(m_ControlHandle, visible);
  }

  void CControlEdit::SetEnabled(bool enabled)
  {
    g_interProcess.Control_Edit_SetEnabled(m_ControlHandle, enabled);
  }

  void CControlEdit::SetLabel(const std::string& label)
  {
    g_interProcess.Control_Edit_SetLabel(m_ControlHandle, label);
  }

  std::string CControlEdit::GetLabel() const
  {
    return g_interProcess.Control_Edit_GetLabel(m_ControlHandle);
  }

  void CControlEdit::SetText(const std::string& text)
  {
    g_interProcess.Control_Edit_SetText(m_ControlHandle, text);
  }

  std::string CControlEdit::GetText() const
  {
    return g_interProcess.Control_Edit_GetText(m_ControlHandle);
  }

  void CControlEdit::SetCursorPosition(unsigned int iPosition)
  {
    g_interProcess.Control_Edit_SetCursorPosition(m_ControlHandle, iPosition);
  }

  unsigned int CControlEdit::GetCursorPosition()
  {
    return g_interProcess.Control_Edit_GetCursorPosition(m_ControlHandle);
  }

  void CControlEdit::SetInputType(AddonGUIInputType type, const std::string& heading)
  {
    g_interProcess.Control_Edit_SetInputType(m_ControlHandle, type, heading);
  }

}; /* namespace GUI */

}; /* namespace KodiAPI */
}; /* namespace V2 */
