/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/interfaces/AddonInterfaces.h"
#include "addons/kodi-addon-dev-kit/include/kodi/libKODI_guilib.h"

namespace ADDON
{
  class CAddon;
}

namespace KodiAPI
{
namespace GUI
{

class CAddonCallbacksGUI
{
public:
  explicit CAddonCallbacksGUI(ADDON::CAddon* addon);
  virtual ~CAddonCallbacksGUI();

  /**! \name General Functions */
  CB_GUILib *GetCallbacks() { return m_callbacks; }

  static void         Lock();
  static void         Unlock();
  static int          GetScreenHeight();
  static int          GetScreenWidth();
  static int          GetVideoResolution();

  static GUIHANDLE    Window_New(void *addonData, const char *xmlFilename, const char *defaultSkin, bool forceFallback, bool asDialog);
  static void         Window_Delete(void *addonData, GUIHANDLE handle);
  static void         Window_SetCallbacks(void *addonData, GUIHANDLE handle, GUIHANDLE clienthandle, bool (*initCB)(GUIHANDLE), bool (*clickCB)(GUIHANDLE, int), bool (*focusCB)(GUIHANDLE, int), bool (*onActionCB)(GUIHANDLE handle, int));
  static bool         Window_Show(void *addonData, GUIHANDLE handle);
  static bool         Window_Close(void *addonData, GUIHANDLE handle);
  static bool         Window_DoModal(void *addonData, GUIHANDLE handle);
  static bool         Window_SetFocusId(void *addonData, GUIHANDLE handle, int iControlId);
  static int          Window_GetFocusId(void *addonData, GUIHANDLE handle);
  static bool         Window_SetCoordinateResolution(void *addonData, GUIHANDLE handle, int res);
  static void         Window_SetProperty(void *addonData, GUIHANDLE handle, const char *key, const char *value);
  static void         Window_SetPropertyInt(void *addonData, GUIHANDLE handle, const char *key, int value);
  static void         Window_SetPropertyBool(void *addonData, GUIHANDLE handle, const char *key, bool value);
  static void         Window_SetPropertyDouble(void *addonData, GUIHANDLE handle, const char *key, double value);
  static const char * Window_GetProperty(void *addonData, GUIHANDLE handle, const char *key);
  static int          Window_GetPropertyInt(void *addonData, GUIHANDLE handle, const char *key);
  static bool         Window_GetPropertyBool(void *addonData, GUIHANDLE handle, const char *key);
  static double       Window_GetPropertyDouble(void *addonData, GUIHANDLE handle, const char *key);
  static void         Window_ClearProperties(void *addonData, GUIHANDLE handle);
  static int          Window_GetListSize(void *addonData, GUIHANDLE handle);
  static void         Window_ClearList(void *addonData, GUIHANDLE handle);
  static GUIHANDLE    Window_AddItem(void *addonData, GUIHANDLE handle, GUIHANDLE item, int itemPosition);
  static GUIHANDLE    Window_AddStringItem(void *addonData, GUIHANDLE handle, const char *itemName, int itemPosition);
  static void         Window_RemoveItem(void *addonData, GUIHANDLE handle, int itemPosition);
  static GUIHANDLE    Window_GetListItem(void *addonData, GUIHANDLE handle, int listPos);
  static void         Window_SetCurrentListPosition(void *addonData, GUIHANDLE handle, int listPos);
  static int          Window_GetCurrentListPosition(void *addonData, GUIHANDLE handle);
  static GUIHANDLE    Window_GetControl_Spin(void *addonData, GUIHANDLE handle, int controlId);
  static GUIHANDLE    Window_GetControl_Button(void *addonData, GUIHANDLE handle, int controlId);
  static GUIHANDLE    Window_GetControl_RadioButton(void *addonData, GUIHANDLE handle, int controlId);
  static GUIHANDLE    Window_GetControl_Edit(void *addonData, GUIHANDLE handle, int controlId);
  static GUIHANDLE    Window_GetControl_Progress(void *addonData, GUIHANDLE handle, int controlId);
  static GUIHANDLE    Window_GetControl_RenderAddon(void *addonData, GUIHANDLE handle, int controlId);
  static void         Window_SetControlLabel(void *addonData, GUIHANDLE handle, int controlId, const char *label);
  static void         Window_MarkDirtyRegion(void *addonData, GUIHANDLE handle);
  static void         Control_Spin_SetVisible(void *addonData, GUIHANDLE spinhandle, bool yesNo);
  static void         Control_Spin_SetText(void *addonData, GUIHANDLE spinhandle, const char *label);
  static void         Control_Spin_Clear(void *addonData, GUIHANDLE spinhandle);
  static void         Control_Spin_AddLabel(void *addonData, GUIHANDLE spinhandle, const char *label, int iValue);
  static int          Control_Spin_GetValue(void *addonData, GUIHANDLE spinhandle);
  static void         Control_Spin_SetValue(void *addonData, GUIHANDLE spinhandle, int iValue);
  static void         Control_RadioButton_SetVisible(void *addonData, GUIHANDLE handle, bool yesNo);
  static void         Control_RadioButton_SetText(void *addonData, GUIHANDLE handle, const char *label);
  static void         Control_RadioButton_SetSelected(void *addonData, GUIHANDLE handle, bool yesNo);
  static bool         Control_RadioButton_IsSelected(void *addonData, GUIHANDLE handle);
  static void         Control_Progress_SetPercentage(void *addonData, GUIHANDLE handle, float fPercent);
  static float        Control_Progress_GetPercentage(void *addonData, GUIHANDLE handle);
  static void         Control_Progress_SetInfo(void *addonData, GUIHANDLE handle, int iInfo);
  static int          Control_Progress_GetInfo(void *addonData, GUIHANDLE handle);
  static const char * Control_Progress_GetDescription(void *addonData, GUIHANDLE handle);

