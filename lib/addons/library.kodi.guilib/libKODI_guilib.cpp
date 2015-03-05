/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "../../../addons/library.kodi.guilib/libKODI_guilib.h"

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include "addons/AddonCallbacks.h"

#ifdef _WIN32
#include <windows.h>
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif

using namespace std;

extern "C"
{

DLLEXPORT void* GUI_register_me(void *hdl)
{
  CB_GUILib *cb = NULL;
  if (!hdl)
    fprintf(stderr, "libXBMC_gui-ERROR: GUILib_register_me is called with NULL handle !!!\n");
  else
  {
    cb = ((AddonCB*)hdl)->GUILib_RegisterMe(((AddonCB*)hdl)->addonData);
    if (!cb)
      fprintf(stderr, "libXBMC_gui-ERROR: GUILib_register_me can't get callback table from XBMC !!!\n");
  }
  return cb;
}

DLLEXPORT void GUI_unregister_me(void *hdl, void *cb)
{
  if (hdl && cb)
    ((AddonCB*)hdl)->GUILib_UnRegisterMe(((AddonCB*)hdl)->addonData, (CB_GUILib*)cb);
}

DLLEXPORT void GUI_lock(void *hdl, void *cb)
{
  ((CB_GUILib*)cb)->Lock();
}

DLLEXPORT void GUI_unlock(void *hdl, void *cb)
{
  ((CB_GUILib*)cb)->Unlock();
}

DLLEXPORT int GUI_get_screen_height(void *hdl, void *cb)
{
  return ((CB_GUILib*)cb)->GetScreenHeight();
}

DLLEXPORT int GUI_get_screen_width(void *hdl, void *cb)
{
  return ((CB_GUILib*)cb)->GetScreenWidth();
}

DLLEXPORT int GUI_get_video_resolution(void *hdl, void *cb)
{
  return ((CB_GUILib*)cb)->GetVideoResolution();
}

/*! @name GUI Keyboard functions */
//@{
DLLEXPORT bool GUI_dialog_keyboard_show_and_get_input_with_head(void *hdl, void *cb, char &aTextString, unsigned int iMaxStringSize, const char *strHeading, bool allowEmptyResult, bool hiddenInput, unsigned int autoCloseMs)
{
  return ((CB_GUILib*)cb)->Dialog_Keyboard_ShowAndGetInputWithHead(aTextString, iMaxStringSize, strHeading, allowEmptyResult, hiddenInput, autoCloseMs);
}

DLLEXPORT bool GUI_dialog_keyboard_show_and_get_input(void *hdl, void *cb, char &aTextString, unsigned int iMaxStringSize, bool allowEmptyResult, unsigned int autoCloseMs)
{
  return ((CB_GUILib*)cb)->Dialog_Keyboard_ShowAndGetInput(aTextString, iMaxStringSize, allowEmptyResult, autoCloseMs);
}

DLLEXPORT bool GUI_dialog_keyboard_show_and_get_new_password_with_head(void *hdl, void *cb, char &newPassword, unsigned int iMaxStringSize, const char *strHeading, bool allowEmptyResult, unsigned int autoCloseMs)
{
  return ((CB_GUILib*)cb)->Dialog_Keyboard_ShowAndGetNewPasswordWithHead(newPassword, iMaxStringSize, strHeading, allowEmptyResult, autoCloseMs);
}

DLLEXPORT bool GUI_dialog_keyboard_show_and_get_new_password(void *hdl, void *cb, char &strNewPassword, unsigned int iMaxStringSize, unsigned int autoCloseMs)
{
  return ((CB_GUILib*)cb)->Dialog_Keyboard_ShowAndGetNewPassword(strNewPassword, iMaxStringSize, autoCloseMs);
}

DLLEXPORT bool GUI_dialog_keyboard_show_and_verify_new_password_with_head(void *hdl, void *cb, char &strNewPassword, unsigned int iMaxStringSize, const char *strHeading, bool allowEmptyResult, unsigned int autoCloseMs)
{
  return ((CB_GUILib*)cb)->Dialog_Keyboard_ShowAndVerifyNewPasswordWithHead(strNewPassword, iMaxStringSize, strHeading, allowEmptyResult, autoCloseMs);
}

DLLEXPORT bool GUI_dialog_keyboard_show_and_verify_new_password(void *hdl, void *cb, char &strNewPassword, unsigned int iMaxStringSize, unsigned int autoCloseMs)
{
  return ((CB_GUILib*)cb)->Dialog_Keyboard_ShowAndVerifyNewPassword(strNewPassword, iMaxStringSize, autoCloseMs);
}

DLLEXPORT int GUI_dialog_keyboard_show_and_verify_password(void *hdl, void *cb, char &strPassword, unsigned int iMaxStringSize, const char *strHeading, int iRetries, unsigned int autoCloseMs)
{
  return ((CB_GUILib*)cb)->Dialog_Keyboard_ShowAndVerifyPassword(strPassword, iMaxStringSize, strHeading, iRetries, autoCloseMs);
}

DLLEXPORT bool GUI_dialog_keyboard_show_and_get_filter(void *hdl, void *cb, char &aTextString, unsigned int iMaxStringSize, bool searching, unsigned int autoCloseMs)
{
  return ((CB_GUILib*)cb)->Dialog_Keyboard_ShowAndGetFilter(aTextString, iMaxStringSize, searching, autoCloseMs);
}

DLLEXPORT bool GUI_dialog_keyboard_send_text_to_active_keyboard(void *hdl, void *cb, const char *aTextString, bool closeKeyboard)
{
  return ((CB_GUILib*)cb)->Dialog_Keyboard_SendTextToActiveKeyboard(aTextString, closeKeyboard);
}

DLLEXPORT bool GUI_dialog_keyboard_is_activated(void *hdl, void *cb)
{
  return ((CB_GUILib*)cb)->Dialog_Keyboard_isKeyboardActivated();
}
//@}

/*! @name GUI Numeric functions */
//@{
DLLEXPORT bool GUI_dialog_numeric_show_and_verify_new_password(void *hdl, void *cb, char &strNewPassword, unsigned int iMaxStringSize)
{
  return ((CB_GUILib*)cb)->Dialog_Numeric_ShowAndVerifyNewPassword(strNewPassword, iMaxStringSize);
}

DLLEXPORT int GUI_dialog_numeric_show_and_verify_password(void *hdl, void *cb, char &strPassword, unsigned int iMaxStringSize, const char *strHeading, int iRetries)
{
  return ((CB_GUILib*)cb)->Dialog_Numeric_ShowAndVerifyPassword(strPassword, iMaxStringSize, strHeading, iRetries);
}

DLLEXPORT bool GUI_dialog_numeric_show_and_verify_input(void *hdl, void *cb, char &strPassword, unsigned int iMaxStringSize, const char *strHeading, bool bGetUserInput)
{
  return ((CB_GUILib*)cb)->Dialog_Numeric_ShowAndVerifyInput(strPassword, iMaxStringSize, strHeading, bGetUserInput);
}

DLLEXPORT bool GUI_dialog_numeric_show_and_get_time(void *hdl, void *cb, tm &time, const char *strHeading)
{
  return ((CB_GUILib*)cb)->Dialog_Numeric_ShowAndGetTime(time, strHeading);
}

DLLEXPORT bool GUI_dialog_numeric_show_and_get_date(void *hdl, void *cb, tm &date, const char *strHeading)
{
  return ((CB_GUILib*)cb)->Dialog_Numeric_ShowAndGetDate(date, strHeading);
}

DLLEXPORT bool GUI_dialog_numeric_show_and_get_ipaddress(void *hdl, void *cb, char &IPAddress, unsigned int iMaxStringSize, const char *strHeading)
{
  return ((CB_GUILib*)cb)->Dialog_Numeric_ShowAndGetIPAddress(IPAddress, iMaxStringSize, strHeading);
}

DLLEXPORT bool GUI_dialog_numeric_show_and_get_number(void *hdl, void *cb, char &strInput, unsigned int iMaxStringSize, const char *strHeading, unsigned int iAutoCloseTimeoutMs)
{
  return ((CB_GUILib*)cb)->Dialog_Numeric_ShowAndGetNumber(strInput, iMaxStringSize, strHeading, iAutoCloseTimeoutMs);
}

DLLEXPORT bool GUI_dialog_numeric_show_and_get_seconds(void *hdl, void *cb, char &timeString, unsigned int iMaxStringSize, const char *strHeading)
{
  return ((CB_GUILib*)cb)->Dialog_Numeric_ShowAndGetSeconds(timeString, iMaxStringSize, strHeading);
}
//@}

/*! @name GUI File browser functions */
//@{
DLLEXPORT bool GUI_dialog_filebrowser_show_and_get_file(void *hdl, void *cb, const char *directory, const char *mask, const char *heading, char &path, unsigned int iMaxStringSize, bool useThumbs = false, bool useFileDirectories = false, bool singleList = false)
{
  return ((CB_GUILib*)cb)->Dialog_FileBrowser_ShowAndGetFile(directory, mask, heading, path, iMaxStringSize, useThumbs, useFileDirectories, singleList);
}
//@}

/*! @name GUI OK Dialog functions */
//@{
DLLEXPORT void GUI_dialog_ok_show_and_get_input_single_text(void *hdl, void *cb, const char *heading, const char *text)
{
  return ((CB_GUILib*)cb)->Dialog_OK_ShowAndGetInputSingleText(heading, text);
}

DLLEXPORT void GUI_dialog_ok_show_and_get_input_line_text(void *hdl, void *cb, const char *heading, const char *line0, const char *line1, const char *line2)
{
  return ((CB_GUILib*)cb)->Dialog_OK_ShowAndGetInputLineText(heading, line0, line1, line2);
}
//@}

/*! @name GUI OK Dialog functions */
//@{
DLLEXPORT bool GUI_dialog_yesno_show_and_get_input_linetext(void *hdl, void *cb, const char *heading, const char *line0, const char *line1, const char *line2, const char *noLabel, const char *yesLabel)
{
  return ((CB_GUILib*)cb)->Dialog_YesNo_ShowAndGetInputLineText(heading, line0, line1, line2, noLabel, yesLabel);
}

DLLEXPORT bool GUI_dialog_yesno_show_and_get_input_singletext(void *hdl, void *cb, const char *heading, const char *text, bool& bCanceled, const char *noLabel, const char *yesLabel)
{
  return ((CB_GUILib*)cb)->Dialog_YesNo_ShowAndGetInputSingleText(heading, text, bCanceled, noLabel, yesLabel);
}

DLLEXPORT bool GUI_dialog_yesno_show_and_get_input_linebuttontext(void *hdl, void *cb, const char *heading, const char *line0, const char *line1, const char *line2, bool &bCanceled, const char *noLabel, const char *yesLabel)
{
  return ((CB_GUILib*)cb)->Dialog_YesNo_ShowAndGetInputLineButtonText(heading, line0, line1, line2, bCanceled, noLabel, yesLabel);
}
//@}

/*! @name GUI Text viewer Dialog */
//@{
DLLEXPORT void GUI_dialog_text_viewer(void *hdl, void *cb, const char *heading, const char *text)
{
  return ((CB_GUILib*)cb)->Dialog_TextViewer(heading, text);
}
//@}

/*! @name GUI select Dialog */
//@{
DLLEXPORT int GUI_dialog_select(void *hdl, void *cb, const char *heading, const char *entries[], unsigned int size, int selected)
{
  return ((CB_GUILib*)cb)->Dialog_Select(heading, entries, size, selected);
}
//@}


DLLEXPORT CAddonGUIWindow* GUI_Window_create(void *hdl, void *cb, const char *xmlFilename, const char *defaultSkin, bool forceFallback, bool asDialog)
{
  return new CAddonGUIWindow(hdl, cb, xmlFilename, defaultSkin, forceFallback, asDialog);
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

CAddonGUIWindow::CAddonGUIWindow(void *hdl, void *cb, const char *xmlFilename, const char *defaultSkin, bool forceFallback, bool asDialog)
 : m_Handle(hdl)
 , m_cb(cb)
{
  CBOnInit = NULL;
  CBOnClick = NULL;
  CBOnFocus = NULL;
  if (hdl && cb)
  {
    m_WindowHandle = ((CB_GUILib*)m_cb)->Window_New(((AddonCB*)m_Handle)->addonData, xmlFilename, defaultSkin, forceFallback, asDialog);
    if (!m_WindowHandle)
      fprintf(stderr, "libXBMC_gui-ERROR: cGUIWindow can't create window class from XBMC !!!\n");

    ((CB_GUILib*)m_cb)->Window_SetCallbacks(((AddonCB*)m_Handle)->addonData, m_WindowHandle, this, GUI_Window_OnInit, GUI_Window_OnClick, GUI_Window_OnFocus, GUI_Window_OnAction);
  }
}

CAddonGUIWindow::~CAddonGUIWindow()
{
  if (m_Handle && m_cb && m_WindowHandle)
  {
    ((CB_GUILib*)m_cb)->Window_Delete(((AddonCB*)m_Handle)->addonData, m_WindowHandle);
    m_WindowHandle = NULL;
  }
}

bool CAddonGUIWindow::Show()
{
  return ((CB_GUILib*)m_cb)->Window_Show(((AddonCB*)m_Handle)->addonData, m_WindowHandle);
}

void CAddonGUIWindow::Close()
{
  ((CB_GUILib*)m_cb)->Window_Close(((AddonCB*)m_Handle)->addonData, m_WindowHandle);
}

void CAddonGUIWindow::DoModal()
{
  ((CB_GUILib*)m_cb)->Window_DoModal(((AddonCB*)m_Handle)->addonData, m_WindowHandle);
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
  return ((CB_GUILib*)m_cb)->Window_SetFocusId(((AddonCB*)m_Handle)->addonData, m_WindowHandle, iControlId);
}

int CAddonGUIWindow::GetFocusId()
{
  return ((CB_GUILib*)m_cb)->Window_GetFocusId(((AddonCB*)m_Handle)->addonData, m_WindowHandle);
}

bool CAddonGUIWindow::SetCoordinateResolution(int res)
{
  return ((CB_GUILib*)m_cb)->Window_SetCoordinateResolution(((AddonCB*)m_Handle)->addonData, m_WindowHandle, res);
}

void CAddonGUIWindow::SetProperty(const char *key, const char *value)
{
  ((CB_GUILib*)m_cb)->Window_SetProperty(((AddonCB*)m_Handle)->addonData, m_WindowHandle, key, value);
}

void CAddonGUIWindow::SetPropertyInt(const char *key, int value)
{
  ((CB_GUILib*)m_cb)->Window_SetPropertyInt(((AddonCB*)m_Handle)->addonData, m_WindowHandle, key, value);
}

void CAddonGUIWindow::SetPropertyBool(const char *key, bool value)
{
  ((CB_GUILib*)m_cb)->Window_SetPropertyBool(((AddonCB*)m_Handle)->addonData, m_WindowHandle, key, value);
}

void CAddonGUIWindow::SetPropertyDouble(const char *key, double value)
{
  ((CB_GUILib*)m_cb)->Window_SetPropertyDouble(((AddonCB*)m_Handle)->addonData, m_WindowHandle, key, value);
}

const char *CAddonGUIWindow::GetProperty(const char *key) const
{
  return ((CB_GUILib*)m_cb)->Window_GetProperty(((AddonCB*)m_Handle)->addonData, m_WindowHandle, key);
}

int CAddonGUIWindow::GetPropertyInt(const char *key) const
{
  return ((CB_GUILib*)m_cb)->Window_GetPropertyInt(((AddonCB*)m_Handle)->addonData, m_WindowHandle, key);
}

bool CAddonGUIWindow::GetPropertyBool(const char *key) const
{
  return ((CB_GUILib*)m_cb)->Window_GetPropertyBool(((AddonCB*)m_Handle)->addonData, m_WindowHandle, key);
}

double CAddonGUIWindow::GetPropertyDouble(const char *key) const
{
  return ((CB_GUILib*)m_cb)->Window_GetPropertyDouble(((AddonCB*)m_Handle)->addonData, m_WindowHandle, key);
}

void CAddonGUIWindow::ClearProperties()
{
  ((CB_GUILib*)m_cb)->Window_ClearProperties(((AddonCB*)m_Handle)->addonData, m_WindowHandle);
}

int CAddonGUIWindow::GetListSize()
{
  return ((CB_GUILib*)m_cb)->Window_GetListSize(((AddonCB*)m_Handle)->addonData, m_WindowHandle);
}

void CAddonGUIWindow::ClearList()
{
  ((CB_GUILib*)m_cb)->Window_ClearList(((AddonCB*)m_Handle)->addonData, m_WindowHandle);
}

GUIHANDLE CAddonGUIWindow::AddStringItem(const char *name, int itemPosition)
{
  return ((CB_GUILib*)m_cb)->Window_AddStringItem(((AddonCB*)m_Handle)->addonData, m_WindowHandle, name, itemPosition);
}

void CAddonGUIWindow::AddItem(GUIHANDLE item, int itemPosition)
{
  ((CB_GUILib*)m_cb)->Window_AddItem(((AddonCB*)m_Handle)->addonData, m_WindowHandle, item, itemPosition);
}

void CAddonGUIWindow::AddItem(CAddonListItem *item, int itemPosition)
{
  ((CB_GUILib*)m_cb)->Window_AddItem(((AddonCB*)m_Handle)->addonData, m_WindowHandle, item->m_ListItemHandle, itemPosition);
}

void CAddonGUIWindow::RemoveItem(int itemPosition)
{
  ((CB_GUILib*)m_cb)->Window_RemoveItem(((AddonCB*)m_Handle)->addonData, m_WindowHandle, itemPosition);
}

GUIHANDLE CAddonGUIWindow::GetListItem(int listPos)
{
  return ((CB_GUILib*)m_cb)->Window_GetListItem(((AddonCB*)m_Handle)->addonData, m_WindowHandle, listPos);
}

void CAddonGUIWindow::SetCurrentListPosition(int listPos)
{
  ((CB_GUILib*)m_cb)->Window_SetCurrentListPosition(((AddonCB*)m_Handle)->addonData, m_WindowHandle, listPos);
}

int CAddonGUIWindow::GetCurrentListPosition()
{
  return ((CB_GUILib*)m_cb)->Window_GetCurrentListPosition(((AddonCB*)m_Handle)->addonData, m_WindowHandle);
}

void CAddonGUIWindow::SetControlLabel(int controlId, const char *label)
{
  ((CB_GUILib*)m_cb)->Window_SetControlLabel(((AddonCB*)m_Handle)->addonData, m_WindowHandle, controlId, label);
}

void CAddonGUIWindow::MarkDirtyRegion()
{
  ((CB_GUILib*)m_cb)->Window_MarkDirtyRegion(((AddonCB*)m_Handle)->addonData, m_WindowHandle);
}

///-------------------------------------
/// cGUISpinControl

DLLEXPORT CAddonGUISpinControl* GUI_control_get_spin(void *hdl, void *cb, CAddonGUIWindow *window, int controlId)
{
  return new CAddonGUISpinControl(hdl, cb, window, controlId);
}

DLLEXPORT void GUI_control_release_spin(CAddonGUISpinControl* p)
{
  delete p;
}

CAddonGUISpinControl::CAddonGUISpinControl(void *hdl, void *cb, CAddonGUIWindow *window, int controlId)
 : m_Window(window)
 , m_ControlId(controlId)
{
  m_Handle = hdl;
  m_cb = cb;
  m_SpinHandle = ((CB_GUILib*)m_cb)->Window_GetControl_Spin(((AddonCB*)m_Handle)->addonData, m_Window->m_WindowHandle, controlId);
}

void CAddonGUISpinControl::SetVisible(bool yesNo)
{
  if (m_SpinHandle)
    ((CB_GUILib*)m_cb)->Control_Spin_SetVisible(((AddonCB*)m_Handle)->addonData, m_SpinHandle, yesNo);
}

void CAddonGUISpinControl::SetText(const char *label)
{
  if (m_SpinHandle)
    ((CB_GUILib*)m_cb)->Control_Spin_SetText(((AddonCB*)m_Handle)->addonData, m_SpinHandle, label);
}

void CAddonGUISpinControl::Clear()
{
  if (m_SpinHandle)
    ((CB_GUILib*)m_cb)->Control_Spin_Clear(((AddonCB*)m_Handle)->addonData, m_SpinHandle);
}

void CAddonGUISpinControl::AddLabel(const char *label, int iValue)
{
  if (m_SpinHandle)
    ((CB_GUILib*)m_cb)->Control_Spin_AddLabel(((AddonCB*)m_Handle)->addonData, m_SpinHandle, label, iValue);
}

int CAddonGUISpinControl::GetValue()
{
  if (!m_SpinHandle)
    return -1;

  return ((CB_GUILib*)m_cb)->Control_Spin_GetValue(((AddonCB*)m_Handle)->addonData, m_SpinHandle);
}

void CAddonGUISpinControl::SetValue(int iValue)
{
  if (m_SpinHandle)
    ((CB_GUILib*)m_cb)->Control_Spin_SetValue(((AddonCB*)m_Handle)->addonData, m_SpinHandle, iValue);
}

///--m_cb-----------------------------------
/// cGUIRadioButton

DLLEXPORT CAddonGUIRadioButton* GUI_control_get_radiobutton(void *hdl, void *cb, CAddonGUIWindow *window, int controlId)
{
  return new CAddonGUIRadioButton(hdl, cb, window, controlId);
}

DLLEXPORT void GUI_control_release_radiobutton(CAddonGUIRadioButton* p)
{
  delete p;
}

CAddonGUIRadioButton::CAddonGUIRadioButton(void *hdl, void *cb, CAddonGUIWindow *window, int controlId)
 : m_Window(window)
 , m_ControlId(controlId)
 , m_Handle(hdl)
 , m_cb(cb)
{
  m_ButtonHandle = ((CB_GUILib*)m_cb)->Window_GetControl_RadioButton(((AddonCB*)m_Handle)->addonData, m_Window->m_WindowHandle, controlId);
}

void CAddonGUIRadioButton::SetVisible(bool yesNo)
{
  if (m_ButtonHandle)
    ((CB_GUILib*)m_cb)->Control_RadioButton_SetVisible(((AddonCB*)m_Handle)->addonData, m_ButtonHandle, yesNo);
}

void CAddonGUIRadioButton::SetText(const char *label)
{
  if (m_ButtonHandle)
    ((CB_GUILib*)m_cb)->Control_RadioButton_SetText(((AddonCB*)m_Handle)->addonData, m_ButtonHandle, label);
}

void CAddonGUIRadioButton::SetSelected(bool yesNo)
{
  if (m_ButtonHandle)
    ((CB_GUILib*)m_cb)->Control_RadioButton_SetSelected(((AddonCB*)m_Handle)->addonData, m_ButtonHandle, yesNo);
}

bool CAddonGUIRadioButton::IsSelected()
{
  if (!m_ButtonHandle)
    return false;

  return ((CB_GUILib*)m_cb)->Control_RadioButton_IsSelected(((AddonCB*)m_Handle)->addonData, m_ButtonHandle);
}


///-------------------------------------
/// cGUIProgressControl

DLLEXPORT CAddonGUIProgressControl* GUI_control_get_progress(void *hdl, void *cb, CAddonGUIWindow *window, int controlId)
{
  return new CAddonGUIProgressControl(hdl, cb, window, controlId);
}

DLLEXPORT void GUI_control_release_progress(CAddonGUIProgressControl* p)
{
  delete p;
}

CAddonGUIProgressControl::CAddonGUIProgressControl(void *hdl, void *cb, CAddonGUIWindow *window, int controlId)
 : m_Window(window)
 , m_ControlId(controlId)
 , m_Handle(hdl)
 , m_cb(cb)
{
  m_ProgressHandle = ((CB_GUILib*)m_cb)->Window_GetControl_Progress(((AddonCB*)m_Handle)->addonData, m_Window->m_WindowHandle, controlId);
}

void CAddonGUIProgressControl::SetPercentage(float fPercent)
{
  if (m_ProgressHandle)
    ((CB_GUILib*)m_cb)->Control_Progress_SetPercentage(((AddonCB*)m_Handle)->addonData, m_ProgressHandle, fPercent);
}

float CAddonGUIProgressControl::GetPercentage() const
{
  if (!m_ProgressHandle)
    return 0.0f;

  return ((CB_GUILib*)m_cb)->Control_Progress_GetPercentage(((AddonCB*)m_Handle)->addonData, m_ProgressHandle);
}

void CAddonGUIProgressControl::SetInfo(int iInfo)
{
  if (m_ProgressHandle)
    ((CB_GUILib*)m_cb)->Control_Progress_SetInfo(((AddonCB*)m_Handle)->addonData, m_ProgressHandle, iInfo);
}

int CAddonGUIProgressControl::GetInfo() const
{
  if (!m_ProgressHandle)
    return -1;

  return ((CB_GUILib*)m_cb)->Control_Progress_GetInfo(((AddonCB*)m_Handle)->addonData, m_ProgressHandle);
}

string CAddonGUIProgressControl::GetDescription() const
{
  if (!m_ProgressHandle)
    return "";

  return ((CB_GUILib*)m_cb)->Control_Progress_GetDescription(((AddonCB*)m_Handle)->addonData, m_ProgressHandle);
}


///-------------------------------------
/// cGUISliderControl

DLLEXPORT CAddonGUISliderControl* GUI_control_get_slider(void *hdl, void *cb, CAddonGUIWindow *window, int controlId)
{
  return new CAddonGUISliderControl(hdl, cb, window, controlId);
}

DLLEXPORT void GUI_control_release_slider(CAddonGUISliderControl* p)
{
  delete p;
}

CAddonGUISliderControl::CAddonGUISliderControl(void *hdl, void *cb, CAddonGUIWindow *window, int controlId)
 : m_Window(window)
 , m_ControlId(controlId)
 , m_Handle(hdl)
 , m_cb(cb)
{
  m_SliderHandle = ((CB_GUILib*)m_cb)->Window_GetControl_Slider(((AddonCB*)m_Handle)->addonData, m_Window->m_WindowHandle, controlId);
}

void CAddonGUISliderControl::SetVisible(bool yesNo)
{
  if (m_SliderHandle)
    ((CB_GUILib*)m_cb)->Control_Slider_SetVisible(((AddonCB*)m_Handle)->addonData, m_SliderHandle, yesNo);
}

string CAddonGUISliderControl::GetDescription() const
{
  if (!m_SliderHandle)
    return "";

  return ((CB_GUILib*)m_cb)->Control_Slider_GetDescription(((AddonCB*)m_Handle)->addonData, m_SliderHandle);
}

void CAddonGUISliderControl::SetIntRange(int iStart, int iEnd)
{
  if (m_SliderHandle)
    ((CB_GUILib*)m_cb)->Control_Slider_SetIntRange(((AddonCB*)m_Handle)->addonData, m_SliderHandle, iStart, iEnd);
}

void CAddonGUISliderControl::SetIntValue(int iValue)
{
  if (m_SliderHandle)
    ((CB_GUILib*)m_cb)->Control_Slider_SetIntValue(((AddonCB*)m_Handle)->addonData, m_SliderHandle, iValue);
}

int CAddonGUISliderControl::GetIntValue() const
{
  if (!m_SliderHandle)
    return 0;
  return ((CB_GUILib*)m_cb)->Control_Slider_GetIntValue(((AddonCB*)m_Handle)->addonData, m_SliderHandle);
}

void CAddonGUISliderControl::SetIntInterval(int iInterval)
{
  if (m_SliderHandle)
    ((CB_GUILib*)m_cb)->Control_Slider_SetIntInterval(((AddonCB*)m_Handle)->addonData, m_SliderHandle, iInterval);
}

void CAddonGUISliderControl::SetPercentage(float fPercent)
{
  if (m_SliderHandle)
    ((CB_GUILib*)m_cb)->Control_Slider_SetPercentage(((AddonCB*)m_Handle)->addonData, m_SliderHandle, fPercent);
}

float CAddonGUISliderControl::GetPercentage() const
{
  if (!m_SliderHandle)
    return 0.0f;

  return ((CB_GUILib*)m_cb)->Control_Slider_GetPercentage(((AddonCB*)m_Handle)->addonData, m_SliderHandle);
}

void CAddonGUISliderControl::SetFloatRange(float fStart, float fEnd)
{
  if (m_SliderHandle)
    ((CB_GUILib*)m_cb)->Control_Slider_SetFloatRange(((AddonCB*)m_Handle)->addonData, m_SliderHandle, fStart, fEnd);
}

void CAddonGUISliderControl::SetFloatValue(float fValue)
{
  if (m_SliderHandle)
    ((CB_GUILib*)m_cb)->Control_Slider_SetFloatValue(((AddonCB*)m_Handle)->addonData, m_SliderHandle, fValue);
}

float CAddonGUISliderControl::GetFloatValue() const
{
  if (!m_SliderHandle)
    return 0.0f;
  return ((CB_GUILib*)m_cb)->Control_Slider_GetFloatValue(((AddonCB*)m_Handle)->addonData, m_SliderHandle);
}

void CAddonGUISliderControl::SetFloatInterval(float fInterval)
{
  if (m_SliderHandle)
    ((CB_GUILib*)m_cb)->Control_Slider_SetFloatInterval(((AddonCB*)m_Handle)->addonData, m_SliderHandle, fInterval);
}


///-------------------------------------
/// cGUISettingsSliderControl

DLLEXPORT CAddonGUISettingsSliderControl* GUI_control_get_settings_slider(void *hdl, void *cb, CAddonGUIWindow *window, int controlId)
{
  return new CAddonGUISettingsSliderControl(hdl, cb, window, controlId);
}

DLLEXPORT void GUI_control_release_settings_slider(CAddonGUISettingsSliderControl* p)
{
  delete p;
}

CAddonGUISettingsSliderControl::CAddonGUISettingsSliderControl(void *hdl, void *cb, CAddonGUIWindow *window, int controlId)
 : m_Window(window)
 , m_ControlId(controlId)
 , m_Handle(hdl)
 , m_cb(cb)
{
  m_SettingsSliderHandle = ((CB_GUILib*)m_cb)->Window_GetControl_SettingsSlider(((AddonCB*)m_Handle)->addonData, m_Window->m_WindowHandle, controlId);
}

void CAddonGUISettingsSliderControl::SetVisible(bool yesNo)
{
  if (m_SettingsSliderHandle)
    ((CB_GUILib*)m_cb)->Control_SettingsSlider_SetVisible(((AddonCB*)m_Handle)->addonData, m_SettingsSliderHandle, yesNo);
}

void CAddonGUISettingsSliderControl::SetText(const char *label)
{
  if (m_SettingsSliderHandle)
    ((CB_GUILib*)m_cb)->Control_SettingsSlider_SetText(((AddonCB*)m_Handle)->addonData, m_SettingsSliderHandle, label);
}

string CAddonGUISettingsSliderControl::GetDescription() const
{
  if (!m_SettingsSliderHandle)
    return "";

  return ((CB_GUILib*)m_cb)->Control_SettingsSlider_GetDescription(((AddonCB*)m_Handle)->addonData, m_SettingsSliderHandle);
}

void CAddonGUISettingsSliderControl::SetIntRange(int iStart, int iEnd)
{
  if (m_SettingsSliderHandle)
    ((CB_GUILib*)m_cb)->Control_SettingsSlider_SetIntRange(((AddonCB*)m_Handle)->addonData, m_SettingsSliderHandle, iStart, iEnd);
}

void CAddonGUISettingsSliderControl::SetIntValue(int iValue)
{
  if (m_SettingsSliderHandle)
    ((CB_GUILib*)m_cb)->Control_SettingsSlider_SetIntValue(((AddonCB*)m_Handle)->addonData, m_SettingsSliderHandle, iValue);
}

int CAddonGUISettingsSliderControl::GetIntValue() const
{
  if (!m_SettingsSliderHandle)
    return 0;
  return ((CB_GUILib*)m_cb)->Control_SettingsSlider_GetIntValue(((AddonCB*)m_Handle)->addonData, m_SettingsSliderHandle);
}

void CAddonGUISettingsSliderControl::SetIntInterval(int iInterval)
{
  if (m_SettingsSliderHandle)
    ((CB_GUILib*)m_cb)->Control_SettingsSlider_SetIntInterval(((AddonCB*)m_Handle)->addonData, m_SettingsSliderHandle, iInterval);
}

void CAddonGUISettingsSliderControl::SetPercentage(float fPercent)
{
  if (m_SettingsSliderHandle)
    ((CB_GUILib*)m_cb)->Control_SettingsSlider_SetPercentage(((AddonCB*)m_Handle)->addonData, m_SettingsSliderHandle, fPercent);
}

float CAddonGUISettingsSliderControl::GetPercentage() const
{
  if (!m_SettingsSliderHandle)
    return 0.0f;

  return ((CB_GUILib*)m_cb)->Control_SettingsSlider_GetPercentage(((AddonCB*)m_Handle)->addonData, m_SettingsSliderHandle);
}

void CAddonGUISettingsSliderControl::SetFloatRange(float fStart, float fEnd)
{
  if (m_SettingsSliderHandle)
    ((CB_GUILib*)m_cb)->Control_SettingsSlider_SetFloatRange(((AddonCB*)m_Handle)->addonData, m_SettingsSliderHandle, fStart, fEnd);
}

void CAddonGUISettingsSliderControl::SetFloatValue(float fValue)
{
  if (m_SettingsSliderHandle)
    ((CB_GUILib*)m_cb)->Control_SettingsSlider_SetFloatValue(((AddonCB*)m_Handle)->addonData, m_SettingsSliderHandle, fValue);
}

float CAddonGUISettingsSliderControl::GetFloatValue() const
{
  if (!m_SettingsSliderHandle)
    return 0.0f;
  return ((CB_GUILib*)m_cb)->Control_SettingsSlider_GetFloatValue(((AddonCB*)m_Handle)->addonData, m_SettingsSliderHandle);
}

void CAddonGUISettingsSliderControl::SetFloatInterval(float fInterval)
{
  if (m_SettingsSliderHandle)
    ((CB_GUILib*)m_cb)->Control_SettingsSlider_SetFloatInterval(((AddonCB*)m_Handle)->addonData, m_SettingsSliderHandle, fInterval);
}


///-------------------------------------
/// cListItem

DLLEXPORT CAddonListItem* GUI_ListItem_create(void *hdl, void *cb, const char *label, const char *label2, const char *iconImage, const char *thumbnailImage, const char *path)
{
  return new CAddonListItem(hdl, cb, label, label2, iconImage, thumbnailImage, path);
}

DLLEXPORT void GUI_ListItem_destroy(CAddonListItem* p)
{
  delete p;
}


CAddonListItem::CAddonListItem(void *hdl, void *cb, const char *label, const char *label2, const char *iconImage, const char *thumbnailImage, const char *path)
 : m_Handle(hdl)
 , m_cb(cb)
{
  m_ListItemHandle = ((CB_GUILib*)m_cb)->ListItem_Create(((AddonCB*)m_Handle)->addonData, label, label2, iconImage, thumbnailImage, path);
}

const char *CAddonListItem::GetLabel()
{
  if (!m_ListItemHandle)
    return "";

  return ((CB_GUILib*)m_cb)->ListItem_GetLabel(((AddonCB*)m_Handle)->addonData, m_ListItemHandle);
}

void CAddonListItem::SetLabel(const char *label)
{
  if (m_ListItemHandle)
    ((CB_GUILib*)m_cb)->ListItem_SetLabel(((AddonCB*)m_Handle)->addonData, m_ListItemHandle, label);
}

const char *CAddonListItem::GetLabel2()
{
  if (!m_ListItemHandle)
    return "";

  return ((CB_GUILib*)m_cb)->ListItem_GetLabel2(((AddonCB*)m_Handle)->addonData, m_ListItemHandle);
}

void CAddonListItem::SetLabel2(const char *label)
{
  if (m_ListItemHandle)
    ((CB_GUILib*)m_cb)->ListItem_SetLabel2(((AddonCB*)m_Handle)->addonData, m_ListItemHandle, label);
}

void CAddonListItem::SetIconImage(const char *image)
{
  if (m_ListItemHandle)
    ((CB_GUILib*)m_cb)->ListItem_SetIconImage(((AddonCB*)m_Handle)->addonData, m_ListItemHandle, image);
}

void CAddonListItem::SetThumbnailImage(const char *image)
{
  if (m_ListItemHandle)
    ((CB_GUILib*)m_cb)->ListItem_SetThumbnailImage(((AddonCB*)m_Handle)->addonData, m_ListItemHandle, image);
}

void CAddonListItem::SetInfo(const char *Info)
{
  if (m_ListItemHandle)
    ((CB_GUILib*)m_cb)->ListItem_SetInfo(((AddonCB*)m_Handle)->addonData, m_ListItemHandle, Info);
}

void CAddonListItem::SetProperty(const char *key, const char *value)
{
  if (m_ListItemHandle)
    ((CB_GUILib*)m_cb)->ListItem_SetProperty(((AddonCB*)m_Handle)->addonData, m_ListItemHandle, key, value);
}

const char *CAddonListItem::GetProperty(const char *key) const
{
  if (!m_ListItemHandle)
    return "";

  return ((CB_GUILib*)m_cb)->ListItem_GetProperty(((AddonCB*)m_Handle)->addonData, m_ListItemHandle, key);
}

void CAddonListItem::SetPath(const char *Path)
{
  if (m_ListItemHandle)
    ((CB_GUILib*)m_cb)->ListItem_SetPath(((AddonCB*)m_Handle)->addonData, m_ListItemHandle, Path);
}

///-------------------------------------
/// cGUIRenderingControl

DLLEXPORT CAddonGUIRenderingControl* GUI_control_get_rendering(void *hdl, void *cb, CAddonGUIWindow *window, int controlId)
{
  return new CAddonGUIRenderingControl(hdl, cb, window, controlId);
}

DLLEXPORT void GUI_control_release_rendering(CAddonGUIRenderingControl* p)
{
  delete p;
}

DLLEXPORT bool GUI_control_rendering_create(GUIHANDLE handle, int x, int y, int w, int h, void *device)
{
  CAddonGUIRenderingControl *pControl = (CAddonGUIRenderingControl*) handle;
  return pControl->Create(x,y,w,h,device);
}

DLLEXPORT void GUI_control_rendering_render(GUIHANDLE handle)
{
  CAddonGUIRenderingControl *pControl = (CAddonGUIRenderingControl*) handle;
  pControl->Render();
}

DLLEXPORT void GUI_control_rendering_stop(GUIHANDLE handle)
{
  CAddonGUIRenderingControl *pControl = (CAddonGUIRenderingControl*) handle;
  pControl->Stop();
}

DLLEXPORT bool GUI_control_rendering_dirty(GUIHANDLE handle)
{
  CAddonGUIRenderingControl *pControl = (CAddonGUIRenderingControl*) handle;
  return pControl->Dirty();
}

CAddonGUIRenderingControl::CAddonGUIRenderingControl(void *hdl, void *cb, CAddonGUIWindow *window, int controlId)
 : m_Window(window)
 , m_ControlId(controlId)
 , m_Handle(hdl)
 , m_cb(cb)
{
  m_RenderingHandle = ((CB_GUILib*)m_cb)->Window_GetControl_RenderAddon(((AddonCB*)m_Handle)->addonData, m_Window->m_WindowHandle, controlId);
}

CAddonGUIRenderingControl::~CAddonGUIRenderingControl()
{
  ((CB_GUILib*)m_cb)->RenderAddon_Delete(((AddonCB*)m_Handle)->addonData, m_RenderingHandle);
}

void CAddonGUIRenderingControl::Init()
{
  ((CB_GUILib*)m_cb)->RenderAddon_SetCallbacks(((AddonCB*)m_Handle)->addonData, m_RenderingHandle, this, GUI_control_rendering_create, GUI_control_rendering_render, GUI_control_rendering_stop, GUI_control_rendering_dirty);
}

bool CAddonGUIRenderingControl::Create(int x, int y, int w, int h, void *device)
{
  if (!CBCreate)
    return false;

  return CBCreate(m_cbhdl, x, y, w, h, device);
}

void CAddonGUIRenderingControl::Render()
{
  if (!CBRender)
    return;

  CBRender(m_cbhdl);
}

void CAddonGUIRenderingControl::Stop()
{
  if (!CBStop)
    return;

  CBStop(m_cbhdl);
}

bool CAddonGUIRenderingControl::Dirty()
{
  if (!CBDirty)
    return true;

  return CBDirty(m_cbhdl);
}
};
