#pragma once
/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include <string>
#include <vector>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "libXBMC_addon.h"

typedef void* GUIHANDLE;

namespace KodiAPI
{
namespace V1
{
namespace GUI
{

typedef struct CB_GUILib
{
  void (*Lock)();
  void (*Unlock)();
  int (*GetScreenHeight)();
  int (*GetScreenWidth)();
  int (*GetVideoResolution)();
  GUIHANDLE (*Window_New)(void *addonData, const char *xmlFilename, const char *defaultSkin, bool forceFallback, bool asDialog);
  void (*Window_Delete)(void *addonData, GUIHANDLE handle);
  void (*Window_SetCallbacks)(void *addonData, GUIHANDLE handle, GUIHANDLE clienthandle, bool (*)(GUIHANDLE handle), bool (*)(GUIHANDLE handle, int), bool (*)(GUIHANDLE handle, int), bool (*)(GUIHANDLE handle, int));
  bool (*Window_Show)(void *addonData, GUIHANDLE handle);
  bool (*Window_Close)(void *addonData, GUIHANDLE handle);
  bool (*Window_DoModal)(void *addonData, GUIHANDLE handle);
  bool (*Window_SetFocusId)(void *addonData, GUIHANDLE handle, int iControlId);
  int (*Window_GetFocusId)(void *addonData, GUIHANDLE handle);
  bool (*Window_SetCoordinateResolution)(void *addonData, GUIHANDLE handle, int res);
  void (*Window_SetProperty)(void *addonData, GUIHANDLE handle, const char *key, const char *value);
  void (*Window_SetPropertyInt)(void *addonData, GUIHANDLE handle, const char *key, int value);
  void (*Window_SetPropertyBool)(void *addonData, GUIHANDLE handle, const char *key, bool value);
  void (*Window_SetPropertyDouble)(void *addonData, GUIHANDLE handle, const char *key, double value);
  const char* (*Window_GetProperty)(void *addonData, GUIHANDLE handle, const char *key);
  int (*Window_GetPropertyInt)(void *addonData, GUIHANDLE handle, const char *key);
  bool (*Window_GetPropertyBool)(void *addonData, GUIHANDLE handle, const char *key);
  double (*Window_GetPropertyDouble)(void *addonData, GUIHANDLE handle, const char *key);
  void (*Window_ClearProperties)(void *addonData, GUIHANDLE handle);
  int (*Window_GetListSize)(void *addonData, GUIHANDLE handle);
  void (*Window_ClearList)(void *addonData, GUIHANDLE handle);
  GUIHANDLE (*Window_AddItem)(void *addonData, GUIHANDLE handle, GUIHANDLE item, int itemPosition);
  GUIHANDLE (*Window_AddStringItem)(void *addonData, GUIHANDLE handle, const char *itemName, int itemPosition);
  void (*Window_RemoveItem)(void *addonData, GUIHANDLE handle, int itemPosition);
  GUIHANDLE (*Window_GetListItem)(void *addonData, GUIHANDLE handle, int listPos);
  void (*Window_SetCurrentListPosition)(void *addonData, GUIHANDLE handle, int listPos);
  int (*Window_GetCurrentListPosition)(void *addonData, GUIHANDLE handle);
  GUIHANDLE (*Window_GetControl_Spin)(void *addonData, GUIHANDLE handle, int controlId);
  GUIHANDLE (*Window_GetControl_Button)(void *addonData, GUIHANDLE handle, int controlId);
  GUIHANDLE (*Window_GetControl_RadioButton)(void *addonData, GUIHANDLE handle, int controlId);
  GUIHANDLE (*Window_GetControl_Edit)(void *addonData, GUIHANDLE handle, int controlId);
  GUIHANDLE (*Window_GetControl_Progress)(void *addonData, GUIHANDLE handle, int controlId);
  GUIHANDLE (*Window_GetControl_RenderAddon)(void *addonData, GUIHANDLE handle, int controlId);
  void (*Window_SetControlLabel)(void *addonData, GUIHANDLE handle, int controlId, const char *label);
  void (*Window_MarkDirtyRegion)(void *addonData, GUIHANDLE handle);
  void (*Control_Spin_SetVisible)(void *addonData, GUIHANDLE spinhandle, bool yesNo);
  void (*Control_Spin_SetText)(void *addonData, GUIHANDLE spinhandle, const char *label);
  void (*Control_Spin_Clear)(void *addonData, GUIHANDLE spinhandle);
  void (*Control_Spin_AddLabel)(void *addonData, GUIHANDLE spinhandle, const char *label, int iValue);
  int (*Control_Spin_GetValue)(void *addonData, GUIHANDLE spinhandle);
  void (*Control_Spin_SetValue)(void *addonData, GUIHANDLE spinhandle, int iValue);
  void (*Control_RadioButton_SetVisible)(void *addonData, GUIHANDLE handle, bool yesNo);
  void (*Control_RadioButton_SetText)(void *addonData, GUIHANDLE handle, const char *label);
  void (*Control_RadioButton_SetSelected)(void *addonData, GUIHANDLE handle, bool yesNo);
  bool (*Control_RadioButton_IsSelected)(void *addonData, GUIHANDLE handle);
  void (*Control_Progress_SetPercentage)(void *addonData, GUIHANDLE handle, float fPercent);
  float (*Control_Progress_GetPercentage)(void *addonData, GUIHANDLE handle);
  void (*Control_Progress_SetInfo)(void *addonData, GUIHANDLE handle, int iInfo);
  int (*Control_Progress_GetInfo)(void *addonData, GUIHANDLE handle);
  const char* (*Control_Progress_GetDescription)(void *addonData, GUIHANDLE handle);
  GUIHANDLE (*Window_GetControl_Slider)(void *addonData, GUIHANDLE handle, int controlId);
  void (*Control_Slider_SetVisible)(void *addonData, GUIHANDLE handle, bool yesNo);
  const char *(*Control_Slider_GetDescription)(void *addonData, GUIHANDLE handle);
  void (*Control_Slider_SetIntRange)(void *addonData, GUIHANDLE handle, int iStart, int iEnd);
  void (*Control_Slider_SetIntValue)(void *addonData, GUIHANDLE handle, int iValue);
  int (*Control_Slider_GetIntValue)(void *addonData, GUIHANDLE handle);
  void (*Control_Slider_SetIntInterval)(void *addonData, GUIHANDLE handle, int iInterval);
  void (*Control_Slider_SetPercentage)(void *addonData, GUIHANDLE handle, float fPercent);
  float (*Control_Slider_GetPercentage)(void *addonData, GUIHANDLE handle);
  void (*Control_Slider_SetFloatRange)(void *addonData, GUIHANDLE handle, float fStart, float fEnd);
  void (*Control_Slider_SetFloatValue)(void *addonData, GUIHANDLE handle, float fValue);
  float (*Control_Slider_GetFloatValue)(void *addonData, GUIHANDLE handle);
  void (*Control_Slider_SetFloatInterval)(void *addonData, GUIHANDLE handle, float fInterval);
  GUIHANDLE (*Window_GetControl_SettingsSlider)(void *addonData, GUIHANDLE handle, int controlId);
  void (*Control_SettingsSlider_SetVisible)(void *addonData, GUIHANDLE handle, bool yesNo);
  void (*Control_SettingsSlider_SetText)(void *addonData, GUIHANDLE handle, const char *label);
  const char *(*Control_SettingsSlider_GetDescription)(void *addonData, GUIHANDLE handle);
  void (*Control_SettingsSlider_SetIntRange)(void *addonData, GUIHANDLE handle, int iStart, int iEnd);
  void (*Control_SettingsSlider_SetIntValue)(void *addonData, GUIHANDLE handle, int iValue);
  int (*Control_SettingsSlider_GetIntValue)(void *addonData, GUIHANDLE handle);
  void (*Control_SettingsSlider_SetIntInterval)(void *addonData, GUIHANDLE handle, int iInterval);
  void (*Control_SettingsSlider_SetPercentage)(void *addonData, GUIHANDLE handle, float fPercent);
  float (*Control_SettingsSlider_GetPercentage)(void *addonData, GUIHANDLE handle);
  void (*Control_SettingsSlider_SetFloatRange)(void *addonData, GUIHANDLE handle, float fStart, float fEnd);
  void (*Control_SettingsSlider_SetFloatValue)(void *addonData, GUIHANDLE handle, float fValue);
  float (*Control_SettingsSlider_GetFloatValue)(void *addonData, GUIHANDLE handle);
  void (*Control_SettingsSlider_SetFloatInterval)(void *addonData, GUIHANDLE handle, float fInterval);
  GUIHANDLE (*ListItem_Create)(void *addonData, const char *label, const char *label2, const char *iconImage, const char *thumbnailImage, const char *path);
  const char* (*ListItem_GetLabel)(void *addonData, GUIHANDLE handle);
  void (*ListItem_SetLabel)(void *addonData, GUIHANDLE handle, const char *label);
  const char* (*ListItem_GetLabel2)(void *addonData, GUIHANDLE handle);
  void (*ListItem_SetLabel2)(void *addonData, GUIHANDLE handle, const char *label);
  void (*ListItem_SetIconImage)(void *addonData, GUIHANDLE handle, const char *image);
  void (*ListItem_SetThumbnailImage)(void *addonData, GUIHANDLE handle, const char *image);
  void (*ListItem_SetInfo)(void *addonData, GUIHANDLE handle, const char *info);
  void (*ListItem_SetProperty)(void *addonData, GUIHANDLE handle, const char *key, const char *value);
  const char* (*ListItem_GetProperty)(void *addonData, GUIHANDLE handle, const char *key);
  void (*ListItem_SetPath)(void *addonData, GUIHANDLE handle, const char *path);
  void (*RenderAddon_SetCallbacks)(void *addonData, GUIHANDLE handle, GUIHANDLE clienthandle, bool (*createCB)(GUIHANDLE,int,int,int,int,void*), void (*renderCB)(GUIHANDLE), void (*stopCB)(GUIHANDLE), bool (*dirtyCB)(GUIHANDLE));
  void (*RenderAddon_Delete)(void *addonData, GUIHANDLE handle);
  void (*RenderAddon_MarkDirty)(void *addonData, GUIHANDLE handle);

  bool (*Dialog_Keyboard_ShowAndGetInputWithHead)(char &strTextString, unsigned int iMaxStringSize, const char *heading, bool allowEmptyResult, bool hiddenInput, unsigned int autoCloseMs);
  bool (*Dialog_Keyboard_ShowAndGetInput)(char &strTextString, unsigned int iMaxStringSize, bool allowEmptyResult, unsigned int autoCloseMs);
  bool (*Dialog_Keyboard_ShowAndGetNewPasswordWithHead)(char &newPassword, unsigned int iMaxStringSize, const char *strHeading, bool allowEmptyResult, unsigned int autoCloseMs);
  bool (*Dialog_Keyboard_ShowAndGetNewPassword)(char &strNewPassword, unsigned int iMaxStringSize, unsigned int autoCloseMs);
  bool (*Dialog_Keyboard_ShowAndVerifyNewPasswordWithHead)(char &strNewPassword, unsigned int iMaxStringSize, const char *strHeading, bool allowEmpty, unsigned int autoCloseMs);
  bool (*Dialog_Keyboard_ShowAndVerifyNewPassword)(char &strNewPassword, unsigned int iMaxStringSize, unsigned int autoCloseMs);
  int (*Dialog_Keyboard_ShowAndVerifyPassword)(char &strPassword, unsigned int iMaxStringSize, const char *strHeading, int iRetries, unsigned int autoCloseMs);
  bool (*Dialog_Keyboard_ShowAndGetFilter)(char &aTextString, unsigned int iMaxStringSize, bool searching, unsigned int autoCloseMs);
  bool (*Dialog_Keyboard_SendTextToActiveKeyboard)(const char *aTextString, bool closeKeyboard);
  bool (*Dialog_Keyboard_isKeyboardActivated)();

  bool (*Dialog_Numeric_ShowAndVerifyNewPassword)(char &strNewPassword, unsigned int iMaxStringSize);
  int (*Dialog_Numeric_ShowAndVerifyPassword)(char &strPassword, unsigned int iMaxStringSize, const char *strHeading, int iRetries);
  bool (*Dialog_Numeric_ShowAndVerifyInput)(char &strPassword, unsigned int iMaxStringSize, const char *strHeading, bool bGetUserInput);
  bool (*Dialog_Numeric_ShowAndGetTime)(tm &time, const char *strHeading);
  bool (*Dialog_Numeric_ShowAndGetDate)(tm &date, const char *strHeading);
  bool (*Dialog_Numeric_ShowAndGetIPAddress)(char &strIPAddress, unsigned int iMaxStringSize, const char *strHeading);
  bool (*Dialog_Numeric_ShowAndGetNumber)(char &strInput, unsigned int iMaxStringSize, const char *strHeading, unsigned int iAutoCloseTimeoutMs);
  bool (*Dialog_Numeric_ShowAndGetSeconds)(char &timeString, unsigned int iMaxStringSize, const char *strHeading);

  bool (*Dialog_FileBrowser_ShowAndGetFile)(const char *directory, const char *mask, const char *heading, char &path, unsigned int iMaxStringSize, bool useThumbs, bool useFileDirectories, bool singleList);

  void (*Dialog_OK_ShowAndGetInputSingleText)(const char *heading, const char *text);
  void (*Dialog_OK_ShowAndGetInputLineText)(const char *heading, const char *line0, const char *line1, const char *line2);

  bool (*Dialog_YesNo_ShowAndGetInputSingleText)(const char *heading, const char *text, bool& bCanceled, const char *noLabel, const char *yesLabel);
  bool (*Dialog_YesNo_ShowAndGetInputLineText)(const char *heading, const char *line0, const char *line1, const char *line2, const char *noLabel, const char *yesLabel);
  bool (*Dialog_YesNo_ShowAndGetInputLineButtonText)(const char *heading, const char *line0, const char *line1, const char *line2, bool &bCanceled, const char *noLabel, const char *yesLabel);

  void (*Dialog_TextViewer)(const char *heading, const char *text);

  int (*Dialog_Select)(const char *heading, const char *entries[], unsigned int size, int selected);
} CB_GUILib;

} /* namespace GUI */
} /* namespace V1 */
} /* namespace KodiAPI */