  static GUIHANDLE    Window_GetControl_Slider(void *addonData, GUIHANDLE handle, int controlId);
  static void         Control_Slider_SetVisible(void *addonData, GUIHANDLE handle, bool yesNo);
  static const char * Control_Slider_GetDescription(void *addonData, GUIHANDLE handle);
  static void         Control_Slider_SetIntRange(void *addonData, GUIHANDLE handle, int iStart, int iEnd);
  static void         Control_Slider_SetIntValue(void *addonData, GUIHANDLE handle, int iValue);
  static int          Control_Slider_GetIntValue(void *addonData, GUIHANDLE handle);
  static void         Control_Slider_SetIntInterval(void *addonData, GUIHANDLE handle, int iInterval);
  static void         Control_Slider_SetPercentage(void *addonData, GUIHANDLE handle, float fPercent);
  static float        Control_Slider_GetPercentage(void *addonData, GUIHANDLE handle);
  static void         Control_Slider_SetFloatRange(void *addonData, GUIHANDLE handle, float fStart, float fEnd);
  static void         Control_Slider_SetFloatValue(void *addonData, GUIHANDLE handle, float fValue);
  static float        Control_Slider_GetFloatValue(void *addonData, GUIHANDLE handle);
  static void         Control_Slider_SetFloatInterval(void *addonData, GUIHANDLE handle, float fInterval);

  static GUIHANDLE    Window_GetControl_SettingsSlider(void *addonData, GUIHANDLE handle, int controlId);
  static void         Control_SettingsSlider_SetVisible(void *addonData, GUIHANDLE handle, bool yesNo);
  static void         Control_SettingsSlider_SetText(void *addonData, GUIHANDLE handle, const char *label);
  static const char * Control_SettingsSlider_GetDescription(void *addonData, GUIHANDLE handle);
  static void         Control_SettingsSlider_SetIntRange(void *addonData, GUIHANDLE handle, int iStart, int iEnd);
  static void         Control_SettingsSlider_SetIntValue(void *addonData, GUIHANDLE handle, int iValue);
  static int          Control_SettingsSlider_GetIntValue(void *addonData, GUIHANDLE handle);
  static void         Control_SettingsSlider_SetIntInterval(void *addonData, GUIHANDLE handle, int iInterval);
  static void         Control_SettingsSlider_SetPercentage(void *addonData, GUIHANDLE handle, float fPercent);
  static float        Control_SettingsSlider_GetPercentage(void *addonData, GUIHANDLE handle);
  static void         Control_SettingsSlider_SetFloatRange(void *addonData, GUIHANDLE handle, float fStart, float fEnd);
  static void         Control_SettingsSlider_SetFloatValue(void *addonData, GUIHANDLE handle, float fValue);
  static float        Control_SettingsSlider_GetFloatValue(void *addonData, GUIHANDLE handle);
  static void         Control_SettingsSlider_SetFloatInterval(void *addonData, GUIHANDLE handle, float fInterval);

  static GUIHANDLE    ListItem_Create(void *addonData, const char *label, const char *label2, const char *iconImage, const char *thumbnailImage, const char *path);
  static const char * ListItem_GetLabel(void *addonData, GUIHANDLE handle);
  static void         ListItem_SetLabel(void *addonData, GUIHANDLE handle, const char *label);
  static const char * ListItem_GetLabel2(void *addonData, GUIHANDLE handle);
  static void         ListItem_SetLabel2(void *addonData, GUIHANDLE handle, const char *label);
  static void         ListItem_SetIconImage(void *addonData, GUIHANDLE handle, const char *image);
  static void         ListItem_SetThumbnailImage(void *addonData, GUIHANDLE handle, const char *image);
  static void         ListItem_SetInfo(void *addonData, GUIHANDLE handle, const char *info);
  static void         ListItem_SetProperty(void *addonData, GUIHANDLE handle, const char *key, const char *value);
  static const char * ListItem_GetProperty(void *addonData, GUIHANDLE handle, const char *key);
  static void         ListItem_SetPath(void *addonData, GUIHANDLE handle, const char *path);
  static void         RenderAddon_SetCallbacks(void *addonData, GUIHANDLE handle, GUIHANDLE clienthandle, bool (*createCB)(GUIHANDLE,int,int,int,int,void*), void (*renderCB)(GUIHANDLE), void (*stopCB)(GUIHANDLE), bool (*dirtyCB)(GUIHANDLE));
  static void         RenderAddon_Delete(void *addonData, GUIHANDLE handle);
  static void         RenderAddon_MarkDirty(void *addonData, GUIHANDLE handle);

