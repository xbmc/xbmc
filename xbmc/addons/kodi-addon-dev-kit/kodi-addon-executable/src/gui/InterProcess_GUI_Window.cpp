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

#include "InterProcess_GUI_Window.h"
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

GUIHANDLE CKODIAddon_InterProcess_GUI_Window::Window_New(
          const std::string&          xmlFilename,
          const std::string&          defaultSkin,
          bool                        forceFallback,
          bool                        asDialog)
{

}

void CKODIAddon_InterProcess_GUI_Window::Window_Delete(GUIHANDLE handle)
{

}

bool CKODIAddon_InterProcess_GUI_Window::Window_Show(GUIHANDLE handle)
{

}

void CKODIAddon_InterProcess_GUI_Window::Window_Close(GUIHANDLE handle)
{

}

void CKODIAddon_InterProcess_GUI_Window::Window_DoModal(GUIHANDLE handle)
{

}

bool CKODIAddon_InterProcess_GUI_Window::Window_SetFocusId(GUIHANDLE handle, int iControlId)
{

}

int CKODIAddon_InterProcess_GUI_Window::Window_GetFocusId(GUIHANDLE handle)
{

}

void CKODIAddon_InterProcess_GUI_Window::Window_SetProperty(GUIHANDLE handle, const std::string& key, const std::string& value)
{

}

void CKODIAddon_InterProcess_GUI_Window::Window_SetPropertyInt(GUIHANDLE handle, const std::string& key, int value)
{

}

void CKODIAddon_InterProcess_GUI_Window::Window_SetPropertyBool(GUIHANDLE handle, const std::string& key, bool value)
{

}

void CKODIAddon_InterProcess_GUI_Window::Window_SetPropertyDouble(GUIHANDLE handle, const std::string& key, double value)
{

}

std::string CKODIAddon_InterProcess_GUI_Window::Window_GetProperty(GUIHANDLE handle, const std::string& key) const
{

}

int CKODIAddon_InterProcess_GUI_Window::Window_GetPropertyInt(GUIHANDLE handle, const std::string& key) const
{

}

bool CKODIAddon_InterProcess_GUI_Window::Window_GetPropertyBool(GUIHANDLE handle, const std::string& key) const
{

}

double CKODIAddon_InterProcess_GUI_Window::Window_GetPropertyDouble(GUIHANDLE handle, const std::string& key) const
{

}

void CKODIAddon_InterProcess_GUI_Window::Window_ClearProperties(GUIHANDLE handle)
{

}

void CKODIAddon_InterProcess_GUI_Window::Window_ClearProperty(GUIHANDLE handle, const std::string& key)
{

}

int CKODIAddon_InterProcess_GUI_Window::Window_GetListSize(GUIHANDLE handle)
{

}

void CKODIAddon_InterProcess_GUI_Window::Window_ClearList(GUIHANDLE handle)
{

}

GUIHANDLE CKODIAddon_InterProcess_GUI_Window::Window_AddStringItem(GUIHANDLE handle, const std::string& name, int itemPosition)
{

}

void CKODIAddon_InterProcess_GUI_Window::Window_AddItem(GUIHANDLE handle, GUIHANDLE item, int itemPosition)
{

}

void CKODIAddon_InterProcess_GUI_Window::Window_RemoveItem(GUIHANDLE handle, int itemPosition)
{

}

void CKODIAddon_InterProcess_GUI_Window::Window_RemoveItemFile(GUIHANDLE handle, GUIHANDLE item)
{

}

GUIHANDLE CKODIAddon_InterProcess_GUI_Window::Window_GetListItem(GUIHANDLE handle, int listPos)
{

}

void CKODIAddon_InterProcess_GUI_Window::Window_SetCurrentListPosition(GUIHANDLE handle, int listPos)
{

}

int CKODIAddon_InterProcess_GUI_Window::Window_GetCurrentListPosition(GUIHANDLE handle)
{

}

void CKODIAddon_InterProcess_GUI_Window::Window_SetControlLabel(GUIHANDLE handle, int controlId, const std::string& label)
{

}

void CKODIAddon_InterProcess_GUI_Window::Window_MarkDirtyRegion(GUIHANDLE handle)
{

}

void CKODIAddon_InterProcess_GUI_Window::Window_SetCallbacks(
      GUIHANDLE     handle,
      GUIHANDLE     cbhdl,
      bool          (*CBOnInit)  (GUIHANDLE cbhdl),
      bool          (*CBOnFocus) (GUIHANDLE cbhdl, int controlId),
      bool          (*CBOnClick) (GUIHANDLE cbhdl, int controlId),
      bool          (*CBOnAction)(GUIHANDLE cbhdl, int actionId))
{

}

void* CKODIAddon_InterProcess_GUI_Window::Window_GetControl_Button(void* window, int controlId)
{

}

void* CKODIAddon_InterProcess_GUI_Window::Window_GetControl_Edit(void* window, int controlId)
{

}

void* CKODIAddon_InterProcess_GUI_Window::Window_GetControl_FadeLabel(void* window, int controlId)
{

}

void* CKODIAddon_InterProcess_GUI_Window::Window_GetControl_Image(void* window, int controlId)
{

}

void* CKODIAddon_InterProcess_GUI_Window::Window_GetControl_Label(void* window, int controlId)
{

}

void* CKODIAddon_InterProcess_GUI_Window::Window_GetControl_Progress(void* window, int controlId)
{

}

void* CKODIAddon_InterProcess_GUI_Window::Window_GetControl_RadioButton(void* window, int controlId)
{

}

void* CKODIAddon_InterProcess_GUI_Window::Window_GetControl_Rendering(void* window, int controlId)
{

}

void* CKODIAddon_InterProcess_GUI_Window::Window_GetControl_SettingsSlider(void* window, int controlId)
{

}

void* CKODIAddon_InterProcess_GUI_Window::Window_GetControl_Slider(void* window, int controlId)
{

}

void* CKODIAddon_InterProcess_GUI_Window::Window_GetControl_Spin(void* window, int controlId)
{

}

void* CKODIAddon_InterProcess_GUI_Window::Window_GetControl_TextBox(void* window, int controlId)
{

}


}; /* extern "C" */