/* current ADDONGUI API version */
#define KODI_GUILIB_API_VERSION "5.11.0"

/* min. ADDONGUI API version */
#define KODI_GUILIB_MIN_API_VERSION "5.10.0"

#define ADDON_ACTION_PREVIOUS_MENU          10
#define ADDON_ACTION_CLOSE_DIALOG           51
#define ADDON_ACTION_NAV_BACK               92

class CAddonGUISpinControl;
class CAddonGUIRadioButton;
class CAddonGUIProgressControl;
class CAddonGUIRenderingControl;
class CAddonGUISliderControl;
class CAddonGUISettingsSliderControl;

class CAddonListItem
{
friend class CAddonGUIWindow;

public:
  CAddonListItem(AddonCB* hdl, KodiAPI::V1::GUI::CB_GUILib* cb, const char *label, const char *label2, const char *iconImage, const char *thumbnailImage, const char *path)
    : m_Handle(hdl)
    , m_cb(cb)
  {
    m_ListItemHandle = m_cb->ListItem_Create(m_Handle->addonData, label, label2, iconImage, thumbnailImage, path);
  }

  virtual ~CAddonListItem(void) {}

  const char *GetLabel()
  {
    if (!m_ListItemHandle)
      return "";

    return m_cb->ListItem_GetLabel(m_Handle->addonData, m_ListItemHandle);
  }