  static bool         Dialog_Keyboard_ShowAndGetInput(char &aTextString, unsigned int iMaxStringSize, bool allowEmptyResult, unsigned int autoCloseMs);
  static bool         Dialog_Keyboard_ShowAndGetInputWithHead(char &aTextString, unsigned int iMaxStringSize, const char *heading, bool allowEmptyResult, bool hiddenInput, unsigned int autoCloseMs);
  static bool         Dialog_Keyboard_ShowAndGetNewPassword(char &strNewPassword, unsigned int iMaxStringSize, unsigned int autoCloseMs);
  static bool         Dialog_Keyboard_ShowAndGetNewPasswordWithHead(char &newPassword, unsigned int iMaxStringSize, const char *strHeading, bool allowEmptyResult, unsigned int autoCloseMs);
  static bool         Dialog_Keyboard_ShowAndVerifyNewPassword(char &strNewPassword, unsigned int iMaxStringSize, unsigned int autoCloseMs);
  static bool         Dialog_Keyboard_ShowAndVerifyNewPasswordWithHead(char &strNewPassword, unsigned int iMaxStringSize, const char *strHeading, bool allowEmpty, unsigned int autoCloseMs);
  static int          Dialog_Keyboard_ShowAndVerifyPassword(char &strPassword, unsigned int iMaxStringSize, const char *strHeading, int iRetries, unsigned int autoCloseMs);
  static bool         Dialog_Keyboard_ShowAndGetFilter(char &aTextString, unsigned int iMaxStringSize, bool searching, unsigned int autoCloseMs);
  static bool         Dialog_Keyboard_SendTextToActiveKeyboard(const char *aTextString, bool closeKeyboard);
  static bool         Dialog_Keyboard_isKeyboardActivated();

  static bool         Dialog_Numeric_ShowAndVerifyNewPassword(char &strNewPasswor, unsigned int iMaxStringSized);
  static int          Dialog_Numeric_ShowAndVerifyPassword(char &strPassword, unsigned int iMaxStringSize, const char *strHeading, int iRetries);
  static bool         Dialog_Numeric_ShowAndVerifyInput(char &strPassword, unsigned int iMaxStringSize, const char *strHeading, bool bGetUserInput);
  static bool         Dialog_Numeric_ShowAndGetTime(tm &time, const char *strHeading);
  static bool         Dialog_Numeric_ShowAndGetDate(tm &date, const char *strHeading);
  static bool         Dialog_Numeric_ShowAndGetIPAddress(char &strIPAddress, unsigned int iMaxStringSize, const char *strHeading);
  static bool         Dialog_Numeric_ShowAndGetNumber(char &strInput, unsigned int iMaxStringSize, const char *strHeading, unsigned int iAutoCloseTimeoutMs);
  static bool         Dialog_Numeric_ShowAndGetSeconds(char &timeString, unsigned int iMaxStringSize, const char *strHeading);

  static bool         Dialog_FileBrowser_ShowAndGetFile(const char *directory, const char *mask, const char *heading, char &path, unsigned int iMaxStringSize, bool useThumbs, bool useFileDirectories, bool singleList);

  static void         Dialog_OK_ShowAndGetInputSingleText(const char *heading, const char *text);
  static void         Dialog_OK_ShowAndGetInputLineText(const char *heading, const char *line0, const char *line1, const char *line2);

  static bool         Dialog_YesNo_ShowAndGetInputSingleText(const char *heading, const char *text, bool& bCanceled, const char *noLabel, const char *yesLabel);
  static bool         Dialog_YesNo_ShowAndGetInputLineText(const char *heading, const char *line0, const char *line1, const char *line2, const char *noLabel, const char *yesLabel);
  static bool         Dialog_YesNo_ShowAndGetInputLineButtonText(const char *heading, const char *line0, const char *line1, const char *line2, bool &bCanceled, const char *noLabel, const char *yesLabel);

  static void         Dialog_TextViewer(const char *heading, const char *text);
  static int          Dialog_Select(const char *heading, const char *entries[], unsigned int size, int selected);

private:
  ADDON::CAddon* m_addon; /*!< the addon */
  CB_GUILib    *m_callbacks;
};

} /* namespace GUI */
} /* namespace KodiAPI */
