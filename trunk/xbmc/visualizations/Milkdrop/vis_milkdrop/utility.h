/*
  LICENSE
  -------
Copyright 2005 Nullsoft, Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer. 

  * Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution. 

  * Neither the name of Nullsoft nor the names of its contributors may be used to 
    endorse or promote products derived from this software without specific prior written permission. 
 
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR 
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND 
FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR 
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT 
OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#ifndef __NULLSOFT_DX8_PLUGIN_SHELL_UTILITY_H__
#define __NULLSOFT_DX8_PLUGIN_SHELL_UTILITY_H__ 1

#include <windows.h>
//#include <xtl.h>

#define SafeRelease(x) { if (x) {x->Release(); x=NULL;} } 
#define SafeDelete(x) { if (x) {delete x; x=NULL;} }
#define IsNullGuid(lpGUID) ( ((int*)lpGUID)[0]==0 && ((int*)lpGUID)[1]==0 && ((int*)lpGUID)[2]==0 && ((int*)lpGUID)[3]==0 )
#define DlgItemIsChecked(hDlg, nIDDlgItem) ((SendDlgItemMessage(hDlg, nIDDlgItem, BM_GETCHECK, (WPARAM) 0, (LPARAM) 0) == BST_CHECKED) ? true : false)
#define CosineInterp(x) (0.5f - 0.5f*cosf((x) * 3.1415926535898f))
#define InvCosineInterp(x) (acosf(1.0f - 2.0f*(x))/3.1415926535898f)
float   PowCosineInterp(float x, float pow);
float   AdjustRateToFPS(float per_frame_decay_rate_at_fps1, float fps1, float actual_fps);

//int   InternalGetPrivateProfileInt - part of Win32 API
#define GetPrivateProfileBool(w,x,y,z) ((bool)(InternalGetPrivateProfileInt(w,x,y,z) != 0))
#define GetPrivateProfileBOOL(w,x,y,z) ((BOOL)(InternalGetPrivateProfileInt(w,x,y,z) != 0))
int		InternalGetPrivateProfileString(char *szSectionName, char *szKeyName, char *szDefault, char *buffer, int size, char *szIniFile);
int		InternalGetPrivateProfileInt(char *szSectionName, char *szKeyName, int iDefault, char *szIniFile);
float   InternalGetPrivateProfileFloat(char *szSectionName, char *szKeyName, float fDefault, char *szIniFile);
bool    WritePrivateProfileInt(int d, char *szKeyName, char *szIniFile, char *szSectionName);
bool    WritePrivateProfileFloat(float f, char *szKeyName, char *szIniFile, char *szSectionName);

void    SetScrollLock(int bNewState);
void    RemoveExtension(char *str);
void    RemoveSingleAmpersands(char *str);
void    TextToGuid(char *str, GUID *pGUID);
void    GuidToText(GUID *pGUID, char *str, int nStrLen);
//int    GetPentiumTimeRaw(unsigned __int64 *cpu_timestamp);
//double GetPentiumTimeAsDouble(unsigned __int64 frequency);
#ifdef _DEBUG
    void    OutputDebugMessage(char *szStartText, HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam); // only available in RELEASE builds!
#endif
void    MissingDirectX(HWND hwnd);
bool    CheckForMMX();
bool    CheckForSSE();
void    memcpy_MMX(void *pDest, void *pSrc, int nBytes); // CALL CheckForMMX() FIRST!!!
void    memset_MMX(void *pDest, unsigned char value, int nBytes); // CALL CheckForMMX() FIRST!!!
void    GetDesktopFolder(char *szDesktopFolder); // should be MAX_PATH len.



//#include "icon_t.h"
//#include <shlobj.h>
#include <list> 
/*
BOOL    DoExplorerMenu (HWND hwnd, LPCTSTR pszPath,   POINT point);
BOOL    DoExplorerMenu (HWND hwnd, LPITEMIDLIST pidl, POINT point);
UINT    GetItemCount (LPITEMIDLIST pidl);
LPITEMIDLIST GetNextItem (LPITEMIDLIST pidl);
LPITEMIDLIST DuplicateItem (LPMALLOC pMalloc, LPITEMIDLIST pidl);
void    FindDesktopWindows(HWND *desktop_progman, HWND *desktopview_wnd, HWND *listview_wnd);
void    ExecutePidl(LPITEMIDLIST pidl, char *szPathAndFile, char *szWorkingDirectory, HWND hWnd);
//int   DetectWin2kOrLater();
int     GetDesktopIconSize();
*/







#endif