  void SetLabel(const char *label)
  {
    if (m_ListItemHandle)
      m_cb->ListItem_SetLabel(m_Handle->addonData, m_ListItemHandle, label);
  }

  const char *GetLabel2()
  {
    if (!m_ListItemHandle)
      return "";

    return m_cb->ListItem_GetLabel2(m_Handle->addonData, m_ListItemHandle);
  }

  void SetLabel2(const char *label)
  {
    if (m_ListItemHandle)
      m_cb->ListItem_SetLabel2(m_Handle->addonData, m_ListItemHandle, label);
  }

  void SetIconImage(const char *image)
  {
    if (m_ListItemHandle)
      m_cb->ListItem_SetIconImage(m_Handle->addonData, m_ListItemHandle, image);
  }

  void SetThumbnailImage(const char *image)
  {
    if (m_ListItemHandle)
      m_cb->ListItem_SetThumbnailImage(m_Handle->addonData, m_ListItemHandle, image);
  }

  void SetInfo(const char *Info)
  {
    if (m_ListItemHandle)
      m_cb->ListItem_SetInfo(m_Handle->addonData, m_ListItemHandle, Info);
  }

  void SetProperty(const char *key, const char *value)
  {
    if (m_ListItemHandle)
      m_cb->ListItem_SetProperty(m_Handle->addonData, m_ListItemHandle, key, value);
  }

  const char *GetProperty(const char *key) const
  {
    if (!m_ListItemHandle)
      return "";

    return m_cb->ListItem_GetProperty(m_Handle->addonData, m_ListItemHandle, key);
  }

  void SetPath(const char *Path)
  {
    if (m_ListItemHandle)
      m_cb->ListItem_SetPath(m_Handle->addonData, m_ListItemHandle, Path);
  }

protected:
  GUIHANDLE m_ListItemHandle;
  AddonCB* m_Handle;
  KodiAPI::V1::GUI::CB_GUILib* m_cb;
};

class CAddonGUIWindow
{
friend class CAddonGUISpinControl;
friend class CAddonGUIRadioButton;
friend class CAddonGUIProgressControl;
friend class CAddonGUIRenderingControl;
friend class CAddonGUISliderControl;
friend class CAddonGUISettingsSliderControl;

public:
  CAddonGUIWindow(AddonCB* hdl, KodiAPI::V1::GUI::CB_GUILib* cb, const char *xmlFilename, const char *defaultSkin, bool forceFallback, bool asDialog)
    : m_Handle(hdl),
      m_cb(cb)
  {
    CBOnInit = nullptr;
    CBOnClick = nullptr;
    CBOnFocus = nullptr;
    CBOnAction = nullptr;
    m_WindowHandle = nullptr;
    m_cbhdl = nullptr;

    if (hdl && cb)
    {
      m_WindowHandle = m_cb->Window_New(m_Handle->addonData, xmlFilename, defaultSkin, forceFallback, asDialog);
      if (!m_WindowHandle)
        fprintf(stderr, "libXBMC_gui-ERROR: cGUIWindow can't create window class from XBMC !!!\n");

      m_cb->Window_SetCallbacks(m_Handle->addonData, m_WindowHandle, this, OnInitCB, OnClickCB, OnFocusCB, OnActionCB);
    }
  }

  virtual ~CAddonGUIWindow()
  {
    if (m_Handle && m_cb && m_WindowHandle)
    {
      m_cb->Window_Delete(m_Handle->addonData, m_WindowHandle);
      m_WindowHandle = NULL;
    }
  }

  bool Show()
  {
    return m_cb->Window_Show(m_Handle->addonData, m_WindowHandle);
  }

  void Close()
  {
    m_cb->Window_Close(m_Handle->addonData, m_WindowHandle);
  }

  void DoModal()
  {
    m_cb->Window_DoModal(m_Handle->addonData, m_WindowHandle);
  }

  bool SetFocusId(int iControlId)
  {
    return m_cb->Window_SetFocusId(m_Handle->addonData, m_WindowHandle, iControlId);
  }

  int GetFocusId()
  {
    return m_cb->Window_GetFocusId(m_Handle->addonData, m_WindowHandle);
  }

  bool SetCoordinateResolution(int res)
  {
    return m_cb->Window_SetCoordinateResolution(m_Handle->addonData, m_WindowHandle, res);
  }

