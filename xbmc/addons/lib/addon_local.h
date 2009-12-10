/*
 *  Copyright (C) 2004-2006, Eric Lund
 *  http://www.mvpmc.org/
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.

 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/**
 * \file addon_local.h
 * Local definitions which are internal to libaddon
 */

#ifndef __ADDON_LOCAL_H_
#define __ADDON_LOCAL_H_

#ifdef HAS_XBOX_HARDWARE
#include <xtl.h>
#else
#ifndef _LINUX
#include <windows.h>
#else
#ifndef __cdecl
#define __cdecl
#endif
#ifndef __declspec
#define __declspec(x)
#endif
#include <time.h>
#endif
#endif

#include "addons/include/xbmc_addon_types.h"
#include "addons/include/xbmc_pvr_types.h"

/* An individual setting */
struct addon_setting {
  addon_setting_type_t type;
  char *id;
  char *label;
  char *enable;
  char *lvalues;
};

/* A list of settings */
struct addon_settings {
  addon_setting_t *settings_list;
  long settings_count;
};

typedef void (*AddOnLogCallback)(void *addonData, const addon_log_t loglevel, const char *msg);
typedef void (*AddOnStatusCallback)(void *addonData, const addon_status, const char* msg);
typedef bool (*AddOnGetSetting)(void *addonData, const char *settingName, void *settingValue);
typedef void (*AddOnOpenSettings)(void *addonData);
typedef char* (*AddOnGetAddonDirectory)(void *addonData);
typedef char* (*AddOnGetUserDirectory)(void *addonData);

typedef struct CB_AddOn
{
  AddOnStatusCallback    ReportStatus;
  AddOnLogCallback       Log;
  AddOnGetSetting        GetSetting;
  AddOnOpenSettings      OpenSettings;
  AddOnGetAddonDirectory GetAddonDirectory;
  AddOnGetUserDirectory  GetUserDirectory;
} CB_AddOn;

typedef void (*UtilsShutdown)();
typedef void (*UtilsRestart)();
typedef void (*UtilsDashboard)();
typedef void (*UtilsExecuteScript)(const char *script);
typedef void (*UtilsExecuteBuiltIn)(const char *function);
typedef char* (*UtilsExecuteHttpApi)(const char *httpcommand);
typedef char* (*UtilsGetSkinDir)();
typedef char* (*UtilsGetLanguage)();
typedef char* (*UtilsGetIPAddress)();
typedef char* (*UtilsGetInfoLabel)(const char *infotag);
typedef char* (*UtilsGetInfoImage)(const char *infotag);
typedef bool (*UtilsGetCondVisibility)(const char *condition);
typedef bool (*UtilsCreateDirectory)(const char *dir);
typedef void (*UtilsPlaySFX)(const char *filename);
typedef char* (*UtilsGetCacheThumbName)(const char *path);
typedef void (*UtilsEnableNavSounds)(bool yesNo);
typedef char* (*UtilsMakeLegalFilename)(const char *filename);
typedef int (*UtilsGetDVDState)();
typedef int (*UtilsGetFreeMem)();
typedef int (*UtilsGetGlobalIdleTime)();
typedef char* (*UtilsLocStrings)(const void *addonData, long dwCode);
typedef char* (*UtilsGetRegion)(int id);
typedef char* (*UtilsGetSupportedMedia)(int media);
typedef bool (*UtilsSkinHasImage)(const char *filename);
typedef char* (*UtilsTranslatePath)(const char *path);
typedef char* (*UtilsUnknownToUTF8)(const char *sourceDest);

