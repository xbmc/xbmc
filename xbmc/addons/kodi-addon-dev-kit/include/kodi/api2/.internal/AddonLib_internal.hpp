#pragma once
/*
 *      Copyright (C) 2015 Team KODI
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

#include <stdbool.h>
#include <stdint.h>
#include <semaphore.h>

#include "../definitions.hpp"
#include "AddonLib_LibFunc_Base.hpp"

/*
 * This file includes not for add-on developer used parts, but required for the
 * interface to Kodi over the library.
 */

#ifdef __cplusplus
extern "C"
{
#endif

  static constexpr const int KODI_API_ConnectionTimeout = 3;
  static constexpr const int KODI_API_ConnectionPort    = 34687;

  typedef enum KODI_API_Calls
  {
    KODICall_Noop                     = 0,
    KODICall_LoginVerify              = 1,
    KODICall_Logout                   = 2,
    KODICall_Ping                     = 3,
    KODICall_Log                      = 6,
    KODICall_CreateSubThread          = 7,
    KODICall_DeleteSubThread          = 8,

    KODICall_AddOn_General_GetSettingString                               =  97,
    KODICall_AddOn_General_GetSettingBoolean                              =  98,
    KODICall_AddOn_General_GetSettingInt                                  =  99,
    KODICall_AddOn_General_GetSettingFloat                                = 100,
    KODICall_AddOn_General_OpenSettings                                   = 101,
    KODICall_AddOn_General_GetAddonInfo                                   = 102,
    KODICall_AddOn_General_QueueFormattedNotification                     = 103,
    KODICall_AddOn_General_QueueNotificationFromType                      = 104,
    KODICall_AddOn_General_QueueNotificationWithImage                     = 105,
    KODICall_AddOn_General_GetMD5                                         = 106,
    KODICall_AddOn_General_UnknownToUTF8                                  = 107,
    KODICall_AddOn_General_GetLocalizedString                             = 108,
    KODICall_AddOn_General_GetLanguage                                    = 109,
    KODICall_AddOn_General_GetDVDMenuLanguage                             = 110,
    KODICall_AddOn_General_StartServer                                    = 111,
    KODICall_AddOn_General_AudioSuspend                                   = 112,
    KODICall_AddOn_General_AudioResume                                    = 113,
    KODICall_AddOn_General_GetVolume                                      = 114,
    KODICall_AddOn_General_SetVolume                                      = 115,
    KODICall_AddOn_General_IsMuted                                        = 116,
    KODICall_AddOn_General_ToggleMute                                     = 117,
    KODICall_AddOn_General_SetMute                                        = 118,
    KODICall_AddOn_General_GetOpticalDriveState                           = 119,
    KODICall_AddOn_General_EjectOpticalDrive                              = 120,
    KODICall_AddOn_General_KodiVersion                                    = 121,
    KODICall_AddOn_General_KodiQuit                                       = 122,
    KODICall_AddOn_General_HTPCShutdown                                   = 123,
    KODICall_AddOn_General_HTPCRestart                                    = 124,
    KODICall_AddOn_General_ExecuteScript                                  = 125,
    KODICall_AddOn_General_ExecuteBuiltin                                 = 126,
    KODICall_AddOn_General_ExecuteJSONRPC                                 = 127,
    KODICall_AddOn_General_GetRegion                                      = 128,
    KODICall_AddOn_General_GetFreeMem                                     = 129,
    KODICall_AddOn_General_GetGlobalIdleTime                              = 130,
    KODICall_AddOn_General_TranslatePath                                  = 131,

    KODICall_AddOn_Codec_GetCodecByName                                   = 200,
    KODICall_AddOn_Codec_AllocateDemuxPacket                              = 201,
    KODICall_AddOn_Codec_FreeDemuxPacket                                  = 202,

    KODICall_AddOn_Network_WakeOnLan                                      = 220,
    KODICall_AddOn_Network_GetIPAddress                                   = 221,
    KODICall_AddOn_Network_URLEncode                                      = 222,
    KODICall_AddOn_Network_DNSLookup                                      = 223,

    KODICall_AddOn_SoundPlay_GetHandle                                    = 240,
    KODICall_AddOn_SoundPlay_ReleaseHandle                                = 241,
    KODICall_AddOn_SoundPlay_Play                                         = 242,
    KODICall_AddOn_SoundPlay_Stop                                         = 243,
    KODICall_AddOn_SoundPlay_IsPlaying                                    = 244,
    KODICall_AddOn_SoundPlay_SetChannel                                   = 245,
    KODICall_AddOn_SoundPlay_GetChannel                                   = 246,
    KODICall_AddOn_SoundPlay_SetVolume                                    = 247,
    KODICall_AddOn_SoundPlay_GetVolume                                    = 248,

    KODICall_AddOn_VFSUtils_CreateDirectory                               = 260,
    KODICall_AddOn_VFSUtils_DirectoryExists                               = 261,
    KODICall_AddOn_VFSUtils_RemoveDirectory                               = 262,
    KODICall_AddOn_VFSUtils_GetVFSDirectory                               = 263,
    KODICall_AddOn_VFSUtils_FreeVFSDirectory                              = 264,
    KODICall_AddOn_VFSUtils_GetFileMD5                                    = 265,
    KODICall_AddOn_VFSUtils_GetCacheThumbName                             = 266,
    KODICall_AddOn_VFSUtils_MakeLegalFileName                             = 267,
    KODICall_AddOn_VFSUtils_MakeLegalPath                                 = 268,
    KODICall_AddOn_VFSUtils_OpenFile                                      = 269,
    KODICall_AddOn_VFSUtils_OpenFileForWrite                              = 270,
    KODICall_AddOn_VFSUtils_ReadFile                                      = 271,
    KODICall_AddOn_VFSUtils_ReadFileString                                = 272,
    KODICall_AddOn_VFSUtils_WriteFile                                     = 273,
    KODICall_AddOn_VFSUtils_FlushFile                                     = 274,
    KODICall_AddOn_VFSUtils_SeekFile                                      = 275,
    KODICall_AddOn_VFSUtils_TruncateFile                                  = 276,
    KODICall_AddOn_VFSUtils_GetFilePosition                               = 277,
    KODICall_AddOn_VFSUtils_GetFileLength                                 = 278,
    KODICall_AddOn_VFSUtils_CloseFile                                     = 279,
    KODICall_AddOn_VFSUtils_GetFileChunkSize                              = 280,
    KODICall_AddOn_VFSUtils_FileExists                                    = 281,
    KODICall_AddOn_VFSUtils_StatFile                                      = 282,
    KODICall_AddOn_VFSUtils_DeleteFile                                    = 283,

    KODICall_AudioEngine_General_AddDSPMenuHook                           = 300,
    KODICall_AudioEngine_General_RemoveDSPMenuHook                        = 301,
    KODICall_AudioEngine_General_RegisterDSPMode                          = 302,
    KODICall_AudioEngine_General_UnregisterDSPMode                        = 303,
    KODICall_AudioEngine_General_GetCurrentSinkFormat                     = 304,

    KODICall_AudioEngine_Stream_MakeStream                                = 320,
    KODICall_AudioEngine_Stream_FreeStream                                = 321,
    KODICall_AudioEngine_Stream_GetSpace                                  = 322,
    KODICall_AudioEngine_Stream_AddData                                   = 323,
    KODICall_AudioEngine_Stream_GetDelay                                  = 324,
    KODICall_AudioEngine_Stream_IsBuffering                               = 325,
    KODICall_AudioEngine_Stream_GetCacheTime                              = 326,
    KODICall_AudioEngine_Stream_GetCacheTotal                             = 327,
    KODICall_AudioEngine_Stream_Pause                                     = 328,
    KODICall_AudioEngine_Stream_Resume                                    = 329,
    KODICall_AudioEngine_Stream_Drain                                     = 330,
    KODICall_AudioEngine_Stream_IsDraining                                = 331,
    KODICall_AudioEngine_Stream_IsDrained                                 = 332,
    KODICall_AudioEngine_Stream_Flush                                     = 333,
    KODICall_AudioEngine_Stream_GetVolume                                 = 334,
    KODICall_AudioEngine_Stream_SetVolume                                 = 335,
    KODICall_AudioEngine_Stream_GetAmplification                          = 336,
    KODICall_AudioEngine_Stream_SetAmplification                          = 337,
    KODICall_AudioEngine_Stream_GetFrameSize                              = 338,
    KODICall_AudioEngine_Stream_GetChannelCount                           = 339,
    KODICall_AudioEngine_Stream_GetSampleRate                             = 340,
    KODICall_AudioEngine_Stream_GetDataFormat                             = 341,
    KODICall_AudioEngine_Stream_GetResampleRatio                          = 342,
    KODICall_AudioEngine_Stream_SetResampleRatio                          = 343,

    KODICall_GUI_General_Lock                                             = 400,
    KODICall_GUI_General_Unlock                                           = 401,
    KODICall_GUI_General_GetScreenHeight                                  = 402,
    KODICall_GUI_General_GetScreenWidth                                   = 403,
    KODICall_GUI_General_GetVideoResolution                               = 404,
    KODICall_GUI_General_GetCurrentWindowDialogId                         = 405,
    KODICall_GUI_General_GetCurrentWindowId                               = 406,

    KODICall_GUI_Window_New                                               = 420,
    KODICall_GUI_Window_Delete                                            = 421,
    KODICall_GUI_Window_Show                                              = 422,
    KODICall_GUI_Window_Close                                             = 423,
    KODICall_GUI_Window_DoModal                                           = 424,
    KODICall_GUI_Window_SetFocusId                                        = 425,
    KODICall_GUI_Window_GetFocusId                                        = 426,
    KODICall_GUI_Window_SetProperty                                       = 427,
    KODICall_GUI_Window_SetPropertyInt                                    = 428,
    KODICall_GUI_Window_SetPropertyBool                                   = 429,
    KODICall_GUI_Window_SetPropertyDouble                                 = 430,
    KODICall_GUI_Window_GetProperty                                       = 431,
    KODICall_GUI_Window_GetPropertyInt                                    = 432,
    KODICall_GUI_Window_GetPropertyBool                                   = 433,
    KODICall_GUI_Window_GetPropertyDouble                                 = 434,
    KODICall_GUI_Window_ClearProperties                                   = 435,
    KODICall_GUI_Window_ClearProperty                                     = 436,
    KODICall_GUI_Window_GetListSize                                       = 437,
    KODICall_GUI_Window_ClearList                                         = 438,
    KODICall_GUI_Window_AddStringItem                                     = 439,
    KODICall_GUI_Window_AddItem                                           = 440,
    KODICall_GUI_Window_RemoveItem                                        = 441,
    KODICall_GUI_Window_RemoveItemFile                                    = 442,
    KODICall_GUI_Window_GetListItem                                       = 443,
    KODICall_GUI_Window_SetCurrentListPosition                            = 444,
    KODICall_GUI_Window_GetCurrentListPosition                            = 445,
    KODICall_GUI_Window_SetControlLabel                                   = 446,
    KODICall_GUI_Window_MarkDirtyRegion                                   = 447,
    KODICall_GUI_Window_SetCallbacks                                      = 448,
    KODICall_GUI_Window_GetControl_Button                                 = 449,
    KODICall_GUI_Window_GetControl_Edit                                   = 450,
    KODICall_GUI_Window_GetControl_FadeLabel                              = 451,
    KODICall_GUI_Window_GetControl_Image                                  = 452,
    KODICall_GUI_Window_GetControl_Label                                  = 453,
    KODICall_GUI_Window_GetControl_Progress                               = 454,
    KODICall_GUI_Window_GetControl_RadioButton                            = 455,
    KODICall_GUI_Window_GetControl_Rendering                              = 456,
    KODICall_GUI_Window_GetControl_SettingsSlider                         = 457,
    KODICall_GUI_Window_GetControl_Slider                                 = 458,
    KODICall_GUI_Window_GetControl_Spin                                   = 459,
    KODICall_GUI_Window_GetControl_TextBox                                = 460,

    KODICall_GUI_ListItem_Create                                          = 500,
    KODICall_GUI_ListItem_Destroy                                         = 501,
    KODICall_GUI_ListItem_GetLabel                                        = 502,
    KODICall_GUI_ListItem_SetLabel                                        = 503,
    KODICall_GUI_ListItem_GetLabel2                                       = 504,
    KODICall_GUI_ListItem_SetLabel2                                       = 505,
    KODICall_GUI_ListItem_GetIconImage                                    = 506,
    KODICall_GUI_ListItem_SetIconImage                                    = 507,
    KODICall_GUI_ListItem_GetOverlayImage                                 = 508,
    KODICall_GUI_ListItem_SetOverlayImage                                 = 509,
    KODICall_GUI_ListItem_SetThumbnailImage                               = 510,
    KODICall_GUI_ListItem_SetArt                                          = 511,
    KODICall_GUI_ListItem_SetArtFallback                                  = 512,
    KODICall_GUI_ListItem_HasArt                                          = 513,
    KODICall_GUI_ListItem_Select                                          = 514,
    KODICall_GUI_ListItem_IsSelected                                      = 515,
    KODICall_GUI_ListItem_HasIcon                                         = 516,
    KODICall_GUI_ListItem_HasOverlay                                      = 517,
    KODICall_GUI_ListItem_IsFileItem                                      = 518,
    KODICall_GUI_ListItem_IsFolder                                        = 519,
    KODICall_GUI_ListItem_SetProperty                                     = 520,
    KODICall_GUI_ListItem_GetProperty                                     = 521,
    KODICall_GUI_ListItem_ClearProperty                                   = 522,
    KODICall_GUI_ListItem_ClearProperties                                 = 523,
    KODICall_GUI_ListItem_HasProperties                                   = 524,
    KODICall_GUI_ListItem_HasProperty                                     = 525,
    KODICall_GUI_ListItem_SetPath                                         = 526,
    KODICall_GUI_ListItem_GetPath                                         = 527,
    KODICall_GUI_ListItem_GetDuration                                     = 528,
    KODICall_GUI_ListItem_SetSubtitles                                    = 529,
    KODICall_GUI_ListItem_SetMimeType                                     = 530,
    KODICall_GUI_ListItem_SetContentLookup                                = 531,
    KODICall_GUI_ListItem_AddContextMenuItems                             = 532,
    KODICall_GUI_ListItem_AddStreamInfo                                   = 533,
    KODICall_GUI_ListItem_SetMusicInfo_BOOL                               = 534,
    KODICall_GUI_ListItem_SetMusicInfo_INT                                = 535,
    KODICall_GUI_ListItem_SetMusicInfo_UINT                               = 536,
    KODICall_GUI_ListItem_SetMusicInfo_FLOAT                              = 537,
    KODICall_GUI_ListItem_SetMusicInfo_STRING                             = 538,
    KODICall_GUI_ListItem_SetMusicInfo_STRING_LIST                        = 539,
    KODICall_GUI_ListItem_SetVideoInfo_BOOL                               = 540,
    KODICall_GUI_ListItem_SetVideoInfo_INT                                = 541,
    KODICall_GUI_ListItem_SetVideoInfo_UINT                               = 542,
    KODICall_GUI_ListItem_SetVideoInfo_FLOAT                              = 543,
    KODICall_GUI_ListItem_SetVideoInfo_STRING                             = 544,
    KODICall_GUI_ListItem_SetVideoInfo_STRING_LIST                        = 545,
    KODICall_GUI_ListItem_SetVideoInfo_Resume                             = 546,
    KODICall_GUI_ListItem_SetVideoInfo_Cast                               = 547,
    KODICall_GUI_ListItem_SetPictureInfo_BOOL                             = 548,
    KODICall_GUI_ListItem_SetPictureInfo_INT                              = 549,
    KODICall_GUI_ListItem_SetPictureInfo_UINT                             = 550,
    KODICall_GUI_ListItem_SetPictureInfo_FLOAT                            = 551,
    KODICall_GUI_ListItem_SetPictureInfo_STRING                           = 552,
    KODICall_GUI_ListItem_SetPictureInfo_STRING_LIST                      = 553,

    KODICall_GUI_Control_Button_SetVisible                                = 600,
    KODICall_GUI_Control_Button_SetEnabled                                = 601,
    KODICall_GUI_Control_Button_SetLabel                                  = 602,
    KODICall_GUI_Control_Button_GetLabel                                  = 603,
    KODICall_GUI_Control_Button_SetLabel2                                 = 604,
    KODICall_GUI_Control_Button_GetLabel2                                 = 605,

    KODICall_GUI_Control_Edit_SetVisible                                  = 610,
    KODICall_GUI_Control_Edit_SetEnabled                                  = 611,
    KODICall_GUI_Control_Edit_SetLabel                                    = 612,
    KODICall_GUI_Control_Edit_GetLabel                                    = 613,
    KODICall_GUI_Control_Edit_SetText                                     = 614,
    KODICall_GUI_Control_Edit_GetText                                     = 615,
    KODICall_GUI_Control_Edit_SetCursorPosition                           = 616,
    KODICall_GUI_Control_Edit_GetCursorPosition                           = 617,
    KODICall_GUI_Control_Edit_SetInputType                                = 618,

    KODICall_GUI_Control_FadeLabel_SetVisible                             = 630,
    KODICall_GUI_Control_FadeLabel_AddLabel                               = 631,
    KODICall_GUI_Control_FadeLabel_GetLabel                               = 632,
    KODICall_GUI_Control_FadeLabel_SetScrolling                           = 633,
    KODICall_GUI_Control_FadeLabel_Reset                                  = 634,

    KODICall_GUI_Control_Image_SetVisible                                 = 650,
    KODICall_GUI_Control_Image_SetFileName                                = 651,
    KODICall_GUI_Control_Image_SetColorDiffuse                            = 652,

    KODICall_GUI_Control_Label_SetVisible                                 = 670,
    KODICall_GUI_Control_Label_SetLabel                                   = 671,
    KODICall_GUI_Control_Label_GetLabel                                   = 672,

    KODICall_GUI_Control_Progress_SetVisible                              = 690,
    KODICall_GUI_Control_Progress_SetPercentage                           = 691,
    KODICall_GUI_Control_Progress_GetPercentage                           = 692,

    KODICall_GUI_Control_RadioButton_SetVisible                           = 710,
    KODICall_GUI_Control_RadioButton_SetEnabled                           = 711,
    KODICall_GUI_Control_RadioButton_SetLabel                             = 712,
    KODICall_GUI_Control_RadioButton_GetLabel                             = 713,
    KODICall_GUI_Control_RadioButton_SetSelected                          = 714,
    KODICall_GUI_Control_RadioButton_IsSelected                           = 715,

    KODICall_GUI_Control_Rendering_Delete                                 = 730,
    KODICall_GUI_Control_Rendering_SetCallbacks                           = 731,

    KODICall_GUI_Control_SettingsSlider_SetVisible                        = 750,
    KODICall_GUI_Control_SettingsSlider_SetEnabled                        = 751,
    KODICall_GUI_Control_SettingsSlider_SetText                           = 752,
    KODICall_GUI_Control_SettingsSlider_Reset                             = 753,
    KODICall_GUI_Control_SettingsSlider_SetIntRange                       = 754,
    KODICall_GUI_Control_SettingsSlider_SetIntValue                       = 755,
    KODICall_GUI_Control_SettingsSlider_GetIntValue                       = 756,
    KODICall_GUI_Control_SettingsSlider_SetIntInterval                    = 757,
    KODICall_GUI_Control_SettingsSlider_SetPercentage                     = 758,
    KODICall_GUI_Control_SettingsSlider_GetPercentage                     = 759,
    KODICall_GUI_Control_SettingsSlider_SetFloatRange                     = 760,
    KODICall_GUI_Control_SettingsSlider_SetFloatValue                     = 761,
    KODICall_GUI_Control_SettingsSlider_GetFloatValue                     = 762,
    KODICall_GUI_Control_SettingsSlider_SetFloatInterval                  = 763,

    KODICall_GUI_Control_Slider_SetVisible                                = 770,
    KODICall_GUI_Control_Slider_SetEnabled                                = 771,
    KODICall_GUI_Control_Slider_GetDescription                            = 772,
    KODICall_GUI_Control_Slider_SetIntRange                               = 773,
    KODICall_GUI_Control_Slider_SetIntValue                               = 774,
    KODICall_GUI_Control_Slider_GetIntValue                               = 775,
    KODICall_GUI_Control_Slider_SetIntInterval                            = 776,
    KODICall_GUI_Control_Slider_SetPercentage                             = 777,
    KODICall_GUI_Control_Slider_GetPercentage                             = 778,
    KODICall_GUI_Control_Slider_SetFloatRange                             = 779,
    KODICall_GUI_Control_Slider_SetFloatValue                             = 780,
    KODICall_GUI_Control_Slider_GetFloatValue                             = 781,
    KODICall_GUI_Control_Slider_SetFloatInterval                          = 782,

    KODICall_GUI_Control_Spin_SetVisible                                  = 790,
    KODICall_GUI_Control_Spin_SetEnabled                                  = 791,
    KODICall_GUI_Control_Spin_SetText                                     = 792,
    KODICall_GUI_Control_Spin_Reset                                       = 793,
    KODICall_GUI_Control_Spin_SetType                                     = 794,
    KODICall_GUI_Control_Spin_AddStringLabel                              = 795,
    KODICall_GUI_Control_Spin_AddIntLabel                                 = 796,
    KODICall_GUI_Control_Spin_SetStringValue                              = 797,
    KODICall_GUI_Control_Spin_GetStringValue                              = 798,
    KODICall_GUI_Control_Spin_SetIntRange                                 = 799,
    KODICall_GUI_Control_Spin_SetIntValue                                 = 800,
    KODICall_GUI_Control_Spin_GetIntValue                                 = 801,
    KODICall_GUI_Control_Spin_SetFloatRange                               = 802,
    KODICall_GUI_Control_Spin_SetFloatValue                               = 803,
    KODICall_GUI_Control_Spin_GetFloatValue                               = 804,
    KODICall_GUI_Control_Spin_SetFloatInterval                            = 805,

    KODICall_GUI_Control_TextBox_SetVisible                               = 810,
    KODICall_GUI_Control_TextBox_Reset                                    = 811,
    KODICall_GUI_Control_TextBox_SetText                                  = 812,
    KODICall_GUI_Control_TextBox_GetText                                  = 813,
    KODICall_GUI_Control_TextBox_Scroll                                   = 814,
    KODICall_GUI_Control_TextBox_SetAutoScrolling                         = 815,

    KODICall_GUI_Dialogs_ExtendedProgress_New                             = 830,
    KODICall_GUI_Dialogs_ExtendedProgress_Delete                          = 831,
    KODICall_GUI_Dialogs_ExtendedProgress_Title                           = 832,
    KODICall_GUI_Dialogs_ExtendedProgress_SetTitle                        = 833,
    KODICall_GUI_Dialogs_ExtendedProgress_Text                            = 834,
    KODICall_GUI_Dialogs_ExtendedProgress_SetText                         = 835,
    KODICall_GUI_Dialogs_ExtendedProgress_IsFinished                      = 836,
    KODICall_GUI_Dialogs_ExtendedProgress_MarkFinished                    = 837,
    KODICall_GUI_Dialogs_ExtendedProgress_Percentage                      = 838,
    KODICall_GUI_Dialogs_ExtendedProgress_SetPercentage                   = 839,
    KODICall_GUI_Dialogs_ExtendedProgress_SetProgress                     = 840,

    KODICall_GUI_Dialogs_FileBrowser_ShowAndGetDirectory                  = 850,
    KODICall_GUI_Dialogs_FileBrowser_ShowAndGetFile                       = 851,
    KODICall_GUI_Dialogs_FileBrowser_ShowAndGetFileFromDir                = 852,
    KODICall_GUI_Dialogs_FileBrowser_ShowAndGetFileList                   = 853,
    KODICall_GUI_Dialogs_FileBrowser_ShowAndGetSource                     = 854,
    KODICall_GUI_Dialogs_FileBrowser_ShowAndGetImage                      = 855,
    KODICall_GUI_Dialogs_FileBrowser_ShowAndGetImageList                  = 856,

    KODICall_GUI_Dialogs_Keyboard_ShowAndGetInputWithHead                 = 870,
    KODICall_GUI_Dialogs_Keyboard_ShowAndGetInput                         = 871,
    KODICall_GUI_Dialogs_Keyboard_ShowAndGetNewPasswordWithHead           = 872,
    KODICall_GUI_Dialogs_Keyboard_ShowAndGetNewPassword                   = 873,
    KODICall_GUI_Dialogs_Keyboard_ShowAndVerifyNewPasswordWithHead        = 874,
    KODICall_GUI_Dialogs_Keyboard_ShowAndVerifyNewPassword                = 875,
    KODICall_GUI_Dialogs_Keyboard_ShowAndVerifyPassword                   = 876,
    KODICall_GUI_Dialogs_Keyboard_ShowAndGetFilter                        = 877,
    KODICall_GUI_Dialogs_Keyboard_SendTextToActiveKeyboard                = 878,
    KODICall_GUI_Dialogs_Keyboard_isKeyboardActivated                     = 879,

    KODICall_GUI_Dialogs_Numeric_ShowAndVerifyNewPassword                 = 890,
    KODICall_GUI_Dialogs_Numeric_ShowAndVerifyPassword                    = 891,
    KODICall_GUI_Dialogs_Numeric_ShowAndVerifyInput                       = 892,
    KODICall_GUI_Dialogs_Numeric_ShowAndGetTime                           = 893,
    KODICall_GUI_Dialogs_Numeric_ShowAndGetDate                           = 894,
    KODICall_GUI_Dialogs_Numeric_ShowAndGetIPAddress                      = 895,
    KODICall_GUI_Dialogs_Numeric_ShowAndGetNumber                         = 896,
    KODICall_GUI_Dialogs_Numeric_ShowAndGetSeconds                        = 897,

    KODICall_GUI_Dialogs_OK_ShowAndGetInputSingleText                     = 910,
    KODICall_GUI_Dialogs_OK_ShowAndGetInputLineText                       = 911,

    KODICall_GUI_Dialogs_Progress_New                                     = 930,
    KODICall_GUI_Dialogs_Progress_Delete                                  = 931,
    KODICall_GUI_Dialogs_Progress_Open                                    = 932,
    KODICall_GUI_Dialogs_Progress_SetHeading                              = 933,
    KODICall_GUI_Dialogs_Progress_SetLine                                 = 934,
    KODICall_GUI_Dialogs_Progress_SetCanCancel                            = 935,
    KODICall_GUI_Dialogs_Progress_IsCanceled                              = 936,
    KODICall_GUI_Dialogs_Progress_SetPercentage                           = 937,
    KODICall_GUI_Dialogs_Progress_GetPercentage                           = 938,
    KODICall_GUI_Dialogs_Progress_ShowProgressBar                         = 939,
    KODICall_GUI_Dialogs_Progress_SetProgressMax                          = 940,
    KODICall_GUI_Dialogs_Progress_SetProgressAdvance                      = 941,
    KODICall_GUI_Dialogs_Progress_Abort                                   = 942,

    KODICall_GUI_Dialogs_Select_Show                                      = 950,

    KODICall_GUI_Dialogs_TextViewer_Show                                  = 970,

    KODICall_GUI_Dialogs_YesNo_ShowAndGetInputSingleText                  = 980,
    KODICall_GUI_Dialogs_YesNo_ShowAndGetInputLineText                    = 981,
    KODICall_GUI_Dialogs_YesNo_ShowAndGetInputLineButtonText              = 982,

    KODICall_Player_AddonInfoTagMusic_GetFromPlayer                      = 1000,
    KODICall_Player_AddonInfoTagMusic_Release                            = 1001,

    KODICall_Player_AddonInfoTagVideo_GetFromPlayer                      = 1010,
    KODICall_Player_AddonInfoTagVideo_Release                            = 1011,

    KODICall_Player_PlayList_New                                         = 1020,
    KODICall_Player_PlayList_Delete                                      = 1021,
    KODICall_Player_PlayList_LoadPlaylist                                = 1022,
    KODICall_Player_PlayList_AddItemURL                                  = 1023,
    KODICall_Player_PlayList_AddItemList                                 = 1024,
    KODICall_Player_PlayList_RemoveItem                                  = 1025,
    KODICall_Player_PlayList_ClearList                                   = 1026,
    KODICall_Player_PlayList_GetListSize                                 = 1027,
    KODICall_Player_PlayList_GetListPosition                             = 1028,
    KODICall_Player_PlayList_Shuffle                                     = 1029,
    KODICall_Player_PlayList_GetItem                                     = 1030,

    KODICall_Player_AddonPlayer_GetSupportedMedia                        = 1050,
    KODICall_Player_AddonPlayer_New                                      = 1051,
    KODICall_Player_AddonPlayer_Delete                                   = 1052,
    KODICall_Player_AddonPlayer_SetCallbacks                             = 1053,
    KODICall_Player_AddonPlayer_PlayFile                                 = 1054,
    KODICall_Player_AddonPlayer_PlayFileItem                             = 1055,
    KODICall_Player_AddonPlayer_PlayList                                 = 1056,
    KODICall_Player_AddonPlayer_Stop                                     = 1057,
    KODICall_Player_AddonPlayer_Pause                                    = 1058,
    KODICall_Player_AddonPlayer_PlayNext                                 = 1059,
    KODICall_Player_AddonPlayer_PlayPrevious                             = 1060,
    KODICall_Player_AddonPlayer_PlaySelected                             = 1061,
    KODICall_Player_AddonPlayer_IsPlaying                                = 1062,
    KODICall_Player_AddonPlayer_IsPlayingAudio                           = 1063,
    KODICall_Player_AddonPlayer_IsPlayingVideo                           = 1064,
    KODICall_Player_AddonPlayer_IsPlayingRDS                             = 1065,
    KODICall_Player_AddonPlayer_GetPlayingFile                           = 1066,
    KODICall_Player_AddonPlayer_GetTotalTime                             = 1067,
    KODICall_Player_AddonPlayer_GetTime                                  = 1068,
    KODICall_Player_AddonPlayer_SeekTime                                 = 1069,
    KODICall_Player_AddonPlayer_GetAvailableVideoStreams                 = 1070,
    KODICall_Player_AddonPlayer_SetVideoStream                           = 1071,
    KODICall_Player_AddonPlayer_GetAvailableAudioStreams                 = 1072,
    KODICall_Player_AddonPlayer_SetAudioStream                           = 1073,
    KODICall_Player_AddonPlayer_GetAvailableSubtitleStreams              = 1074,
    KODICall_Player_AddonPlayer_SetSubtitleStream                        = 1075,
    KODICall_Player_AddonPlayer_ShowSubtitles                            = 1076,
    KODICall_Player_AddonPlayer_GetCurrentSubtitleName                   = 1077,
    KODICall_Player_AddonPlayer_AddSubtitle                              = 1078,

    KODICall_PVR_AddMenuHook                                             = 1200,
    KODICall_PVR_Recording                                               = 1201,

    KODICall_PVR_EpgEntry                                                = 1210,
    KODICall_PVR_ChannelEntry                                            = 1211,
    KODICall_PVR_TimerEntry                                              = 1212,
    KODICall_PVR_RecordingEntry                                          = 1213,
    KODICall_PVR_ChannelGroup                                            = 1214,
    KODICall_PVR_ChannelGroupMember                                      = 1215,

    KODICall_PVR_TriggerTimerUpdate                                      = 1230,
    KODICall_PVR_TriggerRecordingUpdate                                  = 1231,
    KODICall_PVR_TriggerChannelUpdate                                    = 1232,
    KODICall_PVR_TriggerEpgUpdate                                        = 1233,
    KODICall_PVR_TriggerChannelGroupsUpdate                              = 1234
  } KODI_API_Calls;

  typedef enum KODI_API_Packets
  {
    KODIPacket_RequestedResponse      = 1,
    KODIPacket_Status                 = 2

  } KODI_API_Packets;

  typedef struct KODI_API_ErrorTranslator
  {
    uint32_t    errorCode;
    const char* errorName;
  } KODI_API_ErrorTranslator;

  extern const KODI_API_ErrorTranslator errorTranslator[];

  /// @{
  /// @brief
  typedef enum KODI_API_Datatype
  {
    /// @brief
    API_DATATYPE_NULL           = 0x00000000,

    /// @{
    /// @brief
    API_CHAR                    = 0x00000001,
    /// @brief
    API_SIGNED_CHAR             = 0x00000002,
    /// @brief
    API_UNSIGNED_CHAR           = 0x00000003,
    /// @}

    /// @{
    /// @brief
    API_SHORT                   = 0x00000101,
    /// @brief
    API_SIGNED_SHORT            = 0x00000102,
    /// @brief
    API_UNSIGNED_SHORT          = 0x00000103,
    /// @}

    /// @{
    /// @brief
    API_INT                     = 0x00000201,
    /// @brief
    API_SIGNED_INT              = 0x00000202,
    /// @brief
    API_UNSIGNED_INT            = 0x00000203,
    /// @}

    /// @{
    /// @brief
    API_LONG                    = 0x00000301,
    /// @brief
    API_SIGNED_LONG             = 0x00000302,
    /// @brief
    API_UNSIGNED_LONG           = 0x00000303,
    /// @}

    /// @{
    /// @brief
    API_FLOAT                   = 0x00000501,
    /// @brief
    API_DOUBLE                  = 0x00000511,
    /// @brief
    API_LONG_DOUBLE             = 0x00000521,
    /// @}

    /// @{
    /// @brief
    API_INT8_T                  = 0x00000701,
    /// @brief
    API_INT16_T                 = 0x00000702,
    /// @brief
    API_INT32_T                 = 0x00000703,
    /// @brief
    API_INT64_T                 = 0x00000704,
    /// @}

    /// @{
    /// @brief
    API_UINT8_T                 = 0x00000801,
    /// @brief
    API_UINT16_T                = 0x00000802,
    /// @brief
    API_UINT32_T                = 0x00000803,
    /// @brief
    API_UINT64_T                = 0x00000804,
    /// @}

    API_BOOLEAN                 = 0x00000901,
    API_STRING                  = 0x00000911,

    /// @{
    /// @brief
    API_PACKED                  = 0x00001001,
    /// @brief
    API_LB                      = 0x00001002,
    /// @brief
    API_UB                      = 0x00001003,
    /// @}

    /// @{
    /// @brief The layouts for the types API_DOUBLE_INT etc are simply
    ///
    /// ~~~~~~~~~~~~~
    /// struct
    /// {
    ///   double var;
    ///   int    loc;
    /// }
    ///
    /// This is documented in the man pages on the various datatypes.
    ///

    /// @brief
    API_FLOAT_INT               = 0x00002001,
    /// @brief
    API_DOUBLE_INT              = 0x00002002,
    /// @brief
    API_LONG_INT                = 0x00002003,
    /// @brief
    API_SHORT_INT               = 0x00002004,
    /// @brief
    API_2INT                    = 0x00002005,
    /// @brief
    API_LONG_DOUBLE_INT         = 0x00002006
    /// @}
  } KODI_API_Datatype;
  /// @}

  /**
   * @brief Currently fixed size for global shared memory between Kodi and Add-on.
   */
  #define DEFAULT_SHARED_MEM_SIZE 10*1024

  typedef struct KodiAPI_MessageIn
  {
    uint32_t              m_channel;
    uint32_t              m_serialNumber;
    uint32_t              m_opcode;
    uint32_t              m_dataLength;
    uint8_t               data[DEFAULT_SHARED_MEM_SIZE-(sizeof(uint32_t)*10)];
  } KodiAPI_MessageIn;

  typedef struct KodiAPI_MessageOut
  {
    uint32_t              m_channelId;
    uint32_t              m_requestID;
    uint32_t              m_dataLength;
    uint8_t               data[DEFAULT_SHARED_MEM_SIZE-(sizeof(uint32_t)*10)];
  } KodiAPI_MessageOut;

  typedef union msgType
  {
    KodiAPI_MessageIn   in;
    KodiAPI_MessageOut  out;
  } msgType;

  typedef struct KodiAPI_ShareData
  {
  #if (defined TARGET_WINDOWS)
    HANDLE          shmSegmentToAddon;
    HANDLE          shmSegmentToKodi;
  #elif (defined TARGET_POSIX)
    sem_t           shmSegmentToAddon;
    sem_t           shmSegmentToKodi;
  #endif
    msgType message;
  } KodiAPI_ShareData;

  #define MAX_SEM_COUNT 10 // To use for windows CreateSemaphore(...)

  #define IMPL_STREAM_PROPS                         \
    private:                                        \
      stream_vector              *m_streamVector;   \
      std::map<unsigned int, int> m_streamIndex;    \
      void UpdateIndex();

#ifdef __cplusplus
}; /* extern "C" */
#endif