  void SetProperty(const char *key, const char *value)
  {
    m_cb->Window_SetProperty(m_Handle->addonData, m_WindowHandle, key, value);
  }

  void SetPropertyInt(const char *key, int value)
  {
    m_cb->Window_SetPropertyInt(m_Handle->addonData, m_WindowHandle, key, value);
  }

  void SetPropertyBool(const char *key, bool value)
  {
    m_cb->Window_SetPropertyBool(m_Handle->addonData, m_WindowHandle, key, value);
  }

  void SetPropertyDouble(const char *key, double value)
  {
    m_cb->Window_SetPropertyDouble(m_Handle->addonData, m_WindowHandle, key, value);
  }

  const char *GetProperty(const char *key) const
  {
    return m_cb->Window_GetProperty(m_Handle->addonData, m_WindowHandle, key);
  }

  int GetPropertyInt(const char *key) const
  {
    return m_cb->Window_GetPropertyInt(m_Handle->addonData, m_WindowHandle, key);
  }

  bool GetPropertyBool(const char *key) const
  {
    return m_cb->Window_GetPropertyBool(m_Handle->addonData, m_WindowHandle, key);
  }

  double GetPropertyDouble(const char *key) const
  {
    return m_cb->Window_GetPropertyDouble(m_Handle->addonData, m_WindowHandle, key);
  }

  void ClearProperties()
  {
    m_cb->Window_ClearProperties(m_Handle->addonData, m_WindowHandle);
  }

  int GetListSize()
  {
    return m_cb->Window_GetListSize(m_Handle->addonData, m_WindowHandle);
  }

  void ClearList()
  {
    m_cb->Window_ClearList(m_Handle->addonData, m_WindowHandle);
  }

  GUIHANDLE AddStringItem(const char *name, int itemPosition = -1)
  {
    return m_cb->Window_AddStringItem(m_Handle->addonData, m_WindowHandle, name, itemPosition);
  }

  void AddItem(GUIHANDLE item, int itemPosition = -1)
  {
    m_cb->Window_AddItem(m_Handle->addonData, m_WindowHandle, item, itemPosition);
  }

  void AddItem(CAddonListItem *item, int itemPosition = -1)
  {
    m_cb->Window_AddItem(m_Handle->addonData, m_WindowHandle, item->m_ListItemHandle, itemPosition);
  }

  void RemoveItem(int itemPosition)
  {
    m_cb->Window_RemoveItem(m_Handle->addonData, m_WindowHandle, itemPosition);
  }

  GUIHANDLE GetListItem(int listPos)
  {
    return m_cb->Window_GetListItem(m_Handle->addonData, m_WindowHandle, listPos);
  }

  void SetCurrentListPosition(int listPos)
  {
    m_cb->Window_SetCurrentListPosition(m_Handle->addonData, m_WindowHandle, listPos);
  }

  int GetCurrentListPosition()
  {
    return m_cb->Window_GetCurrentListPosition(m_Handle->addonData, m_WindowHandle);
  }

  void SetControlLabel(int controlId, const char *label)
  {
    m_cb->Window_SetControlLabel(m_Handle->addonData, m_WindowHandle, controlId, label);
  }

  void MarkDirtyRegion()
  {
    m_cb->Window_MarkDirtyRegion(m_Handle->addonData, m_WindowHandle);
  }

  bool OnClick(int controlId)
  {
    if (!CBOnClick)
      return false;

    return CBOnClick(m_cbhdl, controlId);
  }

  bool OnFocus(int controlId)
  {
    if (!CBOnFocus)
      return false;

    return CBOnFocus(m_cbhdl, controlId);
  }

  bool OnInit()
  {
    if (!CBOnInit)
      return false;

    return CBOnInit(m_cbhdl);
  }

  bool OnAction(int actionId)
  {
    if (!CBOnAction)
      return false;

    return CBOnAction(m_cbhdl, actionId);
  }

  GUIHANDLE m_cbhdl;
  bool (*CBOnInit)(GUIHANDLE cbhdl);
  bool (*CBOnFocus)(GUIHANDLE cbhdl, int controlId);
  bool (*CBOnClick)(GUIHANDLE cbhdl, int controlId);
  bool (*CBOnAction)(GUIHANDLE cbhdl, int actionId);

protected:
  GUIHANDLE m_WindowHandle;
  AddonCB* m_Handle;
  KodiAPI::V1::GUI::CB_GUILib* m_cb;

  static bool OnInitCB(GUIHANDLE cbhdl);
  static bool OnClickCB(GUIHANDLE cbhdl, int controlId);
  static bool OnFocusCB(GUIHANDLE cbhdl, int controlId);
  static bool OnActionCB(GUIHANDLE cbhdl, int actionId);
};


inline bool CAddonGUIWindow::OnInitCB(GUIHANDLE cbhdl)
{
  return static_cast<CAddonGUIWindow*>(cbhdl)->OnInit();
}

inline bool CAddonGUIWindow::OnClickCB(GUIHANDLE cbhdl, int controlId)
{
  return static_cast<CAddonGUIWindow*>(cbhdl)->OnClick(controlId);
}

inline bool CAddonGUIWindow::OnFocusCB(GUIHANDLE cbhdl, int controlId)
{
  return static_cast<CAddonGUIWindow*>(cbhdl)->OnFocus(controlId);
}

inline bool CAddonGUIWindow::OnActionCB(GUIHANDLE cbhdl, int actionId)
{
  return static_cast<CAddonGUIWindow*>(cbhdl)->OnAction(actionId);
}

class CAddonGUISpinControl
{
public:
  CAddonGUISpinControl(AddonCB* hdl, KodiAPI::V1::GUI::CB_GUILib* cb, CAddonGUIWindow *window, int controlId)
    : m_Window(window),
      m_ControlId(controlId),
      m_Handle(hdl),
      m_cb(cb)
  {
    m_SpinHandle = m_cb->Window_GetControl_Spin(m_Handle->addonData, m_Window->m_WindowHandle, controlId);
  }
  ~CAddonGUISpinControl(void) {}

  void SetVisible(bool yesNo)
  {
    if (m_SpinHandle)
      m_cb->Control_Spin_SetVisible(m_Handle->addonData, m_SpinHandle, yesNo);
  }

  void SetText(const char *label)
  {
    if (m_SpinHandle)
      m_cb->Control_Spin_SetText(m_Handle->addonData, m_SpinHandle, label);
  }

  void Clear()
  {
    if (m_SpinHandle)
      m_cb->Control_Spin_Clear(m_Handle->addonData, m_SpinHandle);
  }

  void AddLabel(const char *label, int iValue)
  {
    if (m_SpinHandle)
      m_cb->Control_Spin_AddLabel(m_Handle->addonData, m_SpinHandle, label, iValue);
  }

