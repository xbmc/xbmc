#ifndef _EMU_MISC_H_
#define _EMU_MISC_H_

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
 
typedef void ( *PFV)(void);

#ifdef _XBOX
DECLARE_HANDLE(HDRVR);
#endif
typedef INT_PTR (CALLBACK* DLGPROC)(HWND, UINT, WPARAM, LPARAM);
typedef VOID (CALLBACK* WINEVENTPROC)(
  HWINEVENTHOOK hWinEventHook,
  DWORD event,
  HWND hwnd,
  LONG idObject,
  LONG idChild,
  DWORD idEventThread,
  DWORD dwmsEventTime);

typedef struct _DMOMediaType
{
  GUID majortype;
  GUID subtype;
  BOOL bFixedSizeSamples;
  BOOL bTemporalCompression;
  ULONG lSampleSize;
  GUID formattype;
  IUnknown *pUnk;
  ULONG cbFormat;
  BYTE *pbFormat;
}
DMO_MEDIA_TYPE;

#ifdef _XBOX
typedef struct _COSERVERINFO
{
  DWORD dwReserved1;
  LPWSTR pwszName;
  //    COAUTHINFO  * pAuthInfo;
  void * pAuthInfo;
  DWORD dwReserved2;
}
COSERVERINFO;
#endif

extern "C" HRESULT WINAPI dllMoFreeMediaType(DMO_MEDIA_TYPE* pmedia);  //msdmo.dll
extern "C" HRESULT WINAPI dllMoCopyMediaType(DMO_MEDIA_TYPE* pdst, const DMO_MEDIA_TYPE* psrc);
extern "C" HRESULT WINAPI dllMoInitMediaType(DMO_MEDIA_TYPE* pmedia, DWORD cbFormat);

/*extern "C" HRESULT WINAPI CoCreateInstance(
 REFCLSID rclsid,
 LPUNKNOWN pUnkOuter,
 DWORD dwClsContext,
 REFIID iid,
 LPVOID *ppv);*/

///////////////////////
// user32.dll
extern "C" BOOL WINAPI dllIsRectEmpty(const RECT *lprc);
extern "C" BOOL WINAPI dllEnableWindow(HWND hWnd, BOOL bEnable);
extern "C" UINT WINAPI dllGetDlgItemInt(HWND hDlg, int nIDDlgItem, BOOL *lpTranslated, BOOL bSigned);
extern "C" LRESULT WINAPI dllSendDlgItemMessageA(HWND hDlg, int nIDDlgItem, UINT Msg, WPARAM wParam, LPARAM lParam);
extern "C" INT_PTR WINAPI dllDialogBoxParamA(HINSTANCE hInstance, LPCSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam);
extern "C" UINT WINAPI dllGetDlgItemTextA(HWND hDlg, int nIDDlgItem, LPSTR lpString, int nMaxCount);
extern "C" int WINAPI dllMessageBoxA(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType);
extern "C" LONG WINAPI dllGetWindowLongA(HWND hWnd, int nIndex);
extern "C" HWND WINAPI dllGetDlgItem(HWND hDlg, int nIDDlgItem);
extern "C" BOOL WINAPI dllCheckDlgButton(HWND hDlg, int nIDButton, UINT uCheck);
extern "C" HWINEVENTHOOK WINAPI dllSetDlgItemInt(DWORD eventMin, DWORD eventMax, HMODULE hmodWinEventProc, WINEVENTPROC pfnWinEventProc, DWORD idProcess, DWORD idThread, DWORD dwFlags);
extern "C" BOOL WINAPI dllShowWindow(HWND hWnd, int nCmdShow);
extern "C" BOOL WINAPI dllEndDialog(HWND hDlg, INT_PTR nResult);
extern "C" BOOL WINAPI dllSetDlgItemTextA(HWND hDlg, int nIDDlgItem, LPCSTR lpString);
extern "C" LONG WINAPI dllSetWindowLongA(HWND hWnd, int nIndex, LONG dwNewLong);
extern "C" BOOL WINAPI dllDestroyWindow(HWND hWnd);
extern "C" HWND WINAPI dllCreateDialogParamA(HINSTANCE hInstance, LPCSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam);
extern "C" BOOL WINAPI dllPostMessageA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
extern "C" LRESULT WINAPI dllSendMessageA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
extern "C" HWND WINAPI dllSetFocus(HWND hWnd);
extern "C" int WINAPI dllLoadStringA( HINSTANCE instance, UINT resource_id, LPSTR buffer, INT buflen );
///
extern "C" HWND WINAPI dllGetDesktopWindow();
extern "C" UINT WINAPI dllGetDeviceCaps(int hdc, int unk);
extern "C" UINT WINAPI dllReleaseDC(HANDLE hWnd, HANDLE hDC);
extern "C" UINT WINAPI dllGetDC(HANDLE hWnd);
extern "C" UINT WINAPI dllGetWindowRect(HWND win, RECT *r);
extern "C" UINT WINAPI dllShowCursor(int show);
extern "C" int WINAPI dllGetSystemMetrics(int nIndex);
extern "C" int WINAPI dllMonitorFromWindow(HWND win, int flags);
extern "C" int WINAPI dllMonitorFromRect(RECT *r, int flags);
extern "C" int WINAPI dllMonitorFromPoint(void *p, int flags);

