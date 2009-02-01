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
#include <stdio.h>
//#include <xobjbase.h>
#ifndef _XBOX
#include <objbase.h>
#endif

#include "emu_misc.h"
#include "emu_dummy.h"
#include "emu_ole32.h"

#include "FileSystem/SpecialProtocol.h"

/*extern HRESULT WINAPI CoGetClassObject(
    REFCLSID rclsid, DWORD dwClsContext, COSERVERINFO *pServerInfo,
    REFIID iid, LPVOID *ppv);*/

////////////////////////////////////////////////////////////////////////
//msdmo.dll

//MoFreeMediaType  (MSDMO.@)
HRESULT WINAPI dllMoFreeMediaType(DMO_MEDIA_TYPE* pmedia)
{
  if (pmedia == NULL)
    return E_POINTER;

  if (pmedia->pUnk != NULL)
  {
    IUnknown_Release(pmedia->pUnk);
    pmedia->pUnk = NULL;
  }

  if (pmedia->pbFormat != NULL)
  {
    dllCoTaskMemFree(pmedia->pbFormat);
    pmedia->pbFormat = NULL;
  }

  return S_OK;
}

//MoCopyMediaType  (MSDMO.@)
HRESULT WINAPI dllMoCopyMediaType(DMO_MEDIA_TYPE* pdst,
                                  const DMO_MEDIA_TYPE* psrc)
{
  if (pdst == NULL || psrc == NULL)
    return E_POINTER;

  memcpy(&pdst->majortype, &psrc->majortype, sizeof(psrc->majortype));
  memcpy(&pdst->subtype, &psrc->subtype, sizeof(psrc->subtype));
  memcpy(&pdst->formattype, &psrc->formattype, sizeof(psrc->formattype));

  pdst->bFixedSizeSamples = psrc->bFixedSizeSamples;
  pdst->bTemporalCompression = psrc->bTemporalCompression;
  pdst->lSampleSize = psrc->lSampleSize;
  pdst->cbFormat = psrc->cbFormat;

  if (psrc->pbFormat != NULL && psrc->cbFormat > 0)
  {
    pdst->pbFormat = (BYTE *)dllCoTaskMemAlloc(psrc->cbFormat);
    if (pdst->pbFormat == NULL)
      return E_OUTOFMEMORY;

    memcpy(pdst->pbFormat, psrc->pbFormat, psrc->cbFormat);
  }
  else
    pdst->pbFormat = NULL;

  if (psrc->pUnk != NULL)
  {
    pdst->pUnk = psrc->pUnk;
    IUnknown_AddRef(pdst->pUnk);
  }
  else
    pdst->pUnk = NULL;

  return S_OK;
}

//MoInitMediaType  (MSDMO.@)
HRESULT WINAPI dllMoInitMediaType(DMO_MEDIA_TYPE* pmedia, DWORD cbFormat)
{
  if (pmedia == NULL)
    return E_POINTER;

  memset(pmedia, 0, sizeof(DMO_MEDIA_TYPE));

  if (cbFormat > 0)
  {
    pmedia->pbFormat = (BYTE *)dllCoTaskMemAlloc(cbFormat);
    if (pmedia->pbFormat == NULL)
      return E_OUTOFMEMORY;

    pmedia->cbFormat = cbFormat;
  }

  return S_OK;
}

////////////////////////////////////////////////////////////////////////
//user32.dll
BOOL WINAPI dllIsRectEmpty(const RECT *lprc)
{
  int w, h;
  if (lprc)
  {
    w = lprc->right - lprc->left;
    h = lprc->bottom - lprc->top;
    if (w <= 0 || h <= 0)
      return 1;
  }
  else
    return 1;

  return 0;
}

extern "C" BOOL WINAPI dllEnableWindow(HWND hWnd, BOOL bEnable)
{
  not_implement("user32.dll fake function EnableWindow called\n"); //warning
  return NULL;
}

extern "C" UINT WINAPI dllGetDlgItemInt(HWND hDlg, int nIDDlgItem, BOOL *lpTranslated, BOOL bSigned)
{
  not_implement("user32.dll fake function GetDlgItemInt called\n"); //warning
  return NULL;
}