  int GetValue()
  {
    if (!m_SpinHandle)
      return -1;

    return m_cb->Control_Spin_GetValue(m_Handle->addonData, m_SpinHandle);
  }
  
  void SetValue(int iValue)
  {
    if (m_SpinHandle)
      m_cb->Control_Spin_SetValue(m_Handle->addonData, m_SpinHandle, iValue);
  }

private:
  CAddonGUIWindow* m_Window;
  int m_ControlId;
  GUIHANDLE m_SpinHandle;
  AddonCB* m_Handle;
  KodiAPI::V1::GUI::CB_GUILib* m_cb;
};

class CAddonGUIRadioButton
{
public:
  CAddonGUIRadioButton(AddonCB* hdl, KodiAPI::V1::GUI::CB_GUILib* cb, CAddonGUIWindow *window, int controlId)
    : m_Window(window)
    , m_ControlId(controlId)
    , m_Handle(hdl)
    , m_cb(cb)
  {
    m_ButtonHandle = m_cb->Window_GetControl_RadioButton(m_Handle->addonData, m_Window->m_WindowHandle, controlId);
  }
  ~CAddonGUIRadioButton() {}

  void SetVisible(bool yesNo)
  {
    if (m_ButtonHandle)
      m_cb->Control_RadioButton_SetVisible(m_Handle->addonData, m_ButtonHandle, yesNo);
  }

  void SetText(const char *label)
  {
    if (m_ButtonHandle)
      m_cb->Control_RadioButton_SetText(m_Handle->addonData, m_ButtonHandle, label);
  }

  void SetSelected(bool yesNo)
  {
    if (m_ButtonHandle)
      m_cb->Control_RadioButton_SetSelected(m_Handle->addonData, m_ButtonHandle, yesNo);
  }

  bool IsSelected()
  {
    if (!m_ButtonHandle)
      return false;

    return m_cb->Control_RadioButton_IsSelected(m_Handle->addonData, m_ButtonHandle);
  }

private:
  CAddonGUIWindow* m_Window;
  int m_ControlId;
  GUIHANDLE m_ButtonHandle;
  AddonCB* m_Handle;
  KodiAPI::V1::GUI::CB_GUILib* m_cb;
};

class CAddonGUIProgressControl
{
public:
  CAddonGUIProgressControl(AddonCB* hdl, KodiAPI::V1::GUI::CB_GUILib* cb, CAddonGUIWindow *window, int controlId)
    : m_Window(window)
    , m_ControlId(controlId)
    , m_Handle(hdl)
    , m_cb(cb)
  {
    m_ProgressHandle = m_cb->Window_GetControl_Progress(m_Handle->addonData, m_Window->m_WindowHandle, controlId);
  }

  ~CAddonGUIProgressControl(void) {}

  void SetPercentage(float fPercent)
  {
    if (m_ProgressHandle)
      m_cb->Control_Progress_SetPercentage(m_Handle->addonData, m_ProgressHandle, fPercent);
  }

  float GetPercentage() const
  {
    if (!m_ProgressHandle)
      return 0.0f;

    return m_cb->Control_Progress_GetPercentage(m_Handle->addonData, m_ProgressHandle);
  }

  void SetInfo(int iInfo)
  {
    if (m_ProgressHandle)
      m_cb->Control_Progress_SetInfo(m_Handle->addonData, m_ProgressHandle, iInfo);
  }

  int GetInfo() const
  {
    if (!m_ProgressHandle)
      return -1;

    return m_cb->Control_Progress_GetInfo(m_Handle->addonData, m_ProgressHandle);
  }

  std::string GetDescription() const
  {
    if (!m_ProgressHandle)
      return "";

    return m_cb->Control_Progress_GetDescription(m_Handle->addonData, m_ProgressHandle);
  }

private:
  CAddonGUIWindow* m_Window;
  int m_ControlId;
  GUIHANDLE m_ProgressHandle;
  AddonCB* m_Handle;
  KodiAPI::V1::GUI::CB_GUILib* m_cb;
};

class CAddonGUISliderControl
{
public:
  CAddonGUISliderControl(AddonCB* hdl, KodiAPI::V1::GUI::CB_GUILib* cb, CAddonGUIWindow *window, int controlId)
    : m_Window(window)
    , m_ControlId(controlId)
    , m_Handle(hdl)
    , m_cb(cb)
  {
    m_SliderHandle = m_cb->Window_GetControl_Slider(m_Handle->addonData, m_Window->m_WindowHandle, controlId);
  }

  ~CAddonGUISliderControl(void) {}

  void SetVisible(bool yesNo)
  {
    if (m_SliderHandle)
      m_cb->Control_Slider_SetVisible(m_Handle->addonData, m_SliderHandle, yesNo);
  }

  std::string GetDescription() const
  {
    if (!m_SliderHandle)
      return "";

    return m_cb->Control_Slider_GetDescription(m_Handle->addonData, m_SliderHandle);
  }

  void SetIntRange(int iStart, int iEnd)
  {
    if (m_SliderHandle)
      m_cb->Control_Slider_SetIntRange(m_Handle->addonData, m_SliderHandle, iStart, iEnd);
  }

  void SetIntValue(int iValue)
  {
    if (m_SliderHandle)
      m_cb->Control_Slider_SetIntValue(m_Handle->addonData, m_SliderHandle, iValue);
  }

  int GetIntValue() const
  {
    if (!m_SliderHandle)
      return 0;
    return m_cb->Control_Slider_GetIntValue(m_Handle->addonData, m_SliderHandle);
  }

  void SetIntInterval(int iInterval)
  {
    if (m_SliderHandle)
      m_cb->Control_Slider_SetIntInterval(m_Handle->addonData, m_SliderHandle, iInterval);
  }

  void SetPercentage(float fPercent)
  {
    if (m_SliderHandle)
      m_cb->Control_Slider_SetPercentage(m_Handle->addonData, m_SliderHandle, fPercent);
  }

  float GetPercentage() const
  {
    if (!m_SliderHandle)
      return 0.0f;

    return m_cb->Control_Slider_GetPercentage(m_Handle->addonData, m_SliderHandle);
  }

  void SetFloatRange(float fStart, float fEnd)
  {
    if (m_SliderHandle)
      m_cb->Control_Slider_SetFloatRange(m_Handle->addonData, m_SliderHandle, fStart, fEnd);
  }

  void SetFloatValue(float fValue)
  {
    if (m_SliderHandle)
      m_cb->Control_Slider_SetFloatValue(m_Handle->addonData, m_SliderHandle, fValue);
  }

  float GetFloatValue() const
  {
    if (!m_SliderHandle)
      return 0.0f;
    return m_cb->Control_Slider_GetFloatValue(m_Handle->addonData, m_SliderHandle);
  }

