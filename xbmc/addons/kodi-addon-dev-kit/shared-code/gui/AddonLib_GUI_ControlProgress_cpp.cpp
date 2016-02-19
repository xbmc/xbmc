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
#include "kodi/api2/gui/ControlProgress.hpp"
#include "kodi/api2/gui/Window.hpp"

namespace V2
{
namespace KodiAPI
{

namespace GUI
{

  CControlProgress::CControlProgress(CWindow* window, int controlId)
   : m_Window(window),
     m_ControlId(controlId)
  {
    m_ControlHandle = g_interProcess.Window_GetControl_Progress(m_Window->m_WindowHandle, controlId);
    if (!m_ControlHandle)
      fprintf(stderr, "libKODI_guilib-ERROR: CControlProgress can't create control class from Kodi !!!\n");
  }

  CControlProgress::~CControlProgress()
  {
  }

  void CControlProgress::SetVisible(bool visible)
  {
    g_interProcess.Control_Progress_SetVisible(m_ControlHandle, visible);
  }

  void CControlProgress::SetPercentage(float percent)
  {
    g_interProcess.Control_Progress_SetPercentage(m_ControlHandle, percent);
  }

  float CControlProgress::GetPercentage() const
  {
    return g_interProcess.Control_Progress_GetPercentage(m_ControlHandle);
  }

}; /* namespace GUI */

}; /* namespace KodiAPI */
}; /* namespace V2 */
