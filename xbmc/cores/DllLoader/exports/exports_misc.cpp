
#include "..\..\..\stdafx.h"
#include "..\DllLoaderContainer.h"
#include "emu_misc.h"
#include "emu_msvcrt.h"

void export_winmm()
{
  g_dlls.winmm.AddExport("timeGetTime", (unsigned long)timeGetTime);
  g_dlls.winmm.AddExport("DefDriverProc", (unsigned long)dllDefDriverProc);
  g_dlls.winmm.AddExport("timeGetDevCaps", (unsigned long)dlltimeGetDevCaps);
  g_dlls.winmm.AddExport("timeBeginPeriod", (unsigned long)dlltimeBeginPeriod);
  g_dlls.winmm.AddExport("timeEndPeriod", (unsigned long)dlltimeEndPeriod);
  g_dlls.winmm.AddExport("waveOutGetNumDevs", (unsigned long)dllwaveOutGetNumDevs);
}

void export_msdmo()
{
  g_dlls.msdmo.AddExport("MoFreeMediaType", (unsigned long)dllMoFreeMediaType);
  g_dlls.msdmo.AddExport("MoCopyMediaType", (unsigned long)dllMoCopyMediaType);
  g_dlls.msdmo.AddExport("MoInitMediaType", (unsigned long)dllMoInitMediaType);
}

void export_user32()
{
  g_dlls.user32.AddExport("IsRectEmpty", (unsigned long)dllIsRectEmpty);
  g_dlls.user32.AddExport("EnableWindow", (unsigned long)dllEnableWindow);
  g_dlls.user32.AddExport("GetDlgItemInt", (unsigned long)dllGetDlgItemInt);
  g_dlls.user32.AddExport("SendDlgItemMessageA", (unsigned long)dllSendDlgItemMessageA);
  g_dlls.user32.AddExport("DialogBoxParamA", (unsigned long)dllDialogBoxParamA);
  g_dlls.user32.AddExport("GetDlgItemTextA", (unsigned long)dllGetDlgItemTextA);
  g_dlls.user32.AddExport("MessageBoxA", (unsigned long)dllMessageBoxA);
  g_dlls.user32.AddExport("GetWindowLongA", (unsigned long)dllGetWindowLongA);
  g_dlls.user32.AddExport("GetDlgItem", (unsigned long)dllGetDlgItem);
  g_dlls.user32.AddExport("CheckDlgButton", (unsigned long)dllCheckDlgButton);
  g_dlls.user32.AddExport("SetDlgItemInt", (unsigned long)dllSetDlgItemInt);
  g_dlls.user32.AddExport("ShowWindow", (unsigned long)dllShowWindow);
  g_dlls.user32.AddExport("EndDialog", (unsigned long)dllEndDialog);
  g_dlls.user32.AddExport("SetDlgItemTextA", (unsigned long)dllSetDlgItemTextA);
  g_dlls.user32.AddExport("SetWindowLongA", (unsigned long)dllSetWindowLongA);
  g_dlls.user32.AddExport("DestroyWindow", (unsigned long)dllDestroyWindow);
  g_dlls.user32.AddExport("CreateDialogParamA", (unsigned long)dllCreateDialogParamA);
  g_dlls.user32.AddExport("PostMessageA", (unsigned long)dllPostMessageA);
  g_dlls.user32.AddExport("SendMessageA", (unsigned long)dllSendMessageA);
  g_dlls.user32.AddExport("SetFocus", (unsigned long)dllSetFocus);
  g_dlls.user32.AddExport("wsprintfA", (unsigned long)dllwsprintfA);

  g_dlls.user32.AddExport("GetDesktopWindow", (unsigned long)dllGetDesktopWindow);
  g_dlls.user32.AddExport("GetDC", (unsigned long)dllGetDC);
  g_dlls.user32.AddExport("ReleaseDC", (unsigned long)dllReleaseDC);
  g_dlls.user32.AddExport("GetWindowRect", (unsigned long)dllGetWindowRect);
  g_dlls.user32.AddExport("ShowCursor", (unsigned long)dllShowCursor);
  g_dlls.user32.AddExport("GetSystemMetrics", (unsigned long)dllGetSystemMetrics);
  g_dlls.user32.AddExport("MonitorFromWindow", (unsigned long)dllMonitorFromWindow);
  g_dlls.user32.AddExport("MonitorFromRect", (unsigned long)dllMonitorFromRect);
  g_dlls.user32.AddExport("MonitorFromPoint", (unsigned long)dllMonitorFromPoint);
  g_dlls.user32.AddExport("EnumDisplayMonitors", (unsigned long)dllEnumDisplayMonitors);
  g_dlls.user32.AddExport("GetMonitorInfoA", (unsigned long)dllGetMonitorInfoA);

  g_dlls.user32.AddExport("EnumDisplayDevicesA", (unsigned long)dllEnumDisplayDevicesA);
  g_dlls.user32.AddExport("IsWindowVisible", (unsigned long)dllIsWindowVisible);
  g_dlls.user32.AddExport("GetActiveWindow", (unsigned long)dllGetActiveWindow);
}