  void SetFloatInterval(float fInterval)
  {
    if (m_SliderHandle)
      m_cb->Control_Slider_SetFloatInterval(m_Handle->addonData, m_SliderHandle, fInterval);
  }

private:
  CAddonGUIWindow* m_Window;
  int m_ControlId;
  GUIHANDLE m_SliderHandle;
  AddonCB* m_Handle;
  KodiAPI::V1::GUI::CB_GUILib* m_cb;
};

class CAddonGUISettingsSliderControl
{
public:
  CAddonGUISettingsSliderControl(AddonCB* hdl, KodiAPI::V1::GUI::CB_GUILib* cb, CAddonGUIWindow *window, int controlId)
    : m_Window(window)
    , m_ControlId(controlId)
    , m_Handle(hdl)
    , m_cb(cb)
  {
    m_SettingsSliderHandle = m_cb->Window_GetControl_SettingsSlider(m_Handle->addonData, m_Window->m_WindowHandle, controlId);
  }
  
  ~CAddonGUISettingsSliderControl(void) {}

  void SetVisible(bool yesNo)
  {
    if (m_SettingsSliderHandle)
      m_cb->Control_SettingsSlider_SetVisible(m_Handle->addonData, m_SettingsSliderHandle, yesNo);
  }

  void SetText(const char *label)
  {
    if (m_SettingsSliderHandle)
      m_cb->Control_SettingsSlider_SetText(m_Handle->addonData, m_SettingsSliderHandle, label);
  }

  std::string GetDescription() const
  {
    if (!m_SettingsSliderHandle)
      return "";

    return m_cb->Control_SettingsSlider_GetDescription(m_Handle->addonData, m_SettingsSliderHandle);
  }

  void SetIntRange(int iStart, int iEnd)
  {
    if (m_SettingsSliderHandle)
      m_cb->Control_SettingsSlider_SetIntRange(m_Handle->addonData, m_SettingsSliderHandle, iStart, iEnd);
  }

  void SetIntValue(int iValue)
  {
    if (m_SettingsSliderHandle)
      m_cb->Control_SettingsSlider_SetIntValue(m_Handle->addonData, m_SettingsSliderHandle, iValue);
  }

  int GetIntValue() const
  {
    if (!m_SettingsSliderHandle)
      return 0;
    return m_cb->Control_SettingsSlider_GetIntValue(m_Handle->addonData, m_SettingsSliderHandle);
  }

  void SetIntInterval(int iInterval)
  {
    if (m_SettingsSliderHandle)
      m_cb->Control_SettingsSlider_SetIntInterval(m_Handle->addonData, m_SettingsSliderHandle, iInterval);
  }

  void SetPercentage(float fPercent)
  {
    if (m_SettingsSliderHandle)
      m_cb->Control_SettingsSlider_SetPercentage(m_Handle->addonData, m_SettingsSliderHandle, fPercent);
  }

  float GetPercentage() const
  {
    if (!m_SettingsSliderHandle)
      return 0.0f;

    return m_cb->Control_SettingsSlider_GetPercentage(m_Handle->addonData, m_SettingsSliderHandle);
  }

  void SetFloatRange(float fStart, float fEnd)
  {
    if (m_SettingsSliderHandle)
      m_cb->Control_SettingsSlider_SetFloatRange(m_Handle->addonData, m_SettingsSliderHandle, fStart, fEnd);
  }

  void SetFloatValue(float fValue)
  {
    if (m_SettingsSliderHandle)
      m_cb->Control_SettingsSlider_SetFloatValue(m_Handle->addonData, m_SettingsSliderHandle, fValue);
  }

  float GetFloatValue() const
  {
    if (!m_SettingsSliderHandle)
      return 0.0f;
    return m_cb->Control_SettingsSlider_GetFloatValue(m_Handle->addonData, m_SettingsSliderHandle);
  }

  void SetFloatInterval(float fInterval)
  {
    if (m_SettingsSliderHandle)
      m_cb->Control_SettingsSlider_SetFloatInterval(m_Handle->addonData, m_SettingsSliderHandle, fInterval);
  }

private:
  CAddonGUIWindow* m_Window;
  int m_ControlId;
  GUIHANDLE m_SettingsSliderHandle;
  AddonCB* m_Handle;
  KodiAPI::V1::GUI::CB_GUILib* m_cb;
};

class CAddonGUIRenderingControl
{
public:
  CAddonGUIRenderingControl(AddonCB* hdl, KodiAPI::V1::GUI::CB_GUILib* cb, CAddonGUIWindow *window, int controlId)
    : m_Window(window)
    , m_ControlId(controlId)
    , m_Handle(hdl)
    , m_cb(cb)
    , m_cbhdl(nullptr)
    , CBCreate(nullptr)
    , CBRender(nullptr)
    , CBStop(nullptr)
    , CBDirty(nullptr)
  {
    m_RenderingHandle = m_cb->Window_GetControl_RenderAddon(m_Handle->addonData, m_Window->m_WindowHandle, controlId);
  }
  virtual ~CAddonGUIRenderingControl()
  {
    m_cb->RenderAddon_Delete(m_Handle->addonData, m_RenderingHandle);
  }

  void Init()
  {
    m_cb->RenderAddon_SetCallbacks(m_Handle->addonData, m_RenderingHandle, this, OnCreateCB, OnRenderCB, OnStopCB, OnDirtyCB);
  }

  bool Create(int x, int y, int w, int h, void *device)
  {
    if (!CBCreate)
      return false;

    return CBCreate(m_cbhdl, x, y, w, h, device);
  }

  void Render()
  {
    if (!CBRender)
      return;

    CBRender(m_cbhdl);
  }

  void Stop()
  {
    if (!CBStop)
      return;

    CBStop(m_cbhdl);
  }

  bool Dirty()
  {
    if (!CBDirty)
      return true;

    return CBDirty(m_cbhdl);
  }

  GUIHANDLE m_cbhdl;
  bool (*CBCreate)(GUIHANDLE cbhdl, int x, int y, int w, int h, void *device);
  void (*CBRender)(GUIHANDLE cbhdl);
  void (*CBStop)(GUIHANDLE cbhdl);
  bool (*CBDirty)(GUIHANDLE cbhdl);

private:
  CAddonGUIWindow* m_Window;
  int m_ControlId;
  GUIHANDLE m_RenderingHandle;
  AddonCB* m_Handle;
  KodiAPI::V1::GUI::CB_GUILib* m_cb;

  static bool OnCreateCB(GUIHANDLE cbhdl, int x, int y, int w, int h, void* device);
  static void OnRenderCB(GUIHANDLE cbhdl);
  static void OnStopCB(GUIHANDLE cbhdl);
  static bool OnDirtyCB(GUIHANDLE cbhdl);
};

