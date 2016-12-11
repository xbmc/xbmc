#pragma once
/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      Copyright (C) 2015-2016 Team KODI
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

#include "addons/binary/interfaces/AddonInterfaces.h"
#include "addons/kodi-addon-dev-kit/include/kodi/libKODI_guilib.h"

namespace ADDON
{
  class CAddon;
}

namespace KodiAPI
{
namespace V1
{
namespace GUI
{

typedef void (*GUILock)();
typedef void (*GUIUnlock)();
typedef int (*GUIGetScreenHeight)();
typedef int (*GUIGetScreenWidth)();
typedef int (*GUIGetVideoResolution)();
typedef GUIHANDLE   (*GUIWindow_New)(void *addonData, const char *xmlFilename, const char *defaultSkin, bool forceFallback, bool asDialog);
typedef void        (*GUIWindow_Delete)(void *addonData, GUIHANDLE handle);
typedef void        (*GUIWindow_SetCallbacks)(void *addonData, GUIHANDLE handle, GUIHANDLE clienthandle, bool (*)(GUIHANDLE handle), bool (*)(GUIHANDLE handle, int), bool (*)(GUIHANDLE handle, int), bool (*)(GUIHANDLE handle, int));
typedef bool        (*GUIWindow_Show)(void *addonData, GUIHANDLE handle);
typedef bool        (*GUIWindow_Close)(void *addonData, GUIHANDLE handle);
typedef bool        (*GUIWindow_DoModal)(void *addonData, GUIHANDLE handle);
typedef bool        (*GUIWindow_SetFocusId)(void *addonData, GUIHANDLE handle, int iControlId);
typedef int         (*GUIWindow_GetFocusId)(void *addonData, GUIHANDLE handle);
typedef bool        (*GUIWindow_SetCoordinateResolution)(void *addonData, GUIHANDLE handle, int res);
typedef void        (*GUIWindow_SetProperty)(void *addonData, GUIHANDLE handle, const char *key, const char *value);
typedef void        (*GUIWindow_SetPropertyInt)(void *addonData, GUIHANDLE handle, const char *key, int value);
typedef void        (*GUIWindow_SetPropertyBool)(void *addonData, GUIHANDLE handle, const char *key, bool value);
typedef void        (*GUIWindow_SetPropertyDouble)(void *addonData, GUIHANDLE handle, const char *key, double value);
typedef const char* (*GUIWindow_GetProperty)(void *addonData, GUIHANDLE handle, const char *key);
typedef int         (*GUIWindow_GetPropertyInt)(void *addonData, GUIHANDLE handle, const char *key);
typedef bool        (*GUIWindow_GetPropertyBool)(void *addonData, GUIHANDLE handle, const char *key);
typedef double      (*GUIWindow_GetPropertyDouble)(void *addonData, GUIHANDLE handle, const char *key);
typedef void        (*GUIWindow_ClearProperties)(void *addonData, GUIHANDLE handle);
typedef int         (*GUIWindow_GetListSize)(void *addonData, GUIHANDLE handle);
typedef void        (*GUIWindow_ClearList)(void *addonData, GUIHANDLE handle);
typedef GUIHANDLE   (*GUIWindow_AddItem)(void *addonData, GUIHANDLE handle, GUIHANDLE item, int itemPosition);
typedef GUIHANDLE   (*GUIWindow_AddStringItem)(void *addonData, GUIHANDLE handle, const char *itemName, int itemPosition);
typedef void        (*GUIWindow_RemoveItem)(void *addonData, GUIHANDLE handle, int itemPosition);
typedef GUIHANDLE   (*GUIWindow_GetListItem)(void *addonData, GUIHANDLE handle, int listPos);
typedef void        (*GUIWindow_SetCurrentListPosition)(void *addonData, GUIHANDLE handle, int listPos);
typedef int         (*GUIWindow_GetCurrentListPosition)(void *addonData, GUIHANDLE handle);
typedef GUIHANDLE   (*GUIWindow_GetControl_Spin)(void *addonData, GUIHANDLE handle, int controlId);
typedef GUIHANDLE   (*GUIWindow_GetControl_Button)(void *addonData, GUIHANDLE handle, int controlId);
typedef GUIHANDLE   (*GUIWindow_GetControl_RadioButton)(void *addonData, GUIHANDLE handle, int controlId);
typedef GUIHANDLE   (*GUIWindow_GetControl_Edit)(void *addonData, GUIHANDLE handle, int controlId);
typedef GUIHANDLE   (*GUIWindow_GetControl_Progress)(void *addonData, GUIHANDLE handle, int controlId);
typedef GUIHANDLE   (*GUIWindow_GetControl_RenderAddon)(void *addonData, GUIHANDLE handle, int controlId);
typedef void        (*GUIWindow_SetControlLabel)(void *addonData, GUIHANDLE handle, int controlId, const char *label);
typedef void        (*GUIWindow_MarkDirtyRegion)(void *addonData, GUIHANDLE handle);
typedef void        (*GUIControl_Spin_SetVisible)(void *addonData, GUIHANDLE spinhandle, bool yesNo);
typedef void        (*GUIControl_Spin_SetText)(void *addonData, GUIHANDLE spinhandle, const char *label);
typedef void        (*GUIControl_Spin_Clear)(void *addonData, GUIHANDLE spinhandle);
typedef void        (*GUIControl_Spin_AddLabel)(void *addonData, GUIHANDLE spinhandle, const char *label, int iValue);
typedef int         (*GUIControl_Spin_GetValue)(void *addonData, GUIHANDLE spinhandle);
typedef void        (*GUIControl_Spin_SetValue)(void *addonData, GUIHANDLE spinhandle, int iValue);
typedef void        (*GUIControl_RadioButton_SetVisible)(void *addonData, GUIHANDLE handle, bool yesNo);
typedef void        (*GUIControl_RadioButton_SetText)(void *addonData, GUIHANDLE handle, const char *label);
typedef void        (*GUIControl_RadioButton_SetSelected)(void *addonData, GUIHANDLE handle, bool yesNo);
typedef bool        (*GUIControl_RadioButton_IsSelected)(void *addonData, GUIHANDLE handle);
typedef void        (*GUIControl_Progress_SetPercentage)(void *addonData, GUIHANDLE handle, float fPercent);
typedef float       (*GUIControl_Progress_GetPercentage)(void *addonData, GUIHANDLE handle);
typedef void        (*GUIControl_Progress_SetInfo)(void *addonData, GUIHANDLE handle, int iInfo);
typedef int         (*GUIControl_Progress_GetInfo)(void *addonData, GUIHANDLE handle);
typedef const char* (*GUIControl_Progress_GetDescription)(void *addonData, GUIHANDLE handle);
typedef GUIHANDLE   (*GUIWindow_GetControl_Slider)(void *addonData, GUIHANDLE handle, int controlId);
typedef void        (*GUIControl_Slider_SetVisible)(void *addonData, GUIHANDLE handle, bool yesNo);
typedef const char *(*GUIControl_Slider_GetDescription)(void *addonData, GUIHANDLE handle);
typedef void        (*GUIControl_Slider_SetIntRange)(void *addonData, GUIHANDLE handle, int iStart, int iEnd);
typedef void        (*GUIControl_Slider_SetIntValue)(void *addonData, GUIHANDLE handle, int iValue);
typedef int         (*GUIControl_Slider_GetIntValue)(void *addonData, GUIHANDLE handle);
typedef void        (*GUIControl_Slider_SetIntInterval)(void *addonData, GUIHANDLE handle, int iInterval);
typedef void        (*GUIControl_Slider_SetPercentage)(void *addonData, GUIHANDLE handle, float fPercent);
typedef float       (*GUIControl_Slider_GetPercentage)(void *addonData, GUIHANDLE handle);
typedef void        (*GUIControl_Slider_SetFloatRange)(void *addonData, GUIHANDLE handle, float fStart, float fEnd);
typedef void        (*GUIControl_Slider_SetFloatValue)(void *addonData, GUIHANDLE handle, float fValue);
typedef float       (*GUIControl_Slider_GetFloatValue)(void *addonData, GUIHANDLE handle);
typedef void        (*GUIControl_Slider_SetFloatInterval)(void *addonData, GUIHANDLE handle, float fInterval);
typedef GUIHANDLE   (*GUIWindow_GetControl_SettingsSlider)(void *addonData, GUIHANDLE handle, int controlId);
typedef void        (*GUIControl_SettingsSlider_SetVisible)(void *addonData, GUIHANDLE handle, bool yesNo);
typedef void        (*GUIControl_SettingsSlider_SetText)(void *addonData, GUIHANDLE handle, const char *label);
typedef const char *(*GUIControl_SettingsSlider_GetDescription)(void *addonData, GUIHANDLE handle);
typedef void        (*GUIControl_SettingsSlider_SetIntRange)(void *addonData, GUIHANDLE handle, int iStart, int iEnd);
typedef void        (*GUIControl_SettingsSlider_SetIntValue)(void *addonData, GUIHANDLE handle, int iValue);
typedef int         (*GUIControl_SettingsSlider_GetIntValue)(void *addonData, GUIHANDLE handle);
typedef void        (*GUIControl_SettingsSlider_SetIntInterval)(void *addonData, GUIHANDLE handle, int iInterval);
typedef void        (*GUIControl_SettingsSlider_SetPercentage)(void *addonData, GUIHANDLE handle, float fPercent);
typedef float       (*GUIControl_SettingsSlider_GetPercentage)(void *addonData, GUIHANDLE handle);
typedef void        (*GUIControl_SettingsSlider_SetFloatRange)(void *addonData, GUIHANDLE handle, float fStart, float fEnd);
typedef void        (*GUIControl_SettingsSlider_SetFloatValue)(void *addonData, GUIHANDLE handle, float fValue);
typedef float       (*GUIControl_SettingsSlider_GetFloatValue)(void *addonData, GUIHANDLE handle);
typedef void        (*GUIControl_SettingsSlider_SetFloatInterval)(void *addonData, GUIHANDLE handle, float fInterval);
typedef GUIHANDLE   (*GUIListItem_Create)(void *addonData, const char *label, const char *label2, const char *iconImage, const char *thumbnailImage, const char *path);
typedef const char* (*GUIListItem_GetLabel)(void *addonData, GUIHANDLE handle);
typedef void        (*GUIListItem_SetLabel)(void *addonData, GUIHANDLE handle, const char *label);
typedef const char* (*GUIListItem_GetLabel2)(void *addonData, GUIHANDLE handle);
typedef void        (*GUIListItem_SetLabel2)(void *addonData, GUIHANDLE handle, const char *label);
typedef void        (*GUIListItem_SetIconImage)(void *addonData, GUIHANDLE handle, const char *image);
typedef void        (*GUIListItem_SetThumbnailImage)(void *addonData, GUIHANDLE handle, const char *image);
typedef void        (*GUIListItem_SetInfo)(void *addonData, GUIHANDLE handle, const char *info);
typedef void        (*GUIListItem_SetProperty)(void *addonData, GUIHANDLE handle, const char *key, const char *value);
typedef const char* (*GUIListItem_GetProperty)(void *addonData, GUIHANDLE handle, const char *key);
typedef void        (*GUIListItem_SetPath)(void *addonData, GUIHANDLE handle, const char *path);
typedef void        (*GUIRenderAddon_SetCallbacks)(void *addonData, GUIHANDLE handle, GUIHANDLE clienthandle, bool (*createCB)(GUIHANDLE,int,int,int,int,void*), void (*renderCB)(GUIHANDLE), void (*stopCB)(GUIHANDLE), bool (*dirtyCB)(GUIHANDLE));
typedef void        (*GUIRenderAddon_Delete)(void *addonData, GUIHANDLE handle);
typedef void        (*GUIRenderAddon_MarkDirty)(void *addonData, GUIHANDLE handle);

typedef bool        (*GUIDialog_Keyboard_ShowAndGetInputWithHead)(char &strTextString, unsigned int iMaxStringSize, const char *heading, bool allowEmptyResult, bool hiddenInput, unsigned int autoCloseMs);
typedef bool        (*GUIDialog_Keyboard_ShowAndGetInput)(char &strTextString, unsigned int iMaxStringSize, bool allowEmptyResult, unsigned int autoCloseMs);
typedef bool        (*GUIDialog_Keyboard_ShowAndGetNewPasswordWithHead)(char &newPassword, unsigned int iMaxStringSize, const char *strHeading, bool allowEmptyResult, unsigned int autoCloseMs);
typedef bool        (*GUIDialog_Keyboard_ShowAndGetNewPassword)(char &strNewPassword, unsigned int iMaxStringSize, unsigned int autoCloseMs);
typedef bool        (*GUIDialog_Keyboard_ShowAndVerifyNewPasswordWithHead)(char &strNewPassword, unsigned int iMaxStringSize, const char *strHeading, bool allowEmpty, unsigned int autoCloseMs);
typedef bool        (*GUIDialog_Keyboard_ShowAndVerifyNewPassword)(char &strNewPassword, unsigned int iMaxStringSize, unsigned int autoCloseMs);
typedef int         (*GUIDialog_Keyboard_ShowAndVerifyPassword)(char &strPassword, unsigned int iMaxStringSize, const char *strHeading, int iRetries, unsigned int autoCloseMs);
typedef bool        (*GUIDialog_Keyboard_ShowAndGetFilter)(char &aTextString, unsigned int iMaxStringSize, bool searching, unsigned int autoCloseMs);
typedef bool        (*GUIDialog_Keyboard_SendTextToActiveKeyboard)(const char *aTextString, bool closeKeyboard);
typedef bool        (*GUIDialog_Keyboard_isKeyboardActivated)();

typedef bool        (*GUIDialog_Numeric_ShowAndVerifyNewPassword)(char &strNewPassword, unsigned int iMaxStringSize);
typedef int         (*GUIDialog_Numeric_ShowAndVerifyPassword)(char &strPassword, unsigned int iMaxStringSize, const char *strHeading, int iRetries);
typedef bool        (*GUIDialog_Numeric_ShowAndVerifyInput)(char &strPassword, unsigned int iMaxStringSize, const char *strHeading, bool bGetUserInput);
typedef bool        (*GUIDialog_Numeric_ShowAndGetTime)(tm &time, const char *strHeading);
typedef bool        (*GUIDialog_Numeric_ShowAndGetDate)(tm &date, const char *strHeading);
typedef bool        (*GUIDialog_Numeric_ShowAndGetIPAddress)(char &strIPAddress, unsigned int iMaxStringSize, const char *strHeading);
typedef bool        (*GUIDialog_Numeric_ShowAndGetNumber)(char &strInput, unsigned int iMaxStringSize, const char *strHeading, unsigned int iAutoCloseTimeoutMs);
typedef bool        (*GUIDialog_Numeric_ShowAndGetSeconds)(char &timeString, unsigned int iMaxStringSize, const char *strHeading);

typedef bool        (*GUIDialog_FileBrowser_ShowAndGetFile)(const char *directory, const char *mask, const char *heading, char &path, unsigned int iMaxStringSize, bool useThumbs, bool useFileDirectories, bool singleList);

typedef void        (*GUIDialog_OK_ShowAndGetInputSingleText)(const char *heading, const char *text);
typedef void        (*GUIDialog_OK_ShowAndGetInputLineText)(const char *heading, const char *line0, const char *line1, const char *line2);

typedef bool        (*GUIDialog_YesNo_ShowAndGetInputSingleText)(const char *heading, const char *text, bool& bCanceled, const char *noLabel, const char *yesLabel);
typedef bool        (*GUIDialog_YesNo_ShowAndGetInputLineText)(const char *heading, const char *line0, const char *line1, const char *line2, const char *noLabel, const char *yesLabel);
typedef bool        (*GUIDialog_YesNo_ShowAndGetInputLineButtonText)(const char *heading, const char *line0, const char *line1, const char *line2, bool &bCanceled, const char *noLabel, const char *yesLabel);

typedef void        (*GUIDialog_TextViewer)(const char *heading, const char *text);

typedef int         (*GUIDialog_Select)(const char *heading, const char *entries[], unsigned int size, int selected);

typedef struct CB_GUILib
{
  GUILock                             Lock;
  GUIUnlock                           Unlock;
  GUIGetScreenHeight                  GetScreenHeight;
  GUIGetScreenWidth                   GetScreenWidth;
  GUIGetVideoResolution               GetVideoResolution;
  GUIWindow_New                       Window_New;
  GUIWindow_Delete                    Window_Delete;
  GUIWindow_SetCallbacks              Window_SetCallbacks;
  GUIWindow_Show                      Window_Show;
  GUIWindow_Close                     Window_Close;
  GUIWindow_DoModal                   Window_DoModal;
  GUIWindow_SetFocusId                Window_SetFocusId;
  GUIWindow_GetFocusId                Window_GetFocusId;
  GUIWindow_SetCoordinateResolution   Window_SetCoordinateResolution;
  GUIWindow_SetProperty               Window_SetProperty;
  GUIWindow_SetPropertyInt            Window_SetPropertyInt;
  GUIWindow_SetPropertyBool           Window_SetPropertyBool;
  GUIWindow_SetPropertyDouble         Window_SetPropertyDouble;
  GUIWindow_GetProperty               Window_GetProperty;
  GUIWindow_GetPropertyInt            Window_GetPropertyInt;
  GUIWindow_GetPropertyBool           Window_GetPropertyBool;
  GUIWindow_GetPropertyDouble         Window_GetPropertyDouble;
  GUIWindow_ClearProperties           Window_ClearProperties;
  GUIWindow_GetListSize               Window_GetListSize;
  GUIWindow_ClearList                 Window_ClearList;
  GUIWindow_AddItem                   Window_AddItem;
  GUIWindow_AddStringItem             Window_AddStringItem;
  GUIWindow_RemoveItem                Window_RemoveItem;
  GUIWindow_GetListItem               Window_GetListItem;
  GUIWindow_SetCurrentListPosition    Window_SetCurrentListPosition;
  GUIWindow_GetCurrentListPosition    Window_GetCurrentListPosition;
  GUIWindow_GetControl_Spin           Window_GetControl_Spin;
  GUIWindow_GetControl_Button         Window_GetControl_Button;
  GUIWindow_GetControl_RadioButton    Window_GetControl_RadioButton;
  GUIWindow_GetControl_Edit           Window_GetControl_Edit;
  GUIWindow_GetControl_Progress       Window_GetControl_Progress;
  GUIWindow_GetControl_RenderAddon    Window_GetControl_RenderAddon;
  GUIWindow_SetControlLabel           Window_SetControlLabel;
  GUIWindow_MarkDirtyRegion           Window_MarkDirtyRegion;
  GUIControl_Spin_SetVisible          Control_Spin_SetVisible;
  GUIControl_Spin_SetText             Control_Spin_SetText;
  GUIControl_Spin_Clear               Control_Spin_Clear;
  GUIControl_Spin_AddLabel            Control_Spin_AddLabel;
  GUIControl_Spin_GetValue            Control_Spin_GetValue;
  GUIControl_Spin_SetValue            Control_Spin_SetValue;
  GUIControl_RadioButton_SetVisible   Control_RadioButton_SetVisible;
  GUIControl_RadioButton_SetText      Control_RadioButton_SetText;
  GUIControl_RadioButton_SetSelected  Control_RadioButton_SetSelected;
  GUIControl_RadioButton_IsSelected   Control_RadioButton_IsSelected;
  GUIControl_Progress_SetPercentage   Control_Progress_SetPercentage;
  GUIControl_Progress_GetPercentage   Control_Progress_GetPercentage;
  GUIControl_Progress_SetInfo         Control_Progress_SetInfo;
  GUIControl_Progress_GetInfo         Control_Progress_GetInfo;
  GUIControl_Progress_GetDescription  Control_Progress_GetDescription;
  GUIListItem_Create                  ListItem_Create;
  GUIListItem_GetLabel                ListItem_GetLabel;
  GUIListItem_SetLabel                ListItem_SetLabel;
  GUIListItem_GetLabel2               ListItem_GetLabel2;
  GUIListItem_SetLabel2               ListItem_SetLabel2;
  GUIListItem_SetIconImage            ListItem_SetIconImage;
  GUIListItem_SetThumbnailImage       ListItem_SetThumbnailImage;
  GUIListItem_SetInfo                 ListItem_SetInfo;
  GUIListItem_SetProperty             ListItem_SetProperty;
  GUIListItem_GetProperty             ListItem_GetProperty;
  GUIListItem_SetPath                 ListItem_SetPath;
  GUIRenderAddon_SetCallbacks         RenderAddon_SetCallbacks;
  GUIRenderAddon_Delete               RenderAddon_Delete;

  GUIWindow_GetControl_Slider                         Window_GetControl_Slider;
  GUIControl_Slider_SetVisible                        Control_Slider_SetVisible;
  GUIControl_Slider_GetDescription                    Control_Slider_GetDescription;
  GUIControl_Slider_SetIntRange                       Control_Slider_SetIntRange;
  GUIControl_Slider_SetIntValue                       Control_Slider_SetIntValue;
  GUIControl_Slider_GetIntValue                       Control_Slider_GetIntValue;
  GUIControl_Slider_SetIntInterval                    Control_Slider_SetIntInterval;
  GUIControl_Slider_SetPercentage                     Control_Slider_SetPercentage;
  GUIControl_Slider_GetPercentage                     Control_Slider_GetPercentage;
  GUIControl_Slider_SetFloatRange                     Control_Slider_SetFloatRange;
  GUIControl_Slider_SetFloatValue                     Control_Slider_SetFloatValue;
  GUIControl_Slider_GetFloatValue                     Control_Slider_GetFloatValue;
  GUIControl_Slider_SetFloatInterval                  Control_Slider_SetFloatInterval;

  GUIWindow_GetControl_SettingsSlider                 Window_GetControl_SettingsSlider;
  GUIControl_SettingsSlider_SetVisible                Control_SettingsSlider_SetVisible;
  GUIControl_SettingsSlider_SetText                   Control_SettingsSlider_SetText;
  GUIControl_SettingsSlider_GetDescription            Control_SettingsSlider_GetDescription;
  GUIControl_SettingsSlider_SetIntRange               Control_SettingsSlider_SetIntRange;
  GUIControl_SettingsSlider_SetIntValue               Control_SettingsSlider_SetIntValue;
  GUIControl_SettingsSlider_GetIntValue               Control_SettingsSlider_GetIntValue;
  GUIControl_SettingsSlider_SetIntInterval            Control_SettingsSlider_SetIntInterval;
  GUIControl_SettingsSlider_SetPercentage             Control_SettingsSlider_SetPercentage;
  GUIControl_SettingsSlider_GetPercentage             Control_SettingsSlider_GetPercentage;
  GUIControl_SettingsSlider_SetFloatRange             Control_SettingsSlider_SetFloatRange;
  GUIControl_SettingsSlider_SetFloatValue             Control_SettingsSlider_SetFloatValue;
  GUIControl_SettingsSlider_GetFloatValue             Control_SettingsSlider_GetFloatValue;
  GUIControl_SettingsSlider_SetFloatInterval          Control_SettingsSlider_SetFloatInterval;

  GUIDialog_Keyboard_ShowAndGetInputWithHead          Dialog_Keyboard_ShowAndGetInputWithHead;
  GUIDialog_Keyboard_ShowAndGetInput                  Dialog_Keyboard_ShowAndGetInput;
  GUIDialog_Keyboard_ShowAndGetNewPasswordWithHead    Dialog_Keyboard_ShowAndGetNewPasswordWithHead;
  GUIDialog_Keyboard_ShowAndGetNewPassword            Dialog_Keyboard_ShowAndGetNewPassword;
  GUIDialog_Keyboard_ShowAndVerifyNewPasswordWithHead Dialog_Keyboard_ShowAndVerifyNewPasswordWithHead;
  GUIDialog_Keyboard_ShowAndVerifyNewPassword         Dialog_Keyboard_ShowAndVerifyNewPassword;
  GUIDialog_Keyboard_ShowAndVerifyPassword            Dialog_Keyboard_ShowAndVerifyPassword;
  GUIDialog_Keyboard_ShowAndGetFilter                 Dialog_Keyboard_ShowAndGetFilter;
  GUIDialog_Keyboard_SendTextToActiveKeyboard         Dialog_Keyboard_SendTextToActiveKeyboard;
  GUIDialog_Keyboard_isKeyboardActivated              Dialog_Keyboard_isKeyboardActivated;

  GUIDialog_Numeric_ShowAndVerifyNewPassword          Dialog_Numeric_ShowAndVerifyNewPassword;
  GUIDialog_Numeric_ShowAndVerifyPassword             Dialog_Numeric_ShowAndVerifyPassword;
  GUIDialog_Numeric_ShowAndVerifyInput                Dialog_Numeric_ShowAndVerifyInput;
  GUIDialog_Numeric_ShowAndGetTime                    Dialog_Numeric_ShowAndGetTime;
  GUIDialog_Numeric_ShowAndGetDate                    Dialog_Numeric_ShowAndGetDate;
  GUIDialog_Numeric_ShowAndGetIPAddress               Dialog_Numeric_ShowAndGetIPAddress;
  GUIDialog_Numeric_ShowAndGetNumber                  Dialog_Numeric_ShowAndGetNumber;
  GUIDialog_Numeric_ShowAndGetSeconds                 Dialog_Numeric_ShowAndGetSeconds;

  GUIDialog_FileBrowser_ShowAndGetFile                Dialog_FileBrowser_ShowAndGetFile;

  GUIDialog_OK_ShowAndGetInputSingleText              Dialog_OK_ShowAndGetInputSingleText;
  GUIDialog_OK_ShowAndGetInputLineText                Dialog_OK_ShowAndGetInputLineText;

  GUIDialog_YesNo_ShowAndGetInputSingleText           Dialog_YesNo_ShowAndGetInputSingleText;
  GUIDialog_YesNo_ShowAndGetInputLineText             Dialog_YesNo_ShowAndGetInputLineText;
  GUIDialog_YesNo_ShowAndGetInputLineButtonText       Dialog_YesNo_ShowAndGetInputLineButtonText;

  GUIDialog_TextViewer                                Dialog_TextViewer;
  GUIDialog_Select                                    Dialog_Select;
} CB_GUILib;

class CAddonCallbacksGUI : public ADDON::IAddonInterface
{
public:
  CAddonCallbacksGUI(ADDON::CAddon* addon);
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
  CB_GUILib    *m_callbacks;
};

} /* namespace GUI */
} /* namespace V1 */
} /* namespace KodiAPI */
