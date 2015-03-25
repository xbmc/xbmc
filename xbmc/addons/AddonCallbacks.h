#pragma once
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

#include <stdint.h>

#include "../../addons/library.kodi.guilib/libKODI_guilib.h"
#include "cores/dvdplayer/DVDDemuxers/DVDDemuxUtils.h"
#include "addons/include/xbmc_pvr_types.h"
#include "addons/include/xbmc_codec_types.h"

#ifdef TARGET_WINDOWS
#ifndef _SSIZE_T_DEFINED
typedef intptr_t ssize_t;
#define _SSIZE_T_DEFINED
#endif // !_SSIZE_T_DEFINED
#endif // TARGET_WINDOWS

typedef void (*AddOnLogCallback)(void *addonData, const ADDON::addon_log_t loglevel, const char *msg);
typedef void (*AddOnQueueNotification)(void *addonData, const ADDON::queue_msg_t type, const char *msg);
typedef bool (*AddOnWakeOnLan)(const char* mac);
typedef bool (*AddOnGetSetting)(void *addonData, const char *settingName, void *settingValue);
typedef char* (*AddOnUnknownToUTF8)(const char *sourceDest);
typedef char* (*AddOnGetLocalizedString)(const void* addonData, long dwCode);
typedef char* (*AddOnGetDVDMenuLanguage)(const void* addonData);
typedef void (*AddOnFreeString)(const void* addonData, char* str);

typedef void* (*AddOnOpenFile)(const void* addonData, const char* strFileName, unsigned int flags);
typedef void* (*AddOnOpenFileForWrite)(const void* addonData, const char* strFileName, bool bOverWrite);
typedef ssize_t (*AddOnReadFile)(const void* addonData, void* file, void* lpBuf, size_t uiBufSize);
typedef bool (*AddOnReadFileString)(const void* addonData, void* file, char *szLine, int iLineLength);
typedef ssize_t (*AddOnWriteFile)(const void* addonData, void* file, const void* lpBuf, size_t uiBufSize);
typedef void (*AddOnFlushFile)(const void* addonData, void* file);
typedef int64_t (*AddOnSeekFile)(const void* addonData, void* file, int64_t iFilePosition, int iWhence);
typedef int (*AddOnTruncateFile)(const void* addonData, void* file, int64_t iSize);
typedef int64_t (*AddOnGetFilePosition)(const void* addonData, void* file);
typedef int64_t (*AddOnGetFileLength)(const void* addonData, void* file);
typedef void (*AddOnCloseFile)(const void* addonData, void* file);
typedef int (*AddOnGetFileChunkSize)(const void* addonData, void* file);
typedef bool (*AddOnFileExists)(const void* addonData, const char *strFileName, bool bUseCache);
typedef int (*AddOnStatFile)(const void* addonData, const char *strFileName, struct __stat64* buffer);
typedef bool (*AddOnDeleteFile)(const void* addonData, const char *strFileName);
typedef bool (*AddOnCanOpenDirectory)(const void* addonData, const char* strURL);
typedef bool (*AddOnCreateDirectory)(const void* addonData, const char *strPath);
typedef bool (*AddOnDirectoryExists)(const void* addonData, const char *strPath);
typedef bool (*AddOnRemoveDirectory)(const void* addonData, const char *strPath);

typedef struct CB_AddOn
{
  AddOnLogCallback        Log;
  AddOnQueueNotification  QueueNotification;
  AddOnWakeOnLan          WakeOnLan;
  AddOnGetSetting         GetSetting;
  AddOnUnknownToUTF8      UnknownToUTF8;
  AddOnGetLocalizedString GetLocalizedString;
  AddOnGetDVDMenuLanguage GetDVDMenuLanguage;
  AddOnFreeString         FreeString;

  AddOnOpenFile           OpenFile;
  AddOnOpenFileForWrite   OpenFileForWrite;
  AddOnReadFile           ReadFile;
  AddOnReadFileString     ReadFileString;
  AddOnWriteFile          WriteFile;
  AddOnFlushFile          FlushFile;
  AddOnSeekFile           SeekFile;
  AddOnTruncateFile       TruncateFile;
  AddOnGetFilePosition    GetFilePosition;
  AddOnGetFileLength      GetFileLength;
  AddOnCloseFile          CloseFile;
  AddOnGetFileChunkSize   GetFileChunkSize;
  AddOnFileExists         FileExists;
  AddOnStatFile           StatFile;
  AddOnDeleteFile         DeleteFile;
  AddOnCanOpenDirectory   CanOpenDirectory;
  AddOnCreateDirectory    CreateDirectory;
  AddOnDirectoryExists    DirectoryExists;
  AddOnRemoveDirectory    RemoveDirectory;
} CB_AddOnLib;

typedef xbmc_codec_t (*CODECGetCodecByName)(const void* addonData, const char* strCodecName);

typedef struct CB_CODEC
{
  CODECGetCodecByName   GetCodecByName;
} CB_CODECLib;

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

typedef void (*PVRTransferEpgEntry)(void *userData, const ADDON_HANDLE handle, const EPG_TAG *epgentry);
typedef void (*PVRTransferChannelEntry)(void *userData, const ADDON_HANDLE handle, const PVR_CHANNEL *chan);
typedef void (*PVRTransferTimerEntry)(void *userData, const ADDON_HANDLE handle, const PVR_TIMER *timer);
typedef void (*PVRTransferRecordingEntry)(void *userData, const ADDON_HANDLE handle, const PVR_RECORDING *recording);
typedef void (*PVRAddMenuHook)(void *addonData, PVR_MENUHOOK *hook);
typedef void (*PVRRecording)(void *addonData, const char *Name, const char *FileName, bool On);
typedef void (*PVRTriggerChannelUpdate)(void *addonData);
typedef void (*PVRTriggerTimerUpdate)(void *addonData);
typedef void (*PVRTriggerRecordingUpdate)(void *addonData);
typedef void (*PVRTriggerChannelGroupsUpdate)(void *addonData);
typedef void (*PVRTriggerEpgUpdate)(void *addonData, unsigned int iChannelUid);