inline bool CAddonGUIRenderingControl::OnCreateCB(GUIHANDLE cbhdl, int x, int y, int w, int h, void* device)
{
  return static_cast<CAddonGUIRenderingControl*>(cbhdl)->Create(x, y, w, h, device);
}

inline void CAddonGUIRenderingControl::OnRenderCB(GUIHANDLE cbhdl)
{
  static_cast<CAddonGUIRenderingControl*>(cbhdl)->Render();
}

inline void CAddonGUIRenderingControl::OnStopCB(GUIHANDLE cbhdl)
{
  static_cast<CAddonGUIRenderingControl*>(cbhdl)->Stop();
}

inline bool CAddonGUIRenderingControl::OnDirtyCB(GUIHANDLE cbhdl)
{
  return static_cast<CAddonGUIRenderingControl*>(cbhdl)->Dirty();
}
  
class CHelper_libKODI_guilib
{
public:
  CHelper_libKODI_guilib()
  {
    m_Handle = nullptr;
    m_Callbacks = nullptr;
  }

  ~CHelper_libKODI_guilib()
  {
    if (m_Handle && m_Callbacks)
    {
      m_Handle->GUILib_UnRegisterMe(m_Handle->addonData, m_Callbacks);
    }
  }

  bool RegisterMe(void *handle)
  {
    m_Handle = static_cast<AddonCB*>(handle);
    if (m_Handle)
      m_Callbacks = (KodiAPI::V1::GUI::CB_GUILib*)m_Handle->GUILib_RegisterMe(m_Handle->addonData);
    if (!m_Callbacks)
      fprintf(stderr, "libKODI_guilib-ERROR: GUILib_RegisterMe can't get callback table from Kodi !!!\n");
  
    return m_Callbacks != nullptr;
  }

  void Lock()
  {
    m_Callbacks->Lock();
  }

  void Unlock()
  {
    m_Callbacks->Unlock();
  }

  int GetScreenHeight()
  {
    return m_Callbacks->GetScreenHeight();
  }

  int GetScreenWidth()
  {
    return m_Callbacks->GetScreenWidth();
  }

  int GetVideoResolution()
  {
    return m_Callbacks->GetVideoResolution();
  }

  CAddonGUIWindow* Window_create(const char *xmlFilename, const char *defaultSkin, bool forceFallback, bool asDialog)
  {
    return new CAddonGUIWindow(m_Handle, m_Callbacks, xmlFilename, defaultSkin, forceFallback, asDialog);
  }

  void Window_destroy(CAddonGUIWindow* p)
  {
    delete p;
  }

  CAddonGUISpinControl* Control_getSpin(CAddonGUIWindow *window, int controlId)
  {
    return new CAddonGUISpinControl(m_Handle, m_Callbacks, window, controlId);
  }

  void Control_releaseSpin(CAddonGUISpinControl* p)
  {
    delete p;
  }

  CAddonGUIRadioButton* Control_getRadioButton(CAddonGUIWindow *window, int controlId)
  {
    return new CAddonGUIRadioButton(m_Handle, m_Callbacks, window, controlId);
  }

  void Control_releaseRadioButton(CAddonGUIRadioButton* p)
  {
    delete p;
  }

  CAddonGUIProgressControl* Control_getProgress(CAddonGUIWindow *window, int controlId)
  {
    return new CAddonGUIProgressControl(m_Handle, m_Callbacks, window, controlId);
  }

  void Control_releaseProgress(CAddonGUIProgressControl* p)
  {
    delete p;
  }

  CAddonListItem* ListItem_create(const char *label, const char *label2, const char *iconImage, const char *thumbnailImage, const char *path)
  {
    return new CAddonListItem(m_Handle, m_Callbacks, label, label2, iconImage, thumbnailImage, path);
  }

  void ListItem_destroy(CAddonListItem* p)
  {
    delete p;
  }

  CAddonGUIRenderingControl* Control_getRendering(CAddonGUIWindow *window, int controlId)
  {
    return new CAddonGUIRenderingControl(m_Handle, m_Callbacks, window, controlId);
  }

  void Control_releaseRendering(CAddonGUIRenderingControl* p)
  {
    delete p;
  }

  CAddonGUISliderControl* Control_getSlider(CAddonGUIWindow *window, int controlId)
  {
    return new CAddonGUISliderControl(m_Handle, m_Callbacks, window, controlId);
  }

  void Control_releaseSlider(CAddonGUISliderControl* p)
  {
    delete p;
  }

  CAddonGUISettingsSliderControl* Control_getSettingsSlider(CAddonGUIWindow *window, int controlId)
  {
    return new CAddonGUISettingsSliderControl(m_Handle, m_Callbacks, window, controlId);
  }

  void Control_releaseSettingsSlider(CAddonGUISettingsSliderControl* p)
  {
    delete p;
  }

  /*! @name GUI Keyboard functions */
  //@{
  bool Dialog_Keyboard_ShowAndGetInput(char &strText, unsigned int iMaxStringSize, const char *strHeading, bool allowEmptyResult, bool hiddenInput, unsigned int autoCloseMs = 0)
  {
    return m_Callbacks->Dialog_Keyboard_ShowAndGetInputWithHead(strText, iMaxStringSize, strHeading, allowEmptyResult, hiddenInput, autoCloseMs);
  }

  bool Dialog_Keyboard_ShowAndGetInput(char &strText, unsigned int iMaxStringSize, bool allowEmptyResult, unsigned int autoCloseMs = 0)
  {
    return m_Callbacks->Dialog_Keyboard_ShowAndGetInput(strText, iMaxStringSize, allowEmptyResult, autoCloseMs);
  }

  bool Dialog_Keyboard_ShowAndGetNewPassword(char &strNewPassword, unsigned int iMaxStringSize, const char *strHeading, bool allowEmptyResult, unsigned int autoCloseMs = 0)
  {
    return m_Callbacks->Dialog_Keyboard_ShowAndGetNewPasswordWithHead(strNewPassword, iMaxStringSize, strHeading, allowEmptyResult, autoCloseMs);
  }

  bool Dialog_Keyboard_ShowAndGetNewPassword(char &strNewPassword, unsigned int iMaxStringSize, unsigned int autoCloseMs = 0)
  {
    return m_Callbacks->Dialog_Keyboard_ShowAndGetNewPassword(strNewPassword, iMaxStringSize, autoCloseMs);
  }

