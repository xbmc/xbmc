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
#include KITINCLUDE(ADDON_API_LEVEL, gui/ControlImage.hpp)
#include KITINCLUDE(ADDON_API_LEVEL, gui/Window.hpp)

API_NAMESPACE

namespace KodiAPI
{
namespace GUI
{

  CControlImage::CControlImage(CWindow* window, int controlId)
   : m_Window(window),
     m_ControlId(controlId)
  {
    m_ControlHandle = g_interProcess.m_Callbacks->GUI.Window.GetControl_Image(g_interProcess.m_Handle, m_Window->m_WindowHandle, controlId);
    if (!m_ControlHandle)
      fprintf(stderr, "libKODI_guilib-ERROR: CControlImage can't create control class from Kodi !!!\n");
  }

  CControlImage::~CControlImage()
  {
  }

  void CControlImage::SetVisible(bool visible)
  {
    g_interProcess.m_Callbacks->GUI.Control.Image.SetVisible(g_interProcess.m_Handle, m_ControlHandle, visible);
  }

  void CControlImage::SetFileName(const std::string& strFileName, const bool useCache)
  {
    g_interProcess.m_Callbacks->GUI.Control.Image.SetFileName(g_interProcess.m_Handle, m_ControlHandle, strFileName.c_str(), useCache);
  }

  void CControlImage::SetColorDiffuse(uint32_t colorDiffuse)
  {
    g_interProcess.m_Callbacks->GUI.Control.Image.SetColorDiffuse(g_interProcess.m_Handle, m_ControlHandle, colorDiffuse);
  }

} /* namespace GUI */
} /* namespace KodiAPI */

END_NAMESPACE()
