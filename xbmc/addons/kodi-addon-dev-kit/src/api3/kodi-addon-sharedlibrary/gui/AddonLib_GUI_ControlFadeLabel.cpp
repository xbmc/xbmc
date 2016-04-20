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
#include KITINCLUDE(ADDON_API_LEVEL, gui/ControlFadeLabel.hpp)
#include KITINCLUDE(ADDON_API_LEVEL, gui/Window.hpp)

API_NAMESPACE

namespace KodiAPI
{
namespace GUI
{

  CControlFadeLabel::CControlFadeLabel(CWindow* window, int controlId)
   : m_Window(window),
     m_ControlId(controlId)
  {
    m_ControlHandle = g_interProcess.m_Callbacks->GUI.Window.GetControl_FadeLabel(g_interProcess.m_Handle, m_Window->m_WindowHandle, controlId);
    if (!m_ControlHandle)
      fprintf(stderr, "libKODI_guilib-ERROR: CControlFadeLabel can't create control class from Kodi !!!\n");
  }

  CControlFadeLabel::~CControlFadeLabel()
  {
  }

  void CControlFadeLabel::SetVisible(bool visible)
  {
    g_interProcess.m_Callbacks->GUI.Control.FadeLabel.SetVisible(g_interProcess.m_Handle, m_ControlHandle, visible);
  }

  void CControlFadeLabel::AddLabel(const std::string& text)
  {
    g_interProcess.m_Callbacks->GUI.Control.FadeLabel.AddLabel(g_interProcess.m_Handle, m_ControlHandle, text.c_str());
  }

  std::string CControlFadeLabel::GetLabel() const
  {
    std::string text;
    text.resize(1024);
    unsigned int size = (unsigned int)text.capacity();
    g_interProcess.m_Callbacks->GUI.Control.FadeLabel.GetLabel(g_interProcess.m_Handle, m_ControlHandle, text[0], size);
    text.resize(size);
    text.shrink_to_fit();
    return text;
  }

  void CControlFadeLabel::SetScrolling(bool scroll)
  {
    g_interProcess.m_Callbacks->GUI.Control.FadeLabel.SetScrolling(g_interProcess.m_Handle, m_ControlHandle, scroll);
  }

  void CControlFadeLabel::Reset()
  {
    g_interProcess.m_Callbacks->GUI.Control.FadeLabel.Reset(g_interProcess.m_Handle, m_ControlHandle);
  }

} /* namespace GUI */
} /* namespace KodiAPI */

END_NAMESPACE()