extern "C" BOOL    WINAPI dllGetCursorPos(LPPOINT lpPoint);
extern "C" HCURSOR WINAPI dllSetCursor(HCURSOR hCursor);
extern "C" HCURSOR WINAPI dllLoadCursorA(HINSTANCE hInstance, LPCSTR lpCursorName);
extern "C" UINT    WINAPI dllRegisterWindowMessageA(LPCSTR lpString);
extern "C" HBRUSH  WINAPI dllGetSysColorBrush(int nIndex);
extern "C" DWORD   WINAPI dllGetSysColor(int nIndex);
extern "C" UINT    WINAPI dllRegisterClipboardFormatA(LPCSTR lpszFormat);

#ifdef _XBOX
typedef BOOL (*MONITORENUMPROC)(HMONITOR, HDC, LPRECT, LPARAM);
#endif
extern "C" int WINAPI dllEnumDisplayMonitors(HDC hdc, LPRECT lprcClip, MONITORENUMPROC lpfnEnum, LPARAM dwData);

#ifdef _XBOX
typedef struct tagMONITORINFO
{
  DWORD cbSize;
  RECT rcMonitor;
  RECT rcWork;
  DWORD dwFlags;
}
MONITORINFO, *LPMONITORINFO;
#define CCHDEVICENAME 8
typedef struct tagMONITORINFOEX
{
  DWORD cbSize;
  RECT rcMonitor;
  RECT rcWork;
  DWORD dwFlags;
  TCHAR szDevice[CCHDEVICENAME];
}
MONITORINFOEX, *LPMONITORINFOEX;
#endif
extern "C" int WINAPI dllGetMonitorInfoA(void *mon, LPMONITORINFO lpmi);

extern "C" int WINAPI dllEnumDisplayDevicesA(const char *device, int devnum, void *dispdev, int flags);
extern "C" int WINAPI dllIsWindowVisible(HWND win);
extern "C" HWND WINAPI dllGetActiveWindow(void);


#ifdef _XBOX
typedef struct tagLOGPALETTE
{
  WORD palNumEntries;
  WORD palVersion;
  PALETTEENTRY palPalEntry[1];
}
LOGPALETTE;
#endif
extern "C" HPALETTE WINAPI dllCreatePalette(CONST LOGPALETTE *lpgpl);

long VobSubPFSeek(unsigned long pf, long offset);
unsigned long VobSubPFWrite(unsigned long pf, void* data, unsigned long size);
unsigned long VobSubPFRead(unsigned long pf, void* data, unsigned long size);
void VobSubPFReserve(unsigned long pf, unsigned long size);
unsigned long VobSubPFOpen(int);
void VobSubPFClose(unsigned long pf);

extern "C" LRESULT WINAPI dllDefDriverProc(DWORD_PTR dwDriverIdentifier, HDRVR hdrvr, UINT uMsg, LPARAM lParam1, LPARAM lParam2);

//VERSION.DLL
extern "C" DWORD APIENTRY dllGetFileVersionInfoSizeA(LPCSTR lptstrFilename, LPDWORD lpdwHandle);
extern "C" BOOL APIENTRY dllVerQueryValueA(const LPVOID pBlock, LPSTR lpSubBlock, LPVOID * lplpBuffer, PUINT puLen);
extern "C" BOOL APIENTRY dllGetFileVersionInfoA(LPCSTR lptstrFilename, DWORD dwHandle, DWORD dwLen, LPVOID lpData);

