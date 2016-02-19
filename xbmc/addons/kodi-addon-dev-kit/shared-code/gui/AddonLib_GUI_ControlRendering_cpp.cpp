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
#include "kodi/api2/gui/ControlRendering.hpp"
#include "kodi/api2/gui/Window.hpp"

namespace V2
{
namespace KodiAPI
{

namespace GUI
{

  CControlRendering::CControlRendering(
        CWindow*      window,
        int           controlId)
   : m_Window(window),
     m_ControlId(controlId)
  {
    m_ControlHandle = g_interProcess.Window_GetControl_Rendering(m_Window->m_WindowHandle, controlId);
    if (m_ControlHandle)
      g_interProcess.Control_Rendering_SetCallbacks(m_ControlHandle, this,
                                             OnCreateCB, OnRenderCB, OnStopCB, OnDirtyCB);
    else
      fprintf(stderr, "ERROR: CControlRendering can't create control class from Kodi !!!\n");
  }

  CControlRendering::~CControlRendering()
  {
    g_interProcess.Control_Rendering_Delete(m_ControlHandle);
  }

  /*!
   * Kodi to add-on override defination function to use if class becomes used
   * independet.
   */

  void CControlRendering::SetIndependentCallbacks(
        GUIHANDLE             cbhdl,
        bool      (*CBCreate)(GUIHANDLE cbhdl,
                              int       x,
                              int       y,
                              int       w,
                              int       h,
                              void*     device),
        void      (*CBRender)(GUIHANDLE cbhdl),
        void      (*CBStop)  (GUIHANDLE cbhdl),
        bool      (*CBDirty) (GUIHANDLE cbhdl))
  {
    if (!cbhdl ||
        !CBCreate || !CBRender || !CBStop || !CBDirty)
    {
      fprintf(stderr, "ERROR: CControlRendering - SetIndependentCallbacks called with nullptr !!!\n");
      return;
    }

    g_interProcess.Control_Rendering_SetCallbacks(m_ControlHandle, cbhdl,
                                         CBCreate, CBRender, CBStop, CBDirty);
  }

  /*!
   * Defined callback functions from Kodi to add-on, for use in parent / child system
   * (is private)!
   */

  bool CControlRendering::OnCreateCB(GUIHANDLE cbhdl, int x, int y, int w, int h, void* device)
  {
    return static_cast<CControlRendering*>(cbhdl)->Create(x, y, w, h, device);
  }

  void CControlRendering::OnRenderCB(GUIHANDLE cbhdl)
  {
    static_cast<CControlRendering*>(cbhdl)->Render();
  }

  void CControlRendering::OnStopCB(GUIHANDLE cbhdl)
  {
    static_cast<CControlRendering*>(cbhdl)->Stop();
  }

  bool CControlRendering::OnDirtyCB(GUIHANDLE cbhdl)
  {
    return static_cast<CControlRendering*>(cbhdl)->Dirty();
  }

}; /* namespace GUI */

}; /* namespace KodiAPI */
}; /* namespace V2 */
