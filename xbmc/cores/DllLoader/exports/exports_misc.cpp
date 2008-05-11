
#include "stdafx.h"
#include "../DllLoader.h"
#include "emu_misc.h"
#include "emu_msvcrt.h"

Export export_winmm[] = 
{
  { "timeGetTime",                -1, timeGetTime,                   NULL },
  { "DefDriverProc",              -1, dllDefDriverProc,              NULL },
  { "timeGetDevCaps",             -1, dlltimeGetDevCaps,             NULL },
  { "timeBeginPeriod",            -1, dlltimeBeginPeriod,            NULL },
  { "timeEndPeriod",              -1, dlltimeEndPeriod,              NULL },
  { "waveOutGetNumDevs",          -1, dllwaveOutGetNumDevs,          NULL },
  { NULL, NULL, NULL, NULL }
};

Export export_msdmo[] = 
{
  { "MoFreeMediaType",            -1, dllMoFreeMediaType,            NULL },
  { "MoCopyMediaType",            -1, dllMoCopyMediaType,            NULL },
  { "MoInitMediaType",            -1, dllMoInitMediaType,            NULL },
  { NULL, NULL, NULL, NULL }
};

Export export_user32[] =
{
  { "IsRectEmpty",                -1, dllIsRectEmpty,                NULL },
  { "EnableWindow",               -1, dllEnableWindow,               NULL },
  { "GetDlgItemInt",              -1, dllGetDlgItemInt,              NULL },
  { "SendDlgItemMessageA",        -1, dllSendDlgItemMessageA,        NULL },
  { "DialogBoxParamA",            -1, dllDialogBoxParamA,            NULL },
  { "GetDlgItemTextA",            -1, dllGetDlgItemTextA,            NULL },
  { "MessageBoxA",                -1, dllMessageBoxA,                NULL },
  { "GetWindowLongA",             -1, dllGetWindowLongA,             NULL },
  { "GetDlgItem",                 -1, dllGetDlgItem,                 NULL },
  { "CheckDlgButton",             -1, dllCheckDlgButton,             NULL },
  { "SetDlgItemInt",              -1, dllSetDlgItemInt,              NULL },
  { "ShowWindow",                 -1, dllShowWindow,                 NULL },
  { "EndDialog",                  -1, dllEndDialog,                  NULL },
  { "SetDlgItemTextA",            -1, dllSetDlgItemTextA,            NULL },
  { "SetWindowLongA",             -1, dllSetWindowLongA,             NULL },
  { "DestroyWindow",              -1, dllDestroyWindow,              NULL },
  { "CreateDialogParamA",         -1, dllCreateDialogParamA,         NULL },
  { "PostMessageA",               -1, dllPostMessageA,               NULL },
  { "SendMessageA",               -1, dllSendMessageA,               NULL },
  { "SetFocus",                   -1, dllSetFocus,                   NULL },
  { "wsprintfA",                  -1, dllwsprintfA,                  NULL },

  { "GetDesktopWindow",           -1, dllGetDesktopWindow,           NULL },
  { "GetDC",                      -1, dllGetDC,                      NULL },
  { "ReleaseDC",                  -1, dllReleaseDC,                  NULL },
  { "GetWindowRect",              -1, dllGetWindowRect,              NULL },
  { "ShowCursor",                 -1, dllShowCursor,                 NULL },
  { "GetSystemMetrics",           -1, dllGetSystemMetrics,           NULL },
  { "MonitorFromWindow",          -1, dllMonitorFromWindow,          NULL },
  { "MonitorFromRect",            -1, dllMonitorFromRect,            NULL },
  { "MonitorFromPoint",           -1, dllMonitorFromPoint,           NULL },
  { "EnumDisplayMonitors",        -1, dllEnumDisplayMonitors,        NULL },
  { "GetMonitorInfoA",            -1, dllGetMonitorInfoA,            NULL },

  { "EnumDisplayDevicesA",        -1, dllEnumDisplayDevicesA,        NULL },
  { "IsWindowVisible",            -1, dllIsWindowVisible,            NULL },
  { "GetActiveWindow",            -1, dllGetActiveWindow,            NULL },
  { "LoadStringA",                -1, dllLoadStringA,                NULL },
  { "GetCursorPos",               -1, dllGetCursorPos,               NULL },
  { "LoadCursorA",                -1, dllLoadCursorA,                NULL },
  { "SetCursor",                  -1, dllSetCursor,                  NULL },
  { "RegisterWindowMessageA",     -1, dllRegisterWindowMessageA,     NULL },
  { "GetSysColorBrush",           -1, dllGetSysColorBrush,           NULL },
  { "GetSysColor",                -1, dllGetSysColor,                NULL },
  { "RegisterClipboardFormatA",   -1, dllRegisterClipboardFormatA,   NULL },
  { "GetIconInfo",                -1, dllGetIconInfo,                NULL },
  { NULL, NULL, NULL, NULL }
};

