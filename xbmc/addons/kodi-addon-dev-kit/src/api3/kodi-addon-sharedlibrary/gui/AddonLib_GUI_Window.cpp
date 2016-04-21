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
#include KITINCLUDE(ADDON_API_LEVEL, gui/Window.hpp)
#include KITINCLUDE(ADDON_API_LEVEL, gui/ListItem.hpp)

API_NAMESPACE

namespace KodiAPI
{
namespace GUI
{

  CWindow::CWindow(
          const std::string&          xmlFilename,
          const std::string&          defaultSkin,
          bool                        forceFallback,
          bool                        asDialog)
  {
    m_WindowHandle = g_interProcess.m_Callbacks->GUI.Window.New(g_interProcess.m_Handle, xmlFilename.c_str(),
                          defaultSkin.c_str(), forceFallback, asDialog);
    if (!m_WindowHandle)
      fprintf(stderr, "libKODI_gui-ERROR: cGUIWindow can't create window class from Kodi !!!\n");

    g_interProcess.m_Callbacks->GUI.Window.SetCallbacks(g_interProcess.m_Handle, m_WindowHandle, this,
                              OnInitCB, OnClickCB, OnFocusCB, OnActionCB);
  }

  CWindow::~CWindow()
  {
    if (m_WindowHandle)
      g_interProcess.m_Callbacks->GUI.Window.Delete(g_interProcess.m_Handle, m_WindowHandle);
  }

  bool CWindow::Show()
  {
    return g_interProcess.m_Callbacks->GUI.Window.Show(g_interProcess.m_Handle, m_WindowHandle);
  }

  void CWindow::Close()
  {
    g_interProcess.m_Callbacks->GUI.Window.Close(g_interProcess.m_Handle, m_WindowHandle);
  }

  void CWindow::DoModal()
  {
    g_interProcess.m_Callbacks->GUI.Window.DoModal(g_interProcess.m_Handle, m_WindowHandle);
  }

  bool CWindow::SetFocusId(int iControlId)
  {
    return g_interProcess.m_Callbacks->GUI.Window.SetFocusId(g_interProcess.m_Handle, m_WindowHandle, iControlId);
  }

  int CWindow::GetFocusId()
  {
    return g_interProcess.m_Callbacks->GUI.Window.GetFocusId(g_interProcess.m_Handle, m_WindowHandle);
  }

  void CWindow::SetProperty(const std::string& key, const std::string& value)
  {
    g_interProcess.m_Callbacks->GUI.Window.SetProperty(g_interProcess.m_Handle, m_WindowHandle, key.c_str(), value.c_str());
  }

  void CWindow::SetPropertyInt(const std::string& key, int value)
  {
    g_interProcess.m_Callbacks->GUI.Window.SetPropertyInt(g_interProcess.m_Handle, m_WindowHandle, key.c_str(), value);
  }

  void CWindow::SetPropertyBool(const std::string& key, bool value)
  {
    g_interProcess.m_Callbacks->GUI.Window.SetPropertyBool(g_interProcess.m_Handle, m_WindowHandle, key.c_str(), value);
  }

  void CWindow::SetPropertyDouble(const std::string& key, double value)
  {
    g_interProcess.m_Callbacks->GUI.Window.SetPropertyDouble(g_interProcess.m_Handle, m_WindowHandle, key.c_str(), value);
  }

  std::string CWindow::GetProperty(const std::string& key) const
  {
    std::string label;
    label.resize(1024);
    unsigned int size = (unsigned int)label.capacity();
    g_interProcess.m_Callbacks->GUI.Window.GetProperty(g_interProcess.m_Handle, m_WindowHandle, key.c_str(), label[0], size);
    label.resize(size);
    label.shrink_to_fit();
    return label.c_str();
  }

  int CWindow::GetPropertyInt(const std::string& key) const
  {
    return g_interProcess.m_Callbacks->GUI.Window.GetPropertyInt(g_interProcess.m_Handle, m_WindowHandle, key.c_str());
  }

  bool CWindow::GetPropertyBool(const std::string& key) const
  {
    return g_interProcess.m_Callbacks->GUI.Window.GetPropertyBool(g_interProcess.m_Handle, m_WindowHandle, key.c_str());
  }

  double CWindow::GetPropertyDouble(const std::string& key) const
  {
    return g_interProcess.m_Callbacks->GUI.Window.GetPropertyDouble(g_interProcess.m_Handle, m_WindowHandle, key.c_str());
  }

