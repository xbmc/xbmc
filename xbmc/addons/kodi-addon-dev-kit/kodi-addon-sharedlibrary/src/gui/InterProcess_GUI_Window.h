#pragma once
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

#include "kodi/api2/.internal/AddonLib_internal.hpp"

#include <string>

extern "C"
{

  struct CKODIAddon_InterProcess_GUI_Window
  {
    GUIHANDLE Window_New(
          const std::string&          xmlFilename,
          const std::string&          defaultSkin,
          bool                        forceFallback,
          bool                        asDialog);
    void Window_Delete(GUIHANDLE handle);
    bool Window_Show(GUIHANDLE handle);
    void Window_Close(GUIHANDLE handle);
    void Window_DoModal(GUIHANDLE handle);
    bool Window_SetFocusId(GUIHANDLE handle, int iControlId);
    int Window_GetFocusId(GUIHANDLE handle);
    void Window_SetProperty(GUIHANDLE handle, const std::string& key, const std::string& value);
    void Window_SetPropertyInt(GUIHANDLE handle, const std::string& key, int value);
    void Window_SetPropertyBool(GUIHANDLE handle, const std::string& key, bool value);
    void Window_SetPropertyDouble(GUIHANDLE handle, const std::string& key, double value);
    std::string Window_GetProperty(GUIHANDLE handle, const std::string& key) const;
    int Window_GetPropertyInt(GUIHANDLE handle, const std::string& key) const;
    bool Window_GetPropertyBool(GUIHANDLE handle, const std::string& key) const;
    double Window_GetPropertyDouble(GUIHANDLE handle, const std::string& key) const;
    void Window_ClearProperties(GUIHANDLE handle);
    void Window_ClearProperty(GUIHANDLE handle, const std::string& key);
    int Window_GetListSize(GUIHANDLE handle);
    void Window_ClearList(GUIHANDLE handle);
    GUIHANDLE Window_AddStringItem(GUIHANDLE handle, const std::string& name, int itemPosition);
    void Window_AddItem(GUIHANDLE handle, GUIHANDLE item, int itemPosition);
    void Window_RemoveItem(GUIHANDLE handle, int itemPosition);
    void Window_RemoveItemFile(GUIHANDLE handle, GUIHANDLE item);
    GUIHANDLE Window_GetListItem(GUIHANDLE handle, int listPos);
    void Window_SetCurrentListPosition(GUIHANDLE handle, int listPos);
    int Window_GetCurrentListPosition(GUIHANDLE handle);
    void Window_SetControlLabel(GUIHANDLE handle, int controlId, const std::string& label);
    void Window_MarkDirtyRegion(GUIHANDLE handle);
    void Window_SetCallbacks(
      GUIHANDLE     handle,
      GUIHANDLE     cbhdl,
      bool          (*CBOnInit)  (GUIHANDLE cbhdl),
      bool          (*CBOnFocus) (GUIHANDLE cbhdl, int controlId),
      bool          (*CBOnClick) (GUIHANDLE cbhdl, int controlId),
      bool          (*CBOnAction)(GUIHANDLE cbhdl, int actionId));
    void* Window_GetControl_Button(void* window, int controlId);
    void* Window_GetControl_Edit(void* window, int controlId);
    void* Window_GetControl_FadeLabel(void* window, int controlId);
    void* Window_GetControl_Image(void* window, int controlId);
    void* Window_GetControl_Label(void* window, int controlId);
    void* Window_GetControl_Progress(void* window, int controlId);
    void* Window_GetControl_RadioButton(void* window, int controlId);
    void* Window_GetControl_Rendering(void* window, int controlId);
    void* Window_GetControl_SettingsSlider(void* window, int controlId);
    void* Window_GetControl_Slider(void* window, int controlId);
    void* Window_GetControl_Spin(void* window, int controlId);
    void* Window_GetControl_TextBox(void* window, int controlId);

  };

}; /* extern "C" */
