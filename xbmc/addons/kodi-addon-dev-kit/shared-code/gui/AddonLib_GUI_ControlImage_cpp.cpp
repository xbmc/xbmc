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
#include "kodi/api2/gui/ControlImage.hpp"
#include "kodi/api2/gui/Window.hpp"

namespace V2
{
namespace KodiAPI
{

namespace GUI
{

  CControlImage::CControlImage(CWindow* window, int controlId)
   : m_Window(window),
     m_ControlId(controlId)
  {
    m_ControlHandle = g_interProcess.Window_GetControl_Image(m_Window->m_WindowHandle, controlId);
    if (!m_ControlHandle)
      fprintf(stderr, "libKODI_guilib-ERROR: CControlImage can't create control class from Kodi !!!\n");
  }

  CControlImage::~CControlImage()
  {
  }

  void CControlImage::SetVisible(bool visible)
  {
    g_interProcess.Control_Image_SetVisible(m_ControlHandle, visible);
  }

  void CControlImage::SetFileName(const std::string& strFileName, const bool useCache)
  {
    g_interProcess.Control_Image_SetFileName(m_ControlHandle, strFileName, useCache);
  }

  void CControlImage::SetColorDiffuse(uint32_t colorDiffuse)
  {
    g_interProcess.Control_Image_SetColorDiffuse(m_ControlHandle, colorDiffuse);
  }

}; /* namespace GUI */

}; /* namespace KodiAPI */
}; /* namespace V2 */
