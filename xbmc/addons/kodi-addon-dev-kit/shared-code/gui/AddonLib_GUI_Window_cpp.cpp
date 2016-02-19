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
#include "kodi/api2/gui/Window.hpp"
#include "kodi/api2/gui/ListItem.hpp"

namespace V2
{
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
    m_WindowHandle = g_interProcess.Window_New(xmlFilename, defaultSkin, forceFallback, asDialog);
    if (!m_WindowHandle)
      fprintf(stderr, "libKODI_gui-ERROR: cGUIWindow can't create window class from Kodi !!!\n");

    g_interProcess.Window_SetCallbacks(m_WindowHandle, this,
                              OnInitCB, OnClickCB, OnFocusCB, OnActionCB);
  }

  CWindow::~CWindow()
  {
    if (m_WindowHandle)
      g_interProcess.Window_Delete(m_WindowHandle);
  }

  bool CWindow::Show()
  {
    return g_interProcess.Window_Show(m_WindowHandle);
  }

  void CWindow::Close()
  {
    g_interProcess.Window_Close(m_WindowHandle);
  }

  void CWindow::DoModal()
  {
    g_interProcess.Window_DoModal(m_WindowHandle);
  }

  bool CWindow::SetFocusId(int iControlId)
  {
    return g_interProcess.Window_SetFocusId(m_WindowHandle, iControlId);
  }

  int CWindow::GetFocusId()
  {
    return g_interProcess.Window_GetFocusId(m_WindowHandle);
  }

  void CWindow::SetProperty(const std::string& key, const std::string& value)
  {
    g_interProcess.Window_SetProperty(m_WindowHandle, key, value);
  }
  
  void CWindow::SetPropertyInt(const std::string& key, int value)
  {
    g_interProcess.Window_SetPropertyInt(m_WindowHandle, key, value);
  }

  void CWindow::SetPropertyBool(const std::string& key, bool value)
  {
    g_interProcess.Window_SetPropertyBool(m_WindowHandle, key, value);
  }

  void CWindow::SetPropertyDouble(const std::string& key, double value)
  {
    g_interProcess.Window_SetPropertyDouble(m_WindowHandle, key, value);
  }

  std::string CWindow::GetProperty(const std::string& key) const
  {
    return g_interProcess.Window_GetProperty(m_WindowHandle, key);
  }

  int CWindow::GetPropertyInt(const std::string& key) const
  {
    return g_interProcess.Window_GetPropertyInt(m_WindowHandle, key);
  }
  
  bool CWindow::GetPropertyBool(const std::string& key) const
  {
    return g_interProcess.Window_GetPropertyBool(m_WindowHandle, key);
  }
  
  double CWindow::GetPropertyDouble(const std::string& key) const
  {
    return g_interProcess.Window_GetPropertyDouble(m_WindowHandle, key);
  }
  
  void CWindow::ClearProperties()
  {
    g_interProcess.Window_ClearProperties(m_WindowHandle);
  }
  
  void CWindow::ClearProperty(const std::string& key)
  {
    g_interProcess.Window_ClearProperty(m_WindowHandle, key);
  }
  
  int CWindow::GetListSize()
  {
    return g_interProcess.Window_GetListSize(m_WindowHandle);
  }
  
  void CWindow::ClearList()
  {
    g_interProcess.Window_ClearList(m_WindowHandle);
  }
  
  CListItem* CWindow::AddStringItem(const std::string& name, int itemPosition)
  {
    GUIHANDLE handle = g_interProcess.Window_AddStringItem(m_WindowHandle, name, itemPosition);
    if (handle)
      return new CListItem(handle);

    fprintf(stderr, "libKODI_gui-ERROR: cGUIWindow Failed to get control on list ṕosition %i !!!\n", itemPosition);
    return nullptr;
  }

  void CWindow::AddItem(CListItem *item, int itemPosition)
  {
    g_interProcess.Window_AddItem(m_WindowHandle, item->m_ListItemHandle, itemPosition);
  }

  void CWindow::RemoveItem(int itemPosition)
  {
    g_interProcess.Window_RemoveItem(m_WindowHandle, itemPosition);
  }

  void CWindow::RemoveItem(CListItem* item)
  {
    g_interProcess.Window_RemoveItemFile(m_WindowHandle, item->m_ListItemHandle);
  }

  CListItem* CWindow::GetListItem(int listPos)
  {
    GUIHANDLE handle = g_interProcess.Window_GetListItem(m_WindowHandle, listPos);
    if (handle)
      return new CListItem(handle);
    
    fprintf(stderr, "libKODI_gui-ERROR: cGUIWindow Failed to get control on list ṕosition %i !!!\n", listPos);
    return nullptr;
  }
    
  void CWindow::SetCurrentListPosition(int listPos)
  {
    g_interProcess.Window_SetCurrentListPosition(m_WindowHandle, listPos);
  }
    
  int CWindow::GetCurrentListPosition()
  {
    return g_interProcess.Window_GetCurrentListPosition(m_WindowHandle);
  }
    
  void CWindow::SetControlLabel(int controlId, const std::string& label)
  {
    g_interProcess.Window_SetControlLabel(m_WindowHandle, controlId, label);
  }
    
  void CWindow::MarkDirtyRegion()
  {
    g_interProcess.Window_MarkDirtyRegion(m_WindowHandle);
  }
    
  /*!
   * Kodi to add-on override defination function to use if class becomes used
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

    g_interProcess.Window_SetCallbacks(m_WindowHandle, cbhdl,
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

}; /* namespace GUI */

}; /* namespace KodiAPI */
}; /* namespace V2 */