typedef void (*PVRTransferChannelGroup)(void *addonData, const ADDON_HANDLE handle, const PVR_CHANNEL_GROUP *group);
typedef void (*PVRTransferChannelGroupMember)(void *addonData, const ADDON_HANDLE handle, const PVR_CHANNEL_GROUP_MEMBER *member);

typedef void (*PVRFreeDemuxPacket)(void *addonData, DemuxPacket* pPacket);
typedef DemuxPacket* (*PVRAllocateDemuxPacket)(void *addonData, int iDataSize);

typedef struct CB_PVRLib
{
  PVRTransferEpgEntry           TransferEpgEntry;
  PVRTransferChannelEntry       TransferChannelEntry;
  PVRTransferTimerEntry         TransferTimerEntry;
  PVRTransferRecordingEntry     TransferRecordingEntry;
  PVRAddMenuHook                AddMenuHook;
  PVRRecording                  Recording;
  PVRTriggerChannelUpdate       TriggerChannelUpdate;
  PVRTriggerTimerUpdate         TriggerTimerUpdate;
  PVRTriggerRecordingUpdate     TriggerRecordingUpdate;
  PVRTriggerChannelGroupsUpdate TriggerChannelGroupsUpdate;
  PVRTriggerEpgUpdate           TriggerEpgUpdate;
  PVRFreeDemuxPacket            FreeDemuxPacket;
  PVRAllocateDemuxPacket        AllocateDemuxPacket;
  PVRTransferChannelGroup       TransferChannelGroup;
  PVRTransferChannelGroupMember TransferChannelGroupMember;

} CB_PVRLib;


typedef CB_AddOnLib* (*XBMCAddOnLib_RegisterMe)(void *addonData);
typedef void (*XBMCAddOnLib_UnRegisterMe)(void *addonData, CB_AddOnLib *cbTable);
typedef CB_CODECLib* (*XBMCCODECLib_RegisterMe)(void *addonData);
typedef void (*XBMCCODECLib_UnRegisterMe)(void *addonData, CB_CODECLib *cbTable);
typedef CB_GUILib* (*XBMCGUILib_RegisterMe)(void *addonData);
typedef void (*XBMCGUILib_UnRegisterMe)(void *addonData, CB_GUILib *cbTable);
typedef CB_PVRLib* (*XBMCPVRLib_RegisterMe)(void *addonData);
typedef void (*XBMCPVRLib_UnRegisterMe)(void *addonData, CB_PVRLib *cbTable);

typedef struct AddonCB
{
  const char                *libBasePath;                  ///> Never, never change this!!!
  void                      *addonData;
  XBMCAddOnLib_RegisterMe    AddOnLib_RegisterMe;
  XBMCAddOnLib_UnRegisterMe  AddOnLib_UnRegisterMe;
  XBMCCODECLib_RegisterMe    CODECLib_RegisterMe;
  XBMCCODECLib_UnRegisterMe  CODECLib_UnRegisterMe;
  XBMCGUILib_RegisterMe      GUILib_RegisterMe;
  XBMCGUILib_UnRegisterMe    GUILib_UnRegisterMe;
  XBMCPVRLib_RegisterMe      PVRLib_RegisterMe;
  XBMCPVRLib_UnRegisterMe    PVRLib_UnRegisterMe;
} AddonCB;


namespace ADDON
{

class CAddon;
class CAddonCallbacksAddon;
class CAddonCallbacksCodec;
class CAddonCallbacksGUI;
class CAddonCallbacksPVR;

class CAddonCallbacks
{
public:
  CAddonCallbacks(CAddon* addon);
  ~CAddonCallbacks();
  AddonCB *GetCallbacks() { return m_callbacks; }

  static CB_AddOnLib* AddOnLib_RegisterMe(void *addonData);
  static void AddOnLib_UnRegisterMe(void *addonData, CB_AddOnLib *cbTable);
  static CB_CODECLib* CODECLib_RegisterMe(void *addonData);
  static void CODECLib_UnRegisterMe(void *addonData, CB_CODECLib *cbTable);
  static CB_GUILib* GUILib_RegisterMe(void *addonData);
  static void GUILib_UnRegisterMe(void *addonData, CB_GUILib *cbTable);
  static CB_PVRLib* PVRLib_RegisterMe(void *addonData);
  static void PVRLib_UnRegisterMe(void *addonData, CB_PVRLib *cbTable);

  CAddonCallbacksAddon *GetHelperAddon() { return m_helperAddon; }
  CAddonCallbacksCodec *GetHelperCodec() { return m_helperCODEC; }
  CAddonCallbacksGUI *GetHelperGUI() { return m_helperGUI; }
  CAddonCallbacksPVR *GetHelperPVR() { return m_helperPVR; }

private:
  AddonCB             *m_callbacks;
  CAddon              *m_addon;
  CAddonCallbacksAddon *m_helperAddon;
  CAddonCallbacksCodec *m_helperCODEC;
  CAddonCallbacksGUI   *m_helperGUI;
  CAddonCallbacksPVR   *m_helperPVR;
};

}; /* namespace ADDON */