  void CWindow::ClearProperties()
  {
    g_interProcess.m_Callbacks->GUI.Window.ClearProperties(g_interProcess.m_Handle, m_WindowHandle);
  }

  void CWindow::ClearProperty(const std::string& key)
  {
    g_interProcess.m_Callbacks->GUI.Window.ClearProperty(g_interProcess.m_Handle, m_WindowHandle, key.c_str());
  }
  
  int CWindow::GetListSize()
  {
    return g_interProcess.m_Callbacks->GUI.Window.GetListSize(g_interProcess.m_Handle, m_WindowHandle);
  }
  
  void CWindow::ClearList()
  {
    g_interProcess.m_Callbacks->GUI.Window.ClearList(g_interProcess.m_Handle, m_WindowHandle);
  }
  
  CListItem* CWindow::AddStringItem(const std::string& name, int itemPosition)
  {
    GUIHANDLE handle = g_interProcess.m_Callbacks->GUI.Window.AddStringItem(g_interProcess.m_Handle, m_WindowHandle, name.c_str(), itemPosition);
    if (handle)
      return new CListItem(handle);

    fprintf(stderr, "libKODI_gui-ERROR: cGUIWindow Failed to get control on list ṕosition %i !!!\n", itemPosition);
    return nullptr;
  }

  void CWindow::AddItem(CListItem *item, int itemPosition)
  {
    g_interProcess.m_Callbacks->GUI.Window.AddItem(g_interProcess.m_Handle, m_WindowHandle, item->m_ListItemHandle, itemPosition);
  }

  void CWindow::RemoveItem(int itemPosition)
  {
    g_interProcess.m_Callbacks->GUI.Window.RemoveItem(g_interProcess.m_Handle, m_WindowHandle, itemPosition);
  }

  void CWindow::RemoveItem(CListItem* item)
  {
    g_interProcess.m_Callbacks->GUI.Window.RemoveItemFile(g_interProcess.m_Handle, m_WindowHandle, item->m_ListItemHandle);
  }

  CListItem* CWindow::GetListItem(int listPos)
  {
    GUIHANDLE handle = g_interProcess.m_Callbacks->GUI.Window.GetListItem(g_interProcess.m_Handle, m_WindowHandle, listPos);
    if (handle)
      return new CListItem(handle);

    fprintf(stderr, "libKODI_gui-ERROR: cGUIWindow Failed to get control on list ṕosition %i !!!\n", listPos);
    return nullptr;
  }

  void CWindow::SetCurrentListPosition(int listPos)
  {
    g_interProcess.m_Callbacks->GUI.Window.SetCurrentListPosition(g_interProcess.m_Handle, m_WindowHandle, listPos);
  }

  int CWindow::GetCurrentListPosition()
  {
    return g_interProcess.m_Callbacks->GUI.Window.GetCurrentListPosition(g_interProcess.m_Handle, m_WindowHandle);
  }

  void CWindow::SetControlLabel(int controlId, const std::string& label)
  {
    g_interProcess.m_Callbacks->GUI.Window.SetControlLabel(g_interProcess.m_Handle, m_WindowHandle, controlId, label.c_str());
  }

  void CWindow::MarkDirtyRegion()
  {
    g_interProcess.m_Callbacks->GUI.Window.MarkDirtyRegion(g_interProcess.m_Handle, m_WindowHandle);
  }

  /*!
   * Kodi to add-on override definition function to use if class becomes used
   * independet.
   */
  void CWindow::SetIndependentCallbacks(
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

    g_interProcess.m_Callbacks->GUI.Window.SetCallbacks(g_interProcess.m_Handle, m_WindowHandle, cbhdl,
                              CBOnInit, CBOnFocus, CBOnClick, CBOnAction);
  }

  /*!
   * Defined callback functions from Kodi to add-on, for use in parent / child system
   * (is private)!
   */

  bool CWindow::OnInitCB(GUIHANDLE cbhdl)
  {
    return static_cast<CWindow*>(cbhdl)->OnInit();
  }

  bool CWindow::OnClickCB(GUIHANDLE cbhdl, int controlId)
  {
    return static_cast<CWindow*>(cbhdl)->OnClick(controlId);
  }

  bool CWindow::OnFocusCB(GUIHANDLE cbhdl, int controlId)
  {
    return static_cast<CWindow*>(cbhdl)->OnFocus(controlId);
  }

  bool CWindow::OnActionCB(GUIHANDLE cbhdl, int actionId)
  {
    return static_cast<CWindow*>(cbhdl)->OnAction(actionId);
  }

} /* namespace GUI */
} /* namespace KodiAPI */

END_NAMESPACE()
