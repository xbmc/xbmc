/*
 *      Copyright (C) 2005-2010 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string>
#include "../../../addons/library.xbmc.gui/libXBMC_gui.h"
#include "addons/AddonCallbacks.h"

#ifdef _WIN32
#include <windows.h>
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif

using namespace std;

AddonCB *m_Handle = NULL;
CB_GUILib *m_cb   = NULL;

extern "C"
{

DLLEXPORT int GUI_register_me(void *hdl)
{
  if (!hdl)
    fprintf(stderr, "libXBMC_gui-ERROR: GUILib_register_me is called with NULL handle !!!\n");
  else
  {
    m_Handle = (AddonCB*) hdl;
    m_cb     = m_Handle->GUILib_RegisterMe(m_Handle->addonData);
    if (!m_cb)
      fprintf(stderr, "libXBMC_gui-ERROR: GUILib_register_me can't get callback table from XBMC !!!\n");
    else
      return 1;
  }
  return 0;
}

DLLEXPORT void GUI_unregister_me()
{
  if (m_Handle && m_cb)
    m_Handle->GUILib_UnRegisterMe(m_Handle->addonData, m_cb);
}

DLLEXPORT void GUI_lock()
{
  m_cb->Lock();
}

DLLEXPORT void GUI_unlock()
{
  m_cb->Unlock();
}

DLLEXPORT int GUI_get_screen_height()
{
  return m_cb->GetScreenHeight();
}

DLLEXPORT int GUI_get_screen_width()
{
  return m_cb->GetScreenWidth();
}

DLLEXPORT int GUI_get_video_resolution()
{
  return m_cb->GetVideoResolution();
}

DLLEXPORT CAddonGUIWindow* GUI_Window_create(const char *xmlFilename, const char *defaultSkin, bool forceFallback, bool asDialog)
{
  return new CAddonGUIWindow(xmlFilename, defaultSkin, forceFallback, asDialog);
}

DLLEXPORT void GUI_Window_destroy(CAddonGUIWindow* p)
{
  delete p;
}

DLLEXPORT bool GUI_Window_OnClick(GUIHANDLE handle, int controlId)
{
  CAddonGUIWindow *window = (CAddonGUIWindow*) handle;
  return window->OnClick(controlId);
}

DLLEXPORT bool GUI_Window_OnFocus(GUIHANDLE handle, int controlId)
{
  CAddonGUIWindow *window = (CAddonGUIWindow*) handle;
  return window->OnFocus(controlId);
}

DLLEXPORT bool GUI_Window_OnInit(GUIHANDLE handle)
{
  CAddonGUIWindow *window = (CAddonGUIWindow*) handle;
  return window->OnInit();
}

DLLEXPORT bool GUI_Window_OnAction(GUIHANDLE handle, int actionId)
{
  CAddonGUIWindow *window = (CAddonGUIWindow*) handle;
  return window->OnAction(actionId);
}


CAddonGUIWindow::CAddonGUIWindow(const char *xmlFilename, const char *defaultSkin, bool forceFallback, bool asDialog)
{
  CBOnInit = NULL;
  CBOnClick = NULL;
  CBOnFocus = NULL;
  if (m_Handle && m_cb)
  {
    m_WindowHandle = m_cb->Window_New(m_Handle->addonData, xmlFilename, defaultSkin, forceFallback, asDialog);
    if (!m_WindowHandle)
      fprintf(stderr, "libXBMC_gui-ERROR: cGUIWindow can't create window class from XBMC !!!\n");

    m_cb->Window_SetCallbacks(m_Handle->addonData, m_WindowHandle, this, GUI_Window_OnInit, GUI_Window_OnClick, GUI_Window_OnFocus, GUI_Window_OnAction);
  }
}

CAddonGUIWindow::~CAddonGUIWindow()
{
  if (m_Handle && m_cb && m_WindowHandle)
  {
    m_cb->Window_Delete(m_Handle->addonData, m_WindowHandle);
    m_WindowHandle = NULL;
  }
}

bool CAddonGUIWindow::Show()
{
  return m_cb->Window_Show(m_Handle->addonData, m_WindowHandle);
}

void CAddonGUIWindow::Close()
{
  m_cb->Window_Close(m_Handle->addonData, m_WindowHandle);
}

void CAddonGUIWindow::DoModal()
{
  m_cb->Window_DoModal(m_Handle->addonData, m_WindowHandle);
}

bool CAddonGUIWindow::OnInit()
{
  if (!CBOnInit)
    return false;

  return CBOnInit(m_cbhdl);
}

bool CAddonGUIWindow::OnClick(int controlId)
{
  if (!CBOnClick)
    return false;

  return CBOnClick(m_cbhdl, controlId);
}

bool CAddonGUIWindow::OnFocus(int controlId)
{
  if (!CBOnFocus)
    return false;

  return CBOnFocus(m_cbhdl, controlId);
}

bool CAddonGUIWindow::OnAction(int actionId)
{
  if (!CBOnAction)
    return false;

  return CBOnAction(m_cbhdl, actionId);
}

bool CAddonGUIWindow::SetFocusId(int iControlId)
{
  return m_cb->Window_SetFocusId(m_Handle->addonData, m_WindowHandle, iControlId);
}

int CAddonGUIWindow::GetFocusId()
{
  return m_cb->Window_GetFocusId(m_Handle->addonData, m_WindowHandle);
}

bool CAddonGUIWindow::SetCoordinateResolution(int res)
{
  return m_cb->Window_SetCoordinateResolution(m_Handle->addonData, m_WindowHandle, res);
}

void CAddonGUIWindow::SetProperty(const char *key, const char *value)
{
  m_cb->Window_SetProperty(m_Handle->addonData, m_WindowHandle, key, value);
}

void CAddonGUIWindow::SetPropertyInt(const char *key, int value)
{
  m_cb->Window_SetPropertyInt(m_Handle->addonData, m_WindowHandle, key, value);
}

void CAddonGUIWindow::SetPropertyBool(const char *key, bool value)
{
  m_cb->Window_SetPropertyBool(m_Handle->addonData, m_WindowHandle, key, value);
}

void CAddonGUIWindow::SetPropertyDouble(const char *key, double value)
{
  m_cb->Window_SetPropertyDouble(m_Handle->addonData, m_WindowHandle, key, value);
}

const char *CAddonGUIWindow::GetProperty(const char *key) const
{
  return m_cb->Window_GetProperty(m_Handle->addonData, m_WindowHandle, key);
}

int CAddonGUIWindow::GetPropertyInt(const char *key) const
{
  return m_cb->Window_GetPropertyInt(m_Handle->addonData, m_WindowHandle, key);
}

bool CAddonGUIWindow::GetPropertyBool(const char *key) const
{
  return m_cb->Window_GetPropertyBool(m_Handle->addonData, m_WindowHandle, key);
}

double CAddonGUIWindow::GetPropertyDouble(const char *key) const
{
  return m_cb->Window_GetPropertyDouble(m_Handle->addonData, m_WindowHandle, key);
}

void CAddonGUIWindow::ClearProperties()
{
  m_cb->Window_ClearProperties(m_Handle->addonData, m_WindowHandle);
}

int CAddonGUIWindow::GetListSize()
{
  return m_cb->Window_GetListSize(m_Handle->addonData, m_WindowHandle);
}

void CAddonGUIWindow::ClearList()
{
  m_cb->Window_ClearList(m_Handle->addonData, m_WindowHandle);
}

GUIHANDLE CAddonGUIWindow::AddStringItem(const char *name, int itemPosition)
{
  return m_cb->Window_AddStringItem(m_Handle->addonData, m_WindowHandle, name, itemPosition);
}

void CAddonGUIWindow::AddItem(GUIHANDLE item, int itemPosition)
{
  m_cb->Window_AddItem(m_Handle->addonData, m_WindowHandle, item, itemPosition);
}

void CAddonGUIWindow::AddItem(CAddonListItem *item, int itemPosition)
{
  m_cb->Window_AddItem(m_Handle->addonData, m_WindowHandle, item->m_ListItemHandle, itemPosition);
}

void CAddonGUIWindow::RemoveItem(int itemPosition)
{
  m_cb->Window_RemoveItem(m_Handle->addonData, m_WindowHandle, itemPosition);
}

GUIHANDLE CAddonGUIWindow::GetListItem(int listPos)
{
  return m_cb->Window_GetListItem(m_Handle->addonData, m_WindowHandle, listPos);
}

void CAddonGUIWindow::SetCurrentListPosition(int listPos)
{
  m_cb->Window_SetCurrentListPosition(m_Handle->addonData, m_WindowHandle, listPos);
}

int CAddonGUIWindow::GetCurrentListPosition()
{
  return m_cb->Window_GetCurrentListPosition(m_Handle->addonData, m_WindowHandle);
}

void CAddonGUIWindow::SetControlLabel(int controlId, const char *label)
{
  m_cb->Window_SetControlLabel(m_Handle->addonData, m_WindowHandle, controlId, label);
}

///-------------------------------------
/// cGUISpinControl

DLLEXPORT CAddonGUISpinControl* GUI_control_get_spin(CAddonGUIWindow *window, int controlId)
{
  return new CAddonGUISpinControl(window, controlId);
}

DLLEXPORT void GUI_control_release_spin(CAddonGUISpinControl* p)
{
  delete p;
}

CAddonGUISpinControl::CAddonGUISpinControl(CAddonGUIWindow *window, int controlId)
 : m_Window(window)
 , m_ControlId(controlId)
{
  m_SpinHandle = m_cb->Window_GetControl_Spin(m_Handle->addonData, m_Window->m_WindowHandle, controlId);
}

void CAddonGUISpinControl::SetVisible(bool yesNo)
{
  if (m_SpinHandle)
    m_cb->Control_Spin_SetVisible(m_Handle->addonData, m_SpinHandle, yesNo);
}

void CAddonGUISpinControl::SetText(const char *label)
{
  if (m_SpinHandle)
    m_cb->Control_Spin_SetText(m_Handle->addonData, m_SpinHandle, label);
}

void CAddonGUISpinControl::Clear()
{
  if (m_SpinHandle)
    m_cb->Control_Spin_Clear(m_Handle->addonData, m_SpinHandle);
}

void CAddonGUISpinControl::AddLabel(const char *label, int iValue)
{
  if (m_SpinHandle)
    m_cb->Control_Spin_AddLabel(m_Handle->addonData, m_SpinHandle, label, iValue);
}

int CAddonGUISpinControl::GetValue()
{
  if (!m_SpinHandle)
    return -1;

  return m_cb->Control_Spin_GetValue(m_Handle->addonData, m_SpinHandle);
}

void CAddonGUISpinControl::SetValue(int iValue)
{
  if (m_SpinHandle)
    m_cb->Control_Spin_SetValue(m_Handle->addonData, m_SpinHandle, iValue);
}

///-------------------------------------
/// cGUIRadioButton

DLLEXPORT CAddonGUIRadioButton* GUI_control_get_radiobutton(CAddonGUIWindow *window, int controlId)
{
  return new CAddonGUIRadioButton(window, controlId);
}

DLLEXPORT void GUI_control_release_radiobutton(CAddonGUIRadioButton* p)
{
  delete p;
}

CAddonGUIRadioButton::CAddonGUIRadioButton(CAddonGUIWindow *window, int controlId)
 : m_Window(window)
 , m_ControlId(controlId)
{
  m_ButtonHandle = m_cb->Window_GetControl_RadioButton(m_Handle->addonData, m_Window->m_WindowHandle, controlId);
}

void CAddonGUIRadioButton::SetVisible(bool yesNo)
{
  if (m_ButtonHandle)
    m_cb->Control_RadioButton_SetVisible(m_Handle->addonData, m_ButtonHandle, yesNo);
}

void CAddonGUIRadioButton::SetText(const char *label)
{
  if (m_ButtonHandle)
    m_cb->Control_RadioButton_SetText(m_Handle->addonData, m_ButtonHandle, label);
}

void CAddonGUIRadioButton::SetSelected(bool yesNo)
{
  if (m_ButtonHandle)
    m_cb->Control_RadioButton_SetSelected(m_Handle->addonData, m_ButtonHandle, yesNo);
}

bool CAddonGUIRadioButton::IsSelected()
{
  if (!m_ButtonHandle)
    return false;

  return m_cb->Control_RadioButton_IsSelected(m_Handle->addonData, m_ButtonHandle);
}


///-------------------------------------
/// cGUIProgressControl

DLLEXPORT CAddonGUIProgressControl* GUI_control_get_progress(CAddonGUIWindow *window, int controlId)
{
  return new CAddonGUIProgressControl(window, controlId);
}

DLLEXPORT void GUI_control_release_progress(CAddonGUIProgressControl* p)
{
  delete p;
}

CAddonGUIProgressControl::CAddonGUIProgressControl(CAddonGUIWindow *window, int controlId)
 : m_Window(window)
 , m_ControlId(controlId)
{
  m_ProgressHandle = m_cb->Window_GetControl_Progress(m_Handle->addonData, m_Window->m_WindowHandle, controlId);
}

void CAddonGUIProgressControl::SetPercentage(float fPercent)
{
  if (m_ProgressHandle)
    m_cb->Control_Progress_SetPercentage(m_Handle->addonData, m_ProgressHandle, fPercent);
}

float CAddonGUIProgressControl::GetPercentage() const
{
  if (!m_ProgressHandle)
    return 0.0;

  return m_cb->Control_Progress_GetPercentage(m_Handle->addonData, m_ProgressHandle);
}

void CAddonGUIProgressControl::SetInfo(int iInfo)
{
  if (m_ProgressHandle)
    m_cb->Control_Progress_SetInfo(m_Handle->addonData, m_ProgressHandle, iInfo);
}

int CAddonGUIProgressControl::GetInfo() const
{
  if (!m_ProgressHandle)
    return -1;

  return m_cb->Control_Progress_GetInfo(m_Handle->addonData, m_ProgressHandle);
}

string CAddonGUIProgressControl::GetDescription() const
{
  if (!m_ProgressHandle)
    return "";

  return m_cb->Control_Progress_GetDescription(m_Handle->addonData, m_ProgressHandle);
}


///-------------------------------------
/// cListItem

DLLEXPORT CAddonListItem* GUI_ListItem_create(const char *label, const char *label2, const char *iconImage, const char *thumbnailImage, const char *path)
{
  return new CAddonListItem(label, label2, iconImage, thumbnailImage, path);
}

DLLEXPORT void GUI_ListItem_destroy(CAddonListItem* p)
{
  delete p;
}


CAddonListItem::CAddonListItem(const char *label, const char *label2, const char *iconImage, const char *thumbnailImage, const char *path)
{
  m_ListItemHandle = m_cb->ListItem_Create(m_Handle->addonData, label, label2, iconImage, thumbnailImage, path);
}

const char *CAddonListItem::GetLabel()
{
  if (!m_ListItemHandle)
    return "";

  return m_cb->ListItem_GetLabel(m_Handle->addonData, m_ListItemHandle);
}

void CAddonListItem::SetLabel(const char *label)
{
  if (m_ListItemHandle)
    m_cb->ListItem_SetLabel(m_Handle->addonData, m_ListItemHandle, label);
}

const char *CAddonListItem::GetLabel2()
{
  if (!m_ListItemHandle)
    return "";

  return m_cb->ListItem_GetLabel2(m_Handle->addonData, m_ListItemHandle);
}

void CAddonListItem::SetLabel2(const char *label)
{
  if (m_ListItemHandle)
    m_cb->ListItem_SetLabel2(m_Handle->addonData, m_ListItemHandle, label);
}

void CAddonListItem::SetIconImage(const char *image)
{
  if (m_ListItemHandle)
    m_cb->ListItem_SetIconImage(m_Handle->addonData, m_ListItemHandle, image);
}

void CAddonListItem::SetThumbnailImage(const char *image)
{
  if (m_ListItemHandle)
    m_cb->ListItem_SetThumbnailImage(m_Handle->addonData, m_ListItemHandle, image);
}

void CAddonListItem::SetInfo(const char *Info)
{
  if (m_ListItemHandle)
    m_cb->ListItem_SetInfo(m_Handle->addonData, m_ListItemHandle, Info);
}

void CAddonListItem::SetProperty(const char *key, const char *value)
{
  if (m_ListItemHandle)
    m_cb->ListItem_SetProperty(m_Handle->addonData, m_ListItemHandle, key, value);
}

const char *CAddonListItem::GetProperty(const char *key) const
{
  if (!m_ListItemHandle)
    return "";

  return m_cb->ListItem_GetProperty(m_Handle->addonData, m_ListItemHandle, key);
}

void CAddonListItem::SetPath(const char *Path)
{
  if (m_ListItemHandle)
    m_cb->ListItem_SetPath(m_Handle->addonData, m_ListItemHandle, Path);
}


};