  bool Dialog_Keyboard_ShowAndVerifyNewPassword(char &strNewPassword, unsigned int iMaxStringSize, const char *strHeading, bool allowEmptyResult, unsigned int autoCloseMs = 0)
  {
    return m_Callbacks->Dialog_Keyboard_ShowAndVerifyNewPasswordWithHead(strNewPassword, iMaxStringSize, strHeading, allowEmptyResult, autoCloseMs);
  }

  bool Dialog_Keyboard_ShowAndVerifyNewPassword(char &strNewPassword, unsigned int iMaxStringSize, unsigned int autoCloseMs = 0)
  {
    return m_Callbacks->Dialog_Keyboard_ShowAndVerifyNewPassword(strNewPassword, iMaxStringSize, autoCloseMs);
  }

  int Dialog_Keyboard_ShowAndVerifyPassword(char &strPassword, unsigned int iMaxStringSize, const char *strHeading, int iRetries, unsigned int autoCloseMs = 0)
  {
    return m_Callbacks->Dialog_Keyboard_ShowAndVerifyPassword(strPassword, iMaxStringSize, strHeading, iRetries, autoCloseMs);
  }

  bool Dialog_Keyboard_ShowAndGetFilter(char &strText, unsigned int iMaxStringSize, bool searching, unsigned int autoCloseMs = 0)
  {
    return m_Callbacks->Dialog_Keyboard_ShowAndGetFilter(strText, iMaxStringSize, searching, autoCloseMs);
  }

  bool Dialog_Keyboard_SendTextToActiveKeyboard(const char *aTextString, bool closeKeyboard = false)
  {
    return m_Callbacks->Dialog_Keyboard_SendTextToActiveKeyboard(aTextString, closeKeyboard);
  }

  bool Dialog_Keyboard_isKeyboardActivated()
  {
    return m_Callbacks->Dialog_Keyboard_isKeyboardActivated();
  }
  //@}

  /*! @name GUI Numeric functions */
  //@{
  bool Dialog_Numeric_ShowAndVerifyNewPassword(char &strNewPassword, unsigned int iMaxStringSize)
  {
    return m_Callbacks->Dialog_Numeric_ShowAndVerifyNewPassword(strNewPassword, iMaxStringSize);
  }

  int Dialog_Numeric_ShowAndVerifyPassword(char &strPassword, unsigned int iMaxStringSize, const char *strHeading, int iRetries)
  {
    return m_Callbacks->Dialog_Numeric_ShowAndVerifyPassword(strPassword, iMaxStringSize, strHeading, iRetries);
  }

  bool Dialog_Numeric_ShowAndVerifyInput(char &strPassword, unsigned int iMaxStringSize, const char *strHeading, bool bGetUserInput)
  {
    return m_Callbacks->Dialog_Numeric_ShowAndVerifyInput(strPassword, iMaxStringSize, strHeading, bGetUserInput);
  }

  bool Dialog_Numeric_ShowAndGetTime(tm &time, const char *strHeading)
  {
    return m_Callbacks->Dialog_Numeric_ShowAndGetTime(time, strHeading);
  }

  bool Dialog_Numeric_ShowAndGetDate(tm &date, const char *strHeading)
  {
    return m_Callbacks->Dialog_Numeric_ShowAndGetDate(date, strHeading);
  }

  bool Dialog_Numeric_ShowAndGetIPAddress(char &strIPAddress, unsigned int iMaxStringSize, const char *strHeading)
  {
    return m_Callbacks->Dialog_Numeric_ShowAndGetIPAddress(strIPAddress, iMaxStringSize, strHeading);
  }

  bool Dialog_Numeric_ShowAndGetNumber(char &strInput, unsigned int iMaxStringSize, const char *strHeading, unsigned int iAutoCloseTimeoutMs = 0)
  {
    return m_Callbacks->Dialog_Numeric_ShowAndGetNumber(strInput, iMaxStringSize, strHeading, iAutoCloseTimeoutMs);
  }

  bool Dialog_Numeric_ShowAndGetSeconds(char &strTime, unsigned int iMaxStringSize, const char *strHeading)
  {
    return m_Callbacks->Dialog_Numeric_ShowAndGetSeconds(strTime, iMaxStringSize, strHeading);
  }
  //@}

  /*! @name GUI File browser functions */
  //@{
  bool Dialog_FileBrowser_ShowAndGetFile(const char *directory, const char *mask, const char *heading, char &strPath, unsigned int iMaxStringSize, bool useThumbs = false, bool useFileDirectories = false, bool singleList = false)
  {
    return m_Callbacks->Dialog_FileBrowser_ShowAndGetFile(directory, mask, heading, strPath, iMaxStringSize, useThumbs, useFileDirectories, singleList);
  }
  //@}

  /*! @name GUI OK Dialog functions */
  //@{
  void Dialog_OK_ShowAndGetInput(const char *heading, const char *text)
  {
    return m_Callbacks->Dialog_OK_ShowAndGetInputSingleText(heading, text);
  }

  void Dialog_OK_ShowAndGetInput(const char *heading, const char *line0, const char *line1, const char *line2)
  {
    return m_Callbacks->Dialog_OK_ShowAndGetInputLineText(heading, line0, line1, line2);
  }
  //@}

  /*! @name GUI Yes No Dialog functions */
  //@{
  bool Dialog_YesNo_ShowAndGetInput(const char *heading, const char *text, bool& bCanceled, const char *noLabel = "", const char *yesLabel = "")
  {
    return m_Callbacks->Dialog_YesNo_ShowAndGetInputSingleText(heading, text, bCanceled, noLabel, yesLabel);
  }

  bool Dialog_YesNo_ShowAndGetInput(const char *heading, const char *line0, const char *line1, const char *line2, const char *noLabel = "", const char *yesLabel = "")
  {
    return m_Callbacks->Dialog_YesNo_ShowAndGetInputLineText(heading, line0, line1, line2, noLabel, yesLabel);
  }

  bool Dialog_YesNo_ShowAndGetInput(const char *heading, const char *line0, const char *line1, const char *line2, bool &bCanceled, const char *noLabel = "", const char *yesLabel = "")
  {
    return m_Callbacks->Dialog_YesNo_ShowAndGetInputLineButtonText(heading, line0, line1, line2, bCanceled, noLabel, yesLabel);
  }
  //@}

  /*! @name GUI Text viewer Dialog */
  //@{
  void Dialog_TextViewer(const char *heading, const char *text)
  {
    return m_Callbacks->Dialog_TextViewer(heading, text);
  }
  //@}

  /*! @name GUI select Dialog */
  //@{
  int Dialog_Select(const char *heading, const char *entries[], unsigned int size, int selected = -1)
  {
    return m_Callbacks->Dialog_Select(heading, entries, size, selected);
  }
  //@}

private:
  AddonCB* m_Handle;
  KodiAPI::V1::GUI::CB_GUILib* m_Callbacks;
};