extern "C" LRESULT WINAPI dllSendDlgItemMessageA(HWND hDlg, int nIDDlgItem, UINT Msg, WPARAM wParam, LPARAM lParam)
{
  not_implement("user32.dll fake function SendDlgItemMessageA called\n"); //warning
  return NULL;
}

extern "C" INT_PTR WINAPI dllDialogBoxParamA(HINSTANCE hInstance, LPCSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam)
{
  not_implement("user32.dll fake function DialogBoxParamA called\n"); //warning
  return NULL;
}

extern "C" UINT WINAPI dllGetDlgItemTextA(HWND hDlg, int nIDDlgItem, LPSTR lpString, int nMaxCount)
{
  not_implement("user32.dll fake function GetDlgItemTextA called\n"); //warning
  return NULL;
}

extern "C" int WINAPI dllMessageBoxA(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType)
{
  char szCaption[1024];
  char szText[1024];
  if (lpText == NULL)
    strcpy(szText, "Error");
  else
    strcpy(szText, lpText);
  if (lpCaption == NULL)
    strcpy(szCaption, "Error");
  else
    strcpy(szCaption, lpCaption);

  CLog::Log(LOGDEBUG, "MessageBoxA(hwnd: 0x%x, text: '%s', caption '%s', type: 0x%x", hWnd, szText, szCaption, uType);
  //not_implement("user32.dll fake function MessageBoxA called\n"); //warning
  return NULL;
}

extern "C" LONG WINAPI dllGetWindowLongA(HWND hWnd, int nIndex)
{
  not_implement("user32.dll fake function GetWindowLongA called\n"); //warning
  return NULL;
}

extern "C" HWND WINAPI dllGetDlgItem(HWND hDlg, int nIDDlgItem)
{
  not_implement("user32.dll fake function GetDlgItem called\n"); //warning
  return NULL;
}

extern "C" BOOL WINAPI dllCheckDlgButton(HWND hDlg, int nIDButton, UINT uCheck)
{
  not_implement("user32.dll fake function CheckDlgButton called\n"); //warning
  return NULL;
}

extern "C" HWINEVENTHOOK WINAPI dllSetDlgItemInt(DWORD eventMin, DWORD eventMax, HMODULE hmodWinEventProc, WINEVENTPROC pfnWinEventProc, DWORD idProcess, DWORD idThread, DWORD dwFlags)
{
  not_implement("user32.dll fake function SetDlgItemInt called\n"); //warning
  return NULL;
}

extern "C" BOOL WINAPI dllShowWindow(HWND hWnd, int nCmdShow)
{
  not_implement("user32.dll fake function ShowWindow called\n"); //warning
  return NULL;
}

extern "C" BOOL WINAPI dllEndDialog(HWND hDlg, INT_PTR nResult)
{
  not_implement("user32.dll fake function EndDialog called\n"); //warning
  return NULL;
}

extern "C" BOOL WINAPI dllSetDlgItemTextA(HWND hDlg, int nIDDlgItem, LPCSTR lpString)
{
  not_implement("user32.dll fake function SetDlgItemTextA called\n"); //warning
  return NULL;
}

extern "C" LONG WINAPI dllSetWindowLongA(HWND hWnd, int nIndex, LONG dwNewLong)
{
  not_implement("user32.dll fake function SetWindowLongA called\n"); //warning
  return NULL;
}

extern "C" BOOL WINAPI dllDestroyWindow(HWND hWnd)
{
  not_implement("user32.dll fake function DestroyWindow called\n"); //warning
  return NULL;
}

extern "C" HWND WINAPI dllCreateDialogParamA(HINSTANCE hInstance, LPCSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam)
{
  not_implement("user32.dll fake function CreateDialogParamA called\n"); //warning
  return NULL;
}

extern "C" BOOL WINAPI dllPostMessageA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
  not_implement("user32.dll fake function PostMessageA called\n"); //warning
  return NULL;
}

extern "C" LRESULT WINAPI dllSendMessageA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
  not_implement("user32.dll fake function SendMessageA called\n"); //warning
  return NULL;
}