void export_xbmc_vobsub()
{
  g_dlls.xbmc_vobsub.AddExport("pf_seek", (unsigned long)VobSubPFSeek);
  g_dlls.xbmc_vobsub.AddExport("pf_write", (unsigned long)VobSubPFWrite);
  g_dlls.xbmc_vobsub.AddExport("pf_read", (unsigned long)VobSubPFRead);
  g_dlls.xbmc_vobsub.AddExport("pf_open", (unsigned long)VobSubPFOpen);
  g_dlls.xbmc_vobsub.AddExport("pf_close", (unsigned long)VobSubPFClose);
  g_dlls.xbmc_vobsub.AddExport("pf_reserve", (unsigned long)VobSubPFReserve);
}

void export_version()
{
  g_dlls.version.AddExport("GetFileVersionInfoSizeA", (unsigned long)dllGetFileVersionInfoSizeA);
  g_dlls.version.AddExport("VerQueryValueA", (unsigned long)dllVerQueryValueA);
  g_dlls.version.AddExport("GetFileVersionInfoA", (unsigned long)dllGetFileVersionInfoA);
}

void export_comdlg32()
{
  g_dlls.comdlg32.AddExport("GetOpenFileNameA", (unsigned long)dllGetOpenFileNameA);
}

void export_gdi32()
{
  g_dlls.gdi32.AddExport("SetTextColor", (unsigned long)dllSetTextColor);
  g_dlls.gdi32.AddExport("BitBlt", (unsigned long)dllBitBlt);
  g_dlls.gdi32.AddExport("ExtTextOutA", (unsigned long)dllExtTextOutA);
  g_dlls.gdi32.AddExport("GetStockObject", (unsigned long)dllGetStockObject);
  g_dlls.gdi32.AddExport("SetBkColor", (unsigned long)dllSetBkColor);
  g_dlls.gdi32.AddExport("CreateCompatibleDC", (unsigned long)dllCreateCompatibleDC);
  g_dlls.gdi32.AddExport("CreateBitmap", (unsigned long)dllCreateBitmap);
  g_dlls.gdi32.AddExport("SelectObject", (unsigned long)dllSelectObject);
  g_dlls.gdi32.AddExport("CreateFontA", (unsigned long)dllCreateFontA);
  g_dlls.gdi32.AddExport("DeleteDC", (unsigned long)dllDeleteDC);
  g_dlls.gdi32.AddExport("SetBkMode", (unsigned long)dllSetBkMode);
  g_dlls.gdi32.AddExport("GetPixel", (unsigned long)dllGetPixel);
  g_dlls.gdi32.AddExport("DeleteObject", (unsigned long)dllDeleteObject);
  g_dlls.gdi32.AddExport("GetDeviceCaps", (unsigned long)dllGetDeviceCaps);
  g_dlls.gdi32.AddExport("CreatePalette", (unsigned long)dllCreatePalette);
}

void export_ddraw()
{
  g_dlls.ddraw.AddExport("DirectDrawCreate", (unsigned long)dllDirectDrawCreate);
}

void export_comctl32()
{
  g_dlls.comctl32.AddExport("CreateUpDownControl", 16, (unsigned long)dllCreateUpDownControl);
  g_dlls.comctl32.AddExport("InitCommonControls", 17, (unsigned long)dllCreateUpDownControl);
}