//comdlg32.dll
typedef UINT_PTR (CALLBACK *LPOFNHOOKPROC) (HWND, UINT, WPARAM, LPARAM);

#ifdef _XBOX
typedef struct tagOFNA
{
  DWORD lStructSize;
  HWND hwndOwner;
  HINSTANCE hInstance;
  LPCSTR lpstrFilter;
  LPSTR lpstrCustomFilter;
  DWORD nMaxCustFilter;
  DWORD nFilterIndex;
  LPSTR lpstrFile;
  DWORD nMaxFile;
  LPSTR lpstrFileTitle;
  DWORD nMaxFileTitle;
  LPCSTR lpstrInitialDir;
  LPCSTR lpstrTitle;
  DWORD Flags;
  WORD nFileOffset;
  WORD nFileExtension;
  LPCSTR lpstrDefExt;
  LPARAM lCustData;
  LPOFNHOOKPROC lpfnHook;
  LPCSTR lpTemplateName;
}
OPENFILENAMEA, *LPOPENFILENAMEA;
#endif

extern "C" BOOL APIENTRY dllGetOpenFileNameA(LPOPENFILENAMEA lpFileName); //comdlg32.dll

////////////////////////////////////////////////
//GDI32.dll
extern "C" COLORREF WINAPI dllSetTextColor(HDC hdc, COLORREF crColor);
extern "C" BOOL WINAPI dllBitBlt(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight, HDC hdcSrc, int nXSrc, int nYSrc, DWORD dwRop);
extern "C" BOOL WINAPI dllExtTextOutA(HDC hdc, int X, int Y, UINT fuOptions, CONST RECT* lprc, LPCTSTR lpString, UINT cbCount, CONST INT* lpDx);
extern "C" HGDIOBJ WINAPI dllGetStockObject(int fnObject);
extern "C" COLORREF WINAPI dllSetBkColor(HDC hdc, COLORREF crColor);
extern "C" HDC WINAPI dllCreateCompatibleDC(HDC hdc);
extern "C" HBITMAP WINAPI dllCreateBitmap(int nWidth, int nHeight, UINT cPlanes, UINT cBitsPerPel, CONST VOID *lpvBits);
extern "C" HGDIOBJ WINAPI dllSelectObject(HDC hdc, HGDIOBJ hgdiobj);
extern "C" HFONT WINAPI dllCreateFontA(int nHeight, int nWidth, int nEscapement, int nOrientation, int fnWeight, DWORD fdwItalic, DWORD fdwUnderline, DWORD fdwStrikeOut, DWORD fdwCharSet, DWORD fdwOutputPrecision, DWORD fdwClipPrecision, DWORD fdwQuality, DWORD fdwPitchAndFamily, LPCTSTR lpszFace);
extern "C" BOOL WINAPI dllDeleteDC(HDC hdc);
extern "C" int WINAPI dllSetBkMode(HDC hdc, int iBkMode);
extern "C" COLORREF WINAPI dllGetPixel(HDC hdc, int nXPos, int nYPos);
extern "C" BOOL WINAPI dllDeleteObject(HGDIOBJ hObject);

////////////////////////////////////////////////
//ddraw.dll
extern "C" int WINAPI dllDirectDrawCreate(void);

////////////////////////////////////////////////
//comctl32.dll
extern "C" HWND WINAPI dllCreateUpDownControl (DWORD style, INT x, INT y, INT cx, INT cy,
      HWND parent, INT id, HINSTANCE inst,
      HWND buddy, INT maxVal, INT minVal, INT curVal);

////////////////////////////////////////////////
//winmm.dll
#ifdef _XBOX
typedef struct
{
  UINT wPeriodMin;
  UINT wPeriodMax;
}
TIMECAPS, *LPTIMECAPS;
#endif

extern "C" MMRESULT WINAPI dlltimeGetDevCaps(LPTIMECAPS lpCaps, UINT wSize);
extern "C" MMRESULT WINAPI dlltimeBeginPeriod(UINT wPeriod);
extern "C" MMRESULT WINAPI dlltimeEndPeriod(UINT wPeriod);
extern "C" MMRESULT WINAPI dllwaveOutGetNumDevs(void);
extern "C" int WINAPI dllwsprintfA(char* string, const char* format, ...);

#endif