extern "C" HWND WINAPI dllSetFocus(HWND hWnd)
{
  not_implement("user32.dll fake function SetFocus called\n"); //warning
  return NULL;
}
///
extern "C" HWND WINAPI dllGetDesktopWindow()
{
  CLog::Log(LOGDEBUG, "USER32!GetDesktopWindow() => 0");
  return 0;
}

extern "C" UINT WINAPI dllGetDC(HANDLE hWnd)

{

  CLog::Log(LOGDEBUG, "GetDC(0x%x) => 1\n", hWnd);

  return 1;

}

extern "C" UINT WINAPI dllReleaseDC(HANDLE hWnd, HANDLE hDC)

{

  CLog::Log(LOGDEBUG, "ReleaseDC(0x%x, 0x%x) => 1\n", hWnd, hDC);

  return 1;

}



extern "C" UINT WINAPI dllGetWindowRect(HWND win, RECT *r)

{

  CLog::Log(LOGDEBUG, "GetWindowRect(0x%x, 0x%x) => 1\n", win, r);

#define PSEUDO_SCREEN_WIDTH /*640*/800

#define PSEUDO_SCREEN_HEIGHT /*480*/600



  /* (win == 0) => desktop */

  r->right = PSEUDO_SCREEN_WIDTH;

  r->left = 0;

  r->bottom = PSEUDO_SCREEN_HEIGHT;

  r->top = 0;

  return 1;

}




static int show_cursor = 0;

extern "C" UINT WINAPI dllShowCursor(int show)

{

  CLog::Log(LOGDEBUG, "ShowCursor(%d) => %d\n", show, show);

  if (show)

    show_cursor++;

  else

    show_cursor--;

  return show_cursor;

}


extern "C" int WINAPI dllLoadStringA( HINSTANCE instance, UINT resource_id, LPSTR buffer, INT buflen )
{
  not_implement("user32.dll fake function LoadStringA called\n"); //warning
  return 1;
}

/////////////////////////////////////////////////////////////////////////
// xbmc_vobsub.dll - doesn;t exist just used to fixup some stuff in the streamed vobsub code

long VobSubPFSeek(unsigned long pf, long offset)
{
  HANDLE hFile = (HANDLE)pf;
  return (long)SetFilePointer(hFile, offset, 0, FILE_BEGIN);
}

unsigned long VobSubPFWrite(unsigned long pf, void* data, unsigned long size)
{
  HANDLE hFile = (HANDLE)pf;
  DWORD n;
  WriteFile(hFile, data, size, &n, 0);
  return n;
}

unsigned long VobSubPFRead(unsigned long pf, void* data, unsigned long size)
{
  HANDLE hFile = (HANDLE)pf;
  DWORD n;
  ReadFile(hFile, data, size, &n, 0);
  return n;
}

void VobSubPFReserve(unsigned long pf, unsigned long size)
{
  HANDLE hFile = (HANDLE)pf;
  DWORD fp = SetFilePointer(hFile, 0, 0, FILE_CURRENT);
  SetFilePointer(hFile, (size + 511) & ~511, 0, FILE_BEGIN);
  SetEndOfFile(hFile);
  SetFilePointer(hFile, fp, 0, FILE_BEGIN);
}

unsigned long VobSubPFOpen(int id)
{
  char filename[42] = "special://temp/vobsub_queue_";
  HANDLE hFile = INVALID_HANDLE_VALUE;

  if (id >= 256)
  {
    sprintf(filename + 28, "hdr%d", id - 256);
    hFile = CreateFile(_P(filename), GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM | FILE_FLAG_SEQUENTIAL_SCAN, 0);
  }
  else
  {
    sprintf(filename + 28, "data%d", id);
    hFile = CreateFile(_P(filename), GENERIC_WRITE | GENERIC_READ, 0, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM | FILE_FLAG_NO_BUFFERING, 0);
  }
  if (hFile != INVALID_HANDLE_VALUE)
  {
    CLog::Log(LOGDEBUG,"Open %s", filename);
    return (unsigned long)hFile;
  }
  return -1;
}