Export export_xbmc_vobsub[] =
{
  { "pf_seek",                    -1, VobSubPFSeek,                  NULL },
  { "pf_write",                   -1, VobSubPFWrite,                 NULL },
  { "pf_read",                    -1, VobSubPFRead,                  NULL },
  { "pf_open",                    -1, VobSubPFOpen,                  NULL },
  { "pf_close",                   -1, VobSubPFClose,                 NULL },
  { "pf_reserve",                 -1, VobSubPFReserve,               NULL },
  { NULL, NULL, NULL, NULL }
};

Export export_version[] =
{
  { "GetFileVersionInfoSizeA",    -1, dllGetFileVersionInfoSizeA,    NULL },
  { "VerQueryValueA",             -1, dllVerQueryValueA,             NULL },
  { "GetFileVersionInfoA",        -1, dllGetFileVersionInfoA,        NULL },
  { NULL, NULL, NULL, NULL }
};

Export export_comdlg32[] =
{
  { "GetOpenFileNameA",-1, dllGetOpenFileNameA, NULL },
  { NULL, NULL, NULL, NULL }
};

Export export_gdi32[] = 
{
  { "SetTextColor",               -1, dllSetTextColor,               NULL },
  { "BitBlt",                     -1, dllBitBlt,                     NULL },
  { "ExtTextOutA",                -1, dllExtTextOutA,                NULL },
  { "GetStockObject",             -1, dllGetStockObject,             NULL },
  { "SetBkColor",                 -1, dllSetBkColor,                 NULL },
  { "CreateCompatibleDC",         -1, dllCreateCompatibleDC,         NULL },
  { "CreateBitmap",               -1, dllCreateBitmap,               NULL },
  { "SelectObject",               -1, dllSelectObject,               NULL },
  { "CreateFontA",                -1, dllCreateFontA,                NULL },
  { "DeleteDC",                   -1, dllDeleteDC,                   NULL },
  { "SetBkMode",                  -1, dllSetBkMode,                  NULL },
  { "GetPixel",                   -1, dllGetPixel,                   NULL },
  { "DeleteObject",               -1, dllDeleteObject,               NULL },
  { "GetDeviceCaps",              -1, dllGetDeviceCaps,              NULL },
  { "CreatePalette",              -1, dllCreatePalette,              NULL },
  { "StretchDIBits",              -1, dllStretchDIBits,              NULL },
  { "RectVisible",                -1, dllRectVisible,                NULL },
  { "SaveDC",                     -1, dllSaveDC,                     NULL },
  { "GetClipBox",                 -1, dllGetClipBox,                 NULL },
  { "CreateRectRgnIndirect",      -1, dllCreateRectRgnIndirect,      NULL },
  { "ExtSelectClipRgn",           -1, dllExtSelectClipRgn,           NULL },
  { "SetStretchBltMode",          -1, dllSetStretchBltMode,          NULL },
  { "SetDIBitsToDevice",          -1, dllSetDIBitsToDevice,          NULL },
  { "RestoreDC",                  -1, dllRestoreDC,                  NULL },
  { "GetObjectA",                 -1, dllGetObjectA,                 NULL },
  { "CombineRgn",                 -1, dllCombineRgn,                 NULL },
  { NULL, NULL, NULL, NULL }
};

Export export_ddraw[] = 
{
  { "DirectDrawCreate",-1, dllDirectDrawCreate, NULL },
  { NULL, NULL, NULL, NULL }
};

Export export_comctl32[] = 
{
  { "CreateUpDownControl", 16, dllCreateUpDownControl, NULL },
//  { "InitCommonControls", 17, dllInitCommonControls, NULL },
  { NULL, NULL, NULL, NULL }
};

Export export_iconvx[] = 
{
  //{ "_libiconv_version",-1, &_libiconv_version, NULL },  // seems to be missing in our version
  { "libiconv",                   -1, libiconv,                      NULL },
  { "libiconv_close",             -1, libiconv_close,                NULL },
  { "libiconv_open",              -1, libiconv_open,                 NULL },
  { "libiconv_set_relocation_prefix",-1, libiconv_set_relocation_prefix, NULL },
  { "libiconvctl",                -1, libiconvctl,                   NULL },
  { "libiconvlist",               -1, libiconvlist,                  NULL },
  { NULL, NULL, NULL, NULL }
};
