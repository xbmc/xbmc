/*
 *      Copyright (C) 2005-2008 Team XBMC
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
#ifdef _WIN32PC
  { "DrawTextA",                  -1, DrawTextA,                     NULL },
  { "GetClientRect",              -1, GetClientRect,                 NULL },
  { "GetWindowTextA",             -1, GetWindowTextA,                NULL },
  { "GetWindowTextLengthA",       -1, GetWindowTextLengthA,          NULL },
  { "EnumWindows",                -1, EnumWindows,                   NULL },
  { "CloseClipboard",             -1, CloseClipboard,                NULL },
  { "GetClipboardData",           -1, GetClipboardData,              NULL },
  { "GetClipboardFormatNameA",    -1, GetClipboardFormatNameA,       NULL },
  { "EnumClipboardFormats",       -1, EnumClipboardFormats,          NULL },
  { "OpenClipboard",              -1, OpenClipboard,                 NULL },
  { "InvalidateRect",             -1, InvalidateRect,                NULL },
  { "EndPaint",                   -1, EndPaint,                      NULL },
  { "BeginPaint",                 -1, BeginPaint,                    NULL },
  { "DefWindowProcA",             -1, DefWindowProcA,                NULL },
  { "SetForegroundWindow",        -1, SetForegroundWindow,           NULL },
  { "CreateWindowExA",            -1, CreateWindowExA,               NULL },
  { "RegisterClassA",             -1, RegisterClassA,                NULL },
  { "LoadIconA",                  -1, LoadIconA,                     NULL },
  { "DispatchMessageA",           -1, DispatchMessageA,              NULL },
  { "TranslateMessage",           -1, TranslateMessage,              NULL },
  { "GetMessageA",                -1, GetMessageA,                   NULL },
#endif
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
#ifdef _WIN32PC
  { "SelectPalette",              -1, SelectPalette,                 NULL },
  { "StretchBlt",                 -1, StretchBlt,                    NULL },
  { "CreateFontIndirectA",        -1, CreateFontIndirectA,           NULL },
  { "CreateRectRgn",              -1, CreateRectRgn,                 NULL },
  { "SetWinMetaFileBits",         -1, SetWinMetaFileBits,            NULL },
  { "DeleteEnhMetaFile",          -1, DeleteEnhMetaFile,             NULL }, 
  { "GetEnhMetaFileHeader",       -1, GetEnhMetaFileHeader,          NULL },
  { "SetEnhMetaFileBits",         -1, SetEnhMetaFileBits,            NULL },
  { "GetDIBits",                  -1, GetDIBits,                     NULL },
  { "PlayEnhMetaFile",            -1, PlayEnhMetaFile,               NULL },
  { "RealizePalette",             -1, RealizePalette,                NULL },
  { "GetEnhMetaFilePaletteEntries", -1, GetEnhMetaFilePaletteEntries,NULL },
  { "CreateCompatibleBitmap",     -1, CreateCompatibleBitmap,        NULL },
  { "PatBlt",                     -1, PatBlt,                        NULL },
  { "SetBrushOrgEx",              -1, SetBrushOrgEx,                 NULL },
  { "CreateDIBPatternBrushPt",    -1, CreateDIBPatternBrushPt,       NULL },
  { "CreateDIBSection",           -1, CreateDIBSection,              NULL },
  { "CreateDCA",                  -1, CreateDCA,                     NULL },
  { "GetSystemPaletteEntries",    -1, GetSystemPaletteEntries,       NULL },
  { "SetDIBColorTable",           -1, SetDIBColorTable,              NULL },
#endif
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

extern "C" void* inflate();
extern "C" void* inflateEnd();
extern "C" void* inflateInit2_();
extern "C" void* inflateReset();

Export export_zlib[] =
{
  { "inflate",       -1, inflate,        NULL },
  { "inflateEnd",    -1, inflateEnd,     NULL },
  { "inflateInit2_", -1, inflateInit2_,  NULL },
  { "inflateReset",  -1, inflateReset,   NULL },
  { NULL, NULL, NULL, NULL }
};