void VobSubPFClose(unsigned long pf)
{
  HANDLE hFile = (HANDLE)pf;
  if (hFile != INVALID_HANDLE_VALUE)
    CloseHandle(hFile);
}


/*HRESULT WINAPI CoGetClassObject(
    REFCLSID rclsid, DWORD dwClsContext, COSERVERINFO *pServerInfo,
    REFIID iid, LPVOID *ppv)
{
 return 0;
}*/


/***********************************************************************
 *           CoCreateInstance [COMPOBJ.13]
 *           CoCreateInstance [OLE32.@]
 */ 
/*HRESULT WINAPI CoCreateInstance(
 REFCLSID rclsid,
 LPUNKNOWN pUnkOuter,
 DWORD dwClsContext,
 REFIID iid,
 LPVOID *ppv)
{
 HRESULT hres;
 LPCLASSFACTORY lpclf = 0;
 
   // Sanity check
   if (ppv==0)
  return E_POINTER;
 
   // Initialize the "out" parameter
   *ppv = 0;
 
  //Get a class factory to construct the object we want.
  hres = CoGetClassObject(rclsid,
     dwClsContext,
     NULL,
     IID_IClassFactory,//&IID_IClassFactory,
     (LPVOID *)&lpclf);
 
  if (FAILED(hres)) {
    //FIXME("no classfactory created for CLSID %s, hres is 0x%08lx\n",debugstr_guid(rclsid),hres);
    return hres;
  }
 
   //Create the object and don't forget to release the factory
   
 //hres = IClassFactory_CreateInstance(lpclf, pUnkOuter, iid, ppv);
 //IClassFactory_Release(lpclf);
 
 //hres = lpclf->CreateInstance(pUnkOuter, iid, ppv);
 //lpclf->Release();
 
 //if(FAILED(hres))
   //FIXME("no instance created for interface %s of class %s, hres is 0x%08lx\n",debugstr_guid(iid), debugstr_guid(rclsid),hres);
 
 //return hres;
  return REGDB_E_CLASSNOTREG;
}*/

extern "C" LRESULT WINAPI dllDefDriverProc(DWORD_PTR dwDriverIdentifier, HDRVR hdrvr, UINT uMsg, LPARAM lParam1, LPARAM lParam2)
{
  not_implement("WINMM.DLL fake function DefDriverProc called\n"); //warning
  return 0;
}

/////////////////////////////////////////////////////////////////////////
// VERSION.DLL
extern "C" DWORD APIENTRY dllGetFileVersionInfoSizeA(LPCSTR lptstrFilename, LPDWORD lpdwHandle)
{
  not_implement("VERSION.DLL fake function GetFileVersionInfoSizeA called\n"); //warning
  return NULL;
}

extern "C" BOOL APIENTRY dllVerQueryValueA(const LPVOID pBlock, LPSTR lpSubBlock, LPVOID * lplpBuffer, PUINT puLen)
{
  not_implement("VERSION.DLL fake function VerQueryValueA called\n"); //warning
  return NULL;
}

extern "C" BOOL APIENTRY dllGetFileVersionInfoA(LPCSTR lptstrFilename, DWORD dwHandle, DWORD dwLen, LPVOID lpData)
{
  not_implement("VERSION.DLL fake function GetFileVersionInfoA called\n"); //warning
  return NULL;
}

/////////////////////////////////////////////////////////////////////////
//comdlg32.dll
extern "C" BOOL APIENTRY dllGetOpenFileNameA(LPOPENFILENAMEA lpFileName)
{
  not_implement("comdlg32.dll fake function GetOpenFileNameA called\n"); //warning
  return NULL;
}

////////////////////////////////////////////////
//GDI32.dll
extern "C" COLORREF WINAPI dllSetTextColor(HDC hdc, COLORREF crColor)
{
  not_implement("GDI32.dll fake function SetTextColor called\n"); //warning
  return NULL;
}

extern "C" BOOL WINAPI dllBitBlt(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight, HDC hdcSrc, int nXSrc, int nYSrc, DWORD dwRop)
{
  not_implement("GDI32.dll fake function BitBlt called\n"); //warning
  return NULL;
}

