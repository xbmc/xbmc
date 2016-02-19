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

extern "C"
{

GUIHANDLE CKODIAddon_InterProcess_GUI_Window::Window_New(
          const std::string&          xmlFilename,
          const std::string&          defaultSkin,
          bool                        forceFallback,
          bool                        asDialog)
{
  return g_interProcess.m_Callbacks->GUI.Window.New(g_interProcess.m_Handle->addonData, xmlFilename.c_str(),
                          defaultSkin.c_str(), forceFallback, asDialog);
}

void CKODIAddon_InterProcess_GUI_Window::Window_Delete(GUIHANDLE handle)
{
  g_interProcess.m_Callbacks->GUI.Window.Delete(g_interProcess.m_Handle->addonData, handle);
}

bool CKODIAddon_InterProcess_GUI_Window::Window_Show(GUIHANDLE handle)
{
  return g_interProcess.m_Callbacks->GUI.Window.Show(g_interProcess.m_Handle->addonData, handle);
}

void CKODIAddon_InterProcess_GUI_Window::Window_Close(GUIHANDLE handle)
{
  g_interProcess.m_Callbacks->GUI.Window.Close(g_interProcess.m_Handle->addonData, handle);
}

void CKODIAddon_InterProcess_GUI_Window::Window_DoModal(GUIHANDLE handle)
{
  g_interProcess.m_Callbacks->GUI.Window.DoModal(g_interProcess.m_Handle->addonData, handle);
}

bool CKODIAddon_InterProcess_GUI_Window::Window_SetFocusId(GUIHANDLE handle, int iControlId)
{
  return g_interProcess.m_Callbacks->GUI.Window.SetFocusId(g_interProcess.m_Handle->addonData, handle, iControlId);
}

int CKODIAddon_InterProcess_GUI_Window::Window_GetFocusId(GUIHANDLE handle)
{
  return g_interProcess.m_Callbacks->GUI.Window.GetFocusId(g_interProcess.m_Handle->addonData, handle);
}

void CKODIAddon_InterProcess_GUI_Window::Window_SetProperty(GUIHANDLE handle, const std::string& key, const std::string& value)
{
  g_interProcess.m_Callbacks->GUI.Window.SetProperty(g_interProcess.m_Handle->addonData, handle, key.c_str(), value.c_str());
}

void CKODIAddon_InterProcess_GUI_Window::Window_SetPropertyInt(GUIHANDLE handle, const std::string& key, int value)
{
  g_interProcess.m_Callbacks->GUI.Window.SetPropertyInt(g_interProcess.m_Handle->addonData, handle, key.c_str(), value);
}

void CKODIAddon_InterProcess_GUI_Window::Window_SetPropertyBool(GUIHANDLE handle, const std::string& key, bool value)
{
  g_interProcess.m_Callbacks->GUI.Window.SetPropertyBool(g_interProcess.m_Handle->addonData, handle, key.c_str(), value);
}

void CKODIAddon_InterProcess_GUI_Window::Window_SetPropertyDouble(GUIHANDLE handle, const std::string& key, double value)
{
  g_interProcess.m_Callbacks->GUI.Window.SetPropertyDouble(g_interProcess.m_Handle->addonData, handle, key.c_str(), value);
}

std::string CKODIAddon_InterProcess_GUI_Window::Window_GetProperty(GUIHANDLE handle, const std::string& key) const
{
  std::string label;
  label.resize(1024);
  unsigned int size = (unsigned int)label.capacity();
  g_interProcess.m_Callbacks->GUI.Window.GetProperty(g_interProcess.m_Handle->addonData, handle, key.c_str(), label[0], size);
  label.resize(size);
  label.shrink_to_fit();
  return label.c_str();
}

int CKODIAddon_InterProcess_GUI_Window::Window_GetPropertyInt(GUIHANDLE handle, const std::string& key) const
{
  return g_interProcess.m_Callbacks->GUI.Window.GetPropertyInt(g_interProcess.m_Handle->addonData, handle, key.c_str());
}

bool CKODIAddon_InterProcess_GUI_Window::Window_GetPropertyBool(GUIHANDLE handle, const std::string& key) const
{
  return g_interProcess.m_Callbacks->GUI.Window.GetPropertyBool(g_interProcess.m_Handle->addonData, handle, key.c_str());
}

double CKODIAddon_InterProcess_GUI_Window::Window_GetPropertyDouble(GUIHANDLE handle, const std::string& key) const
{
  return g_interProcess.m_Callbacks->GUI.Window.GetPropertyDouble(g_interProcess.m_Handle->addonData, handle, key.c_str());
}

void CKODIAddon_InterProcess_GUI_Window::Window_ClearProperties(GUIHANDLE handle)
{
  g_interProcess.m_Callbacks->GUI.Window.ClearProperties(g_interProcess.m_Handle->addonData, handle);
}

void CKODIAddon_InterProcess_GUI_Window::Window_ClearProperty(GUIHANDLE handle, const std::string& key)
{
  g_interProcess.m_Callbacks->GUI.Window.ClearProperty(g_interProcess.m_Handle->addonData, handle, key.c_str());
}

int CKODIAddon_InterProcess_GUI_Window::Window_GetListSize(GUIHANDLE handle)
{
  return g_interProcess.m_Callbacks->GUI.Window.GetListSize(g_interProcess.m_Handle->addonData, handle);
}

void CKODIAddon_InterProcess_GUI_Window::Window_ClearList(GUIHANDLE handle)
{
  g_interProcess.m_Callbacks->GUI.Window.ClearList(g_interProcess.m_Handle->addonData, handle);
}

GUIHANDLE CKODIAddon_InterProcess_GUI_Window::Window_AddStringItem(GUIHANDLE handle, const std::string& name, int itemPosition)
{
  return g_interProcess.m_Callbacks->GUI.Window.AddStringItem(g_interProcess.m_Handle->addonData, handle, name.c_str(), itemPosition);
}

void CKODIAddon_InterProcess_GUI_Window::Window_AddItem(GUIHANDLE handle, GUIHANDLE item, int itemPosition)
{
  g_interProcess.m_Callbacks->GUI.Window.AddItem(g_interProcess.m_Handle->addonData, handle, item, itemPosition);
}

void CKODIAddon_InterProcess_GUI_Window::Window_RemoveItem(GUIHANDLE handle, int itemPosition)
{
  g_interProcess.m_Callbacks->GUI.Window.RemoveItem(g_interProcess.m_Handle->addonData, handle, itemPosition);
}

void CKODIAddon_InterProcess_GUI_Window::Window_RemoveItemFile(GUIHANDLE handle, GUIHANDLE item)
{
  g_interProcess.m_Callbacks->GUI.Window.RemoveItemFile(g_interProcess.m_Handle->addonData, handle, item);
}

GUIHANDLE CKODIAddon_InterProcess_GUI_Window::Window_GetListItem(GUIHANDLE handle, int listPos)
{
  return g_interProcess.m_Callbacks->GUI.Window.GetListItem(g_interProcess.m_Handle->addonData, handle, listPos);
}

void CKODIAddon_InterProcess_GUI_Window::Window_SetCurrentListPosition(GUIHANDLE handle, int listPos)
{
  g_interProcess.m_Callbacks->GUI.Window.SetCurrentListPosition(g_interProcess.m_Handle->addonData, handle, listPos);
}

int CKODIAddon_InterProcess_GUI_Window::Window_GetCurrentListPosition(GUIHANDLE handle)
{
  return g_interProcess.m_Callbacks->GUI.Window.GetCurrentListPosition(g_interProcess.m_Handle->addonData, handle);
}

void CKODIAddon_InterProcess_GUI_Window::Window_SetControlLabel(GUIHANDLE handle, int controlId, const std::string& label)
{
  g_interProcess.m_Callbacks->GUI.Window.SetControlLabel(g_interProcess.m_Handle->addonData, handle, controlId, label.c_str());
}

void CKODIAddon_InterProcess_GUI_Window::Window_MarkDirtyRegion(GUIHANDLE handle)
{
  g_interProcess.m_Callbacks->GUI.Window.MarkDirtyRegion(g_interProcess.m_Handle->addonData, handle);
}

void CKODIAddon_InterProcess_GUI_Window::Window_SetCallbacks(
      GUIHANDLE     handle,
      GUIHANDLE     cbhdl,
      bool          (*CBOnInit)  (GUIHANDLE cbhdl),
      bool          (*CBOnFocus) (GUIHANDLE cbhdl, int controlId),
      bool          (*CBOnClick) (GUIHANDLE cbhdl, int controlId),
      bool          (*CBOnAction)(GUIHANDLE cbhdl, int actionId))
{
  if (!cbhdl ||
      !CBOnInit || !CBOnFocus || !CBOnClick || !CBOnAction)
  {
    fprintf(stderr, "libKODI_gui-ERROR: SetIndependentCallbacks called with nullptr !!!\n");
    return;
  }

  g_interProcess.m_Callbacks->GUI.Window.SetCallbacks(g_interProcess.m_Handle->addonData, handle, cbhdl,
                                           CBOnInit, CBOnFocus, CBOnClick, CBOnAction);
}

void* CKODIAddon_InterProcess_GUI_Window::Window_GetControl_Button(void* window, int controlId)
{
  return g_interProcess.m_Callbacks->GUI.Window.GetControl_Button(g_interProcess.m_Handle->addonData, window, controlId);
}

void* CKODIAddon_InterProcess_GUI_Window::Window_GetControl_Edit(void* window, int controlId)
{
  return g_interProcess.m_Callbacks->GUI.Window.GetControl_Edit(g_interProcess.m_Handle->addonData, window, controlId);
}

void* CKODIAddon_InterProcess_GUI_Window::Window_GetControl_FadeLabel(void* window, int controlId)
{
  return g_interProcess.m_Callbacks->GUI.Window.GetControl_FadeLabel(g_interProcess.m_Handle->addonData, window, controlId);
}

void* CKODIAddon_InterProcess_GUI_Window::Window_GetControl_Image(void* window, int controlId)
{
  return g_interProcess.m_Callbacks->GUI.Window.GetControl_Image(g_interProcess.m_Handle->addonData, window, controlId);
}

void* CKODIAddon_InterProcess_GUI_Window::Window_GetControl_Label(void* window, int controlId)
{
  return g_interProcess.m_Callbacks->GUI.Window.GetControl_Label(g_interProcess.m_Handle->addonData, window, controlId);
}

void* CKODIAddon_InterProcess_GUI_Window::Window_GetControl_Progress(void* window, int controlId)
{
  return g_interProcess.m_Callbacks->GUI.Window.GetControl_Progress(g_interProcess.m_Handle->addonData, window, controlId);
}

void* CKODIAddon_InterProcess_GUI_Window::Window_GetControl_RadioButton(void* window, int controlId)
{
  return g_interProcess.m_Callbacks->GUI.Window.GetControl_RadioButton(g_interProcess.m_Handle->addonData, window, controlId);
}

void* CKODIAddon_InterProcess_GUI_Window::Window_GetControl_Rendering(void* window, int controlId)
{
  return g_interProcess.m_Callbacks->GUI.Window.GetControl_RenderAddon(g_interProcess.m_Handle->addonData, window, controlId);
}

void* CKODIAddon_InterProcess_GUI_Window::Window_GetControl_SettingsSlider(void* window, int controlId)
{
  return g_interProcess.m_Callbacks->GUI.Window.GetControl_SettingsSlider(g_interProcess.m_Handle->addonData, window, controlId);
}

void* CKODIAddon_InterProcess_GUI_Window::Window_GetControl_Slider(void* window, int controlId)
{
  return g_interProcess.m_Callbacks->GUI.Window.GetControl_Slider(g_interProcess.m_Handle->addonData, window, controlId);
}

void* CKODIAddon_InterProcess_GUI_Window::Window_GetControl_Spin(void* window, int controlId)
{
  return g_interProcess.m_Callbacks->GUI.Window.GetControl_Spin(g_interProcess.m_Handle->addonData, window, controlId);
}

void* CKODIAddon_InterProcess_GUI_Window::Window_GetControl_TextBox(void* window, int controlId)
{
  return g_interProcess.m_Callbacks->GUI.Window.GetControl_TextBox(g_interProcess.m_Handle->addonData, window, controlId);
}


}; /* extern "C" */