typedef struct CB_Utils
{
  UtilsShutdown             Shutdown;
  UtilsRestart              Restart;
  UtilsDashboard            Dashboard;
  UtilsExecuteScript        ExecuteScript;
  UtilsExecuteBuiltIn       ExecuteBuiltIn;
  UtilsExecuteHttpApi       ExecuteHttpApi;
  UtilsGetSkinDir           GetSkinDir;
  UtilsGetLanguage          GetLanguage;
  UtilsGetIPAddress         GetIPAddress;
  UtilsGetInfoLabel         GetInfoLabel;
  UtilsGetInfoImage         GetInfoImage;
  UtilsGetCondVisibility    GetCondVisibility;
  UtilsPlaySFX              PlaySFX;
  UtilsGetCacheThumbName    GetCacheThumbName;
  UtilsCreateDirectory      CreateDirectory;
  UtilsEnableNavSounds      EnableNavSounds;
  UtilsGetDVDState          GetDVDState;
  UtilsGetFreeMem           GetFreeMem;
  UtilsGetGlobalIdleTime    GetGlobalIdleTime;
  UtilsGetRegion            GetRegion;
  UtilsGetSupportedMedia    GetSupportedMedia;
  UtilsLocStrings           LocalizedString;
  UtilsMakeLegalFilename    MakeLegalFilename;
  UtilsSkinHasImage         SkinHasImage;
  UtilsTranslatePath        TranslatePath;
  UtilsUnknownToUTF8        UnknownToUTF8;
} CB_Utils;


typedef bool (*DialogOpenOK)(const char*, const char*, const char*, const char*);
typedef bool (*DialogOpenYesNo)(const char*, const char*, const char*, const char*, const char*, const char*);
typedef char* (*DialogOpenBrowse)(int, const char*, const char*, const char*, bool, bool, const char*);
typedef char* (*DialogOpenNumeric)(int, const char*, const char*);
typedef char* (*DialogOpenKeyboard)(const char*, const char*, bool);
typedef int (*DialogOpenSelect)(const char*, addon_string_list*);
typedef bool (*DialogProgressCreate)(const char*, const char*, const char*, const char*);
typedef void (*DialogProgressUpdate)(int, const char*, const char*, const char*);
typedef bool (*DialogProgressIsCanceled)();
typedef void (*DialogProgressClose)();

typedef struct CB_Dialog
{
  DialogOpenOK              OpenOK;
  DialogOpenYesNo           OpenYesNo;
  DialogOpenBrowse          OpenBrowse;
  DialogOpenNumeric         OpenNumeric;
  DialogOpenKeyboard        OpenKeyboard;
  DialogOpenSelect          OpenSelect;
  DialogProgressCreate      ProgressCreate;
  DialogProgressUpdate      ProgressUpdate;
  DialogProgressIsCanceled  ProgressIsCanceled;
  DialogProgressClose       ProgressClose;
} CB_Dialog;


typedef void (*GUILock)();
typedef void (*GUIUnlock)();
typedef int (*GUIGetCurrentWindowId)();
typedef int (*GUIGetCurrentWindowDialogId)();

typedef struct CB_GUI
{
  GUILock                       Lock;
  GUIUnlock                     Unlock;
  GUIGetCurrentWindowId         GetCurrentWindowId;
  GUIGetCurrentWindowDialogId   GetCurrentWindowDialogId;
} CB_GUI;


typedef void (*PVREventCallback)(void *userData, const PVR_EVENT, const char*);
typedef void (*PVRTransferEpgEntry)(void *userData, const PVRHANDLE handle, const PVR_PROGINFO *epgentry);
typedef void (*PVRTransferChannelEntry)(void *userData, const PVRHANDLE handle, const PVR_CHANNEL *chan);
typedef void (*PVRTransferTimerEntry)(void *userData, const PVRHANDLE handle, const PVR_TIMERINFO *timer);
typedef void (*PVRTransferRecordingEntry)(void *userData, const PVRHANDLE handle, const PVR_RECORDINGINFO *recording);

typedef struct CB_PVR
{
  PVREventCallback          EventCallback;
  PVRTransferEpgEntry       TransferEpgEntry;
  PVRTransferChannelEntry   TransferChannelEntry;
  PVRTransferTimerEntry     TransferTimerEntry;
  PVRTransferRecordingEntry TransferRecordingEntry;
} CB_PVR;

typedef struct AddonCB
{
//    MainCallbacks               Main;
  CB_AddOn          AddOn;
  CB_Utils          Utils;
  CB_Dialog         Dialog;
  CB_GUI            GUI;
  CB_PVR            PVR;
  void             *userData;
  void             *addonData;
} AddonCB;

#endif /* __ADDON_LOCAL_H */