extern "C" BOOL WINAPI dllExtTextOutA(HDC hdc, int X, int Y, UINT fuOptions, CONST RECT* lprc, LPCTSTR lpString, UINT cbCount, CONST INT* lpDx)
{
  not_implement("GDI32.dll fake function ExtTextOutA called\n"); //warning
  return NULL;
}

extern "C" HGDIOBJ WINAPI dllGetStockObject(int fnObject)
{
  not_implement("GDI32.dll fake function GetStockObject called\n"); //warning
  return NULL;
}

extern "C" COLORREF WINAPI dllSetBkColor(HDC hdc, COLORREF crColor)
{
  not_implement("GDI32.dll fake function SetBkColor called\n"); //warning
  return NULL;
}

extern "C" HDC WINAPI dllCreateCompatibleDC(HDC hdc)
{
  not_implement("GDI32.dll fake function CreateCompatibleDC called\n"); //warning
  return NULL;
}

extern "C" HBITMAP WINAPI dllCreateBitmap(int nWidth, int nHeight, UINT cPlanes, UINT cBitsPerPel, CONST VOID *lpvBits)
{
  not_implement("GDI32.dll fake function CreateBitmap called\n"); //warning
  return NULL;
}

extern "C" HGDIOBJ WINAPI dllSelectObject(HDC hdc, HGDIOBJ hgdiobj)
{
  not_implement("GDI32.dll fake function SelectObject called\n"); //warning
  return NULL;
}

extern "C" HFONT WINAPI dllCreateFontA(int nHeight, int nWidth, int nEscapement, int nOrientation, int fnWeight, DWORD fdwItalic, DWORD fdwUnderline, DWORD fdwStrikeOut, DWORD fdwCharSet, DWORD fdwOutputPrecision, DWORD fdwClipPrecision, DWORD fdwQuality, DWORD fdwPitchAndFamily, LPCTSTR lpszFace)
{
  not_implement("GDI32.dll fake function CreateFontA called\n"); //warning
  return NULL;
}

extern "C" BOOL WINAPI dllDeleteDC(HDC hdc)
{
  not_implement("GDI32.dll fake function DeleteDC called\n"); //warning
  return NULL;
}

extern "C" int WINAPI dllSetBkMode(HDC hdc, int iBkMode)
{
  not_implement("GDI32.dll fake function SetBkMode called\n"); //warning
  return NULL;
}

extern "C" COLORREF WINAPI dllGetPixel(HDC hdc, int nXPos, int nYPos)
{
  not_implement("GDI32.dll fake function GetPixel called\n"); //warning
  return NULL;
}

extern "C" BOOL WINAPI dllDeleteObject(HGDIOBJ hObject)
{
  not_implement("GDI32.dll fake function DeleteObject called\n"); //warning
  return NULL;
}

extern "C" UINT WINAPI dllGetDeviceCaps(int hdc, int unk)

{

  CLog::Log(LOGDEBUG, "GetDeviceCaps(0x%x, %d) => 0\n", hdc, unk);

#define BITSPIXEL 12

#define PLANES    14

  if (unk == BITSPIXEL)

    return 24;

  if (unk == PLANES)

    return 1;

  return 1;

}


extern "C" HPALETTE WINAPI dllCreatePalette(CONST LOGPALETTE *lpgpl)

{

  CLog::Log(LOGDEBUG, "CreatePalette(%x) => NULL\n", lpgpl);

  return NULL;

}

extern "C" int WINAPI dllStretchDIBits(HDC hdc, int XDest, int YDest, int nDestWidth, int nDestHeight, int XSrc,
                                        int YSrc, int nSrcWidth, int nSrcHeight, CONST VOID *lpBits, 
                                        CONST BITMAPINFO *lpBitsInfo, UINT iUsage, DWORD dwRop)
{
#ifdef _WIN32PC
  return StretchDIBits(hdc, XDest, YDest, nDestWidth, nDestHeight, XSrc, YSrc, nSrcWidth, nSrcHeight, lpBits, 
                       lpBitsInfo, iUsage, dwRop);
#else
  not_implement("GDI32.dll fake function StretchDIBits called\n"); //warning
  return NULL;
#endif
}

