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

#include "InterProcess_GUI_ControlRendering.h"
#include "InterProcess.h"
#include "RequestPacket.h"
#include "ResponsePacket.h"

#include <p8-platform/util/StringUtils.h>
#include <cstring>
#include <iostream>       // std::cerr
#include <stdexcept>      // std::out_of_range

using namespace P8PLATFORM;

extern "C"
{

void CKODIAddon_InterProcess_GUI_ControlRendering::Control_Rendering_Delete(void* handle)
{
}

void CKODIAddon_InterProcess_GUI_ControlRendering::Control_Rendering_SetCallbacks(
    void*                 handle,
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
}


}; /* extern "C" */