extern "C" BOOL WINAPI dllRectVisible(HDC hdc, const RECT* lpRect)
{
#ifdef _WIN32PC
  return RectVisible(hdc, lpRect);
#else
  not_implement("GDI32.dll fake function RectVisible called\n"); //warning
  return NULL;
#endif
}

extern "C" int WINAPI dllSaveDC(HDC hdc)
{
#ifdef _WIN32PC
  return SaveDC(hdc);
#else
  not_implement("GDI32.dll fake function SaveDC called\n"); //warning
  return NULL;
#endif
}

extern "C" int WINAPI dllGetClipBox(HDC hdc, LPRECT lpRect)
{
#ifdef _WIN32PC
  return GetClipBox(hdc, lpRect);
#else
  not_implement("GDI32.dll fake function GetClipBox called\n"); //warning
  return NULL;
#endif
}

extern "C" HRGN WINAPI dllCreateRectRgnIndirect(LPRECT lpRect)
{
#ifdef _WIN32PC
  return CreateRectRgnIndirect(lpRect);
#else
  not_implement("GDI32.dll fake function CreateRectRgnIndirect called\n"); //warning
  return NULL;
#endif
}

extern "C" int WINAPI dllExtSelectClipRgn(HDC hdc, HRGN hrgn, int fnMode)
{
#ifdef _WIN32PC
  return ExtSelectClipRgn(hdc, hrgn, fnMode);
#else
  not_implement("GDI32.dll fake function ExtSelectClipRgn called\n"); //warning
  return NULL;
#endif
}

extern "C" int WINAPI dllSetStretchBltMode(HDC hdc, int nStretchMode)
{
#ifdef _WIN32PC
  return SetStretchBltMode(hdc, nStretchMode);
#else
  not_implement("GDI32.dll fake function SetStretchBltMode called\n"); //warning
  return NULL;
#endif
}

extern "C" int WINAPI dllSetDIBitsToDevice( HDC hdc,                 // handle to DC
                                            int XDest,               // x-coord of destination upper-left corner
                                            int YDest,               // y-coord of destination upper-left corner 
                                            DWORD dwWidth,           // source rectangle width
                                            DWORD dwHeight,          // source rectangle height
                                            int XSrc,                // x-coord of source lower-left corner
                                            int YSrc,                // y-coord of source lower-left corner
                                            UINT uStartScan,         // first scan line in array
                                            UINT cScanLines,         // number of scan lines
                                            CONST VOID *lpvBits,     // array of DIB bits
                                            CONST BITMAPINFO *lpbmi, // bitmap information
                                            UINT fuColorUse          // RGB or palette indexes
                                          )
{
#ifdef _WIN32PC
  return SetDIBitsToDevice(hdc, XDest, YDest, dwWidth, dwHeight, XSrc, YSrc, uStartScan, cScanLines, lpvBits, lpbmi, fuColorUse);
#else
  not_implement("GDI32.dll fake function SetDIBitsToDevice called\n"); //warning
  return NULL;
#endif
}

extern "C" BOOL WINAPI dllRestoreDC(HDC hdc, int nSavedDC)
{
#ifdef _WIN32PC
  return RestoreDC(hdc, nSavedDC);
#else
  not_implement("GDI32.dll fake function RestoreDC called\n"); //warning
  return NULL;
#endif
}

extern "C" int WINAPI dllGetObjectA(HGDIOBJ hgdiobj, int cbBuffer, LPVOID lpvObject)
{
#ifdef _WIN32PC
  return GetObjectA(hgdiobj, cbBuffer, lpvObject);
#else
  not_implement("GDI32.dll fake function GetObjectA called\n"); //warning
  return NULL;
#endif
}

extern "C" int WINAPI dllCombineRgn(HRGN hrgnDest, HRGN hrgnSrc1, HRGN hrgnSrc2, int fnCombineMode)
{
#ifdef _WIN32PC
  return CombineRgn(hrgnDest, hrgnSrc1, hrgnSrc2, fnCombineMode);
#else
  not_implement("GDI32.dll fake function CombineRgn called\n"); //warning
  return NULL;
#endif
}


extern "C" int WINAPI dllGetSystemMetrics(int nIndex)
{
#define SM_CXSCREEN  0

#define SM_CYSCREEN  1

#define SM_XVIRTUALSCREEN 76

#define SM_YVIRTUALSCREEN 77

#define SM_CXVIRTUALSCREEN  78

#define SM_CYVIRTUALSCREEN 79

#define SM_CMONITORS  80

  CLog::Log(LOGDEBUG, "GetSystemMetrics(%d)\n", nIndex);

  switch (nIndex)

  {

  case SM_XVIRTUALSCREEN:

  case SM_YVIRTUALSCREEN:

    return 0;

  case SM_CXSCREEN:

  case SM_CXVIRTUALSCREEN:

    return PSEUDO_SCREEN_WIDTH;

  case SM_CYSCREEN:

  case SM_CYVIRTUALSCREEN:

    return PSEUDO_SCREEN_HEIGHT;

  case SM_CMONITORS:

    return 1;

  }

  return 1;

}

extern "C" int WINAPI dllMonitorFromWindow(HWND win, int flags)
{
  CLog::Log(LOGDEBUG, "MonitorFromWindow(0x%x, 0x%x) => 0\n", win, flags);
  return 0;
}

extern "C" int WINAPI dllMonitorFromRect(RECT *r, int flags)
{
  CLog::Log(LOGDEBUG, "MonitorFromRect(0x%x, 0x%x) => 0\n", r, flags);
  return 0;
}

extern "C" int WINAPI dllMonitorFromPoint(void *p, int flags)

{

  CLog::Log(LOGDEBUG, "MonitorFromPoint(0x%x, 0x%x) => 0\n", p, flags);

  return 0;

}


extern "C" int WINAPI dllEnumDisplayMonitors(HDC hdc, LPRECT lprcClip, MONITORENUMPROC lpfnEnum, LPARAM dwData)

{

  CLog::Log(LOGDEBUG, "EnumDisplayMonitors(0x%x, 0x%x, 0x%x, 0x%x) => ?\n",

            hdc, lprcClip, lpfnEnum, dwData);

  if (lpfnEnum != NULL)

    return lpfnEnum(0, hdc, lprcClip, dwData);

  else

    return 0;

}



extern "C" int WINAPI dllGetMonitorInfoA(void *mon, LPMONITORINFO lpmi)

{

  CLog::Log(LOGDEBUG, "GetMonitorInfoA(0x%x, 0x%x) => 1\n", mon, lpmi);



  lpmi->rcMonitor.right = lpmi->rcWork.right = PSEUDO_SCREEN_WIDTH;

  lpmi->rcMonitor.left = lpmi->rcWork.left = 0;

  lpmi->rcMonitor.bottom = lpmi->rcWork.bottom = PSEUDO_SCREEN_HEIGHT;

  lpmi->rcMonitor.top = lpmi->rcWork.top = 0;



  lpmi->dwFlags = 1; /* primary monitor */



  if (lpmi->cbSize == sizeof(MONITORINFOEX))
  {

    LPMONITORINFOEX lpmiex = (LPMONITORINFOEX)lpmi;

    CLog::Log(LOGDEBUG, "MONITORINFOEX!\n");

    strncpy(lpmiex->szDevice, "Monitor1", CCHDEVICENAME);

  }



  return 1;

}





extern "C" int WINAPI dllEnumDisplayDevicesA(const char *device, int devnum,

      void *dispdev, int flags)

{

  CLog::Log(LOGDEBUG, "EnumDisplayDevicesA(0x%x = %s, %d, 0x%x, %x) => 1\n",

            device, device, devnum, dispdev, flags);

  return 1;

}



extern "C" int WINAPI dllIsWindowVisible(HWND win)

{

  CLog::Log(LOGDEBUG, "IsWindowVisible(0x%x) => 1\n", win);

  return 1;

}



extern "C" HWND WINAPI dllGetActiveWindow(void)

{

  CLog::Log(LOGDEBUG, "GetActiveWindow() => 0\n");

  return (HWND)0;

}

extern "C" int WINAPI dllDirectDrawCreate(void)
{
  CLog::Log(LOGDEBUG, "DirectDrawCreate(...) => NULL\n");
  return 0;
}

extern "C" HWND WINAPI dllCreateUpDownControl (DWORD style, INT x, INT y, INT cx, INT cy,
      HWND parent, INT id, HINSTANCE inst,
      HWND buddy, INT maxVal, INT minVal, INT curVal)
{
  CLog::Log(LOGDEBUG, "CreateUpDownControl(...)\n");
  return 0;
}

extern "C" MMRESULT WINAPI dlltimeGetDevCaps(LPTIMECAPS lpCaps, UINT wSize)
{
  CLog::Log(LOGDEBUG, "timeGetDevCaps(%p, %u) !\n", lpCaps, wSize);
  lpCaps->wPeriodMin = 1;
  lpCaps->wPeriodMax = 65535;
  return 0;
}

extern "C" MMRESULT WINAPI dlltimeBeginPeriod(UINT wPeriod)
{
  CLog::Log(LOGDEBUG, "timeBeginPeriod(%u) !\n", wPeriod);
  if (wPeriod < 1 || wPeriod > 65535) return 96 + 1; //TIMERR_NOCANDO;
  return 0;
}

extern "C" MMRESULT WINAPI dlltimeEndPeriod(UINT wPeriod)
{
  CLog::Log(LOGDEBUG, "timeEndPeriod(%u) !\n", wPeriod);
  if (wPeriod < 1 || wPeriod > 65535) return 96 + 1; //TIMERR_NOCANDO;
  return 0;
}

extern "C" MMRESULT WINAPI dllwaveOutGetNumDevs(void)
{
  CLog::Log(LOGDEBUG, "waveOutGetNumDevs() => 0\n");
  return 0;
}

extern "C" int WINAPI dllwsprintfA(char* string, const char* format, ...)
{
  va_list va;
  int result;
  va_start(va, format);
  result = vsprintf(string, format, va);
  CLog::Log(LOGDEBUG, "wsprintfA(0x%x, '%s', ...) => %d\n", string, format, result);
  va_end(va);
  return result;
}

extern "C" BOOL WINAPI dllGetCursorPos(LPPOINT lpPoint)
{
  CLog::Log(LOGDEBUG, "GetCursorPos() -> false");
  return FALSE;
}

extern "C" HCURSOR WINAPI dllLoadCursorA(HINSTANCE hInstance, LPCSTR lpCursorName)
{
  CLog::Log(LOGDEBUG, "LoadCursor() -> NULL");
  return NULL;
}

extern "C" HCURSOR WINAPI dllSetCursor(HCURSOR hCursor)
{
  CLog::Log(LOGDEBUG, "SetCursor() -> NULL");
  return NULL;
}

extern "C" UINT WINAPI dllRegisterWindowMessageA(LPCSTR lpString)
{
  CLog::Log(LOGDEBUG, "RegisterWindowMessage() -> 0");
  return 0;
}

extern "C" HBRUSH WINAPI dllGetSysColorBrush(int nIndex) 
{
  CLog::Log(LOGDEBUG, "GetSysColorBrush() -> NULL");
  return NULL;
}

extern "C" DWORD WINAPI dllGetSysColor(int nIndex)
{
  CLog::Log(LOGDEBUG, "GetSysColor() -> 0");
  return 1;
}

extern "C" UINT WINAPI dllRegisterClipboardFormatA(LPCSTR lpszFormat)
{
  CLog::Log(LOGDEBUG, "RegisterClipboardFormatA() -> 0");
  return 0;
}

extern "C" BOOL WINAPI dllGetIconInfo(HICON hIcon, PICONINFO piconinfo)
{
#ifdef _WIN32PC
  return GetIconInfo(hIcon, piconinfo);
#else
  CLog::Log(LOGDEBUG, "GetIconInfo() -> 0");
  return 0;
#endif
}
